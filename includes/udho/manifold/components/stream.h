#ifndef UDHO_MANIFOLD_COMPONENTS_STREAM_H
#define UDHO_MANIFOLD_COMPONENTS_STREAM_H

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <udho/utils/string_view.h>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/strand.hpp>
#include <udho/net/common.h>
#include <queue>
#include <charconv>
#include <udho/utils/traits.h>
#include <udho/utils/format.h>
#include <boost/beast/core/ostream.hpp>
#include <iostream>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/empty_body.hpp>

namespace udho{
namespace manifold{

namespace detail{

/**
 * @brief Helper for HTTP chunked transfer framing.
 * @note Intended to be used only from within a strand.
 */
struct chunking_helper{
    chunking_helper(): _crlf({0x0d, 0x0a}), _last_chunk({'0', 0x0d, 0x0a, 0x0d, 0x0a}) {}

    /**
     * @brief Convert chunk size to hexadecimal ASCII.
     * @param size Payload size.
     * @param buffered_bytes_size_hex Output buffer to write hexadecimal ASCII digits into.
     * @return Number of characters written to the buffer.
     * @pre buffered_bytes_size_hex has sufficient size for the conversion (20 is plenty for size_t).
     */
    std::size_t make_chunk_header(std::size_t size, std::array<char, 20>& buffered_bytes_size_hex) {
        std::size_t buffered_bytes_size = size;
        std::to_chars_result result = std::to_chars(buffered_bytes_size_hex.data(), buffered_bytes_size_hex.data()+buffered_bytes_size_hex.size(), buffered_bytes_size, 16);
        assert (result.ec == std::errc());
        std::size_t buffered_bytes_size_hex_len   = std::distance(buffered_bytes_size_hex.data(), result.ptr);
        return buffered_bytes_size_hex_len;
    }

    /**
     * @brief Prepare `buffer` with "<hex-size>\\r\\n".
     * @param buffer Destination buffer (must be empty).
     * @param size Chunk payload size.
     *
     * @pre buffer.size() == 0
     * @post buffer contains the size header followed by CRLF, committed.
     */
    void prepare(boost::beast::flat_buffer& buffer, std::size_t size) {
        std::array<char, 20> buffered_bytes_size_hex = {0};
        std::size_t buffered_bytes_size_hex_len      = make_chunk_header(size, buffered_bytes_size_hex);
        const char* buffered_bytes_size_hex_begin    = buffered_bytes_size_hex.data();

        assert(buffer.size() == 0);
        auto mutable_buffer = buffer.prepare(buffered_bytes_size_hex_len + 2);
        mutable_buffer += boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(buffered_bytes_size_hex_begin, buffered_bytes_size_hex_len));
        mutable_buffer += boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(_crlf.data(), 2));
        buffer.commit(buffered_bytes_size_hex_len + 2);
    }
public:
    std::array<char, 2>        _crlf;
    std::array<char, 5>        _last_chunk;
};

/**
 * @brief Buffered ostream that accumulates payload in memory and flushes later.
 *
 * Intended usage:
 * - User code calls write() multiple times (data accumulates in a `multibuffer`).
 * - System calls async_flush() at a controlled point.
 * - Depending on encoding:
 *   - plain: writes `multibuffer`
 *   - chunked: writes "<hex>\\r\\n" + payload + "\\r\\n" as a single async_write
 * - Completion path is split:
 *   - on_flush_cb: indicates the *payload flush* completed (and may be followed by finish())
 *   - on_finish_cb: indicates the overall stream completion (after terminal chunk if needed)
 *
 * @tparam StreamT A Boost.Asio AsyncWriteStream (e.g., tcp::socket, beast test stream, etc.).
 *
 * @thread_safety
 * All public methods use `dispatch(_strand, ...)` and therefore are safe to call from any thread,
 * assuming the referenced `strand` remains alive. All internal state is mutated only on the strand.
 */
template <typename StreamT>
struct basic_buffered_ostream: private chunking_helper{
    using stream_type       = StreamT;
    using executor_type     = typename stream_type::executor_type;
    using strand_type       = boost::asio::strand<executor_type>;
    using encoding_type     = udho::net::types::transfer_encoding;
    using completion_callback_type = std::function<void (boost::system::error_code, std::size_t)>;

    /**
     * @brief Construct a buffered ostream.
     * @param stream Underlying async write stream.
     * @param strand Strand used to serialize all operations across header/payload writers.
     * @param encoding Transfer encoding (plain or chunked).
     * @param flush_callback Called when a flush of currently buffered payload completes.
     * @param completion_callback Called on final completion (after finish()).
     *
     * @note This type stores references to stream, strand, and encoding.
     */
    basic_buffered_ostream(stream_type& stream, strand_type& strand, const encoding_type& encoding, completion_callback_type&& flush_callback, completion_callback_type&& completion_callback)
        : _stream(stream), _flush(std::move(flush_callback)), _completion(std::move(completion_callback)), _encoding(encoding), _strand(strand), _bytes_written(0), _write_ongoing(false) {}

    /**
     * @brief Append ostreamable value into the internal buffer.
     * @tparam T Value type, must be move-constructible and ostreamable.
     *
     * @note This does not write to the network; it buffers.
     * @warning If a flush is in progress, the write is silently ignored.
     */
    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T> && !udho::utils::traits::is_string<T>::value, bool> = true>
    void write(T&& value) {
        boost::asio::dispatch(_strand, [this, val = std::move(value)]() {
            if(_write_ongoing) {
                // error
                return;
            }
            boost::beast::ostream(_multibuff) << val;
        });
    }

    /// @brief Append owned std::string into the internal buffer.
    void write(std::string&& str) {
        boost::asio::dispatch(_strand, [this, str = std::move(str)]() {
            if(_write_ongoing) {
                // error
                return;
            }
            auto mutable_buffer = _multibuff.prepare(str.size());
            boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(str.c_str(), str.size()));
            _multibuff.commit(str.size());
        });
    }

    /// @brief Append string_view into the internal buffer (copies into multi_buffer).
    void write(udho::utils::string_view str) {
        boost::asio::dispatch(_strand, [this, str]() {
            if(_write_ongoing) {
                // error
                return;
            }
            auto mutable_buffer = _multibuff.prepare(str.size());
            boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(str.data(), str.size()));
            _multibuff.commit(str.size());
        });
    }

    /**
     * @brief Append borrowed data into the internal buffer (copies into multi_buffer).
     * @param data Pointer to bytes.
     * @param size Number of bytes.
     *
     * @pre The data must remain valid until the dispatch handler runs (since dispatch may defer).
     * @note This overload still copies; it is only “borrowed” until dispatch executes.
     */
    void write(const char* data, std::size_t size) {
        boost::asio::dispatch(_strand, [this, data, size]() {
            if(_write_ongoing) {
                // error
                return;
            }
            auto mutable_buffer = _multibuff.prepare(size);
            boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data, size));
            _multibuff.commit(size);
        });
    }

    /**
     * @brief Append a flat_buffer by copying into the internal multi_buffer.
     * @param buffer Source buffer (moved in).
     *
     * @note This introduces a copy.
     */
    void write(boost::beast::flat_buffer&& buffer) {
        boost::asio::dispatch(_strand, [this, buff = std::move(buffer)]() mutable {
            if(_write_ongoing) {
                // error
                return;
            }
            auto mutable_buffer = _multibuff.prepare(buff.size());
            std::size_t bytes_copied = boost::asio::buffer_copy(mutable_buffer, buff.data());
            _multibuff.commit(bytes_copied);
        });
    }

    /**
     * @brief Asynchronously flush currently buffered payload to the stream.
     *
     * For chunked encoding, this writes header + payload + CRLF as one write.
     * On completion, calls the flush callback (not the completion callback).
     *
     * @post `_write_ongoing` is set true until the flush completes (success or error).
     */
    void async_flush() {
        boost::asio::dispatch(_strand, [this]() {
            _write_ongoing = true;
            async_write();
        });
    }

    /**
     * @brief Finish the stream.
     *
     * For chunked encoding, writes the terminal chunk "0\\r\\n\\r\\n".
     * For plain encoding, leads to completion callback through boost::asio::dispatch.
     *
     * @pre buffered payload has been flushed already (async_flush)
     * @note Typically called after a successful flush callback if in buffered mode.
     */
    void finish() {
        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            boost::asio::dispatch(_strand, [this]() {
                async_write_terminal();
            });
        } else {
            boost::asio::dispatch(_strand, [this]() {
                on_finish_cb(boost::system::error_code{}, _bytes_written);
            });
        }
    }

private:

    /**
     * @brief Flush implementation: writes buffered payload (and framing if chunked).
     *
     * Success path calls on_flush_cb().
     * Error path calls on_finish_cb() (i.e., error terminates).
     */
    void async_write() {
        if (_multibuff.size() == 0) {
            on_flush_cb({}, _bytes_written);
            return;
        }
        if(_encoding.encoding() == udho::net::types::transfer::encoding::plain) {
            async_write_payload();
        } else if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            prepare(_ongoing_header_buffer, _multibuff.size());

            auto out = boost::beast::buffers_cat(
                _ongoing_header_buffer.data(),    // _ongoing_header_buffer is member variable
                _multibuff.data(),                // _multibuff.data() would remain alive
                boost::asio::buffer(_crlf)        // _crlf is member variable
            );

            boost::asio::async_write(
                _stream, std::move(out),
                boost::asio::bind_executor( _strand,
                    [this](boost::system::error_code ec, std::size_t bytes_written) {
                        _ongoing_header_buffer.clear();
                        _multibuff.consume(bytes_written);
                        _bytes_written += bytes_written;
                        if(!ec) on_flush_cb(ec, _bytes_written);
                        else    on_finish_cb(ec, _bytes_written);
                    }
                )
            );
        }
    }

    /// @brief Flush for plain encoding: write payload only.
    void async_write_payload() {
        boost::asio::async_write(
            _stream, _multibuff.data(),
            boost::asio::bind_executor(_strand,
                [this](boost::system::error_code ec, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
                    _multibuff.consume(bytes_written);
                    if(!ec) on_flush_cb(ec, _bytes_written);
                    else    on_finish_cb(ec, _bytes_written);
                }
            )
        );
    }

    /// @brief Write terminal chunk for chunked encoding.
    void async_write_terminal() {
        boost::asio::async_write(
            _stream, boost::asio::buffer(_last_chunk, 5),
            boost::asio::bind_executor(_strand,
               [this](boost::system::error_code ec, std::size_t bytes_written) {
                   _bytes_written += bytes_written;
                   on_finish_cb(ec, _bytes_written);
               }
            )
        );
    }

    /**
     * @brief Flush callback hook.
     */
    void on_flush_cb(boost::system::error_code ec, std::size_t bytes_written) {
        // TODO _write_ongoing = false should set it to false?
        _write_ongoing = false;
        if(_flush) {
            _flush(ec, bytes_written);
        }
    }

    /// @brief Final completion callback hook.
    void on_finish_cb(boost::system::error_code ec, std::size_t bytes_written) {
        _write_ongoing = false;
        if(_completion) {
            _completion(ec, bytes_written);
        }
    }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after all buffered content has been flushed to the socket
     *          and the completion callback has been called
     * @pre _ongoing_header_buffer is cleared
     * @pre _multibuff is cleared
     */
    void reset() {
        assert(_ongoing_header_buffer.size() == 0);
        assert(_multibuff.size() == 0);

        _bytes_written = 0;
        _write_ongoing = 0;
    }
private:
    stream_type&               _stream;
    completion_callback_type   _flush;
    completion_callback_type   _completion;
    const encoding_type&       _encoding;
    boost::beast::multi_buffer _multibuff;
    strand_type&               _strand;
    boost::beast::flat_buffer  _ongoing_header_buffer;
    std::size_t                _bytes_written;
    bool                       _write_ongoing;
};

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

/**
 * @brief Streaming ostream that writes as data arrives (queued, ordered, strand-serialized).
 *
 * This stream:
 * - accepts owned data (copied into owned queue) and borrowed data (pointer queue)
 * - preserves original write order across both via monotonically increasing ids
 * - runs an async pump loop that writes one payload at a time
 *
 * End-of-stream behavior:
 * - finish() sets `_eoq` (end-of-queue); once queues drain, pump injects a terminal marker
 * - for chunked encoding: emits "0\\r\\n\\r\\n" then calls completion callback
 * - for plain: calls completion callback after terminal marker
 *
 * Flow-control:
 * - pause()/resume() toggles `_paused` which temporarily blocks pump from progressing
 *
 * @tparam StreamT A Boost.Asio AsyncWriteStream
 *
 * @thread_safety All public methods dispatch onto the strand; safe to call from any thread.
 */
template <typename StreamT>
struct basic_queued_ostream: private detail::buffer_queue, private chunking_helper{
    using stream_type       = StreamT;
    using executor_type     = typename stream_type::executor_type;
    using strand_type       = boost::asio::strand<executor_type>;
    using encoding_type     = udho::net::types::transfer_encoding;
    using payload_type      = detail::buffer_queue::payload_borrowed;
    using completion_callback_type = std::function<void (boost::system::error_code, std::size_t)>;

    basic_queued_ostream(stream_type& stream, strand_type& strand, const encoding_type& encoding, completion_callback_type&& callback)
        : _stream(stream), _completion(std::move(callback)), _encoding(encoding), _strand(strand), _write_ongoing(false), _bytes_written(0), _finished(false), _paused(false), _eoq(false) {}

    /**
     * @brief write ostreamable value (copied into owned queue).
     * @tparam T Value type, must be move-constructible and ostreamable.
     */
    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T> && !udho::utils::traits::is_string<T>::value, bool> = true>
    void write(T&& value) {
        boost::asio::dispatch(_strand, [this, val = std::move(value)]() {
            if(_eoq) return;
            boost::beast::flat_buffer buffer;
            boost::beast::ostream(buffer) << val;
            push_data(std::move(buffer));
            pump();
        });
    }

    /// @brief Write owned string (copied into owned queue).
    void write(std::string&& str) {
        boost::asio::dispatch(_strand, [this, str = std::move(str)]() {
            if(_eoq) return;
            push_data(str.data(), str.size());
            pump();
        });
    }

    /// @brief Write borrowed view (caller must ensure lifetime).
    void write(udho::utils::string_view str) {
        boost::asio::dispatch(_strand, [this, str]() {
            if(_eoq) return;
            push_ptr(str.data(), str.size());
            pump();
        });
    }

    /**
     * @brief No-copy write (borrowed).
     * @pre data must remain valid until async_write completion.
     */
    void write(const char* data, std::size_t size) {
        boost::asio::dispatch(_strand, [this, data, size]() {
            if(_eoq) return;
            push_ptr(data, size);
            pump();
        });
    }

    /// @brief Write owned flat_buffer (moved into owned queue).
    void write(boost::beast::flat_buffer&& buffer) {
        boost::asio::dispatch(_strand, [this, buff = std::move(buffer)]() mutable {
            if(_eoq) return;
            push_data(std::move(buff));
            pump();
        });
    }

    /**
     * @brief Signal end-of-stream.
     *
     * This does not immediately write the terminal marker; it marks `_eoq`.
     * The pump will inject the terminal marker once pending/available payloads drain.
     */
    void finish() {
        boost::asio::dispatch(_strand, [this]() {
            if(!_eoq) {
                _eoq = true;
                pump();
            }
        });
    }

    /**
     * @brief Pause/unpause the pump.
     * @param state true to pause, false to resume.
     */
    void pause(bool state = true) {
        boost::asio::dispatch(_strand, [this, state]() {
            if(_paused && !state){
                // paused before, resumed now
                _paused = false;
                pump();
            } else {
                _paused = state;
            }
        });
    }

    /// @brief Convenience resume.
    void resume(bool state = true) { pause(!state); }

private:

    /// @brief Final completion callback.
    void on_finish_cb(boost::system::error_code ec, std::size_t bytes_written) {
        _finished = true;
        if(_completion) {
            _completion(ec, bytes_written);
        }
    }

    /// @brief Write terminal chunk if needed, otherwise complete.
    void on_finish() {
        _write_ongoing = true;
        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            boost::asio::async_write(
                _stream, boost::asio::buffer(_last_chunk, 5),
                boost::asio::bind_executor(_strand,
                    [this](boost::system::error_code ec, std::size_t bytes_written) {
                        _bytes_written += bytes_written;
                        on_finish_cb(ec, _bytes_written);
                    }
                )
            );
        } else {
            boost::asio::dispatch(_strand, [this]() {
                on_finish_cb(boost::system::error_code{}, _bytes_written);
            });
        }
    }

    /// @brief Write a single plain payload.
    void async_write_payload(payload_type&& p) {
        _write_ongoing = true;
        boost::asio::async_write(
            _stream, boost::asio::buffer(p.buffer(), p.buffer().size()),
            boost::asio::bind_executor(_strand,
                [this, p](boost::system::error_code error, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
                    pop_payload(p.id());
                    if (!error) {
                       _write_ongoing = false;
                       pump();
                    } else {
                        on_finish_cb(error, _bytes_written);
                    }
                }
            )
        );
    }

    /// @brief Write a single chunked payload: [size\\r\\n][payload][\\r\\n]
    void async_write_chunked_payload(payload_type&& p) {
        prepare(_ongoing_header_buffer, p.buffer().size());

        std::array<boost::asio::const_buffer, 3> bufs = {
            _ongoing_header_buffer.data(),            // _ongoing_header_buffer is member variable
            p.buffer(),                               // p.buff is kept alive in the data queue or the caller ensures lifetime
            boost::asio::buffer(_crlf)                // _crlf is member variable
        };
        _write_ongoing = true;
        boost::asio::async_write(
            _stream, std::move(bufs), // bufs is moved
            boost::asio::bind_executor( _strand,
                [this, p = std::move(p)](boost::system::error_code error, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
                    _ongoing_header_buffer.clear();
                    pop_payload(p.id());
                    if (!error) {
                        _write_ongoing = false;
                        pump();
                    } else {
                        on_finish_cb(error, _bytes_written);
                    }
                }
            )
        );
    }

    /**
     * @brief Pump loop: schedules exactly one async write at a time.
     *
     * Algorithm:
     * - If a write is ongoing or paused => return.
     * - If no pending delivery payload:
     *   - try enqueue() one payload (ordered by id)
     *   - if cannot enqueue and `_eoq` is true => inject terminal marker and enqueue it
     *   - else return (nothing to do yet)
     * - Take payload from delivery queue:
     *   - if terminal => pop it and on_finish()
     *   - else write it (chunked or plain), then on completion pump() again
     */
    void pump() {
        if(_write_ongoing) return;              // once the ongoing write finishes it will comeback to process_queue again
        if(_paused) return;

        if(!pending()) {
            std::size_t enqueued = 0;
            if(available()) {
                enqueued = enqueue();
            }
            if(!enqueued) {
                if(_eoq) {
                    push_ptr();
                    enqueued = enqueue();
                    assert(enqueued == 1);
                } else {
                    return;
                }
            }
        }

        assert(pending());

        payload_type p = take_payload();
        if(p.terminal()) {
            pop_payload(p.id());
            on_finish();
            return;
        }

        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked)
            async_write_chunked_payload(std::move(p));
        else
            async_write_payload(std::move(p));
    }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after all queued content has been flushed to the socket
     *          and the completion callback has been called
     * @pre all queues are empty
     * @pre _finished is set to true
     * @pre _eoq is set to true
     * @pre _ongoing_header_buffer is cleared
     */
    void reset() {
        detail::buffer_queue::reset();
        assert(_ongoing_header_buffer.size() == 0);
        assert(_finished);
        assert(_eoq);

        _write_ongoing  = false;
        _finished       = false;
        _paused         = true;
        _bytes_written  = 0;
        _eoq            = false;
    }
private:
    stream_type&               _stream;
    strand_type&               _strand;
    boost::beast::flat_buffer  _ongoing_header_buffer;
    bool                       _write_ongoing;
    bool                       _finished;
    bool                       _paused;
    const encoding_type&       _encoding;
    std::size_t                _bytes_written;
    completion_callback_type   _completion;
    bool                       _eoq; // end of queue
};

/**
 * @brief Writes HTTP response headers once.
 *
 * Uses Beast serializer over an empty_body response constructed from `udho::net::types::headers::response`.
 * Ensures flush() is idempotent: subsequent calls are ignored once started.
 *
 * @tparam StreamT A Boost.Asio AsyncWriteStream.
 *
 * @thread_safety flush() dispatches on strand; safe from any thread.
 */
template <typename StreamT>
struct basic_header_writer{
    using stream_type               = StreamT;
    using executor_type             = typename stream_type::executor_type;
    using strand_type               = boost::asio::strand<executor_type>;
    using encoding_type             = udho::net::types::transfer_encoding;
    using completion_callback_type  = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type     = udho::net::types::headers::response;
    using response_type             = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type           = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using opt_serializer_type       = std::optional<serializer_type>;

    basic_header_writer(stream_type& stream, strand_type& strand, const response_headers_type& headers, completion_callback_type&& callback)
        : _stream(stream), _strand(strand), _headers(headers), _response(_headers), _serializer(_response), _completion(std::move(callback)), _bytes_written(0), _started(false), _finished(false) {}

    /**
     * @brief Asynchronously write headers (only once).
     *
     * If already started, this is a no-op.
     */
    void flush() {
        boost::asio::dispatch(_strand, [this]() {
            if (_started) {
                return;
            }
            _started = true;
            boost::beast::http::async_write_header(
                _stream, *_serializer,
                boost::asio::bind_executor(_strand,
                    [this](boost::system::error_code ec, std::size_t bytes) {
                        _bytes_written = bytes;
                        _finished      = true;
                        if (_completion)
                            _completion(ec, bytes);
                    }
                )
            );
        });
    }

    /// @brief True once a header write has been initiated.
    bool started() const { return _started; }

    /// @brief True once header write completion handler has run.
    bool finished() const { return _finished; }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after the response has been flushed to the socket and the
     *          completion callback has been called
     */
    void reset() {
        assert(_started);
        assert(_finished);
        assert(_serializer->is_header_done());

        _response.clear();
        _serializer.emplace(_response);

        _bytes_written  = 0;
        _started        = false;
        _finished       = false;
    }

private:
    stream_type&                    _stream;
    strand_type&                    _strand;
    const response_headers_type&    _headers;
    response_type                   _response;
    opt_serializer_type             _serializer;
    completion_callback_type        _completion;
    std::size_t                     _bytes_written;
    bool                            _started;
    bool                            _finished;
};

}


/**
 * @brief Composite ostream selecting between buffered and queued output modes.
 *
 * Default mode: buffered
 * - write() appends to `basic_buffered_ostream`
 * - finish() flushes buffered payload; for chunked it then emits terminal chunk via buffered.finish()
 *
 * After disable_buffering():
 * - headers are flushed
 * - `_buffering` becomes false
 * - buffered payload is flushed
 * - queued stream is resumed after buffered flush completes
 * - subsequent writes go to `basic_queued_ostream` and are pumped to socket
 *
 * Completion semantics:
 * - Buffered mode: completion is delivered from buffered completion callback after finish()
 * - Queued mode: completion is delivered from queued completion callback after queued finish()
 *
 * @tparam StreamT A Boost.Asio AsyncWriteStream.
 *
 * @thread_safety Public API methods post onto the strand; safe to call from any thread.
 * The class assumes it outlives all posted handlers (typical Asio lifetime rule).
 */
template <typename StreamT>
struct basic_ostream{
    using stream_type               = StreamT;
    using executor_type             = typename stream_type::executor_type;
    using strand_type               = boost::asio::strand<executor_type>;
    using encoding_type             = udho::net::types::transfer_encoding;
    using header_writer_type        = detail::basic_header_writer<StreamT>;
    using buffered_stream_type      = detail::basic_buffered_ostream<StreamT>;
    using queued_stream_type        = detail::basic_queued_ostream<StreamT>;
    using completion_callback_type  = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type     = udho::net::types::headers::response;
    using ostream_type              = basic_ostream<StreamT>;

    /**
     * @brief Construct composite ostream.
     * @param stream Underlying async write stream.
     * @param callback Completion callback (final completion).
     *
     * @note The queued stream starts paused, it is resumed only after switching away from buffering; if never resumed then uses buffered stream only
     */
    basic_ostream(stream_type& stream, completion_callback_type&& callback)
        : _stream(stream), _strand(stream.get_executor()), _header_sealed(false), _buffering(true), _finishing(false)
        , _header_stream(stream, _strand, _headers, std::bind(&ostream_type::on_header_completion, this, std::placeholders::_1, std::placeholders::_2))
        , _queued_stream(stream, _strand, _encoding,
            std::bind(&ostream_type::on_queued_completion,   this, std::placeholders::_1, std::placeholders::_2)
        )
        , _buffered_stream(stream, _strand, _encoding,
            std::bind(&ostream_type::on_buffered_flush,      this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&ostream_type::on_buffered_completion, this, std::placeholders::_1, std::placeholders::_2)
        )
        , _headers_sent(false), _bytes_written(0), _completion(std::move(callback))
    {
        _queued_stream.pause();
    }

    basic_ostream(const basic_ostream&) = delete;
    basic_ostream(basic_ostream&&) = delete;

    udho::net::types::transfer::encoding encoding() const { return _encoding.encoding(); }

    udho::net::types::transfer::compression compression() const { return _encoding.compression(); }

    void encoding(udho::net::types::transfer::encoding enc) {
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("encoding must be set before the headers are sent to the socket"));
        }
        _encoding.encoding(enc);
    }

    void compression(udho::net::types::transfer::compression cmp) {
        if(_header_sealed) {
            throw std::runtime_error(udho::utils::format("compression must be set before the headers are sent to the socket"));
        }
        _encoding.compression(cmp);
    }


    /**
     * @brief Permanently disable buffering (switch to queued streaming).
     *
     * This posts onto the strand and calls switch_stream() once.
     * Subsequent calls are ignored.
     */
    void disable_buffering() {
        _header_sealed = true;
        boost::asio::post(_strand, [this](){
            if(!_buffering) return;
            else switch_stream();
        });
    }

public:
     /// @brief Write an ostreamable value to the active output stream.
    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T> && !udho::utils::traits::is_string<T>::value, bool> = true>
    void write(T&& value) {
        boost::asio::post(_strand, [this, value = std::move(value)](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(std::move(value));
            } else {
                _queued_stream.write(std::move(value));
            }
        });
    }

    /// @brief Write owned string to the active output stream.
    void write(std::string&& str) {
        boost::asio::post(_strand, [this, str = std::move(str)](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(std::move(str));
            } else {
                _queued_stream.write(std::move(str));
            }
        });
    }

    /// @brief Write string_view to the active output stream (copied in buffered, borrowed in queued).
    void write(udho::utils::string_view str) {
        boost::asio::post(_strand, [this, str](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(str);
            } else {
                _queued_stream.write(str);
            }
        });
    }

    /**
     * @brief No-copy write in queued mode; buffered mode still copies into internal buffer.
     * @param data Pointer to bytes.
     * @param size Number of bytes.
     *
     * @warning In queued mode, caller must ensure lifetime until async write completion.
     */
    void write(const char* data, std::size_t size) {
        boost::asio::post(_strand, [this, data, size](){
            if(_finishing) return;
            if(_buffering) {
                _buffered_stream.write(data, size);
            } else {
                _queued_stream.write(data, size);
            }
        });
    }

    /**
     * @brief Finish the response.
     *
     * Semantics:
     * - Marks `_finishing` to reject subsequent writes.
     * - Ensures headers are flushed.
     * - If buffering: flush buffered payload; completion continues in callbacks:
     *     - on_buffered_flush => if finishing && chunked => buffered.finish()
     *     - on_buffered_completion => user completion callback (buffering case)
     * - If queued: queued.finish() which eventually completes (and chunked terminal if needed)
     */
    void finish() {
        _header_sealed = true;
        boost::asio::post(_strand, [this](){
            if(_finishing) return;
            _finishing = true;
            if(!_headers_sent) {
                _header_stream.flush();
            }

            if(_buffering) {
                _buffered_stream.async_flush();
            } else {
                _queued_stream.finish();
            }
        });
    }

private:

    /**
     * @brief Switch from buffered to queued mode.
     *
     * Ordering guarantee (all on same strand):
     * 1) flush headers
     * 2) set `_buffering = false`
     * 3) initiate buffered flush
     *
     * Because queued stream is paused until buffered flush completes,
     * any writes routed to queued during the transition will queue up,
     * but will not reach the socket until resume() is called in on_buffered_flush.
     */
    void switch_stream() {
        // all these calls will happen on the strand and the same strand is used by
        // all write calls, so the order of headers flush, setting buffering, flushing
        // buffered_stream and resuming _queued_stream prohibits existance of unflushed
        // data in the buffered stream, transmission of data from queued_stream until
        // _buffered_stream flush completes
        assert(_buffering);
        // call flush on all streams, no need to wait for them to finish.
        // we are good as soon as these calls are queued to the strand
        _header_stream.flush();
        // _buffering is atomic, so other threads calling write and thus checking
        // write is not a problem.
        _buffering = false;
        // subsequent calls to write will now use queued_stream, however write only
        // posts to the strand, if some other thread calls write now, it will post
        // a _queued_stream.write on strand and _queued_stream is paused
        _buffered_stream.async_flush();
        // flush operation is queued on the strand
    }

    /// @brief Header completion handler: marks headers sent and accumulates bytes.
    void on_header_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _headers_sent = true;
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
    }

    /**
     * @brief Buffered flush handler (payload flush completed).
     *
     * - If still buffering and finish() has been requested and encoding is chunked: emit terminal chunk.
     * - If switched to queued: resume queued pump so queued writes drain after buffered flush.
     */
    void on_buffered_flush(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
        else {
            if(_buffering) {
                if(_finishing) {
                    _buffered_stream.finish();
                }
            } else {
                _queued_stream.resume();
                // Any intermediate writes routed to the _queued_stream now gets
                // pumped out to the socket
            }
        }
    }

    /// @brief Buffered completion handler (after finish if needed).
    void on_buffered_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
        else {
            if(_buffering) {
                if(_completion) {
                    _completion(ec, _bytes_written);
                }
            }
        }
    }

    /// @brief Queued completion handler (called after queued.finish drains and terminal sent if needed).
    void on_queued_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
        else {
            if(_completion) {
                _completion(ec, _bytes_written);
            }
        }
    }

    /// @brief Common error path: forward error to user completion callback.
    void on_error(boost::system::error_code ec) {
        _completion(ec, _bytes_written);
    }

public:

    /**
     * @brief reset the internal state before reusing the stream for another request
     * @note intended to be used to respond to multiple requests through the same socket
     * @warning must be called after the response has been flushed to the socket and the
     *          completion callback has been called
     */
    void reset() {
        assert(_header_sealed);

        _header_stream.reset();
        _buffered_stream.reset();
        if(!_buffering) {
            _queued_stream.reset();
        }

        _headers.clear();

        _buffering      = true;
        _headers_sent   = false;
        _bytes_written  = 0;
        _finishing      = false;
        _header_sealed  = false;

        _encoding.encoding(udho::net::types::transfer::encoding::plain);
    }

private:
    stream_type&                 _stream;
    strand_type                  _strand;
    response_headers_type        _headers;
    encoding_type                _encoding;
private:
    header_writer_type           _header_stream;
    queued_stream_type           _queued_stream;
    buffered_stream_type         _buffered_stream;
private:
    std::atomic_bool             _header_sealed;
    bool                         _buffering;
    bool                         _headers_sent;
    std::size_t                  _bytes_written;
    bool                         _finishing;
private:
    completion_callback_type     _completion;
};

}
}


#endif // UDHO_MANIFOLD_COMPONENTS_STREAM_H
