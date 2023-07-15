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

/**
 * \brief A connection object wraps a socket.
 * Follows a protocol (e.g. HTTP, FastCGI, SCGI, wscgi etc..) to parse the headers.
 * Uses ProtocolT to follow the protocol and prepare a request object.
 * Once the request object is created it passes that to an asynchronous resolver to resolve a slot that will process that request.
 */
template <typename ProtocolT>
struct connection: public std::enable_shared_from_this<connection<ProtocolT>>{
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket_type;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket_type;
#endif
    using protocol_type  = ProtocolT;
    using reader_type    = typename protocol_type::reader;
    // using writer_type   = typename protocol_type::writer;
    using clock_type     = std::chrono::time_point<std::chrono::system_clock>;
    using self_type      = connection<ProtocolT>;
    using processer_type = std::function<void (udho::net::types::headers::request)>;

    connection(boost::asio::io_service& service, socket_type socket): _service(service), _socket(std::move(socket)), _strand(service), _stage(types::stages::accepted), _reader(std::make_shared<reader_type>(_request)){}
    void start(processer_type&& processor){
        _processor = std::move(processor);
        _start = std::chrono::system_clock::now();
        _reader->start(
            _socket,
            std::bind(&self_type::on_read_header, std::enable_shared_from_this<self_type>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );
    }
    private:
        void on_read_header(boost::system::error_code ec, std::size_t bytes_transferred){
            if(ec){
                _stage = types::stages::rejected;
                return;
            }
            _stage = types::stages::headers_read;
            // std::cout << _request << std::endl;
            _service.post(std::bind(&self_type::process, std::enable_shared_from_this<self_type>::shared_from_this()));
        }
        void process(){
            std::cout << "processing" << std::endl;
            _processor(_request);
        }
    private:
        boost::asio::io_service& _service;
        socket_type _socket;
        boost::asio::io_service::strand _strand;
        udho::net::types::headers::request _request;
        udho::net::types::headers::response _response;
        clock_type _start, _end;
        types::stages _stage;
        std::shared_ptr<reader_type> _reader;
        processer_type                       _processor;
};



}
}

#endif // UDHO_NET_CONNECTION_H
