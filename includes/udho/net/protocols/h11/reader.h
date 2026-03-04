#ifndef UDHO_NET_H11_READER_H
#define UDHO_NET_H11_READER_H

#include <memory>
#include <udho/net/protocols/h11/header_reader.h>
#include <udho/net/protocols/h11/body_reader.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>

namespace udho{
namespace net{
namespace protocols{

namespace h11{

template <typename StreamT>
struct reader: public std::enable_shared_from_this<reader<StreamT>>{
    using http_request_type     = boost::beast::http::header<true, boost::beast::http::fields>;
    using header_reader_type    = h11::header_reader<StreamT>;
    using stream_type           = StreamT;

    explicit reader(stream_type& stream): _stream(stream), _header(_stream, _header_buffer) {}

public:
    const boost::beast::flat_buffer& buffer() const { return _header_buffer; }
public:
    template <typename Handler>
    void start(Handler&& handler, std::size_t seconds){
        _header.start([h = std::move(handler), this](http_request_type&& request, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
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
    void upload(const http_request_type& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        using body_reader_type = udho::net::protocols::h11::body_reader<Buffer, StreamT>;

        auto body = std::make_shared<body_reader_type>(request, _stream);
        body->start([h = std::move(handler)](Buffer&& buffer, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            // moving buffer is always legal irrespective of ec
            // std::cout << "ec.message(): " << ec.message() << std::endl;
            h(std::move(buffer), ec, bytes_transferred);
            // transfer body buffer to header buffer
        }, _header_buffer, seconds, limit);
    }

    template <typename Handler>
    void upload_to_flat_buffer(const http_request_type& request, Handler&& handler, std::size_t seconds, std::size_t limit){
        upload<Handler, boost::beast::flat_buffer>(request, std::forward<Handler>(handler), seconds, limit);
    }

    template <typename Handler>
    void upload_to_multi_buffer(const http_request_type& request, Handler&& handler, std::size_t seconds, std::size_t limit){
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

}

}
}
}

#endif // UDHO_NET_H11_READER_H
