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
                std::cout << "finished" << std::endl;
            }
            _handler(ec, bytes_transferred);
        }
    private:
        udho::net::types::headers::request& _request;
        http_request_parser_type            _parser;
        boost::beast::static_buffer<4096>   _buffer;
        handler_type                        _handler;
};

struct http{
    using reader = http_reader;
};

}
}
}

#endif // UDHO_NET_PROTOCOL_HTTP_H

