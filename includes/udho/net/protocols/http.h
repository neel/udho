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
#include <boost/beast/core/multi_buffer.hpp>

namespace udho{
namespace net{
namespace protocols{

template <typename StreamT>
struct http_reader: public std::enable_shared_from_this<http_reader<StreamT>>{
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using handler_type              = std::function<void (boost::system::error_code, std::size_t)>;
    using stream_type               = StreamT;

    inline explicit http_reader(types::headers::request& request, stream_type& stream): _request(request), _stream(stream) {}
    ~http_reader() {
        std::cout << "~http_reader" << std::endl;
    }
    template <typename Handler>
    void start(Handler&& handler){
        _handler = std::move(handler);
        boost::beast::http::async_read_header(
            _stream, _buffer, _parser,
            std::bind(&http_reader::finished, std::enable_shared_from_this<http_reader<StreamT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    private:
        void finished(boost::system::error_code ec, std::size_t bytes_transferred){
            if(!ec){
                _request = _parser.release();
                // std::cout << "request parsed" << std::endl << _request << std::endl;
            }
            _handler(ec, bytes_transferred);
        }
    private:
        udho::net::types::headers::request& _request;
        http_request_parser_type            _parser;
        boost::beast::flat_buffer           _buffer;
        handler_type                        _handler;
        stream_type&                        _stream;
};

template <typename StreamT>
struct http_header_reader{
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using stream_type               = StreamT;
    using timer_type                = boost::asio::steady_timer;

    http_header_reader(stream_type& stream, boost::beast::flat_buffer& buffer)
        : _stream(stream), _buffer(buffer), _finished(false), _timer(_stream.get_executor()) {}

    /**
     * @brief start reading HTTP headers asynchronously
     * @param handler must take the following arguments (udho::net::types::headers::request&& request, std::error_code ec, std::size_t bytes_transferred)
     * @param seconds
     */
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _timer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                _timeout();
            }
        });
        boost::beast::http::async_read_header(
            _stream, _buffer, _parser,
            [this, handler = std::move(handler)] (boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                finished(std::move(handler), ec, bytes_transferred);
            }
        );
        _timer.expires_after(std::chrono::seconds(seconds));
    }

    bool is_finished() const { return _finished; }

private:
    void _timeout(){
        // _stream.cancel(boost::system::error_code{});
        // works with boost::beast::test::basic_stream
        _stream.close();
        _stream.close_remote();
    }

private:
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        if(!ec){
            _request = _parser.release();
        }
        // _request is empty in case of error but it exists and it is legal to move it
        handler(std::move(_request), ec, bytes_transferred);
        _finished = true;
    }
private:
    udho::net::types::headers::request  _request; // gets moved in finished
    stream_type&                        _stream;
    boost::beast::flat_buffer&          _buffer;
    http_request_parser_type            _parser;
    bool                                _finished;
    timer_type                          _timer;
};

template <typename StreamT>
struct http_body_reader{
    using stream_type               = StreamT;
    using body_request_type         = boost::beast::http::request<boost::beast::http::string_body>;
    using body_parser_type          = boost::beast::http::parser<true, boost::beast::http::string_body>;
    using timer_type                = boost::asio::steady_timer;

    http_body_reader(const udho::net::types::headers::request& request, stream_type& stream, boost::beast::flat_buffer& buffer)
        : _request(request), _stream(stream), _buffer(buffer), _parser(request), _finished(false), _timer(_stream.get_executor()) {}

    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds, std::size_t limit = 0){
        if(limit > 0) _parser.body_limit(limit);
        _timer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                _timeout();
            }
        });
        boost::beast::http::async_read(
            _stream, _buffer, _parser,
            [this, handler = std::forward<Handler>(handler)](boost::system::error_code ec, std::size_t bytes_transferred){
                finished(std::move(handler), ec, bytes_transferred);
            }
        );
        _timer.expires_after(std::chrono::seconds(seconds));
    }

    bool is_finished() const { return _finished; }

private:
    void _timeout(){
        _stream.cancel(boost::system::error_code{});
    }

private:
    template <typename Handler>
    void finished(Handler&& handler, boost::system::error_code ec, std::size_t bytes_transferred){
        _timer.cancel();
        if(!ec){
            // ignore request because it has been moved to journal already
            // body contents are already in the buffer and that memory is persistent
            // ignore the return of release
            _parser.release();
        }
        handler(ec, bytes_transferred);
        _finished = true;
    }
private:
    const udho::net::types::headers::request& _request;
    stream_type&                        _stream;
    boost::beast::flat_buffer&          _buffer;
    body_parser_type                    _parser;
    bool                                _finished;
    timer_type                          _timer;
};

template <typename StreamT>
struct http_reader2: public std::enable_shared_from_this<http_reader2<StreamT>>{
    using header_reader_type    = http_header_reader<StreamT>;
    using body_reader_type      = http_body_reader<StreamT>;
    using body_reader_ptr_type  = std::shared_ptr<body_reader_type>;
    using stream_type           = StreamT;
    using buffer_type           = boost::beast::flat_buffer;

    explicit http_reader2(stream_type& stream): _stream(stream), _header(_stream, _buffer) {}

public:
    const boost::beast::flat_buffer& buffer() const { return _buffer; }
public:
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _body.reset();
        _header.start(std::forward<Handler>(handler), seconds);
    }

    /**
     * @brief reads the contents part of an HTTP request
     * @pre expects the request has already been parsed
     * @param request HTTP request
     * @param handler must take the following arguments (buffer_type&& buffer, std::error_code ec, std::size_t bytes_transferred)
     * @param seconds
     * @param limit
     */
    template <typename Handler>
    void upload(const udho::net::types::headers::request& request, Handler&& handler, std::size_t seconds, std::size_t limit = 0){
        _body = std::make_shared<body_reader_type>(request, _stream, _buffer);
        _body.start([h = std::move(handler), this](boost::system::error_code ec, std::size_t bytes_transferred){
            // moving _buffer is always legal irrespective of ec
            h(std::move(_buffer), ec, bytes_transferred);
            _body.reset();
        }, seconds, limit);
    }
public:
    header_reader_type& header() { return _header; }
    const header_reader_type& header() const { return _header; }
private:
    stream_type&          _stream;
    buffer_type           _buffer;
    header_reader_type    _header;
    body_reader_ptr_type  _body;
};

template <typename Handler, typename StreamT>
struct http_writer_internal: public std::enable_shared_from_this<http_writer_internal<Handler, StreamT>>{
    using self_type             = http_writer_internal<Handler, StreamT>;
    using handler_type          = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type = udho::net::types::headers::response;
    using response_type         = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type       = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type           = StreamT;

    explicit http_writer_internal(boost::asio::io_context& io, udho::net::types::strand& strand, const types::headers::response& headers, stream_type& stream, Handler&& handler)
        : _io(io), _strand(strand), _headers(headers), _response(_headers), _serializer(_response), _stream(stream), _handler(std::move(handler)) {}
    http_writer_internal(const http_writer_internal&) = delete;
    ~http_writer_internal() { std::cout << "~http_writer_internal" << std::endl; }

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
struct http_writer{
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using response_type   = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type     = StreamT;

    explicit http_writer(const types::headers::response& headers, stream_type& stream): _headers(headers), _stream(stream) {}
    http_writer(const http_writer&) = delete;
    ~http_writer() { std::cout << "~http_writer" << std::endl; }

    template <typename Handler>
    void operator()(boost::asio::io_context& io, udho::net::types::strand& strand_write, udho::net::types::strand& strand_finished, Handler&& handler){
        using internal_writer_type = http_writer_internal<Handler, stream_type>;
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

