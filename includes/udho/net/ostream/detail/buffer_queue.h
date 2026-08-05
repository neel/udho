#ifndef UDHO_NET_OSTREAM_DETAIL_BUFFER_QUEUE_H
#define UDHO_NET_OSTREAM_DETAIL_BUFFER_QUEUE_H

#include <queue>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace udho{
namespace net{

namespace detail{

/** @addtogroup DoxyG_net
 *  @{
 */


/**
 * @brief Queue-based buffer manager that preserves write order across owned and borrowed payloads.
 *
 * This buffer_queue maintains three queues:
 * - `_dat_queue` (owned payloads): data copied into flat_buffers owned by the queue
 * - `_ptr_queue` (borrowed payloads): const_buffer references whose lifetime must be ensured by caller
 * - `_del_queue` (delivery queue): the next payloads ready to be written on the wire, in strict order
 *
 * Ordering is enforced via a monotonic `_count` id assigned at push-time for both owned and borrowed items.
 * enqueue() compares front ids and moves the lower id item into the delivery queue.
 *
 * @note All member functions are expected to be invoked from a strand by the owning stream implementation.
 */
struct buffer_queue{

    /**
     * @brief Borrowed payload descriptor (either real data or terminal marker).
     * @ingroup DoxyG_net
     */
    class payload_borrowed{
        boost::asio::const_buffer _buf;
        bool _terminal   = false;
        bool _enqueued   = false;
        std::size_t _id;
    public:
        payload_borrowed(std::size_t id, bool terminal): _terminal(terminal), _id(id) {}
        payload_borrowed(boost::asio::const_buffer&& buff, std::size_t id, bool terminal): _buf(std::move(buff)), _terminal(terminal), _id(id) {}
        boost::asio::const_buffer& buffer() { return _buf;}
        bool terminal() const { return _terminal; }
        bool enqueued() const { return _enqueued; }
        void enqueued(bool flag) { _enqueued = flag; }
        std::size_t id() const { return _id; }
    };

    /**
     * @brief Owned payload descriptor stored in `_dat_queue`.
     * @ingroup DoxyG_net
     *
     * Owns data (flat_buffer) and can yield a borrowed view via borrowed().
     */
    class payload_owned{
        boost::beast::flat_buffer _buf;
        bool _enqueued = false;
        std::size_t _id;
    public:
        payload_owned(std::size_t id): _id(id) {}
        payload_owned(boost::beast::flat_buffer&& buff, std::size_t id): _buf(std::move(buff)), _id(id) {}
        boost::beast::flat_buffer& buffer() { return _buf;}
        bool enqueued() const { return _enqueued; }
        void enqueued(bool flag) { _enqueued = flag; }
        std::size_t id() const { return _id; }

        /// @brief Produce a borrowed view referencing the owned buffer data.
        payload_borrowed borrowed() { return payload_borrowed(_buf.data(), _id, false); }
    };

    using borrowed_queue_type = std::queue<payload_borrowed>;
    using owned_queue_type    = std::queue<payload_owned>;
    using delivery_queue_type = std::queue<payload_borrowed>;

public:

    /// @brief Copy data into an owned buffer and enqueue it.
    std::size_t push_data(const char* data, std::size_t size) {
        _dat_queue.emplace(last_id());
        payload_owned& par_back = _dat_queue.back();
        boost::beast::flat_buffer& buffer = par_back.buffer();
        auto mutable_buffer = buffer.prepare(size);
        boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data, size));
        buffer.commit(size);
        // std::cout << "push_data: " << size << std::endl;
        return _dat_queue.size();
    }

    /// @brief Move an already-built flat_buffer into the owned queue.
    std::size_t push_data(boost::beast::flat_buffer&& buffer) {
        // std::cout << "push_data(buffer) : " << buffer.size() << std::endl;
        _dat_queue.emplace(payload_owned(std::move(buffer), last_id()));
        return _dat_queue.size();
    }

    /**
     * @brief Enqueue a borrowed payload pointer.
     * @warning Caller must ensure lifetime until written.
     */
    std::size_t push_ptr(const char* data, std::size_t size) {
        // std::cout << "push_ptr: " << data << std::endl;
        _ptr_queue.emplace(payload_borrowed(boost::asio::const_buffer(data, size), last_id(), false));
        return _ptr_queue.size();
    }

    /// @brief Enqueue a terminal marker (no data, terminal=true).
    std::size_t push_ptr() {
        // std::cout << "push_ptr: " << std::endl;
        _ptr_queue.emplace(payload_borrowed(boost::asio::const_buffer(), last_id(), true));
        return _ptr_queue.size();
    }

    /**
     * @brief Move the next-in-order item into the delivery queue.
     * @return 1 if an item was moved to `_del_queue`, else 0.
     *
     * Picks the lower id between the fronts of `_dat_queue` and `_ptr_queue`.
     * Uses each item’s `enqueued()` to prevent duplicating an already-delivered item.
     *
     * @note does not loop, enqueues at most one item per call.
     */
    std::size_t enqueue() {
        // std::cout << "enqueue_data: |datQ|: " << _dat_queue.size() << " |ptrQ|: " << _ptr_queue.size() << std::endl;

        if(_dat_queue.empty() && _ptr_queue.empty()) return 0;
        if(_dat_queue.size() > 0 && _ptr_queue.empty()) {
            payload_owned&    dat_front = _dat_queue.front();
            dat_front.enqueued(true);
            _del_queue.emplace(dat_front.borrowed());
            // std::cout << "> enqueue_data: |datQ.front()|: " << dat_front.buffer().size() << " |dQ|: " << _del_queue.size() << std::endl;
            assert(_del_queue.front().id() == dat_front.id());
            return 1;
        }
        if(_ptr_queue.size() > 0 && _dat_queue.empty()) {
            payload_borrowed& ptr_front = _ptr_queue.front();
            ptr_front.enqueued(true);
            _del_queue.push(ptr_front);
            // std::cout << "> enqueue_data: |ptrQ.front()|: " << ptr_front.buffer().size() << " |dQ|: " << _del_queue.size() << std::endl;
            assert(_del_queue.front().id() == ptr_front.id());
            return 1;
        }

        payload_owned&    dat_front = _dat_queue.front();
        payload_borrowed& ptr_front = _ptr_queue.front();

        assert(dat_front.id() != ptr_front.id());

        if(dat_front.id() < ptr_front.id()) {
            if(dat_front.enqueued()) {
                return 0;
            } else {
                dat_front.enqueued(true);
                _del_queue.emplace(dat_front.borrowed());
                // std::cout << "> enqueue_data: |datQ.front()|: " << dat_front.buffer().size() << " |dQ|: " << _del_queue.size() << std::endl;
                assert(_del_queue.front().id() == dat_front.id());
                return 1;
            }
        } else {
            if(ptr_front.enqueued()) {
                return 0;
            } else {
                ptr_front.enqueued(true);
                _del_queue.push(ptr_front);
                // std::cout << "> enqueue_data: |ptrQ.front()|: " << ptr_front.buffer().size() << " |dQ|: " << _del_queue.size() << std::endl;
                assert(_del_queue.front().id() == ptr_front.id());
                return 1;
            }
        }
    }

    /// @brief True if a payload is already staged for delivery.
    bool pending() const { return !_del_queue.empty(); }

    /// @brief True if there is anything available to enqueue (owned or borrowed).
    bool available() const { return has_data() || has_ptr(); }

    /**
     * @brief Take the next staged payload from the delivery queue.
     * @return A borrowed payload descriptor (may be terminal()).
     */
    payload_borrowed take_payload() {
        // std::cout << "pop_ptr: " << _del_queue.size() << std::endl;
        assert(!_del_queue.empty());
        payload_borrowed p = _del_queue.front();
        _del_queue.pop();

        return p;
    }

    /**
     * @brief Pop the payload with the given id from the underlying source queue.
     *
     * Called after the payload has been written (or otherwise consumed).
     * Determines whether the id corresponds to `_dat_queue.front()` or `_ptr_queue.front()`.
     *
     * @param id Payload id previously obtained from take_payload().
     * @return 1 if an item was popped, else 0 (should not happen).
     */
    std::size_t pop_payload(std::size_t id) {
        assert(!_dat_queue.empty() || !_ptr_queue.empty());

        if(_dat_queue.empty() && !_ptr_queue.empty()) {
            assert(_ptr_queue.front().id() == id);
            _ptr_queue.pop();
            return 1;
        }

        if(_ptr_queue.empty() && !_dat_queue.empty()) {
            assert(_dat_queue.front().id() == id);
            _dat_queue.pop();
            return 1;
        }

        payload_owned&    dat_front = _dat_queue.front();
        payload_borrowed& ptr_front = _ptr_queue.front();

        std::size_t found_dat_item = dat_front.id() == id;
        std::size_t found_ptr_item = ptr_front.id() == id;

        assert(found_dat_item || found_ptr_item);
        assert(!(found_dat_item && found_ptr_item));

        if(found_dat_item) {
            _dat_queue.pop();
            return 1;
        } else {
            _ptr_queue.pop();
            return 1;
        }
        return 0;
    }

private:
    std::size_t last_id() { return _count++; }

    bool has_data() const { return !_dat_queue.empty(); }
    bool has_ptr() const { return !_ptr_queue.empty(); }

protected:

    /**
     * @brief reset the internal state to restart the three queue system
     * @pre expects all the queues are empty implying everything queued has
     *      been transmitted to the socket
     */
    void reset() {
        assert(_del_queue.empty());
        assert(_ptr_queue.empty());
        assert(_dat_queue.empty());
        _count = 0;
    }

private:
    borrowed_queue_type     _ptr_queue;
    owned_queue_type        _dat_queue;
    delivery_queue_type     _del_queue;
    std::size_t             _count = 0;
};


/** @} */

}

}
}

#endif // UDHO_NET_OSTREAM_DETAIL_BUFFER_QUEUE_H
