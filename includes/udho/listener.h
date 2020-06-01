#ifndef UDHO_LISTENER_H
#define UDHO_LISTENER_H

#include <udho/connection.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <udho/configuration.h>

namespace udho{

/**
 * listener runs accept loop for HTTP sockets
 */
template <typename RouterT, typename AttachmentT>
class listener : public std::enable_shared_from_this<listener<RouterT, AttachmentT>>{
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket_type;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket_type;
#endif 

    
    typedef listener<RouterT, AttachmentT> self_type;
    typedef AttachmentT attachment_type;
    typedef connection<RouterT, AttachmentT> connection_type;
    
    boost::asio::io_service& _service;
    boost::asio::ip::tcp::acceptor _acceptor;
    socket_type _socket;
    boost::asio::signal_set _signals;
    RouterT& _router;
    attachment_type& _attachment;
  public:
    /**
     * @param router HTTP url mapping router
     * @param service I/O service
     * @param endpoint HTTP server endpoint to listen on
     */
    listener(RouterT& router, boost::asio::io_service& service, attachment_type& attachment, const boost::asio::ip::tcp::endpoint& endpoint): _service(service), _acceptor(service), _socket(service), _signals(service, SIGINT, SIGTERM), _router(router), _attachment(attachment){
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
        _acceptor.async_accept(_socket, std::bind(&self_type::on_accept, std::enable_shared_from_this<listener<RouterT, AttachmentT>>::shared_from_this(), std::placeholders::_1));
    }
    void on_accept(boost::system::error_code ec){
        if(ec){
            // TODO failed to accept
            _attachment << udho::logging::messages::formatted::info("listener", "failed to accept new connection from %1%") % _socket.remote_endpoint().address();
        }else{
            _attachment << udho::logging::messages::formatted::info("listener", "accepting new connection from %1%") % _socket.remote_endpoint().address();
            std::shared_ptr<connection_type> conn = std::make_shared<connection<RouterT, AttachmentT>>(_router, _attachment, std::move(_socket));
            conn->run();
        }
        accept();
    }
};

}

#endif // UDHO_LISTENER_H
