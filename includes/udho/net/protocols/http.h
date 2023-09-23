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
struct http_writer: public std::enable_shared_from_this<http_writer<StreamT>>{
    using handler_type    = std::function<void (boost::system::error_code, std::size_t)>;
    using response_type   = boost::beast::http::response<boost::beast::http::empty_body>;
    using serializer_type = boost::beast::http::response_serializer<boost::beast::http::empty_body>;
    using stream_type     = StreamT;

    template <typename Handler>
    struct writer_initiate{
        using self_type = writer_initiate<Handler>;

        writer_initiate(boost::asio::io_service& io, udho::net::types::strand& strand, stream_type& stream, serializer_type& serializer, Handler&& handler): _io(io), _strand(strand), _stream(stream), _serializer(serializer), _handler(std::move(handler)) {}
        void operator()(){
            boost::beast::http::async_write_header(
                _stream, _serializer,
                std::bind(&self_type::finished, std::move(*this), std::placeholders::_1, std::placeholders::_2)
            );
        }
        void finished(boost::system::error_code ec, std::size_t bytes_transferred){
            // _handler(ec, bytes_transferred);
            _io.dispatch(
                boost::asio::bind_executor(
                    _strand,
                    std::bind(std::move(_handler), ec, bytes_transferred)
                )
            );
        }

        boost::asio::io_service&  _io;
        udho::net::types::strand& _strand;
        stream_type&              _stream;
        serializer_type&          _serializer;
        Handler                   _handler;
    };

    explicit http_writer(const types::headers::response& headers, stream_type& stream): _headers(headers), _stream(stream) {}
    http_writer(const http_writer&) = delete;
    ~http_writer() { std::cout << "http_writer dtor" << std::endl; }

    template <typename Handler>
    void start(boost::asio::io_service& io, udho::net::types::strand& strand_write, udho::net::types::strand& strand_finished, Handler&& handler){
        // std::cout << "_headers" << std::endl << _headers << std::endl;
        _response = std::make_shared<response_type>(_headers);
        _serializer = std::make_shared<serializer_type>(*_response);
        // // _handler = std::move(handler);
        // boost::beast::http::async_write_header(_stream, *_serializer, std::move(handler));
        //
        // // boost::asio::bind_executor(strand, std::bind(&http_writer::finished, std::enable_shared_from_this<http_writer<StreamT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2))
        writer_initiate<Handler> initiate(io, strand_finished, _stream, *_serializer, std::move(handler));
        io.dispatch(
            boost::asio::bind_executor(
                strand_write,
                std::move(initiate)
            )
        );
    }
    // private:
    //     void finished(boost::system::error_code ec, std::size_t bytes_transferred){
    //         // std::cout << "_serializer.is_header_done(): " << _serializer->is_header_done() << std::endl;
    //         // std::cout << "bytes_transferred: " << bytes_transferred << std::endl;
    //         if(!ec){
    //             // std::cout << "finished" << std::endl;
    //         }else{
    //             std::cout << "error: " << ec << std::endl;
    //         }
    //         _handler(ec, bytes_transferred);
    //     }
    private:
        const udho::net::types::headers::response& _headers;
        std::shared_ptr<response_type>             _response;
        std::shared_ptr<serializer_type>           _serializer;
        // handler_type                               _handler;
        stream_type&                               _stream;
};

}
}
}

#endif // UDHO_NET_PROTOCOL_HTTP_H

