#ifndef UDHO_NET_LISTENER_H
#define UDHO_NET_LISTENER_H

#include <boost/enable_shared_from_this.hpp>
#include <udho/net/common.h>
#include <udho/net/stream.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <udho/url/summary.h>

namespace udho{
namespace net{

/**
 * listener runs accept loop for HTTP sockets
 * \ingroup server
 */
template <typename ConnectionT>
class listener: public std::enable_shared_from_this<listener<ConnectionT>>{
    using socket_type      = udho::net::types::socket;
    using self_type        = listener<ConnectionT>;
    using connection_type  = ConnectionT;
    using processer_type   = std::function<void (boost::asio::ip::address, udho::net::stream&&)>;

    boost::asio::io_service&          _service;
    boost::asio::ip::tcp::acceptor    _acceptor;
    socket_type                       _socket;
    boost::asio::signal_set           _signals;
    processer_type                    _processor;
    std::atomic<bool>                 _running;
  public:
    /**
     * @brief Construct a socket listener that accepts an incoming connection into a socket and moves it into a newly constructed ConnectionT object and then call's it's start method to start parsing the received message.
     * @param router HTTP url mapping router
     * @param service I/O service
     * @param endpoint HTTP server endpoint to listen on
     */
    listener(boost::asio::io_service& service, const boost::asio::ip::tcp::endpoint& endpoint): _service(service), _acceptor(service), _socket(service), _signals(service, SIGINT, SIGTERM), _running(false) {
        boost::system::error_code ec;
        _acceptor.open(endpoint.protocol(), ec);
        if(ec) throw std::runtime_error((boost::format("Failed to open acceptor %1%") % ec.message()).str());
        _acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec) throw std::runtime_error((boost::format("Failed to set reusable option %1%") % ec.message()).str());
        _acceptor.bind(endpoint, ec);
        if(ec) throw std::runtime_error((boost::format("Failed to bind acceptor %1%") % ec.message()).str());
        _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if(ec) throw std::runtime_error((boost::format("Failed to listen %1%") % ec.message()).str());

        _signals.async_wait(std::bind(&self_type::stop, this));
    }
    /**
     * stops accepting incomming connections
     */
    void stop(){
        _running = false;
        _acceptor.close();
        _service.stop();
    }
    ~listener(){
        stop();
    }
    /**
     * starts the async accept loop
     */
    void listen(processer_type&& processor){
        _processor = std::move(processor);
        if(! _acceptor.is_open())
            return;
        accept();
    }
    private:
        auto shared_from_this(){
            return std::enable_shared_from_this<listener<ConnectionT>>::shared_from_this();
        }
        /**
         * accept an incomming connection
         */
        void accept(){
            _running = true;
            _acceptor.async_accept(_socket, std::bind(&self_type::on_accept, std::enable_shared_from_this<self_type>::shared_from_this(), std::placeholders::_1));
        }
        void on_accept(boost::system::error_code ec){
            if(!_running) return;
            if(!ec){
                boost::asio::ip::address remote_address = _socket.remote_endpoint().address();
                // std::cout << "accepted " << remote_address << std::endl;
                std::shared_ptr<connection_type> conn = std::make_shared<connection_type>(_service, std::move(_socket));
                conn->start(std::bind(&self_type::on_ready, shared_from_this(), remote_address, std::placeholders::_1));
            }else{
                // TODO failed to accept
                std::cout << "Server: Error while accepting " << ec.category().name() << " : " << ec.value() << " : " << ec.message() << std::endl;
            }
            accept();
        }
        void on_ready(boost::asio::ip::address address, udho::net::stream&& context){
            _service.post([address, context = std::move(context), this] () mutable {
                _processor(address, std::move(context));
                // std::cout << __FILE__ << " :" << __LINE__ << std::endl;
            });
        }
};

}
}

#endif // UDHO_NET_LISTENER_H
