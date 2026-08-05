#ifndef UDHO_NET_OSTREAM_DETAIL_QUEUED_STREAM_H
#define UDHO_NET_OSTREAM_DETAIL_QUEUED_STREAM_H

#include <iostream>
#include <udho/net/ostream/detail/buffer_queue.h>
#include <udho/net/ostream/detail/chunking_helper.h>
#include <boost/asio/strand.hpp>
#include <udho/net/common.h>
#include <udho/utils/traits.h>
#include <udho/utils/string_view.h>
#include <boost/beast/core/ostream.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/bind_executor.hpp>

namespace udho{
namespace net{

namespace detail{

/** @addtogroup DoxyG_net
 *  @{
 */

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
 * ### Thread safety
 *
 * All public methods dispatch onto the strand; safe to call from any thread.
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
            if(buffer.size() == 0) return;
            push_data(std::move(buffer));
            pump();
        });
    }

    /// @brief Write owned string (copied into owned queue).
    void write(std::string&& str) {
        boost::asio::dispatch(_strand, [this, str = std::move(str)]() {
            if(_eoq) return;
            if(str.size() == 0) return;
            push_data(str.data(), str.size());
            pump();
        });
    }

    /// @brief Write borrowed view (caller must ensure lifetime).
    void write(udho::utils::string_view str) {
        boost::asio::dispatch(_strand, [this, str]() {
            if(_eoq) return;
            if(str.size() == 0) return;
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
            if(size == 0) return;
            push_ptr(data, size);
            pump();
        });
    }

    /// @brief Write owned flat_buffer (moved into owned queue).
    void write(boost::beast::flat_buffer&& buffer) {
        boost::asio::dispatch(_strand, [this, buff = std::move(buffer)]() mutable {
            if(_eoq) return;
            if(buff.size() == 0) return;
            push_data(std::move(buff));
            pump();
        });
    }

    // void write(boost::iostreams::mapped_file_source&& mmaped_file) {
    //     boost::asio::post(_strand, [this, file = std::move(mmaped_file)]() mutable {
    //         if(_eoq) return;
    //         push_data(file.data(), file.size());
    //         file.close();
    //         pump();
    //     });
    // }

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
                std::cout << "resuming" << std::endl;
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
     * @param ec error code if reset is called after some error occured
     */
    void reset(boost::system::error_code ec = {}) {
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


/** @} */

}

}
}

#endif // UDHO_NET_OSTREAM_DETAIL_QUEUED_STREAM_H
