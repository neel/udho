#ifndef HTTP_LISTENER_H
#define HTTP_LISTENER_H

#include "session.h"
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace udho{

template <typename RouterT>
class listener : public std::enable_shared_from_this<listener<RouterT>>{
    typedef listener<RouterT> self_type;
    boost::asio::io_service& _service;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::asio::ip::tcp::socket _socket;
    std::shared_ptr<std::string const> _docroot;
    boost::asio::signal_set _signals;
    RouterT& _router;
  public:
    listener(RouterT& router, boost::asio::io_service& service, const boost::asio::ip::tcp::endpoint& endpoint, std::shared_ptr<std::string const> const& docroot): _service(service), _router(router), _acceptor(service), _socket(service), _docroot(docroot), _signals(service, SIGINT, SIGTERM){
        
//         boost::asio::socket_base::linger option(true, 0);
//         _acceptor.set_option(option);
        
        boost::system::error_code ec;
        _acceptor.open(endpoint.protocol(), ec);
        if(ec) throw std::runtime_error((boost::format("Failed to open acceptor %1%") % ec.message()).str());
        _acceptor.bind(endpoint, ec);
        if(ec) throw std::runtime_error((boost::format("Failed to bind acceptor %1%") % ec.message()).str());
        _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if(ec) throw std::runtime_error((boost::format("Failed to listen %1%") % ec.message()).str());
        
        _signals.async_wait(boost::bind(&self_type::stop, this));
    }
    void stop(){
        _acceptor.close();
        _service.stop();
    }
    ~listener(){
        stop();
    }
    void run(){
        if(! _acceptor.is_open())
            return;
        accept();
    }
    void accept(){
        _acceptor.async_accept(_socket, std::bind(&self_type::on_accept, std::enable_shared_from_this<listener<RouterT>>::shared_from_this(), std::placeholders::_1));
    }
    void on_accept(boost::system::error_code ec){
        if(ec){
            // TODO failed to accept
        }else{
            std::make_shared<session<RouterT>>(_router, std::move(_socket), _docroot)->run();
        }
        accept();
    }
};

}

#endif // HTTP_LISTENER_H
