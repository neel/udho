#ifndef UDHO_NET_LISTENER_H
#define UDHO_NET_LISTENER_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>

namespace udho{
namespace net{

/**
 * listener runs accept loop for HTTP sockets
 * \ingroup server
 */
template <typename ConnectionT>
class listener : public std::enable_shared_from_this<listener<ConnectionT>>{
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket_type;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket_type;
#endif


    typedef listener<ConnectionT> self_type;
    typedef ConnectionT connection_type;

    boost::asio::io_service& _service;
    boost::asio::ip::tcp::acceptor _acceptor;
    socket_type _socket;
    boost::asio::signal_set _signals;
  public:
    /**
     * @brief Construct a socket listener that accepts an incoming connection into a socket and moves it into a newly constructed ConnectionT object and then call's it's start method to start parsing the received message.
     * @param router HTTP url mapping router
     * @param service I/O service
     * @param endpoint HTTP server endpoint to listen on
     */
    listener(boost::asio::io_service& service, const boost::asio::ip::tcp::endpoint& endpoint): _service(service), _acceptor(service), _socket(service), _signals(service, SIGINT, SIGTERM) {
        boost::system::error_code ec;
        _acceptor.open(endpoint.protocol(), ec);
        if(ec) throw std::runtime_error((boost::format("Failed to open acceptor %1%") % ec.message()).str());
        _acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec) throw std::runtime_error((boost::format("Failed to set reusable option %1%") % ec.message()).str());
        _acceptor.bind(endpoint, ec);
        if(ec) throw std::runtime_error((boost::format("Failed to bind acceptor %1%") % ec.message()).str());
        _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if(ec) throw std::runtime_error((boost::format("Failed to listen %1%") % ec.message()).str());

        _signals.async_wait(boost::bind(&self_type::stop, this));
    }
    /**
     * stops accepting incomming connections
     */
    void stop(){
        _acceptor.close();
        _service.stop();
    }
    ~listener(){
        stop();
    }
    /**
     * starts the async accept loop
     */
    void run(){
        if(! _acceptor.is_open())
            return;
        accept();
    }
    /**
     * accept an incomming connection
     */
    void accept(){
        _acceptor.async_accept(_socket, std::bind(&self_type::on_accept, std::enable_shared_from_this<self_type>::shared_from_this(), std::placeholders::_1));
    }
    void on_accept(boost::system::error_code ec){
        boost::asio::ip::address remote_address = _socket.remote_endpoint().address();
        if(ec){
            // TODO failed to accept
        }else{
            std::shared_ptr<connection_type> conn = std::make_shared<connection_type>(_service, std::move(_socket));
            conn->start();
        }
        accept();
    }
};

}
}

#endif // UDHO_NET_LISTENER_H
