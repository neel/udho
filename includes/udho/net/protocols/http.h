#ifndef UDHO_NET_PROTOCOL_HTTP_H
#define UDHO_NET_PROTOCOL_HTTP_H


#include <iostream>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <udho/net/common.h>
#include <boost/beast/core/static_buffer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/utils/string_view.h>
// #include <boost/charconv.hpp>
#include <charconv>
#include <boost/system/error_code.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/system_error.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <deque>
#include <udho/utils/encoding.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/system/system_error.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/asio/buffer.hpp>

namespace udho{
namespace net{
namespace protocols{

namespace detail{

template <typename StreamT>
struct stream_termination{
    static boost::system::error_code apply(StreamT& stream) {
        boost::system::error_code error;
        stream.cancel(error);
        return error;
    }
};

template <>
struct stream_termination<boost::beast::test::stream>{
    static boost::system::error_code apply(boost::beast::test::stream& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
    }
};

}

template <typename StreamT>
struct h11_header_reader{
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using opt_http_req_parser_type  = std::optional<http_request_parser_type>;
    using stream_type               = StreamT;
    using timer_type                = boost::asio::steady_timer;

    h11_header_reader(stream_type& stream, boost::beast::flat_buffer& buffer)
        : _stream(stream), _buffer(buffer), _finished(false), _timer(_stream.get_executor()) {}

    /**
     * @brief start reading HTTP headers asynchronously
     * @param handler must take the following arguments (udho::net::types::headers::request&& request, std::error_code ec, std::size_t bytes_transferred)
     * @param seconds
     */
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _parser.emplace();
        assert(_parser.has_value());
        start_timer(seconds);
        boost::beast::http::async_read_header(
            _stream, _buffer, parser(),
            [this, handler = std::move(handler)] (boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                assert(_parser.has_value());
                finished(std::move(handler), ec, bytes_transferred);
            }
        );
    }

    bool is_finished() const { return _finished; }

    http_request_parser_type& parser() { return *_parser; }

private:
    void start_timer(std::size_t seconds) {
        _timer.expires_after(std::chrono::seconds(seconds));
        _timer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                _timeout();
            }
        });
    }

    void _timeout(){
        detail::stream_termination<StreamT>::apply(_stream);
    }

private:
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        if(!ec){
            assert(_parser.has_value());
            _request = std::move(_parser->release());
        }
        // _request is empty in case of error but it exists and it is legal to move it
        std::cout << "h finished: " << ec.message() << std::endl;
        handler(std::move(_request), ec, bytes_transferred);
        _finished = true;
    }

private:
    udho::net::types::headers::request  _request; // gets moved in finished
    stream_type&                        _stream;
    boost::beast::flat_buffer&          _buffer;
    opt_http_req_parser_type            _parser;
    bool                                _finished;
    timer_type                          _timer;
};

namespace detail{

template <typename Buffer>
struct transfer_leftover{
    using target_buffer_type = Buffer;

    transfer_leftover(target_buffer_type& target): _target(target), _bytes_transferred(0) {}

    target_buffer_type& operator()(boost::beast::flat_buffer& source, std::size_t content_length) {
        auto src   = source.data();
        auto limit = std::min(content_length, source.size());
        _target.commit(boost::asio::buffer_copy(_target.prepare(limit), src));
        source.consume(limit);
        _bytes_transferred = limit;
        return _target;
    }

    target_buffer_type& operator()(boost::beast::flat_buffer& source) {
        auto src   = source.data();
        auto limit = source.size();
        _target.commit(boost::asio::buffer_copy(_target.prepare(limit), src));
        source.consume(limit);
        _bytes_transferred = limit;
        return _target;
    }

    std::size_t bytes_transferred() const { return _bytes_transferred; }
private:
    target_buffer_type& _target;
    std::size_t         _bytes_transferred;
};

}

namespace detail{

/**
 * @brief The beast_buffer_ref class
 * Satisfies https://www.boost.org/doc/libs/latest/doc/html/boost_asio/reference/DynamicBuffer_v1.html
 */
template <typename BeastBuffer>
class beast_buffer_ref {
public:
    using const_buffers_type = typename BeastBuffer::const_buffers_type;
    using mutable_buffers_type = typename BeastBuffer::mutable_buffers_type;

    explicit beast_buffer_ref(BeastBuffer& buf) : _buffer(buf) {}
    beast_buffer_ref(beast_buffer_ref&& other): _buffer(other._buffer) {}

    std::size_t size() const { return _buffer.size(); }
    std::size_t max_size() const { return _buffer.max_size(); }
    std::size_t capacity() const { return _buffer.capacity(); }

    // { v1
    const_buffers_type data() const { return _buffer.data(); }

    mutable_buffers_type prepare(std::size_t n) { return _buffer.prepare(n); }
    void commit(std::size_t n) { _buffer.commit(n); }
    void consume(std::size_t n) { _buffer.consume(n); }
    // }
private:
    BeastBuffer& _buffer; // Reference to the actual storage
};

}

template <typename Buffer, typename StreamT>
struct h11_body_reader: std::enable_shared_from_this<h11_body_reader<Buffer, StreamT>> {
    using stream_type           = StreamT;
    using buffer_type           = Buffer;
    using timer_type            = boost::asio::steady_timer;
    using request_type          = udho::net::types::headers::request;
    using field_value_type      = std::variant<std::string, boost::filesystem::path>;
    using form_container_type   = std::multimap<std::string, field_value_type>;
    using form_iterator         = form_container_type::iterator;

    h11_body_reader(const udho::net::types::headers::request& request, stream_type& stream, std::size_t buffer_capacity = std::allocator_traits<typename Buffer::allocator_type>::max_size(typename Buffer::allocator_type{}))
        : _request(request), _stream(stream), _buffer(buffer_capacity), _target_buffer(buffer_capacity), _finished(false), _timer(_stream.get_executor()), _bytes_consumed(0) {}

    /**
     * @brief start reading body
     * @param handler
     * @param hbuff reference to the buffer used to store the HTTP headers
     * @param seconds total seconds to spend until the HTTP request body is read from the socket
     * @param limit max number of bytes permitted for HTTP request body
     *
     * @note HTTP request body size must be finite. In order to enfore finiteness the usercode is expected to
     *       provide an explicite limit that the request must obey. Therefore, the framework intentionally
     *       doesn't allow unlimitedly large payload for HTTP requests. This helps enforcing determinism and
     *       reducing chances of vulnerabilities.
     *
     * @warning limit = 0 doesn't have any special meaning. if limit is set to 0 then dosn't expect any body
     */
    template <typename Handler>
    void start(Handler&& handler, boost::beast::flat_buffer& hbuff, std::size_t seconds, std::size_t limit = 0){
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

        if(count_content_length && content_length > limit) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::value_too_large), 0);
            return;
        }
        // }

        std::string content_type = "application/octet-stream";
        if(_request.count(boost::beast::http::field::content_type))
            content_type = _request.at(boost::beast::http::field::content_type);

        bool is_multipart = false;
        std::string boundary;

        if(content_type.find("multipart/form-data") != std::string::npos) {
            udho::utils::string_view boundary_key("boundary=");
            std::size_t boundary_pos   = content_type.find(boundary_key);
            if(boundary_pos == std::string::npos) {
                finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), 0);
                return;
            }
            std::size_t boundary_start = boundary_pos + boundary_key.size();
            std::size_t semicolon_pos  = content_type.find(';', boundary_start);
            std::size_t boundary_len   = semicolon_pos == std::string::npos ? std::string::npos : (semicolon_pos - boundary_start);
            std::string magic_sequence = content_type.substr(boundary_start, boundary_len);
            boost::trim(magic_sequence);
            boost::trim_if(magic_sequence, boost::is_any_of("\""));

            boundary = "--" + magic_sequence;
            is_multipart = true;
        }

        if(count_content_length && content_length > 0) {
            start_timer(seconds);
            if(is_multipart) {
                read_multipart_body(std::move(handler), hbuff, boundary, content_length);
            } else {
                read_body(std::move(handler), hbuff, content_length);
            }
        } else {
            assert(is_chunked);
            start_timer(seconds);

            std::size_t transferred = transfer_leftovers(hbuff, _buffer);

            if(is_multipart) {
                // read_chunked_multipart_header(std::move(handler));
            } else {
                read_chunk_header(std::move(handler));
            }
        }
    }

    bool is_finished() const { return _finished; }

    std::shared_ptr<h11_body_reader> self() { return std::enable_shared_from_this<h11_body_reader<Buffer, StreamT>>::shared_from_this(); }

    const form_container_type& fields() const { return _fields; }
private:

    /**
     * @brief Read a plain (non‑multipart) body of known content length.
     * @param handler        Completion handler.
     * @param hbuff          Header buffer (leftovers already transferred).
     * @param content_length Total expected body size.
     *
     * @note if content_length bytes are not received before the timer
     *       expires the operation will be cancelled.
     */
    template <typename Handler>
    void read_body(Handler&& handler, boost::beast::flat_buffer& hbuff, std::size_t content_length) {
        std::size_t transferred = transfer_leftovers(hbuff, _target_buffer, content_length);
        assert(content_length >= transferred);
        std::size_t pending_size = content_length - transferred;
        if(pending_size == 0) {
            finished(std::move(handler), boost::system::error_code{}, transferred);
            return;
        }
        boost::asio::async_read(
            _stream, detail::beast_buffer_ref(_target_buffer), boost::asio::transfer_exactly(pending_size),
            [self = self(), handler = std::move(handler), this](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                self->finished(std::move(handler), ec, _target_buffer.size());
            }
        );
    }

    /**
     * @brief Start reading a multipart/form-data body.
     * @param handler        Completion handler.
     * @param hbuff          Header buffer (leftovers already transferred).
     * @param boundary       The multipart boundary string (including leading "--").
     * @param content_length Total body size as given by Content-Length.
     */
    template <typename Handler>
    void read_multipart_body(Handler&& handler, boost::beast::flat_buffer& hbuff, std::string boundary, std::size_t content_length) {
        std::size_t transferred = transfer_leftovers(hbuff, _buffer, content_length);
        assert(content_length >= transferred);
        _bytes_consumed += transferred;
        read_plain_multipart_header(std::move(handler), boundary, true, content_length);
        // passing content_length instead of (content_length - transferred)
        // because async_read_* will read from the _buffer as well as from the _stream
        // content_length accounts for both of them
    }

private:

    /**
     * @brief Read the next part’s boundary line.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param is_first       True for the first part (boundary appears alone), false otherwise (preceded by CRLF).
     * @param content_length Total body size (used for progress tracking).
     *
     * async_read_until --boundary if first, CRLF--boundary otherwise
     * calls read_plain_multipart_follow which checks the ending of that line
     */
    template <typename Handler>
    void read_plain_multipart_header(Handler&& handler, std::string boundary, bool is_first, std::size_t content_length) {
        if(_bytes_consumed >= content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        std::string expected_delim = (is_first ? boundary : "\r\n"+boundary);
        boost::asio::async_read_until(
            _stream, detail::beast_buffer_ref(_buffer), expected_delim,
            [self = self(), handler = std::move(handler), this, boundary, content_length](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                std::size_t remaining_bytes = content_length - _bytes_consumed;

                // expecting exactly remaining_bytes from here
                // if bytes_transferred > remaining_bytes then async_read_until must have
                // found the delimiter by going beyond the first request which implies
                // that the request one was invalid. Because, at this point the delimiter
                // was expected which was not found within the request 1.

                if(bytes_transferred > remaining_bytes) {
                    // remaining (promised in Content-Type header) bytes of the current request
                    // is consumed so that the next request headers can be read properly
                    // Given that remaining_bytes < bytes_transferred and alreday bytes_transferred
                    // bytes are available to be consumed implies that remaining_bytes bytes
                    // can definitely be consumed.
                    _buffer.consume(remaining_bytes);
                    _bytes_consumed += remaining_bytes;
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                    return;
                }

                assert(bytes_transferred <= remaining_bytes);

                std::size_t consumable_bytes = bytes_transferred;

                _buffer.consume(consumable_bytes);
                _bytes_consumed += consumable_bytes;
                read_plain_multipart_follow(std::move(handler), boundary, content_length);
            }
        );
    }

    /**
     * @brief completes reading the multipart part header line
     * expects to be called asynchronously from read_plain_multipart_header
     * which implies that either --boundary if first, CRLF--boundary otherwise
     * have been read already.
     *
     * This function confirms that teh boundary string is not a coincedence
     * by checking the next two bytes that follow. The next two bytes are
     * expected to be either -- (in case of last part) or CRLF.
     *
     * If next two bytes are -- then read_plain_multipart_final is called
     * asynchronously.
     *
     * Otherwise calls read_plain_multipart_meta asynchronously.
     *
     * @param handler
     * @param boundary
     * @param pending_size
     */
    template <typename Handler>
    void read_plain_multipart_follow(Handler&& handler, std::string boundary, std::size_t content_length) {
        if(_bytes_consumed +2 > content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        std::size_t buffer_size   = _buffer.size();
        std::size_t transfer_size = buffer_size >= 2 ? 0 : (2-buffer_size);

        boost::asio::async_read(
            _stream, detail::beast_buffer_ref(_buffer), boost::asio::transfer_exactly(transfer_size),
            [self = self(), handler = std::move(handler), this, boundary, content_length, transfer_size](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                std::string two_bytes(static_cast<const char*>(_buffer.data().data()), 2);
                assert(bytes_transferred == transfer_size);
                _buffer.consume(2);
                _bytes_consumed += 2;
                if(two_bytes == "--") {
                    read_plain_multipart_final(std::move(handler), content_length);
                } else {
                    if(two_bytes == "\r\n") {
                        read_plain_multipart_meta(std::move(handler), boundary, content_length);
                    } else {
                        finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                    }
                }
            }
        );
    }

    /**
     * @brief Read the final CRLF after the closing boundary ("--") and finish.
     * @param handler        Completion handler.
     * @param content_length Total body size.
     */
    template <typename Handler>
    void read_plain_multipart_final(Handler&& handler, std::size_t content_length) {
        if(_bytes_consumed +2 > content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        std::size_t buffer_size   = _buffer.size();
        std::size_t transfer_size = buffer_size >= 2 ? 0 : (2-buffer_size);
        boost::asio::async_read(
            _stream, detail::beast_buffer_ref(_buffer), boost::asio::transfer_exactly(transfer_size),
            [self = self(), handler = std::move(handler), this, content_length](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                udho::utils::string_view expected_str = "\r\n";
                const char* data  = static_cast<const char*>(_buffer.data().data());
                udho::utils::string_view buffer_data(data, 2);

                if(expected_str != buffer_data) {
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                    return;
                }

                _buffer.consume(2);
                _bytes_consumed += 2;
                finished(std::move(handler), ec, _bytes_consumed);
            }
        );
    }

    template <typename Handler>
    void read_plain_multipart_meta_buffered(Handler&& handler, std::string boundary, std::size_t content_length, std::size_t length) {
        std::string meta(static_cast<const char*>(_buffer.data().data()), length);
        _buffer.consume(length);
        _bytes_consumed += length;

        auto range = boost::ifind_first(meta, "Content-Disposition:");
        if(range.empty()) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        std::size_t disposition_pos = std::distance(meta.begin(), range.end());
        std::size_t crlf_pos        = meta.find("\r\n", disposition_pos);       // must exist because we are using async_read_until with "\r\n\r\n"
        std::string disposition     = meta.substr(disposition_pos, (crlf_pos - disposition_pos));

        std::deque<std::string> disposition_parts;
        boost::split(disposition_parts, disposition, boost::is_any_of(";"));
        std::string form_data = disposition_parts.front();
        boost::trim(form_data);
        if(!boost::iequals(form_data, "form-data")) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }
        disposition_parts.pop_front();

        std::string name;
        std::string filename;

        for (std::string& disposition_part : disposition_parts) {
            auto pos = disposition_part.find('=');
            if (pos == std::string::npos)
                continue;
            std::string key   = disposition_part.substr(0,pos);
            std::string value = disposition_part.substr(pos+1);

            boost::trim(key);
            boost::trim(value);
            boost::trim_if(value, boost::is_any_of("\""));

            if (boost::iequals(key, "name")) name = value;
            if (boost::iequals(key, "filename")) filename = value;
        }

        if(name.empty()) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        if(filename.empty()) {
            // not upload -> append to a map
            read_plain_multipart_field(std::move(handler), boundary, name, content_length);
        } else {
            // upload -> redirect stream to temp file
            read_plain_multipart_file(std::move(handler), boundary, name, filename, content_length);
        }
    }

    /**
     * @brief Read the part headers (e.g., Content-Disposition) until CRLFCRLF.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param content_length Total body size.
     *
     * Parses the `Content-Disposition` header to extract the field name and optional filename.
     * Then dispatches to either field reading or file reading.
     */
    template <typename Handler>
    void read_plain_multipart_meta(Handler&& handler, std::string boundary, std::size_t content_length) {
        if(_bytes_consumed >= content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        boost::asio::async_read_until(
            _stream, detail::beast_buffer_ref(_buffer), "\r\n\r\n",
            [self = self(), handler = std::move(handler), this, boundary, content_length](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                std::size_t remaining_bytes = content_length - _bytes_consumed;

                // expecting exactly remaining_bytes from here
                // if bytes_transferred > remaining_bytes then async_read_until must have
                // found the delimiter by going beyond the first request which implies
                // that the request one was invalid. Because, at this point the delimiter
                // was expected which was not found within the request 1.

                if(bytes_transferred > remaining_bytes) {
                    _buffer.consume(remaining_bytes);
                    _bytes_consumed += remaining_bytes;
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
                    return;
                }

                assert(bytes_transferred <= remaining_bytes);

                read_plain_multipart_meta_buffered(std::move(handler), boundary, content_length, bytes_transferred);
            }
        );
    }

    /**
     * @brief Process data already present in the buffer for a field.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param field_it       Iterator into `_fields`.
     * @param content_length Total body size.
     *
     * Scans the buffer for the next boundary. If found and verified, consumes data up to the boundary
     * and then calls `read_plain_multipart_header()` to start the next part.
     * If not found, consumes all data except a trailing safety margin (to avoid missing a partial boundary)
     * and continues reading by calling `read_plain_multipart_field_readsome()`.
     */
    template <typename Handler>
    void read_plain_multipart_field_readsome_buffered(Handler&& handler, std::string boundary, form_iterator field_it, std::size_t content_length) {
        std::string delim_str = "\r\n"+boundary;

        const char* data  = static_cast<const char*>(_buffer.data().data());
        const char* begin = data;
        const char* end   = data + _buffer.size();

        bool boundary_not_found = false;
        auto it = begin;
        while(true) {
            it = std::search(it, end, delim_str.begin(), delim_str.end());
            // possible values of it
            //  a) end                                                      # if not matched
            //  b) it <= x < end where distance(x, end) >= delim_str.size() # if matched

            // in case of (b) delim_str.size() > 2
            // therefore distance(x, end) >= 2
            // in case of (a) distance(x, end) == 0

            // check next two characters after delim_str
            // if it is \r\n or -- then real boundary is
            // found, otherwise it is not

            if(it == end) {                                 // case (a)
                boundary_not_found = true;                  // assignment 1T
                break;
            } else {                                        // case (b)
                auto delim_end = it + delim_str.size();     // legal because of (b)
                std::size_t leftover_size = std::distance(delim_end, end);
                if(leftover_size < 2) {
                    boundary_not_found = true;              // assignment 2T
                    // break because delim_str.size() > 2
                    // repeating the loop wont take us anywhere
                    break;
                } else {
                    udho::utils::string_view two_chars(delim_end, 2);
                    if(two_chars == "\r\n" || two_chars == "--") {
                        boundary_not_found = false;         // assignment 1F
                        break;
                    } else {
                        // unintended boundary
                        boundary_not_found = true;          // assignment 3T
                    }
                    std::advance(it, 1);
                }
            }
        }

        // We did not check for partial matches with the boundary
        // We leave that for the next run of async_read_some
        // Therefore we leave (delim_str.size()-1) bytes in the buffer
        // However, if distance(begin, it) < (delim_str.size()-1)
        // then we leave the whole for the next run if boundary_not_found
        if(boundary_not_found) {
            std::size_t safe_partial_leftover_size = std::min<std::size_t>((delim_str.size()-1), std::distance(begin, it));
            it = it - safe_partial_leftover_size;
        }

        std::size_t length = std::distance(begin, it);

        if(length > content_length - _bytes_consumed) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        assert(_bytes_consumed <= content_length - length);

        if(length > 0) {
            auto& value = field_it->second;
            std::string& str = std::get<std::string>(value);
            str.append(begin, length);
            _buffer.consume(length);
            _bytes_consumed += length;
        }

        assert(_bytes_consumed <= content_length);

        if(boundary_not_found) {
            read_plain_multipart_field_readsome(std::move(handler), boundary, field_it, content_length);
        } else {
            // case 1F
            // we have already consumed length
            // but length is distance(begin, it)
            // and it is the begining of delim_str
            // But delim_str should be consumed by read_plain_multipart_header
            // so here we don't consume the delim_str.size()

            read_plain_multipart_header(std::move(handler), boundary, false, content_length);
        }
    }

    /**
     * @brief Read more data for a text field, possibly using already buffered data.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param field_it       Iterator into `_fields` pointing to the current field’s value.
     * @param content_length Total body size.
     *
     * If the internal buffer already contains data, it calls `read_plain_multipart_field_readsome_buffered`; otherwise it issues
     * `async_read_some` to fill the buffer and then calls `read_plain_multipart_field_readsome_buffered`.
     */
    template <typename Handler>
    void read_plain_multipart_field_readsome(Handler&& handler, std::string boundary, form_iterator field_it, std::size_t content_length) {
        if(_bytes_consumed >= content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        if(_buffer.size() > 0) {
            read_plain_multipart_field_readsome_buffered(std::move(handler), boundary, field_it, content_length);
            return;
        }

        std::size_t mbuffer_size = std::min<std::size_t>(256, content_length - _bytes_consumed);
        auto mutable_buffer      = _buffer.prepare(mbuffer_size);

        _stream.async_read_some(mutable_buffer,
            [self = self(), handler = std::move(handler), this, boundary, field_it, content_length](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                _buffer.commit(bytes_transferred);
                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                read_plain_multipart_field_readsome_buffered(std::move(handler), boundary, field_it, content_length);
            }
        );
    }

    /**
     * @brief Start reading a field part.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param name           Field name (from Content-Disposition).
     * @param content_length Total body size.
     */
    template <typename Handler>
    void read_plain_multipart_field(Handler&& handler, std::string boundary, std::string name, std::size_t content_length) {
        if(_bytes_consumed >= content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        std::string value;
        form_iterator field_it = _fields.emplace(name, std::move(value));
        // call originates from read_plain_multipart_meta
        // which consumes the mata part from the buffer
        // including the \r\n\r\n
        // So, we expect _buffer to be empty at this point
        // However, async_read_until may read past \r\n\r\n
        read_plain_multipart_field_readsome(std::move(handler), boundary, field_it, content_length);
    }

    /**
     * @brief Process buffered data for a file part.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param field_it       Iterator into `_fields`.
     * @param stream         Owning pointer to the output file stream.
     * @param content_length Total body size.
     *
     * Scans the buffer for the next boundary. If found, writes data up to the boundary to the file,
     * flushes and closes the stream, and proceeds to the next part.
     * If not found, writes all but a safety margin to the file and continues reading.
     * Checks the stream state after each write.
     */
    template <typename Handler>
    void read_plain_multipart_file_readsome_buffered(Handler&& handler, std::string boundary, form_iterator field_it, std::unique_ptr<std::ofstream>&& stream, std::size_t content_length) {
        std::string delim_str = "\r\n"+boundary;

        const char* data  = static_cast<const char*>(_buffer.data().data());
        const char* begin = data;
        const char* end   = data + _buffer.size();

        bool boundary_not_found = false;
        auto it = begin;
        while(true) {
            it = std::search(it, end, delim_str.begin(), delim_str.end());
            // possible values of it
            //  a) end                                                      # if not matched
            //  b) it <= x < end where distance(x, end) >= delim_str.size() # if matched

            // in case of (b) delim_str.size() > 2
            // therefore distance(x, end) >= 2
            // in case of (a) distance(x, end) == 0

            // check next two characters after delim_str
            // if it is \r\n or -- then real boundary is
            // found, otherwise it is not

            if(it == end) {                                 // case (a)
                boundary_not_found = true;                  // 1T
                break;
            } else {                                        // case (b)
                auto delim_end = it + delim_str.size();     // legal because of (b)
                std::size_t leftover_size = std::distance(delim_end, end);
                if(leftover_size < 2) {
                    boundary_not_found = true;              // 2T
                    // break because delim_str.size() > 2
                    // repeating the loop wont take us anywhere
                    break;
                } else {
                    udho::utils::string_view two_chars(delim_end, 2);
                    if(two_chars == "\r\n" || two_chars == "--") {
                        boundary_not_found = false;         // 1F
                        break;
                    } else {
                        // unintended boundary
                        boundary_not_found = true;          // 3T
                    }
                    std::advance(it, 1);
                }
            }
        }

        // We did not check for partial matches with the boundary
        // We leave that for the next run of async_read_some
        // Therefore we leave (delim_str.size()-1) bytes in the buffer
        // However, if distance(begin, it) < (delim_str.size()-1)
        // then we leave the whole for the next run if boundary_not_found
        if(boundary_not_found) {
            std::size_t safe_partial_leftover_size = std::min<std::size_t>((delim_str.size()-1), std::distance(begin, it));
            it = it - safe_partial_leftover_size;
        }

        std::size_t length = std::distance(begin, it);

        if(length > content_length - _bytes_consumed) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        assert(_bytes_consumed <= content_length - length);

        if(length > 0) {
            stream->write(begin, length);
            _buffer.consume(length);
            _bytes_consumed += length;

            if (!stream->good()) {
                finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::io_error), _bytes_consumed);
                return;
            }
        }

        assert(_bytes_consumed <= content_length);

        if(boundary_not_found) {
            read_plain_multipart_file_readsome(std::move(handler), boundary, field_it, std::move(stream), content_length);
        } else {
            stream->flush();
            stream.reset();
            // case 1F
            // we have already consumed length
            // but length is distance(begin, it)
            // and it is the begining of delim_str
            // But delim_str should be consumed by read_plain_multipart_header
            // so here we don't consume the delim_str.size()

            read_plain_multipart_header(std::move(handler), boundary, false, content_length);
        }
    }

    /**
     * @brief Read more data for a file part, using buffered data if available.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param field_it       Iterator into `_fields` (pointing to the stored path).
     * @param stream         Owning pointer to the output file stream.
     * @param content_length Total body size.
     */
    template <typename Handler>
    void read_plain_multipart_file_readsome(Handler&& handler, std::string boundary, form_iterator field_it, std::unique_ptr<std::ofstream>&& stream, std::size_t content_length) {
        if(_bytes_consumed >= content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }


        if(_buffer.size() > 0) {
            read_plain_multipart_file_readsome_buffered(std::move(handler), boundary, field_it, std::move(stream), content_length);
            return;
        }

        std::size_t mbuffer_size = std::min<std::size_t>(256, content_length - _bytes_consumed);
        auto mutable_buffer      = _buffer.prepare(mbuffer_size);

        _stream.async_read_some(mutable_buffer,
            [self = self(), handler = std::move(handler), this, boundary, field_it, stream = std::move(stream), content_length](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                _buffer.commit(bytes_transferred);

                if (ec) {
                    finished(std::move(handler), ec, _bytes_consumed);
                    return;
                }

                read_plain_multipart_file_readsome_buffered(std::move(handler), boundary, field_it, std::move(stream), content_length);
            }
        );
    }

    /**
     * @brief Start reading a file upload part.
     * @param handler        Completion handler.
     * @param boundary       Multipart boundary string.
     * @param name           Field name.
     * @param filename       Original filename.
     * @param content_length Total body size.
     *
     * Creates a temporary file and stores its path in `_fields`. Then begins streaming data into the file.
     */
    template <typename Handler>
    void read_plain_multipart_file(Handler&& handler, std::string boundary, std::string name, std::string filename, std::size_t content_length) {
        if(_bytes_consumed >= content_length) {
            finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _bytes_consumed);
            return;
        }

        boost::filesystem::path temp_dir  = boost::filesystem::temp_directory_path();
        boost::filesystem::path temp_file = temp_dir / boost::filesystem::unique_path(udho::url::format("%%%%-%%%%-%%%%-%%%%-{}", filename));

        std::unique_ptr<std::ofstream> ostream = std::make_unique<std::ofstream>(temp_file.string().c_str(), std::ios::binary);
        form_iterator field_it = _fields.emplace(name, std::move(temp_file));
        // call originates from read_plain_multipart_meta
        // which consumes the mata part from the buffer
        // including the \r\n\r\n
        // So, we expect _buffer to be empty at this point
        // However, async_read_until may read past \r\n\r\n
        read_plain_multipart_file_readsome(std::move(handler), boundary, field_it, std::move(ostream), content_length);
    }

private:

    /**
     * @brief Read a chunk header line (e.g., "1F\r\n").
     * @param handler Completion handler.
     *
     * Parses the hexadecimal chunk size, ignoring chunk extensions. If size is zero, proceeds to trailers.
     */
    template <typename Handler>
    void read_chunk_header(Handler&& handler){
        boost::asio::async_read_until(
            _stream, detail::beast_buffer_ref(_buffer), "\r\n",
            [this, self = self(), handler = std::move(handler)](boost::system::error_code error, std::size_t bytes_transferred) mutable {
                if(error) {
                    finished(std::move(handler), error, _bytes_consumed);
                    return;
                }

                assert(bytes_transferred > 2);

                auto begin = static_cast<const char*>(_buffer.data().data());   // _buffer is flat_buffer
                auto end   = begin + (bytes_transferred -2);
                auto p     = begin;
                while (p != end && std::isxdigit(static_cast<unsigned char>(*p))) ++p;
                if (p == begin) {
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _target_buffer.size());
                    return;
                }
                std::size_t chunk_size = 0;
                auto chunk_size_result = std::from_chars(begin, p, chunk_size, 16);
                _buffer.consume(bytes_transferred);
                if(chunk_size_result.ec != std::errc{}){
                    finished(std::move(handler), boost::system::errc::make_error_code(boost::system::errc::protocol_error), _target_buffer.size());
                } else {
                    read_chunk_payload(std::move(handler), chunk_size);
                }
            }
        );
    }

    /**
     * @brief Read a chunk payload (data + trailing CRLF).
     * @param handler     Completion handler.
     * @param chunk_size  Size of the chunk (from header).
     *
     * Copies the chunk data into `_target_buffer` and consumes the trailing CRLF.
     * Then reads the next chunk header.
     */
    template <typename Handler>
    void read_chunk_payload(Handler&& handler, std::size_t chunk_size){
        if(chunk_size == 0) {
            read_chunk_trailers(std::move(handler));
            return;
        }

        std::size_t bytes_needed    = chunk_size + 2;
        std::size_t bytes_stored    = _buffer.size();
        std::size_t bytes_expecting = (bytes_stored < bytes_needed) ? (bytes_needed - bytes_stored) : 0;

        boost::asio::async_read(
            _stream, detail::beast_buffer_ref(_buffer), boost::asio::transfer_exactly(bytes_expecting), // trailing \r\n
            [this, self = self(), chunk_size, handler = std::move(handler)](boost::system::error_code error, std::size_t bytes_transferred) mutable {
                if(error) {
                    finished(std::move(handler), error, _target_buffer.size());
                    return;
                }

                auto mbuff = _target_buffer.prepare(chunk_size);
                boost::asio::buffer_copy(mbuff, _buffer.data());
                _target_buffer.commit(chunk_size);
                _buffer.consume(chunk_size);
                _buffer.consume(2);
                read_chunk_header(std::move(handler));
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
    void read_chunk_trailers(Handler&& handler){
        boost::asio::async_read_until(
            _stream, detail::beast_buffer_ref(_buffer), "\r\n",
            [this, self = self(), handler = std::move(handler)](boost::system::error_code error, std::size_t bytes_transferred) mutable {
                if(error) {
                    finished(std::move(handler), error, _target_buffer.size());
                    return;
                }

                assert(bytes_transferred >= 2);

                std::size_t trailer_size = bytes_transferred -2;
                if(trailer_size == 0) {
                    _buffer.consume(2);
                    finished(std::move(handler), boost::system::error_code{}, _target_buffer.size());
                    return;
                } else {
                    const char* begin = static_cast<char const*>(_buffer.data().data());
                    std::string trailer(begin, trailer_size);
                    _buffer.consume(trailer_size +2);
                    read_chunk_trailers(std::move(handler));
                }
            }
        );
    }

private:

    /**
     * @brief Transfer up to `content_length` bytes from the header buffer into a target buffer.
     * @param hbuff          Header buffer.
     * @param buff           Target buffer (either `_buffer` for multipart or `_target_buffer` for plain).
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
    void start_timer(std::size_t seconds) {
        _timer.expires_after(std::chrono::seconds(seconds));
        _timer.async_wait([self = self()](boost::system::error_code ec) {
            if (!ec) {
                self->_timeout();
            }
        });
    }

    /// Timeout handler: forcibly terminates the stream.
    void _timeout(){
        detail::stream_termination<StreamT>::apply(_stream);
    }

private:

    /**
     * @brief Final completion function – cancels timer, invokes user handler, marks finished.
     * @param handler           User completion handler.
     * @param ec                Error code (or success).
     * @param bytes_transferred Total bytes processed from the socket.
     */
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        handler(std::move(_target_buffer), ec, bytes_transferred);
        _finished = true;
    }
private:
    const request_type&         _request;
    stream_type&                _stream;
    boost::beast::flat_buffer   _buffer;
    buffer_type                 _target_buffer;
    bool                        _finished;
    std::size_t                 _bytes_consumed;
    timer_type                  _timer;
    form_container_type         _fields;
};

template <typename StreamT>
struct h11_reader: public std::enable_shared_from_this<h11_reader<StreamT>>{
    using header_reader_type    = h11_header_reader<StreamT>;
    using stream_type           = StreamT;

    explicit h11_reader(stream_type& stream): _stream(stream), _header(_stream, _header_buffer) {}

public:
    const boost::beast::flat_buffer& buffer() const { return _header_buffer; }
public:
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _header.start([h = std::move(handler), this](udho::net::types::headers::request&& request, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            h(std::move(request), ec, bytes_transferred);
        }, seconds);
    }

    /**
     * @brief reads the contents part of an HTTP request
     * @pre expects the request has already been parsed
     * @param request HTTP request
     * @param handler must take the following arguments (buffer_type&& buffer, std::error_code ec, std::size_t bytes_transferred)
     * @param seconds
     * @param limit
     */
    template <typename Handler, typename Buffer>
    void upload(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        using body_reader_type = h11_body_reader<Buffer, StreamT>;

        auto body = std::make_shared<body_reader_type>(request, _stream);
        body->start([h = std::move(handler)](Buffer&& buffer, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            // moving buffer is always legal irrespective of ec
            std::cout << "ec.message(): " << ec.message() << std::endl;
            h(std::move(buffer), ec, bytes_transferred);
            // transfer body buffer to header buffer
        }, _header_buffer, seconds, limit);
    }

    template <typename Handler>
    void upload_to_flat_buffer(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        upload<Handler, boost::beast::flat_buffer>(request, std::forward<Handler>(handler), seconds, limit);
    }

    template <typename Handler>
    void upload_to_multi_buffer(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        upload<Handler, boost::beast::multi_buffer>(request, std::forward<Handler>(handler), seconds, limit);
    }

public:
    header_reader_type& header() { return _header; }
    const header_reader_type& header() const { return _header; }
private:
    stream_type&                  _stream;
    boost::beast::flat_buffer     _header_buffer;
    header_reader_type            _header;
};

template <typename Handler, typename StreamT>
struct h11_writer_internal: public std::enable_shared_from_this<h11_writer_internal<Handler, StreamT>>{
    using self_type             = h11_writer_internal<Handler, StreamT>;
    using handler_type          = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type = udho::net::types::headers::response;
    using response_type         = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type       = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type           = StreamT;

    explicit h11_writer_internal(boost::asio::io_context& io, udho::net::types::strand& strand, const types::headers::response& headers, stream_type& stream, Handler&& handler)
        : _io(io), _strand(strand), _headers(headers), _response(_headers), _serializer(_response), _stream(stream), _handler(std::move(handler)) {}
    h11_writer_internal(const h11_writer_internal&) = delete;
    ~h11_writer_internal() { std::cout << "~http_writer_internal" << std::endl; }

    void start() {
        boost::beast::http::async_write_header(
            _stream, _serializer,
            std::bind(&self_type::finished, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    void finished(boost::system::error_code ec, std::size_t bytes_transferred){
        boost::asio::dispatch(_io,
            boost::asio::bind_executor(
                _strand,
                std::bind(std::move(_handler), ec, bytes_transferred)
            )
        );
    }
    private:
        auto shared_from_this() {
            return std::enable_shared_from_this<self_type>::shared_from_this();
        }

        auto weak_from_this() {
            return std::enable_shared_from_this<self_type>::weak_from_this();
        }

    private:
        boost::asio::io_context&        _io;
        udho::net::types::strand&       _strand;
        const response_headers_type&    _headers;
        response_type                   _response;
        serializer_type                 _serializer;
        stream_type&                    _stream;
        handler_type                    _handler;
};

template <typename StreamT>
struct h11_writer{
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using response_type   = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type     = StreamT;

    explicit h11_writer(const types::headers::response& headers, stream_type& stream): _headers(headers), _stream(stream) {}
    h11_writer(const h11_writer&) = delete;
    ~h11_writer() { std::cout << "~http_writer" << std::endl; }

    template <typename Handler>
    void operator()(boost::asio::io_context& io, udho::net::types::strand& strand_write, udho::net::types::strand& strand_finished, Handler&& handler){
        using internal_writer_type = h11_writer_internal<Handler, stream_type>;
        auto internal_writer = std::make_shared<internal_writer_type>(io, strand_finished, _headers, _stream, std::move(handler));
        boost::asio::dispatch(io,
            boost::asio::bind_executor(
                strand_write,
                std::bind(&internal_writer_type::start, internal_writer)
            )
        );
    }

    private:
        const udho::net::types::headers::response& _headers;
        stream_type&                               _stream;
};

}
}
}

#endif // UDHO_NET_PROTOCOL_HTTP_H

