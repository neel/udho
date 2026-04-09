#ifndef UDHO_LOGGING_IPC_QUEUE_H
#define UDHO_LOGGING_IPC_QUEUE_H

#include <boost/interprocess/ipc/message_queue.hpp>
#include <udho/logging/message.h>
#include <array>

#include <iostream>

#ifndef UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES
#define UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES 1000
#endif

#ifndef UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES_SIZE
#define UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES_SIZE 4*1024
#endif

namespace udho {
namespace logging {

namespace detail {

/**
 * @brief RAII wrapper for a Boost.Interprocess message queue.
 *
 * The IPC queue is used for high‑performance, loss‑tolerant logging between
 * multiple producer processes (workers) and a single consumer process (logger).
 *
 * **Key properties:**
 * - Process‑safe and thread‑safe (underlying Boost queue guarantees this).
 * - Fixed capacity and message size (configurable via macros).
 * - Non‑blocking `try_send` and blocking `send` variants.
 * - Automatic serialisation/deserialisation of `udho::logging::message`.
 * - Single‑threaded consumer uses a fixed read buffer to avoid allocations.
 *
 * @note The consumer side must never call `send` or `try_send`; it only receives.
 * @note The queue does not guarantee message ordering across different producers,
 *       but messages from a single producer are FIFO.
 */
struct ipc_queue {
    using size_type     = boost::interprocess::message_queue::size_type;
    using priority_type = std::uint32_t;
    using message_type  = udho::logging::message;

    /// Default name for the IPC queue (shared memory object).
    static constexpr const char* default_ipcq_name = "udho-log-hiper";
    /// Maximum number of messages the queue can hold (compile‑time).
    static constexpr size_type max_messages        = UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES;
    /// Maximum size in bytes of a single serialised log message.
    static constexpr size_type max_message_size    = UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES_SIZE;

    ipc_queue(const ipc_queue&) = delete;
    ipc_queue& operator=(const ipc_queue&) = delete;

    ipc_queue(ipc_queue&&) = delete;
    ipc_queue& operator=(ipc_queue&&) = delete;

    ~ipc_queue() = default;

    /**
     * @brief Open an existing queue for reading or writing.
     * @param name Queue name (nullptr uses default).
     */
    ipc_queue(boost::interprocess::open_only_t, const char* name = 0x0): _mq(boost::interprocess::open_only, name ? name : default_ipcq_name) {}

    /**
     * @brief Open an existing queue (convenience constructor).
     * @param name Queue name (nullptr uses default).
     */
    ipc_queue(const char* name = 0x0): ipc_queue(boost::interprocess::open_only, name ? name : default_ipcq_name) {}

    /**
     * @brief Create a fresh IPC queue, removing any stale queue with the same name.
     * @param name Queue name; @c nullptr selects the default queue name.
     * @return A newly created queue object.
     *
     * This function is intended to be called once by the logger-side setup path
     * before producers open the queue.
     *
     * @note The implementation returns the queue by value.
     */
    static ipc_queue create(const char* name = 0x0) {
        ipc_queue::remove(name ? name : default_ipcq_name);
        return ipc_queue(boost::interprocess::create_only, name ? name : default_ipcq_name);
    }

    /**
     * @brief Remove the IPC queue from the system.
     * @param name Queue name (nullptr uses default).
     */
    static void remove(const char* name = 0x0) {
        boost::interprocess::message_queue::remove(name ? name : default_ipcq_name);
    }

public:

    /**
     * @brief Attempt to send a serialized log message without blocking.
     * @param message The log message to send.
     * @param prioritize If true, the message severity is converted to the
     *        underlying message-queue priority value; otherwise priority 0 is used.
     * @return @c true if the message was enqueued, @c false if the queue was full,
     *         serialization exceeded @ref max_message_size, or an IPC error occurred.
     *
     * The message is serialized into a binary buffer before transmission.
     *
     * @note When @p prioritize is true, higher-severity messages are submitted with
     *       higher queue priority. Receive order may therefore differ from producer
     *       arrival order across competing messages.
     *
     * @note The serialized record already contains severity as payload data. The
     *       queue priority is an additional transport-level hint used only by the
     *       IPC queue.
     */
    bool try_send(const message_type& message, bool prioritize = false) {
        message_type::buffer_type buffer = message.save();
        priority_type priority = 0;
        if(prioritize) {
            udho::logging::severity severity = message[udho::logging::params::severity::val].value();
            static_assert(std::is_convertible_v<std::underlying_type_t<udho::logging::severity>, priority_type>);
            priority = static_cast<priority_type>(severity);
        }
        std::size_t serialized_message_size = buffer.size();
        if(serialized_message_size > max_message_size) {
            std::cout << message.json() << std::endl;
            return false;
        }

        assert(serialized_message_size <= max_message_size && "Increase UDHO_LOGGING_IPC_QUEUE_MAX_MESSAGES_SIZE");
        return try_send(buffer.data(), serialized_message_size, priority);
    }

    /**
     * @brief Send a serialized log message, blocking while the queue is full.
     * @param message The log message to send.
     * @param prioritize If true, severity is mapped to message-queue priority.
     *
     * @warning This call can block indefinitely if the consumer is not draining the
     *          queue. Prefer @ref try_send in latency-sensitive producer code.
     */
    void send(const message_type& message, bool prioritize = false) {
        message_type::buffer_type buffer = message.save();
        priority_type priority = 0;
        if(prioritize) {
            udho::logging::severity severity = message[udho::logging::params::severity::val].value();
            static_assert(std::is_convertible_v<std::underlying_type_t<udho::logging::severity>, priority_type>);
            priority = static_cast<priority_type>(severity);
        }
        send(buffer.data(), buffer.size(), priority);
    }

    /**
     * @brief Try to receive a log message (non‑blocking).
     * @param message Output parameter for the received message.
     * @return true if a message was received and deserialised successfully.
     *
     * If no message is available, returns false immediately.
     */
    bool try_receive(message_type& message) {
        size_type size = 0;
        auto [success, priority] = try_receive(_read_buffer.data(), _read_buffer.size(), size);
        // priority intentionally unused because severity parameter already exists in the log message
        if(success) {
            return message.load(_read_buffer.data(), size);
        }
        return false;
    }

    /**
     * @brief Receive a log message (blocking).
     * @param message Output parameter for the received message.
     * @return true if a message was received and deserialised successfully.
     *
     * Blocks until a message is available. The internal read buffer is reused.
     */
    bool receive(message_type& message) {
        size_type size = 0;
        priority_type priority = receive(_read_buffer.data(), _read_buffer.size(), size);
        // priority intentionally unused because severity parameter already exists in the log message
        return message.load(_read_buffer.data(), size);
    }

    /**
     * @brief Current number of messages in the queue.
     */
    std::size_t size() const { return _mq.get_num_msg(); }

    /**
     * @brief Maximum capacity of the queue.
     */
    std::size_t capacity() const { return _mq.get_max_msg(); }

private:
    ipc_queue(boost::interprocess::create_only_t, const char* name = 0x0): _mq(boost::interprocess::create_only, name ? name : default_ipcq_name, max_messages, max_message_size) {}

private:

    bool try_send(const void* data, size_type size, priority_type priority = 0) {
        try {
            return _mq.try_send(data, size, priority);
        } catch (const boost::interprocess::interprocess_exception& e) {
            return false;
        }
    }


    void send(const void* data, size_type size, priority_type priority = 0) {
        _mq.send(data, size, priority);
    }

    std::pair<bool, std::uint32_t> try_receive(void* buffer, size_type buffer_size, size_type& received_size) {
        unsigned int priority = 0;
        if (_mq.try_receive(buffer, buffer_size, received_size, priority)) {
            return std::make_pair(true, priority);
        }
        return std::make_pair(false, priority);
    }

    std::uint32_t receive(void* buffer, size_type buffer_size, size_type& received_size) {
        unsigned int priority = 0;
        _mq.receive(buffer, buffer_size, received_size, priority);
        return priority;
    }

    // consumtion is single process, single threaded
    // therefore, it must be sequential
    // therefore, a single read buffer is sufficient
    // This read buffer will get rewritten upon each receive

    boost::interprocess::message_queue         _mq;
    std::array<std::uint8_t, max_message_size> _read_buffer;

};

}

}
}

#endif // UDHO_LOGGING_IPC_QUEUE_H
