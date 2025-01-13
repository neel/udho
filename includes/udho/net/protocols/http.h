#ifndef UDHO_NET_PROTOCOL_HTTP_H
#define UDHO_NET_PROTOCOL_HTTP_H

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

template <typename Handler, typename StreamT>
struct http_writer_internal: public std::enable_shared_from_this<http_writer_internal<Handler, StreamT>>{
    using self_type             = http_writer_internal<Handler, StreamT>;
    using handler_type          = std::function<void (boost::system::error_code, std::size_t)>;
    using response_headers_type = udho::net::types::headers::response;
    using response_type         = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type       = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type           = StreamT;

    explicit http_writer_internal(boost::asio::io_service& io, udho::net::types::strand& strand, const types::headers::response& headers, stream_type& stream, Handler&& handler)
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
        _io.dispatch(
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
        boost::asio::io_service&        _io;
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
    void operator()(boost::asio::io_service& io, udho::net::types::strand& strand_write, udho::net::types::strand& strand_finished, Handler&& handler){
        using internal_writer_type = http_writer_internal<Handler, stream_type>;
        auto internal_writer = std::make_shared<internal_writer_type>(io, strand_finished, _headers, _stream, std::move(handler));
        io.dispatch(
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

