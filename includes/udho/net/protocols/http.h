#ifndef UDHO_NET_PROTOCOL_HTTP_H
#define UDHO_NET_PROTOCOL_HTTP_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/net/common.h>
#include <boost/beast/core/static_buffer.hpp>

namespace udho{
namespace net{
namespace protocols{

struct http_reader: public std::enable_shared_from_this<http_reader>{
    using http_request_parser_type  = boost::beast::http::parser<true, boost::beast::http::empty_body>;
    using handler_type              = std::function<void (boost::system::error_code, std::size_t)>;

    inline explicit http_reader(types::headers::request& request): _request(request){}

    template <typename StreamT, typename Handler>
    void start(StreamT& stream, Handler&& handler){
        _handler = std::move(handler);
        boost::beast::http::async_read_header(
            stream, _buffer, _parser,
            std::bind(&http_reader::finished, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    private:
        void finished(boost::system::error_code ec, std::size_t bytes_transferred){
            if(!ec){
                _request = _parser.release();
                std::cout << "request parsed" << std::endl << _request << std::endl;
            }
            _handler(ec, bytes_transferred);
        }
    private:
        udho::net::types::headers::request& _request;
        http_request_parser_type            _parser;
        boost::beast::flat_buffer           _buffer;
        handler_type                        _handler;
};

struct http_writer: public std::enable_shared_from_this<http_writer>{
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using response_type   = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type = boost::beast::http::response_serializer<boost::beast::http::empty_body>;

    explicit http_writer(const types::headers::response& headers): _headers(headers), _response(headers), _serializer(_response) {}

    template <typename StreamT, typename Handler>
    void start(StreamT& stream, Handler&& handler){
        _handler = std::move(handler);
        boost::beast::http::async_write_header(
            stream, _serializer,
            std::bind(&http_writer::finished, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    private:
        void finished(boost::system::error_code ec, std::size_t bytes_transferred){
            if(!ec){
                std::cout << "finished" << std::endl;
            }
            _handler(ec, bytes_transferred);
        }
    private:
        const udho::net::types::headers::response& _headers;
        response_type                              _response;
        serializer_type                            _serializer;
        handler_type                               _handler;
};

struct http{
    using reader = http_reader;
    using writer = http_writer;
};

}
}
}

#endif // UDHO_NET_PROTOCOL_HTTP_H

