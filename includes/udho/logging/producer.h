#ifndef UDHO_LOGGING_PRODUCER_H
#define UDHO_LOGGING_PRODUCER_H

#include <optional>
#include <udho/logging/ipc_queue.h>
#include <udho/logging/message.h>
#include <type_traits>
#include <atomic>
#include <limits>

namespace udho {
namespace logging {

namespace detail{

template <typename AtomicT, typename Enable = void>
struct atomic_spinner;

/**
 * @brief Spin until the referenced atomic becomes zero, then replace it with @p v.
 *
 * @tparam AtomicT std::atomic<T> where T is an arithmetic type.
 *
 * @note This is a busy-spin primitive. It does not sleep, yield, or back off.
 * @note This is used as a lightweight admission barrier for rare state mutation paths.
 * @note Success means the caller observed the atomic value as zero and changed it
 *       to @p v in one compare-exchange step.
 */
template <typename T>
struct atomic_spinner<std::atomic<T>, std::enable_if_t<std::is_arithmetic<T>::value>>{
    using atomic_type = std::atomic<T>;
    using value_type  = typename atomic_type::value_type;

    atomic_spinner(const atomic_spinner&) = delete;
    atomic_spinner& operator=(const atomic_spinner&) = delete;
    atomic_spinner(atomic_spinner&&) = delete;
    atomic_spinner& operator=(atomic_spinner&&) = delete;

    explicit atomic_spinner(atomic_type& atomic) noexcept: _atomic(atomic) { }

    void wait(const value_type& v) noexcept {
        value_type zero     = 0;
        while(!_atomic.compare_exchange_weak(zero, v, std::memory_order_acquire, std::memory_order_relaxed)) {
            zero = 0;
        }
        return;
    }
private:
    atomic_type& _atomic;
};

template <typename AtomicT>
struct atomic_raii_decrement;

template <typename T>
struct atomic_raii_decrement<std::atomic<T>>{
    static_assert(std::is_unsigned<T>::value);

    atomic_raii_decrement(std::atomic<T>& obj): _obj(obj) {}
    ~atomic_raii_decrement() { _obj.fetch_sub(1, std::memory_order_release); }
    std::atomic<T>& _obj;
};

template <typename AtomicT>
struct atomic_raii_zero;

template <typename T>
struct atomic_raii_zero<std::atomic<T>>{
    static_assert(std::is_unsigned<T>::value);

    atomic_raii_zero(std::atomic<T>& obj): _obj(obj) {}
    ~atomic_raii_zero() { _obj.store(std::numeric_limits<T>::min(), std::memory_order_release); }
    std::atomic<T>& _obj;
};
}


/**
 * @brief Lock-free producer for the IPC based log queue.
 *
 * - log messages are submitted concurrently through log() function
 * - admin operations activate() and deactivate() blocks log
 * - owns and controls lifetime of the optional IPC queue object
 *
 * Design summary:
 * - log() is the hot path and avoids a mutex.
 * - activate() / deactivate() are serialized by the two atomic flags.
 * - _admin performs atomic admission control in admin paths
 * - _atomic counts ongoing log() operations.
 * - deactivate() first acquires the admin lock, then waits for all active log()
 *   operations to drain, then resets the queue.
 * - Once deactivate() acquires admin lock log() discards any log messages sent
 *   to it.
 *
 * Lifetime guarantees:
 * - While a log() call is inside the counted region, _ipc_queue remains valid.
 * - deactivate() does not reset _ipc_queue until all counted log() calls have finished.
 *
 * Filter guarantees:
 * - filter() and reset_filter() are atomic pointer updates only.
 * - A log() call racing with a filter change may observe either the old filter or the new filter.
 *
 * Usage contract:
 * - activate() must be called before log() is expected to succeed.
 * - deactivate() prevents new log() calls from being accepted only after it acquires the admin lock.
 * - Calling deactivate() before process shutdown (e.g. before main returns) is expected user behavior.
 * - The filter function pointer, if set, must refer to code with stable lifetime
 *   while concurrent log() calls may still invoke it.
 */
struct producer{
    using queue_type        = udho::logging::detail::ipc_queue;
    using opt_queue_type    = std::optional<queue_type>;
    using message_type      = udho::logging::message;
    using filter_type       = bool (*)(const message_type&);
    using opt_filter_type   = std::atomic<filter_type>;
    using atomic_type       = std::atomic_uint32_t;
    using locker_type       = detail::atomic_spinner<atomic_type>;

    /**
     * @brief Try to produce a log message into the IPC queue.
     *
     * Fast-path behavior:
     * - Rejects immediately if an admin operation is active.
     * - Otherwise joins the active-log counter.
     * - Re-checks admin state after joining; if admin became active concurrently, the message is discarded.
     * - If the queue exists and the filter passes, forwards the message to queue_type::try_send().
     * - Otherwise returns false
     *
     * Return value:
     * - true  : message accepted by the IPC queue
     * - false : message discarded or queue send failed
     *
     * A false return may mean any of the following:
     * - activate() has not been called yet
     * - deactivate() has already completed
     * - an admin operation is active
     * - this log() call raced with admin-lock acquisition and was discarded
     * - the optional filter rejected the message
     * - the IPC queue rejected the send attempt
     * - the active counter was saturated (practically unreachable)
     *
     * Concurrency guarantees:
     * - Multiple threads may call log() concurrently.
     * - If this call enters the counted region, _ipc_queue will remain valid until
     *   this call leaves that region.
     * - Calls beginning after deactivate() acquires the admin lock, are discarded.
     *
     * Precise deactivate() interaction:
     * - Calls beginning before deactivate() acquires the admin lock may still succeed.
     * - Calls beginning after deactivate() acquires the admin lock are rejected.
     * - A call that races with admin-lock acquisition may increment the active counter
     *   and still be discarded by the second _admin check.
     *
     * @param message log message to forward
     * @return true if queue_type::try_send() succeeds, false otherwise
     */
    inline static bool log(const message_type& message) {
        atomic_type::value_type expected = _atomic.load(std::memory_order_relaxed);
        do {
            if(_admin.load(std::memory_order_acquire)) return false;
            // log messages produced during ongoing admin operation are discarded
            // admin operations wait for all ongoing log operations to finish
            // so, to be precise log messages produced between begin admin ... end admin will get discarded
            // deactivate() admin operation's behaviour implies this behaviour
            // filter()/reset_filter() admin operation's behaviour may not imply the same but I think this is acceptable
            if (expected == _max) return false;
        } while (!_atomic.compare_exchange_weak(expected, expected + 1, std::memory_order_acquire, std::memory_order_relaxed));
        detail::atomic_raii_decrement<atomic_type> decrement(_atomic);
        if(_admin.load(std::memory_order_acquire)) {
            return false;
        }
        if(_ipc_queue) {
            auto filter = _filter.load(std::memory_order_relaxed);
            bool accepted = filter
                                ? filter(message)
                                : (message[params::severity::val].value() >= threshold());

            if(accepted) {
                return (*_ipc_queue).try_send(message);
            }
        } else {
            // discard
        }
        return false;
    }

    /**
     * @brief Activate logging by opening the IPC queue.
     *
     * Behavior:
     * - Serializes against deactivate() and concurrent activate().
     * - Blocks new log() admissions while activation is in progress.
     * - Waits for any already-counted log() calls to finish.
     * - Opens the queue if it is not already open.
     * - Releases the admin lock at function exit.
     *
     * Activation window guarantees:
     * - While activate() holds the admin lock, new log() calls are rejected.
     * - If the queue is re-created during this call, it becomes valid before the
     *   function returns, but log() remains blocked until the admin lock is released.
     *
     * Idempotence:
     * - If the queue is already active, this function leaves it unchanged.
     *
     * @param name IPC queue name passed to boost::interprocess::open_only
     *
     * @pre The named IPC queue already exists.
     * @post After successful return, log() calls may again attempt to send messages.
     */
    inline static void activate(const char* name = "") {
        detail::atomic_spinner<std::atomic_bool> admin_lock(_admin);
        admin_lock.wait(true);

        auto lock = locker_type(_atomic);
        lock.wait(_max);
        detail::atomic_raii_zero<std::atomic_bool> admin_zero(_admin); // 0 is equivalent to false
        detail::atomic_raii_zero<atomic_type>      atomic_zero(_atomic);
        if(!_ipc_queue) {
            _ipc_queue.emplace(boost::interprocess::open_only, name);
        }
    }

    /**
     * @brief Deactivate logging by preventing new admissions, draining active log() calls,
     *        and resetting the IPC queue.
     *
     * Timeline:
     * - D_enter  : function entry
     * - D_locked : admin lock acquired, _admin becomes true
     * - D_ready  : all counted log() calls have finished, _atomic acquired as _max
     * - D_reset  : _ipc_queue.reset() executed (optionally)
     * - D_exit   : function return
     *
     * Strict guarantees:
     * - Calls to log() that begin after D_locked are discarded.
     * - Calls to log() that begin before D_locked may still succeed.
     * - The IPC queue remains valid from D_enter up to D_reset.
     * - The IPC queue is not reset until all counted log() calls have finished.
     * - After D_reset and before a later successful activate(), the queue is invalid.
     *
     * Important non-guarantee:
     * - The rejection boundary is D_locked, not D_enter.
     * - Therefore, messages passed to log() between D_enter and D_locked may still
     *   reach the IPC queue.
     *
     * Safety guarantee:
     * - No log() call can use _ipc_queue after D_reset, because deactivate() waits
     *   for all counted log() calls to finish before resetting it.
     *
     * @post New log() calls are rejected until a later call to activate() completes.
     */
    inline static void deactivate() {                                               // D_enter  -> begin
        detail::atomic_spinner<std::atomic_bool> admin_lock(_admin);
        admin_lock.wait(true);                                                      // D_locked -> admin operation lock acquired

        auto lock = locker_type(_atomic);
        lock.wait(_max);                                                            // D_ready  -> all pending log operations finished
        detail::atomic_raii_zero<std::atomic_bool> admin_zero(_admin);
        // 0 is equivalent to false
        detail::atomic_raii_zero<atomic_type>      atomic_zero(_atomic);
        if(_ipc_queue) {
            _ipc_queue.reset();                                                     // D_reset  -> resets the IPC queue
        }                                                                           // D_exit   -> end
    }

    /**
     * @brief Install or replace the message filter.
     *
     * Behavior:
     * - Atomic pointer store only; does not synchronize with in-flight log() calls.
     * - A concurrent log() call may observe either the previous filter or the new filter.
     *
     * Contract:
     * - The filter function pointer must remain valid while concurrent log() calls
     *   may still invoke it.
     *
     * @param filter filter function pointer, or nullptr-equivalent if desired
     */
    inline static void filter(filter_type filter) {
        _filter.store(filter, std::memory_order_relaxed);
    }

    /**
     * @brief Remove the current message filter.
     *
     * Behavior:
     * - Equivalent to storing nullptr into the filter pointer.
     * - Subsequent log() calls that observe the reset state will treat logging as unfiltered.
     * - In-flight log() calls may still observe the old filter pointer.
     */
    inline static void reset_filter() {
        _filter.store(nullptr, std::memory_order_relaxed);
    }

    /**
     * @brief Set severity threshold to use in absence of a message filter callback
     * @param threshold minimum severity required in order to produce the message to the IPC queue
     */
    inline static void threshold(severity threshold) {
        _threshold.store(threshold, std::memory_order_relaxed);
    }

    /**
     * @brief returns severity threshold to use in absence of a message filter callback
     * @return severity
     */
    static severity threshold() {
        return _threshold.load(std::memory_order_relaxed);
    }

private:
    static std::atomic<severity> _threshold;
    static opt_queue_type       _ipc_queue;
    static opt_filter_type      _filter;
    static atomic_type          _atomic;
    static std::atomic_bool     _admin;
    static const atomic_type::value_type _max;
};

inline std::atomic<severity>     producer::_threshold = severity::trace;
inline producer::opt_queue_type  producer::_ipc_queue = std::nullopt;
inline producer::opt_filter_type producer::_filter    = nullptr;
inline producer::atomic_type     producer::_atomic    = 0;
inline std::atomic_bool          producer::_admin     = false;
inline const producer::atomic_type::value_type producer::_max = std::numeric_limits<producer::atomic_type::value_type>::max();

}
}

#endif // UDHO_LOGGING_PRODUCER_H
