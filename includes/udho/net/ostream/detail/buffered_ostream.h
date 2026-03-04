#ifndef UDHO_NET_OSTREAM_DETAIL_BUFFERED_STREAM_H
#define UDHO_NET_OSTREAM_DETAIL_BUFFERED_STREAM_H

#include <iostream>
#include <udho/net/ostream/detail/chunking_helper.h>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/ostream.hpp>
#include <udho/net/common.h>
#include <udho/utils/traits.h>
#include <udho/utils/string_view.h>
#include <boost/asio/write.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>

namespace udho{
namespace net{

namespace detail{

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

    // void write(boost::iostreams::mapped_file_source&& mmaped_file) {
    //     boost::asio::post(_strand, [this, file = std::move(mmaped_file)]() mutable {
    //         if(_write_ongoing) {
    //             // error
    //             return;
    //         }
    //         auto mutable_buffer = _multibuff.prepare(file.size());
    //         boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(file.data(), file.size()));
    //         _multibuff.commit(file.size());
    //         file.close();
    //     });
    // }

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
            std::cout << "buffered async_flush" << std::endl;
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

    /**
     * @brief size of the internal multibuffer
     */
    std::size_t size() const {
        return _multibuff.size();
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


}

}
}

#endif // UDHO_NET_OSTREAM_DETAIL_BUFFERED_STREAM_H
