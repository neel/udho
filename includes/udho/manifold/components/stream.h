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

    basic_buffered_ostream(stream_type& stream, const encoding_type& encoding, completion_callback_type&& callback): _stream(stream), _completion(std::move(callback)), _encoding(encoding), _strand(_stream.get_executor()), _bytes_written(0), _write_ongoing(false) {}

    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void write(T&& value) {
        boost::asio::post(_strand, [this, val = std::move(value)]() {
            if(_write_ongoing) {
                // error
                return;
            }
            boost::beast::ostream(_multibuff) << val;
        });
    }

    void write(std::string&& str) {
        boost::asio::post(_strand, [this, str = std::move(str)]() {
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
        boost::asio::post(_strand, [this, str]() {
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
     * @pre data outlives the async write operation, garunteed by the caller
     * @warning if lifetime of data cannot be garunteed then use other overloads of write
     */
    void write(const char* data, std::size_t size) {
        // boost::asio::post will not in invoked immediately
        // if we copy the data immediately then will will not be synchronized with the strand
        // if we copy the data into a temporary buffer then it will lead to double copy
        // therefore usercode is responsible to ensure lifetime of this data
        // other overloads are provided that moves or copies the data
        boost::asio::post(_strand, [this, data, size]() {
            if(_write_ongoing) {
                // error
                return;
            }
            auto mutable_buffer = _multibuff.prepare(size);
            boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data, size));
            _multibuff.commit(size);
        });
    }

    void async_flush() {
        boost::asio::post(_strand, [this]() {
            _write_ongoing = true;
            async_write();
        });
    }

    void finish() {
        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            boost::asio::post(_strand, [this]() {
                async_write_terminal();
            });
        } else {
            on_finish_cb({}, _bytes_written);
        }
    }

private:
    void async_write() {
        if(_encoding.encoding() == udho::net::types::transfer::encoding::plain) {
            async_write_payload();
        } else if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            prepare(_ongoing_header_buffer, _multibuff.size());
            boost::asio::async_write(
                _stream, _ongoing_header_buffer.data(),
                boost::asio::bind_executor( _strand,
                    [this](boost::system::error_code ec, std::size_t bytes_written) {
                        _ongoing_header_buffer.clear();
                        _bytes_written += bytes_written;
                        if (!ec) {
                            async_write_payload();
                        } else {
                            on_finish_cb(ec, _bytes_written);
                        }
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
                    if(ec) {
                        on_finish_cb(ec, _bytes_written);
                    } else {
                        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
                            async_write_crlf();
                        } else {
                            on_finish_cb(ec, _bytes_written);
                        }
                    }
                }
            )
        );
    }

    void async_write_crlf() {
        boost::asio::async_write(
            _stream, boost::asio::buffer(_crlf, 2),
            boost::asio::bind_executor(_strand,
                [this](boost::system::error_code ec, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
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
    strand_type                _strand;
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
        std::cout << "enqueue_data: " << _dat_queue.size() << " " << _ptr_queue.size() << std::endl;
        if(_dat_queue.size() == 0) {
            return 0;
        }
        payload_at_rest& par_front = _dat_queue.front();
        const boost::beast::flat_buffer& front = par_front.buf;
        if(!par_front.enqueued) {
            par_front.enqueued = true;
            _ptr_queue.push_back(payload_in_flight{front.data(), true, false});
            std::cout << "> enqueue_data: " << front.size() << " " << _ptr_queue.size() << std::endl;
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

    basic_queued_ostream(stream_type& stream, const encoding_type& encoding, completion_callback_type&& callback): _stream(stream), _completion(std::move(callback)), _encoding(encoding), _strand(_stream.get_executor()), _write_ongoing(false), _bytes_written(0), _finished(false) {}

    template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void write(T&& value) {
        boost::asio::post(_strand, [this, val = std::move(value)]() {
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
        boost::asio::post(_strand, [this, str = std::move(str)]() {
            push_data(str.data(), str.size());
            enqueue_data();
            pump();
        });
    }

    void write(udho::utils::string_view str) {
        boost::asio::post(_strand, [this, str]() {
            push_ptr(str.data(), str.size());
            pump();
        });
    }

    /**
     * @brief write
     * @param data
     * @param size
     * @param owning
     * @pre if owning is fale then the the data is expected to outlives the async write operation, this has to
     *      be ensured by the caller
     */
    void write(const char* data, std::size_t size, bool owning) {
        if(owning){ // has to be owned immediately
            boost::beast::flat_buffer buffer;
            auto mutable_buffer = buffer.prepare(size);
            boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(data, size));
            buffer.commit(size);
            boost::asio::post(_strand, [this, buff = std::move(buffer), size]() mutable {
                push_data(std::move(buff));
                enqueue_data();
                pump();
            });
        } else {
            boost::asio::post(_strand, [this, data, size]() {
                push_ptr(data, size);
                pump();
            });
        }
    }

    /**
     * @brief finish
     * Marks the stream as finished
     */
    void finish() {
        if(_encoding.encoding() == udho::net::types::transfer::encoding::chunked) {
            boost::asio::post(_strand, [this]() {
                push_ptr();
                pump();
            });
        } else {
            on_finish_cb(boost::system::error_code{}, _bytes_written);
        }
    }

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
            on_finish_cb(boost::system::error_code{}, _bytes_written);
        }
    }


    void async_write_crlf() {
        boost::asio::async_write(
            _stream, boost::asio::buffer(_crlf, 2),
            boost::asio::bind_executor(_strand,
               [this](boost::system::error_code ec, std::size_t bytes_written) {
                   _bytes_written += bytes_written;
                   _write_ongoing = false;
                   if (ec) {
                       on_finish_cb(ec, _bytes_written);
                   } else {
                       pump();
                   }
               }
            )
        );
    }


    void async_write_payload(payload_type p, bool is_chunked) {
        boost::asio::async_write(
            _stream, boost::asio::buffer(p.buf, p.buf.size()),
            boost::asio::bind_executor(_strand,
               [this, is_chunked, p](boost::system::error_code ec, std::size_t bytes_written) {
                   _bytes_written += bytes_written;
                    if (p.owned) {
                        pop_data();
                    }
                    if (!ec) {
                        if(is_chunked) {
                           async_write_crlf();
                        } else {
                           _write_ongoing = false;
                           pump();
                        }
                    }
               }
            )
        );
    }

    void async_write_payload_start(payload_type p, bool is_chunked) {
        _write_ongoing = true;
        if(!is_chunked) {
            async_write_payload(p, false);
            return;
        }

        prepare(_ongoing_header_buffer, p.buf.size());

        boost::asio::async_write(
            _stream, _ongoing_header_buffer.data(),
            boost::asio::bind_executor( _strand,
                [this, p = std::move(p)](boost::system::error_code ec, std::size_t bytes_written) {
                    _bytes_written += bytes_written;
                    _ongoing_header_buffer.clear();
                    if (!ec) {
                        async_write_payload(p, true);
                    } else {
                        if (p.owned) {
                            pop_data();
                        }
                        on_finish_cb(ec, _bytes_written);
                    }
                }
            )
        );
    }

    /**
     * @brief Alaways dequeues the _queue from the front and writes that to the wire, loops asynchronously until _queue becomes empty.
     * @param owning
     *
     * write_to_wire -> write_to_wire_payload_init -> write_to_wire_payload -> write_to_wire_crlf -> write_to_wire
     *               -> return                                              -> write_to_wire
     */
    void pump() {
        if(_write_ongoing) return;              // once the ongoing write finishes it will comeback to process_queue again

        _write_ongoing = false;
        if(!has_ptr()) {
            if(has_data()) {
                _write_ongoing = true;
                enqueue_data();
            } else {
                return;
            }
        }

        const payload_type p = pop_ptr();
        if(p.terminal) {
            // finish chunk has been pushed a while ago
            // all chunks have been written and only this
            // chunk is left. Therefore the stream is in
            // finished state


            on_finish();
            return;
        }

        async_write_payload_start(p, _encoding.encoding() == udho::net::types::transfer::encoding::chunked);
    }
private:
    stream_type&               _stream;
    strand_type                _strand;
    boost::beast::flat_buffer  _ongoing_header_buffer;
    bool                       _write_ongoing;
    bool                       _finished;
    const encoding_type&       _encoding;
    std::size_t                _bytes_written;
    completion_callback_type   _completion;
};

}


}
}


#endif // UDHO_MANIFOLD_COMPONENTS_STREAM_H
