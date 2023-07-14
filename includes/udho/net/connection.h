#ifndef UDHO_NET_CONNECTION_H
#define UDHO_NET_CONNECTION_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>
#include <udho/net/common.h>
#include <chrono>

namespace udho{
namespace net{

enum class stages{
    accepted,
    headers_read,
    body_read,
    body_skipped,
    headers_written,
    body_written,
    closed
};

template <typename ProtocolT>
struct connection: public std::enable_shared_from_this<connection<ProtocolT>>{
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket_type;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket_type;
#endif
    using protocol_type = ProtocolT;
    using reader_type   = typename protocol_type::reader_type;
    using writer_type   = typename protocol_type::writer_type;
    using clock_type    = std::chrono::time_point<std::chrono::system_clock>;

    connection(boost::asio::io_service& service, socket_type socket): _service(service), _socket(std::move(socket)), _strand(service), _stage(stages::accepted){}
    void start(){
        _start = std::chrono::system_clock::now();
    }
    void headers_read(){

    }
    private:
        boost::asio::io_service& _service;
        socket_type _socket;
        boost::asio::io_service::strand _strand;
        udho::net::types::headers::request _request;
        udho::net::types::headers::request _response;
        clock_type _start, _end;
        stages _stage;
        protocol_type _protocol;
};



}
}

#endif // UDHO_NET_CONNECTION_H
