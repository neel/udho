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
 * @brief listener runs accept loop for HTTP sockets.
 * Creates a new shared_ptr to the ConnectionT on each successful accept.
 * Then calls the start method of the connection object with a callback to the processor.
 * A processor is callable with two inputs, boost::asio::ip::address, udho::net::stream&&.
 *
 * @ingroup server
 */
template <typename ConnectionT>
class listener: public std::enable_shared_from_this<listener<ConnectionT>>{
    using socket_type      = udho::net::types::socket;
    using self_type        = listener<ConnectionT>;
    using connection_type  = ConnectionT;
    using processer_type   = std::function<void (boost::asio::ip::address, udho::net::stream&&)>;
    using connection_map   = std::map<typename std::add_pointer<connection_type>::type, std::weak_ptr<connection_type>>;

    boost::asio::io_context&          _service;
    boost::asio::ip::tcp::acceptor    _acceptor;
    socket_type                       _socket;
    boost::asio::signal_set           _signals;
    processer_type                    _processor;
    std::atomic<bool>                 _running;
    connection_map                    _connections;
  public:
    /**
     * @brief Construct a socket listener that accepts an incoming connection into a socket and moves it into a newly constructed ConnectionT object and then call's it's start method to start parsing the received message.
     * @param router HTTP url mapping router
     * @param service I/O service
     * @param endpoint HTTP server endpoint to listen on
     */
    listener(boost::asio::io_context& service, const boost::asio::ip::tcp::endpoint& endpoint): _service(service), _acceptor(service), _socket(service), _signals(service, SIGINT, SIGTERM), _running(false) {
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
     * @brief starts the async accept loop
     * @details leads to on_accept once an incoming connection is accepted
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
         * @brief accept an incomming connection asynchronously through on_accept callback
         */
        void accept(){
            _running = true;
            _acceptor.async_accept(_socket, std::bind(&self_type::on_accept, std::enable_shared_from_this<self_type>::shared_from_this(), std::placeholders::_1));
        }

        /**
         * @brief on_accept creates a connection object once an incomming connection is successfully accepted
         * @details calls the connection start method of the connection which starts reading the incomming payload
         * @param ec
         */
        void on_accept(boost::system::error_code ec){
            if(!_running) {
                for(auto pair : _connections){
                    connection_type* conn = pair.first;
                    // TODO force stop conn
                }
                return;
            }
            if(!ec){
                boost::asio::ip::address remote_address = _socket.remote_endpoint().address();
                // std::cout << "accepted " << remote_address << std::endl;
                // Assumption:
                //  The listener outlives all connections created by it from the on_accept function
                //  Support:
                //      The lifetime of the listener is managed by itself through accept -> on_accept -> accept loop
                //      which never termintes until explicitely requested by setting _running to false.
                // Argument:
                //  As the listener always outlives the connection, capturing this in the deleter callback is okay.
                std::shared_ptr<connection_type> conn = std::shared_ptr<connection_type>{
                    new connection_type{_service, std::move(_socket)},
                    [this](connection_type* ptr){
                        std::cout << "deleting connection " << ptr << std::endl;
                        assert(ptr != 0x0);
                        auto it = _connections.find(ptr);
                        assert(it != _connections.end());
                        auto refs = it->second.use_count();
                        assert(refs == 0);
                        _connections.erase(it);
                        delete ptr;
                        ptr = 0x0;
                    }
                };
                _connections.insert(std::make_pair(conn.get(), std::weak_ptr<connection_type>{conn}));
                std::cout << "conn.use_count() " << conn.use_count() << std::endl;
                conn->start(std::bind(&self_type::on_ready, shared_from_this(), remote_address, std::placeholders::_1));
            }else{
                // TODO failed to accept
                std::cout << "Server: Error while accepting " << ec.category().name() << " : " << ec.value() << " : " << ec.message() << std::endl;
            }
            accept();
        }
        void on_ready(boost::asio::ip::address address, udho::net::stream&& context){
            boost::asio::post(_service, [address, context = std::move(context), this] () mutable {
                _processor(address, std::move(context));
                // std::cout << __FILE__ << " :" << __LINE__ << std::endl;
            });
        }
};

}
}

#endif // UDHO_NET_LISTENER_H
