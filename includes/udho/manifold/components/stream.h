#ifndef UDHO_MANIFOLD_COMPONENTS_STREAM_H
#define UDHO_MANIFOLD_COMPONENTS_STREAM_H

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <udho/utils/string_view.h>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/strand.hpp>
#include <udho/net/common.h>
#include <deque>
#include <charconv>
#include <udho/utils/traits.h>
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

struct chunking_helper{
    chunking_helper(): _crlf({0x0d, 0x0a}), _last_chunk({'0', 0x0d, 0x0a, 0x0d, 0x0a}) {}

    std::size_t make_chunk_header(std::size_t size, std::array<char, 20>& buffered_bytes_size_hex) {
        std::size_t buffered_bytes_size = size;
        std::to_chars_result result = std::to_chars(buffered_bytes_size_hex.data(), buffered_bytes_size_hex.data()+buffered_bytes_size_hex.size(), buffered_bytes_size, 16);
        assert (result.ec == std::errc());
        std::size_t buffered_bytes_size_hex_len   = std::distance(buffered_bytes_size_hex.data(), result.ptr);
        return buffered_bytes_size_hex_len;
    }

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

template <typename StreamT>
struct basic_buffered_ostream: private chunking_helper{
    using stream_type       = StreamT;
    using executor_type     = typename stream_type::executor_type;
    using strand_type       = boost::asio::strand<executor_type>;
    using encoding_type     = udho::net::types::transfer_encoding;
    using completion_callback_type = std::function<void (boost::system::error_code, std::size_t)>;

    basic_buffered_ostream(stream_type& stream, strand_type& strand, const encoding_type& encoding, completion_callback_type&& callback)
        : _stream(stream), _completion(std::move(callback)), _encoding(encoding), _strand(strand), _bytes_written(0), _write_ongoing(false) {}

    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void write(T&& value) {
        boost::asio::dispatch(_strand, [this, val = std::move(value)]() {
            if(_write_ongoing) {
                // error
                return;
            }
            boost::beast::ostream(_multibuff) << val;
        });
    }

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
     * @brief write
     * @param data
     * @param size
     * @pre data must outlive until io_context starts processing, garunteed by the caller
     * @warning if lifetime of data cannot be garunteed then use other overloads of write
     */
    void write(const char* data, std::size_t size) {
        // boost::asio::dispatch may or may not be invoked immediately
        // if we copy the data immediately then will will not be synchronized with the strand
        // if we copy the data into a temporary buffer then it will lead to double copy
        // therefore usercode is responsible to ensure lifetime of this data
        // other overloads are provided that moves or copies the data
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

    void async_flush() {
        boost::asio::dispatch(_strand, [this]() {
            _write_ongoing = true;
            async_write();
        });
    }

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
    void async_write() {
        if (_multibuff.size() == 0) {
            on_finish_cb({}, _bytes_written);
            return;
        }
        if(_encoding.encoding() == udho::net::types::transfer::encoding::plain) {
            async_write_payload();
        } else if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            prepare(_ongoing_header_buffer, _multibuff.size());

            const boost::beast::multi_buffer& cmbuff = _multibuff;

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
                        _bytes_written += bytes_written;
                        on_finish_cb(ec, _bytes_written);
                    }
                )
            );
        }
    }

    void async_write_payload() {
        boost::asio::async_write(
            _stream, _multibuff.data(),
            boost::asio::bind_executor(_strand,
                [this](boost::system::error_code ec, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
                    _multibuff.consume(bytes_written);
                    on_finish_cb(ec, _bytes_written);
                }
            )
        );
    }

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

    void on_finish_cb(boost::system::error_code ec, std::size_t bytes_written) {
        _write_ongoing = false;
        if(_completion) {
            _completion(ec, bytes_written);
        }
    }

private:
    stream_type&               _stream;
    completion_callback_type   _completion;
    const encoding_type&       _encoding;
    boost::beast::multi_buffer _multibuff;
    strand_type&               _strand;
    boost::beast::flat_buffer  _ongoing_header_buffer;
    std::size_t                _bytes_written;
    bool                       _write_ongoing;
};

/**
 * @brief Privides buffer management for teh output buffer through multiple queues
 *
 * **data queue**: copies the data into a flat_buffer
 * **ptr  queue**: a queue of items each pointing to some data in teh queue
 *
 * ## Double queue
 *
 * |---------> ptr_queue's front is used for transmission. So, direct ptr data is queued
 *             in the end of ptr_queue, to ensure that the order of push is same as order
 *             or write on wire
 * |~~~~~~~~~> data_queue provides storage for the owned data. So, the owned data is queued
 *             in the end of data_queue. To ensure that the order of push is same as order
 *             or write on wire, we push an item in the ptr_queue's back referencing the
 *             front of the data queue. As the ptr_queue gets cleared via pump's async loop
 *             at one point the ptr in teh ptr_queue referencing the front of the data_queue
 *             gets written to the wire. Then we pop the front of the data_queue. A subsequent
 *             call to pump will again enqueue the next front to the ptr_queue
 *
 * push_data:    copies data to an internal flat buffer to own it untill the sending finishes
 * enqueue_data: loads one data from the front of the data queue to teh back of the ptr queue
 * pop_data:     Once that item is sent, it is no longer necessary to keep that data in memory,
 *               so pop_data pops that buffer from the data queue
 */
struct buffer_queue{
    struct payload_in_flight{
        boost::asio::const_buffer buf;
        bool owned      = false;
        bool terminal   = false;
    };

    struct payload_at_rest{
        boost::beast::flat_buffer buf;
        bool enqueued = false;
    };

    using queue_type        = std::deque<payload_in_flight>;
    using buffer_queue_type = std::deque<payload_at_rest>;

public:

    /**
     * @brief copies data into a flat buffer and moves that buffer into the queue
     *        returns reference to the front of the queue.
     *
     * @note if the queue contains exectly one item only then the returned reference
     *       to buffer points to the last pushed data.
     *
     * @param data
     * @param size
     * @return size of the data queue
     */
    std::size_t push_data(const char* data, std::size_t size) {
        _dat_queue.emplace_back();
        payload_at_rest& par_back = _dat_queue.back();
        boost::beast::flat_buffer& buffer = par_back.buf;
        auto mutable_buffer = buffer.prepare(size);
        boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data, size));
        buffer.commit(size);
        std::cout << "push_data: " << size << std::endl;
        return _dat_queue.size();
    }

    std::size_t push_data(boost::beast::flat_buffer&& buffer) {
        std::cout << "push_data(buffer) : " << buffer.size() << std::endl;
        _dat_queue.emplace_back(payload_at_rest{std::move(buffer), false});
        return _dat_queue.size();
    }

    /**
     * @brief pops one buffer from the front of the data queue
     * @return
     */
    std::size_t pop_data() {
        std::cout << "pop_data: " << _dat_queue.size() << std::endl;
        if(_dat_queue.size() > 0) {
            _dat_queue.pop_front();
            return _dat_queue.size();
        }
        return 0;
    }

    /**
     * @brief copies one buffer from the front of the data queue and pushesh it back
     *        to the ptr queue
     * @return
     */
    std::size_t enqueue_data() {
        std::cout << "enqueue_data: |datQ|: " << _dat_queue.size() << " |ptrQ|: " << _ptr_queue.size() << std::endl;
        if(_dat_queue.size() == 0) {
            return 0;
        }
        payload_at_rest& par_front = _dat_queue.front();
        const boost::beast::flat_buffer& front = par_front.buf;
        if(!par_front.enqueued) {
            par_front.enqueued = true;
            _ptr_queue.push_back(payload_in_flight{front.data(), true, false});
            std::cout << "> enqueue_data: |datQ.front()|: " << front.size() << " |ptrQ|: " << _ptr_queue.size() << std::endl;
            return 1;
        } else {
            return 0;
        }
    }

    bool has_data() const { return !_dat_queue.empty(); }

public:
    bool has_ptr() const { return !_ptr_queue.empty(); }

    std::size_t push_ptr(const char* data, std::size_t size) {
        std::cout << "push_ptr: " << data << std::endl;
        _ptr_queue.emplace_back(payload_in_flight{boost::asio::const_buffer(data, size), false, false});
        return _ptr_queue.size();
    }

    std::size_t push_ptr() {
        std::cout << "push_ptr: " << std::endl;
        _ptr_queue.emplace_back(payload_in_flight{boost::asio::const_buffer(), false, true});
        return _ptr_queue.size();
    }

    /**
     * @brief pops the front of the ptr_queue
     * @return
     */
    payload_in_flight pop_ptr() {
        std::cout << "pop_ptr: " << _ptr_queue.size() << std::endl;
        assert(!_ptr_queue.empty());
        payload_in_flight p = _ptr_queue.front();
        _ptr_queue.pop_front();
        return p;
    }

private:
    queue_type                 _ptr_queue;
    buffer_queue_type          _dat_queue;
};

template <typename StreamT>
struct basic_queued_ostream: private detail::buffer_queue, private chunking_helper{
    using stream_type       = StreamT;
    using executor_type     = typename stream_type::executor_type;
    using strand_type       = boost::asio::strand<executor_type>;
    using encoding_type     = udho::net::types::transfer_encoding;
    using payload_type      = detail::buffer_queue::payload_in_flight;
    using completion_callback_type = std::function<void (boost::system::error_code, std::size_t)>;

    basic_queued_ostream(stream_type& stream, strand_type& strand, const encoding_type& encoding, completion_callback_type&& callback)
        : _stream(stream), _completion(std::move(callback)), _encoding(encoding), _strand(strand), _write_ongoing(false), _bytes_written(0), _finished(false), _paused(false), _eoq(false) {}

    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void write(T&& value) {
        boost::asio::dispatch(_strand, [this, val = std::move(value)]() {
            if(_eoq) return;
            boost::beast::flat_buffer buffer;
            boost::beast::ostream(buffer) << val;
            push_data(std::move(buffer));
            // _write_ongoing implies that the front of the _data_queue has not yet been popped (if last one was owned)
            // enqueue_data() will takes the front the of _data_queue to the back or the ptr_queue
            // However, that front might be under process because _write_ongoing is true.
            // Therefore, unconditional enqueue_data() might cause sending of same data twice
            // update: now enqueue_data tracks whether the data has been queued in ptr_queue or not
            enqueue_data();
            pump();
        });
    }

    /**
     * @brief write
     * @param str
     */
    void write(std::string&& str) {
        boost::asio::dispatch(_strand, [this, str = std::move(str)]() {
            if(_eoq) return;
            push_data(str.data(), str.size());
            enqueue_data();
            pump();
        });
    }

    void write(udho::utils::string_view str) {
        boost::asio::dispatch(_strand, [this, str]() {
            if(_eoq) return;
            push_ptr(str.data(), str.size());
            pump();
        });
    }

    /**
     * @brief write
     * @param data
     * @param size
     * @pre the data is expected to outlives the async write operation, this has to
     *      be ensured by the caller
     */
    void write(const char* data, std::size_t size) {
        boost::asio::dispatch(_strand, [this, data, size]() {
            if(_eoq) return;
            push_ptr(data, size);
            pump();
        });
    }

    void write(boost::beast::flat_buffer&& buffer) {
        boost::asio::dispatch(_strand, [this, buff = std::move(buffer)]() mutable {
            if(_eoq) return;
            push_data(std::move(buff));
            enqueue_data();
            pump();
        });
    }

    /**
     * @brief finish
     * Marks the stream as finished
     */
    void finish() {
        boost::asio::dispatch(_strand, [this]() {
            if(!_eoq) {
                _eoq = true;
                // push_ptr();
                pump();
            }
        });
    }

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
    void resume(bool state = true) { pause(!state); }

private:

    /**
     * @brief For chunked encoding will be called after the terminal chunk is sent
     * @param ec
     * @param bytes_written
     */
    void on_finish_cb(boost::system::error_code ec, std::size_t bytes_written) {
        _finished = true;
        if(_completion) {
            _completion(ec, bytes_written);
        }
    }

    /**
     * @brief if chunked encoding is used then sends the \r\n0\r\n byte sequence, otherwise calls on_finish_cb
     * Called from pump once it reaches the terminal payload
     */
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

    /**
     * @brief sends payload
     * @param p
     */
    void async_write_payload(payload_type p) {
        _write_ongoing = true;
        boost::asio::async_write(
            _stream, boost::asio::buffer(p.buf, p.buf.size()),
            boost::asio::bind_executor(_strand,
               [this, p](boost::system::error_code error, std::size_t bytes_written) {
                   _bytes_written += bytes_written;
                    if (p.owned) {
                        pop_data();
                    }
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
     * @brief sends chunked payload
     * @param p
     */
    void async_write_chunked_payload(payload_type p) {
        prepare(_ongoing_header_buffer, p.buf.size());

        std::array<boost::asio::const_buffer, 3> bufs = {
            _ongoing_header_buffer.data(),            // _ongoing_header_buffer is member variable
            p.buf,                                    // p.buff is kept alive in the data queue or the caller ensures lifetime
            boost::asio::buffer(_crlf)                // _crlf is member variable
        };
        _write_ongoing = true;
        boost::asio::async_write(
            _stream, std::move(bufs), // bufs is moved
            boost::asio::bind_executor( _strand,
                [this, p = std::move(p)](boost::system::error_code error, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
                    _ongoing_header_buffer.clear();
                    if (p.owned) {
                        pop_data();
                    }
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
     * @brief Alaways dequeues the _queue from the front and writes that to the wire, loops asynchronously until _queue becomes empty.
     * @param owning
     *
     * pump -> async_write_chunked_payload -> [if p.owned] pop_data -> pump
     *                                                              -> [error] on_finish_cb
     *      -> async_write_payload         -> [if p.owned] pop_data -> pump
     *                                                              -> [error] on_finish_cb
     */
    void pump() {
        if(_write_ongoing) return;              // once the ongoing write finishes it will comeback to process_queue again
        if(_paused) return;
        if(!has_ptr()) {
            if(has_data()) {
                _write_ongoing = true;
                enqueue_data();
            } else {
                if(_eoq) {
                    push_ptr();
                } else {
                    return;
                }
            }
        }

        payload_type p = pop_ptr();
        if(p.terminal) {
            // finish chunk has been pushed a while ago
            // all chunks have been written and only this
            // chunk is left. Therefore the stream is in
            // finished state
            on_finish();
            return;
        }

        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked)
            async_write_chunked_payload(p);
        else
            async_write_payload(p);
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

    basic_header_writer(stream_type& stream, strand_type& strand, const response_headers_type& headers, completion_callback_type&& callback)
        : _stream(stream), _strand(strand), _headers(headers), _response(_headers), _serializer(_response), _completion(std::move(callback)), _bytes_written(0), _started(false), _finished(false) {}

    void flush() {
        boost::asio::dispatch(_strand, [this]() {
            if (_started) {
                return;
            }
            _started = true;
            boost::beast::http::async_write_header(
                _stream, _serializer,
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

    bool started() const { return _started; }
    bool finished() const { return _finished; }

private:
    stream_type&                    _stream;
    strand_type&                    _strand;
    const response_headers_type&    _headers;
    response_type                   _response;
    serializer_type                 _serializer;
    completion_callback_type        _completion;
    std::size_t                     _bytes_written;
    bool                            _started;
    bool                            _finished;
};

}


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

    basic_ostream(stream_type& stream, const response_headers_type& headers, const encoding_type& encoding, completion_callback_type&& callback)
        : _stream(stream), _strand(stream.get_executor()), _headers(headers), _encoding(encoding), _buffering(true), _finishing(false)
        , _header_stream(stream, _strand, headers, std::bind(&ostream_type::on_header_completion, this, std::placeholders::_1, std::placeholders::_2))
        , _queued_stream(stream, _strand, encoding, std::bind(&ostream_type::on_queued_completion, this, std::placeholders::_1, std::placeholders::_2))
        , _buffered_stream(stream, _strand, encoding, std::bind(&ostream_type::on_buffered_completion, this, std::placeholders::_1, std::placeholders::_2))
        , _headers_sent(false), _bytes_written(0), _completion(std::move(callback))
    {
        _queued_stream.pause();
    }


    /**
     * @brief switcheds output stream from buffered to queued
     * @param flag
     */
    void disable_buffering() {
        boost::asio::post(_strand, [this](){
            if(!_buffering) return;
            else switch_stream();
        });
    }

public:
    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T>, bool> = true>
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
     * @brief no-copy write (usercode must ensure lifetime of the data)
     * @param data
     * @param size
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

    void finish() {
        boost::asio::post(_strand, [this](){
            if(_finishing) return;
            _finishing = true;
            if(!_headers_sent) {
                _header_stream.flush();
            }

            if(_buffering) {
                _buffered_stream.async_flush();
                if(_encoding.encoding() == net::types::transfer::encoding::chunked){
                    _buffered_stream.finish();
                }
            } else {
                _queued_stream.finish();
            }
        });
    }

private:

    /**
     * @brief will be called exactly once, buffering(bool) will throw exception otherwise
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

    void on_header_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _headers_sent = true;
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
    }

    void on_buffered_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
        else {
            if(_buffering) {
                if(_completion) {
                    _completion(ec, _bytes_written);
                }
            } else {
                _queued_stream.resume();
                // Any intermediate writes routed to the _queued_stream now gets
                // pumped out to the socket
            }
        }
    }

    void on_queued_completion(boost::system::error_code ec, std::size_t bytes_written) {
        _bytes_written += bytes_written;
        if(ec) on_error(ec);
        else {
            if(_completion) {
                _completion(ec, _bytes_written);
            }
        }
    }

    void on_error(boost::system::error_code ec) {
        _completion(ec, _bytes_written);
    }

private:
    stream_type&                 _stream;
    strand_type                  _strand;
    const response_headers_type& _headers;
    const encoding_type&         _encoding;
private:
    header_writer_type           _header_stream;
    queued_stream_type           _queued_stream;
    buffered_stream_type         _buffered_stream;
private:
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
