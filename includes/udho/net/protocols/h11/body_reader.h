#ifndef UDHO_NET_H11_BODY_READER_H
#define UDHO_NET_H11_BODY_READER_H

#include <map>
#include <memory>
#include <charconv>
#include <boost/beast/http/message.hpp>
#include <boost/asio/steady_timer.hpp>
#include <udho/net/protocols/multipart_parser.h>
#include <udho/net/protocols/h11/detail.h>
#include <udho/net/protocols/request_parser_config.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/beast/core/buffer_ref.hpp>
#include <udho/utils/encoding.h>
#include <boost/beast/core/buffers_to_string.hpp>
#include <udho/net/protocols/body_reader_result.h>
#include <udho/utils/misc.h>

namespace udho{
namespace net{
namespace protocols{

namespace h11{


/**
 * @brief Asynchronous HTTP/1.1 body reader supporting plain, chunked, and multipart/form-data bodies.
 *
 * This class reads the request body from a stream after the headers have been parsed.
 * It handles four types of payloads:
 *   - **Plain bodies** with a known `Content-Length`.
 *   - **Chunked bodies** (`Transfer-Encoding: chunked`), reassembling the data into a single buffer.
 *   - **Urlencoded form data** with a known `Content-Length` or chunked.
 *     Parsed fields are stored in the result `form_data` container and can be retrieved via `release(hbuff).form()`.
 *   - **Multipart/form-data** bodies, either plain (with Content-Length) or chunked.
 *     Parsed fields and files are stored in the result `form_data` container and can be retrieved via `release(hbuff).form()`.
 *     Files are streamed incrementally to temporary files or stored in-memory depending on config.upload_in_buffer()
 *
 * The reader enforces a total timeout and a size limit (when configured with non-zero values). The
 * Completion delivers only the status and byte count to the handler; the parsed results are obtained
 * by calling `release(hbuff)` after completion. The header buffer hbuff will then contain the leftover
 * bytes which can be used for parsing the next request.
 *
 * @tparam Buffer  The buffer type for the final body (e.g., `boost::beast::flat_buffer`). Must provide
 *                 prepare/commit/consume/data/size and be compatible with boost::beast::buffer_ref(...)
 * @tparam StreamT The stream type (e.g., `boost::asio::ip::tcp::socket` or
 *                 `boost::beast::test::stream`). Must satisfy `AsyncReadStream`.
 *
 * @plantumlfile h11_chunked.puml ["Chunk parsing state machine"]
 */
template <typename Buffer, typename StreamT>
struct body_reader: std::enable_shared_from_this<body_reader<Buffer, StreamT>> {
    using stream_type            = StreamT;
    using buffer_type            = Buffer;
    using http_request_type      = boost::beast::http::header<true, boost::beast::http::fields>;
    using timer_type             = boost::asio::steady_timer;
    using request_type           = http_request_type;
    using multipart_parser_type  = detail::multipart_parser<buffer_type>;
    using result_type            = udho::net::protocols::body_reader_result<Buffer>;
    using form_container_type    = detail::form_data::form_container_type;
    using trailer_container_type = std::multimap<std::string, std::string>;
    using config_type            = udho::net::detail::body_parser_config;

    /**
     * @brief Construct a body reader.
     * @param request          The HTTP request headers (used to determine content type, length, etc.).
     * @param stream           The underlying stream (must outlive the reader).
     * @param config           Parser configuration (timeout, size limits, upload storage policy).
     */
    body_reader(const request_type& request, stream_type& stream, const config_type& config)
        : _request(request), _stream(stream), _config(config) , _finished(false), _timer(_stream.get_executor()), _bytes_consumed(0), _bytes_received(0), _multipart(_result.form(), _config) {}

    /**
     * @brief start reading body
     * @param handler
     * @param hbuff reference to the buffer used to store the HTTP headers
     */
    template <typename Handler>
    void start(Handler&& handler, boost::beast::flat_buffer& hbuff){
        std::size_t content_length = 0;
        bool is_chunked = false;

        std::size_t count_content_length    = _request.count(boost::beast::http::field::content_length);
        std::size_t count_transfer_encoding = _request.count(boost::beast::http::field::transfer_encoding);

        if(count_content_length > 0 && count_transfer_encoding > 0) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), 0);
            return;
        } else if(count_content_length > 1 || count_transfer_encoding > 1) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), 0);
            return;
        } else if(count_content_length) {
            try{
                content_length = std::stoul(_request.at(boost::beast::http::field::content_length));
            } catch(const std::exception& ex) {
                finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::invalid_argument), 0);
                return;
            }
        } else if(count_transfer_encoding) {
            auto encoding = _request.at(boost::beast::http::field::transfer_encoding);
            is_chunked = encoding.contains("chunked");
        } else {
            count_content_length = 1;
            content_length = 0;
        }

        assert(count_content_length == 1 || is_chunked);

        // { sanity
        if(count_content_length && content_length == 0) {
            finished(std::move(handler), boost::system::error_code{}, 0);
            return;
        }

        if(count_content_length && _config.total_content_limit() > 0 && content_length > _config.total_content_limit()) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::value_too_large), 0);
            return;
        }
        // }

        std::string content_type = "application/octet-stream";
        if(_request.count(boost::beast::http::field::content_type))
            content_type = _request.at(boost::beast::http::field::content_type);

        bool is_multipart  = false;
        bool is_urlencoded = false;
        std::string boundary;

        auto range_multipart  = boost::ifind_first(content_type, "multipart/form-data");
        auto range_urlencoded = boost::ifind_first(content_type, "application/x-www-form-urlencoded");

        if(range_multipart.size() > 0) {
            udho::utils::string_view boundary_key("boundary=");
            auto range_boundary = boost::ifind_first(content_type, boundary_key);
            // std::size_t boundary_pos   = content_type.find(boundary_key);
            if(range_boundary.size() == 0) {
                finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), 0);
                return;
            }
            std::size_t boundary_start = std::distance(content_type.begin(), range_boundary.end()); // boundary_pos + boundary_key.size();
            std::size_t semicolon_pos  = content_type.find(';', boundary_start);
            std::size_t boundary_len   = semicolon_pos == std::string::npos ? std::string::npos : (semicolon_pos - boundary_start);
            std::string magic_sequence = content_type.substr(boundary_start, boundary_len);
            boost::trim(magic_sequence);
            boost::trim_if(magic_sequence, boost::is_any_of("\""));

            boundary = "--" + magic_sequence;
            is_multipart = true;
        } else if(range_urlencoded.size() > 0) {
            is_urlencoded = true;
        }

        if(count_content_length && content_length > 0) {
            if(_config.total_timeout().count() > 0) start_timer(_config.total_timeout());
            if(is_multipart) {
                read_multipart_body(std::move(handler), hbuff, boundary, content_length);
            } else {
                read_body(std::move(handler), hbuff, content_length, is_urlencoded);
            }
        } else {
            assert(is_chunked);
            if(_config.total_timeout().count() > 0) start_timer(_config.total_timeout());

            std::size_t transferred = transfer_leftovers(hbuff, _buffer);

            if(is_multipart) {
                read_chunked_multipart_header(std::move(handler), boundary);
            } else {
                read_chunk_header(std::move(handler), false, is_urlencoded);
            }
        }
    }

    /// @return `true` if the body has been fully processed (success or error).
    bool is_finished() const { return _finished; }

    /// @return A shared pointer to this object (used internally for async lifetimes).
    std::shared_ptr<body_reader> self() { return std::enable_shared_from_this<body_reader<Buffer, StreamT>>::shared_from_this(); }

    /**
     * @brief release the results obtained from the parser.
     * @warning release makes the body parser invalid. So no operation should be performed on the body_parser after it has been released
     * @return parser result
     */
    result_type release(boost::beast::flat_buffer& hbuff) {
        if (!_finished)
            throw std::logic_error("release() called before completion");

        // _buffer may contain parts of the next request
        detail::transfer_leftover<boost::beast::flat_buffer> transfer(hbuff);
        transfer(_buffer);

        result_type res = std::move(std::exchange(_result, {}));
        // multipart is not move constructible because it takes reference
        // so release operation makes multipart parser invalid
        // But release intentionally makes the body parser invalid
        return res;
    }
private:

    /**
     * @brief Read a plain (non‑multipart) body of known content length.
     * @param handler        Completion handler.
     * @param hbuff          Header buffer (leftovers already transferred).
     * @param content_length Total expected body size.
     *
     * @note If config.total_timeout() > 0, a timer is started; on expiry the underlying stream is
     *       terminated/cancelled and outstanding operations fail.
     */
    template <typename Handler>
    void read_body(Handler&& handler, boost::beast::flat_buffer& hbuff, std::size_t content_length, bool urlencoded) {
        std::size_t transferred = transfer_leftovers(hbuff, target_buffer(), content_length);
        _bytes_received += transferred;
        assert(content_length >= transferred);
        std::size_t pending_size = content_length - transferred;
        if(pending_size == 0) {
            boost::system::error_code error = {};
            if(urlencoded) {
                std::string buffer_str = boost::beast::buffers_to_string(target_buffer().data());
                _bytes_consumed += target_buffer().size();
                target_buffer().consume(target_buffer().size());
                error = parse_url_encoded_form(buffer_str);
            } else {
                _bytes_consumed += target_buffer().size();
            }

            finished(std::move(handler), error, _bytes_consumed);
            return;
        }
        boost::asio::async_read(
            _stream, boost::beast::buffer_ref(target_buffer()), boost::asio::transfer_exactly(pending_size),
            [self = self(), handler = std::move(handler), this, urlencoded](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                _bytes_received += bytes_transferred;
                if(ec) {
                    // _bytes_consumed may be 0 but nothing is "consumed" yet
                    // so passing 0 to the finished() callback is consistent
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }
                boost::system::error_code error = {};
                if(urlencoded) {
                    std::string buffer_str = boost::beast::buffers_to_string(target_buffer().data());
                    _bytes_consumed += target_buffer().size();
                    target_buffer().consume(target_buffer().size());
                    error = parse_url_encoded_form(buffer_str);
                } else {
                    _bytes_consumed += target_buffer().size();
                }

                self->finished(std::move(handler), error, _bytes_consumed);
            }
        );
    }

    /**
     * @brief Start reading a multipart/form-data body.
     * @param handler        Completion handler.
     * @param hbuff          Header buffer (leftovers already transferred).
     * @param boundary       The multipart boundary string (including leading "--").
     * @param content_length Total expected body size as given by Content-Length.
     */
    template <typename Handler>
    void read_multipart_body(Handler&& handler, boost::beast::flat_buffer& hbuff, std::string boundary, std::size_t content_length) {
        std::size_t transferred = transfer_leftovers(hbuff, target_buffer(), content_length);
        assert(content_length >= transferred);
        _bytes_received += transferred;
        _multipart(boundary);

        boost::system::error_code ec = _multipart(target_buffer());
        _bytes_consumed = _multipart.bytes_consumed();

        if(_bytes_consumed > content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::message_size), _bytes_consumed);
            return;
        }

        if((ec && (ec != boost::asio::error::would_block)) || _multipart.finished()) {
            finished(std::move(handler), ec, _bytes_consumed);
            return;
        }

        if(ec == boost::asio::error::would_block) {
            async_read_multipart(std::move(handler), target_buffer(), content_length);
            return;
        }
    }

private:

    boost::system::error_code parse_url_encoded_form(const std::string& data) {
        std::vector<std::string> parts;
        boost::split(parts, data, boost::is_any_of("&"));
        for(auto& part: parts) {
            if(part.empty()) continue;

            std::string key, val;
            auto eq_pos = part.find('=');

            key = part.substr(0, eq_pos);
            val = (eq_pos == std::string::npos) ? "" : part.substr(eq_pos+1);

            try{
                key = udho::utils::decode::url(key);
                val = udho::utils::decode::url(val);
            } catch(...) {
                return boost::system::errc::make_error_code(boost::system::errc::bad_message);
            }

            if(_config.field_content_limit() > 0 && val.size() > _config.field_content_limit()) {
                return boost::system::errc::make_error_code(boost::system::errc::value_too_large);
            }

            _result.form().emplace(key, detail::field_value_type(key, val));
        }
        return {};
    }

    /**
     * @brief Continue reading a multipart body by issuing `async_read_some`.
     * @param handler        Completion handler.
     * @param buffer         The buffer to read into (e.g. `target_buffer()`).
     * @param content_length Total body size.
     *
     * This function is called when the multipart parser returns `would_block`.
     * It reads more data from the stream and feeds it to the parser.
     */
    template <typename Handler>
    void async_read_multipart(Handler&& handler, buffer_type& buffer, std::size_t content_length){
        if (_bytes_received > content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        std::size_t remaining    = content_length - _bytes_received;
        std::size_t mbuffer_size = std::min<std::size_t>(256, remaining);
        std::size_t available    = udho::utils::misc::detail::stream_available<stream_type>::apply(_stream);

        if (available > 0)
            mbuffer_size = std::min(mbuffer_size, available);

        if(mbuffer_size == 0) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        auto mutable_buffer = buffer.prepare(mbuffer_size);

        _stream.async_read_some(mutable_buffer,
            [self = self(), handler = std::move(handler), &buffer, this, content_length](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                buffer.commit(bytes_transferred);
                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                ec = _multipart(buffer);
                _bytes_received += bytes_transferred;
                _bytes_consumed = _multipart.bytes_consumed();

                if(_bytes_consumed > content_length) {
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::message_size), _bytes_consumed);
                    return;
                }

                if((ec && (ec != boost::asio::error::would_block)) || _multipart.finished()) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                if(ec == boost::asio::error::would_block) {
                    async_read_multipart(std::move(handler), buffer, content_length);
                    return;
                }
            }
        );
    }

private:

    /**
     * @brief Read a chunk header line (e.g., "1F\r\n").
     * @param handler Completion handler.
     *
     * Parses the hexadecimal chunk size, ignoring chunk extensions. If size is zero, proceeds to trailers.
     */
    template <typename Handler>
    void read_chunk_header(Handler&& handler, bool is_multipart, bool urlencoded){
        boost::asio::async_read_until(
            _stream, boost::beast::buffer_ref(_buffer), "\r\n",
            [this, self = self(), handler = std::move(handler), is_multipart, urlencoded](boost::system::error_code error, std::size_t bytes_transferred) mutable {
                if(error) {
                    finished(std::move(handler), error, _bytes_consumed);
                    return;
                }

                assert(bytes_transferred > 2);
                _bytes_received += bytes_transferred;

                auto begin = static_cast<const char*>(_buffer.data().data());   // _buffer is flat_buffer
                auto end   = begin + (bytes_transferred -2);
                auto p     = begin;
                while (p != end && std::isxdigit(static_cast<unsigned char>(*p))) ++p;
                if (p == begin) {
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                    return;
                }
                std::size_t chunk_size = 0;
                auto chunk_size_result = std::from_chars(begin, p, chunk_size, 16);
                _buffer.consume(bytes_transferred);
                // _bytes_consumed not incremented intentionally

                if(chunk_size_result.ec != std::errc{}){
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                } else {
                    if(is_multipart && _multipart.finished()) {
                        if(chunk_size != 0) {
                            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                            return;
                        }
                    }

                    if(_config.total_content_limit() > 0 && target_buffer().size() + chunk_size > _config.total_content_limit()) {
                        finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::value_too_large), _bytes_consumed);
                        return;
                    }
                    read_chunk_payload(std::move(handler), chunk_size, is_multipart, urlencoded);
                }
            }
        );
    }

    /**
     * @brief Read a chunk payload (data + trailing CRLF).
     * @param handler     Completion handler.
     * @param chunk_size  Size of the chunk (from header).
     *
     * Copies the chunk data into `target_buffer()` and consumes the trailing CRLF.
     * Then reads the next chunk header.
     */
    template <typename Handler>
    void read_chunk_payload(Handler&& handler, std::size_t chunk_size, bool is_multipart, bool urlencoded){
        if(chunk_size == 0) {
            read_chunk_trailers(std::move(handler), urlencoded);
            return;
        }

        std::size_t bytes_needed    = chunk_size + 2;
        std::size_t bytes_stored    = _buffer.size();
        std::size_t bytes_expecting = (bytes_stored < bytes_needed) ? (bytes_needed - bytes_stored) : 0;

        boost::asio::async_read(
            _stream, boost::beast::buffer_ref(_buffer), boost::asio::transfer_exactly(bytes_expecting), // trailing \r\n
            [this, self = self(), chunk_size, handler = std::move(handler), is_multipart, urlencoded](boost::system::error_code error, std::size_t bytes_transferred) mutable {
                if(error) {
                    finished(std::move(handler), error, _bytes_consumed);
                    return;
                }

                _bytes_received += bytes_transferred;

                try {
                    auto mbuff = target_buffer().prepare(chunk_size);
                    boost::asio::buffer_copy(mbuff, _buffer.data());
                    target_buffer().commit(chunk_size);
                    _buffer.consume(chunk_size);
                    _bytes_consumed += chunk_size;

                    std::array<char,2> crlf{};
                    boost::asio::buffer_copy(boost::asio::buffer(crlf), boost::beast::buffers_prefix(2, _buffer.data()));
                    if(crlf[0] != '\r' || crlf[1] != '\n') {
                        finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                        return;
                    }

                    _buffer.consume(2);
                } catch(const std::bad_alloc&) {
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::not_enough_memory), _bytes_consumed);
                    return;
                } catch(...) {
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::not_enough_memory), _bytes_consumed);
                    return;
                }

                if(is_multipart) {
                    boost::system::error_code ec = _multipart(target_buffer());
                    _bytes_consumed = _multipart.bytes_consumed();

                    if(ec == boost::asio::error::would_block || _multipart.finished()) {
                        read_chunk_header(std::move(handler), true, urlencoded);
                        return;
                    }

                    if(ec) {
                        finished(std::move(handler), ec, _bytes_consumed);
                        return;
                    } else {
                        read_chunk_header(std::move(handler), true, urlencoded);
                        return;
                    }
                } else {
                    read_chunk_header(std::move(handler), is_multipart, urlencoded);
                }
            }
        );
    }

    /**
     * @brief Read chunk trailers (after the final zero‑length chunk).
     * @param handler Completion handler.
     *
     * Reads lines until an empty line is encountered, then finishes the body read.
     */
    template <typename Handler>
    void read_chunk_trailers(Handler&& handler, bool urlencoded){
        boost::asio::async_read_until(
            _stream, boost::beast::buffer_ref(_buffer), "\r\n",
            [this, self = self(), handler = std::move(handler), urlencoded](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if(ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                assert(bytes_transferred >= 2);
                _bytes_received += bytes_transferred;

                std::size_t trailer_size = bytes_transferred -2;
                if(trailer_size == 0) {
                    _buffer.consume(2);
                    boost::system::error_code error;
                    if(urlencoded) {
                        std::string buffer_str = boost::beast::buffers_to_string(target_buffer().data());
                        // _bytes_consumed += target_buffer().size(); // Already counted
                        target_buffer().consume(target_buffer().size());
                        error = parse_url_encoded_form(buffer_str);
                    }
                    finished(std::move(handler), error, _bytes_consumed);
                    return;
                } else {
                    const char* begin = static_cast<char const*>(_buffer.data().data());
                    std::string trailer(begin, trailer_size);
                    auto pos = trailer.find(':');
                    if (pos != std::string::npos) {
                        std::string key   = trailer.substr(0,pos);
                        std::string value = trailer.substr(pos+1);

                        boost::trim(key);
                        boost::trim(value);

                        _trailers.emplace(key, value); // multimap
                    }
                    _buffer.consume(trailer_size +2);
                    read_chunk_trailers(std::move(handler), urlencoded);
                }
            }
        );
    }

private:

    /**
     * @brief Start reading a chunked multipart body.
     * @param handler  Completion handler.
     * @param boundary The boundary string.
     *
     * Initializes the multipart parser and begins reading chunk headers.
     */
    template <typename Handler>
    void read_chunked_multipart_header(Handler&& handler, std::string boundary){
        _multipart(boundary);
        read_chunk_header(std::move(handler), true, false);
    }


private:

    /**
     * @brief Transfer up to `content_length` bytes from the header buffer into a target buffer.
     * @param hbuff          Header buffer.
     * @param buff           Target buffer (either `_buffer` for multipart or `target_buffer()` for plain).
     * @param content_length Maximum bytes to transfer.
     * @return Number of bytes actually transferred.
     */
    template <typename TargetBuffer>
    std::size_t transfer_leftovers(boost::beast::flat_buffer& hbuff, TargetBuffer& buff, std::size_t content_length) {
        detail::transfer_leftover transfer(buff);
        transfer(hbuff, content_length);
        return transfer.bytes_transferred();
    }

    /**
     * @brief Transfer all remaining data from the header buffer into a target buffer.
     * @param hbuff Header buffer.
     * @param buff  Target buffer.
     * @return Number of bytes transferred.
     */
    template <typename TargetBuffer>
    std::size_t transfer_leftovers(boost::beast::flat_buffer& hbuff, TargetBuffer& buff) {
        detail::transfer_leftover transfer(buff);
        transfer(hbuff);
        return transfer.bytes_transferred();
    }

private:

    /**
     * @brief Start the total timeout timer.
     * @param seconds Number of seconds before timeout.
     */
    void start_timer(std::chrono::seconds seconds) {
        _timer.expires_after(seconds);
        _timer.async_wait([self = self()](boost::system::error_code ec) {
            if (!ec) {
                self->_timeout();
            }
        });
    }

    /// Timeout handler: forcibly terminates the stream.
    void _timeout(){
        udho::utils::misc::detail::stream_termination<StreamT>::apply(_stream);
    }

private:
    buffer_type& target_buffer() {
        return _result.buffer();
    }

    /**
     * @brief Final completion function – cancels timer, invokes user handler, marks finished.
     * @param handler           User completion handler.
     * @param ec                Error code (or success).
     * @param bytes_transferred Total bytes processed from the socket.
     */
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        if(_finished) return;
        _finished = true;
        _timer.cancel();
        handler(ec, bytes_transferred);
    }
private:
    const request_type&         _request;
    stream_type&                _stream;
    config_type                 _config;
    boost::beast::flat_buffer   _buffer;
    bool                        _finished;
    std::size_t                 _bytes_consumed;
    std::size_t                 _bytes_received;
    timer_type                  _timer;
    result_type                 _result;
    multipart_parser_type       _multipart;
    trailer_container_type      _trailers;
};

}

}
}
}

#endif // UDHO_NET_H11_BODY_READER_H
