#ifndef UDHO_CONNECTION_H
#define UDHO_CONNECTION_H

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <boost/format.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <udho/context.h>
#include <udho/logging.h>
#include <udho/page.h>
#include <udho/defs.h>
#include <udho/util.h>

namespace udho{
    
using tcp = boost::asio::ip::tcp; 
namespace http = boost::beast::http;

/**
 * Stateful HTTP Session
 * \ingroup server
 */
template <typename RouterT, typename AttachmentT>
class connection : public std::enable_shared_from_this<connection<RouterT, AttachmentT>>{
    typedef typename AttachmentT::auxiliary_type auxiliary_type;
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket_type;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket_type;
#endif
    
    RouterT& _router;
    AttachmentT& _attachment;
    typedef connection<RouterT, AttachmentT> self_type;
    typedef AttachmentT attachment_type;
    typedef typename attachment_type::shadow_type shadow_type;
    typedef udho::context<auxiliary_type, udho::defs::request_type, shadow_type> context_type;
    
    struct send_lambda{
        self_type& self_;

        explicit send_lambda(self_type& self): self_(self){}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>& msg) const {
            operator()(std::move(msg));
        }
        
        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const {
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));
            self_.res_ = sp;
            http::async_write(self_._socket, *sp, boost::asio::bind_executor(self_._strand, std::bind(&self_type::on_write, self_.shared_from_this(), std::placeholders::_1, std::placeholders::_2, sp->need_eof())));
        }
    };

    socket_type _socket;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
    boost::beast::flat_buffer _buffer;
    udho::defs::request_type _req;
    std::shared_ptr<void> res_;
    send_lambda _lambda;
    boost::posix_time::ptime _time;

    std::mutex _respond_mutex;
    bool _responded;
  public:
    /**
     * session constructor
     * @param router router
     * @param attachment attachment
     * @param socket TCP socket
     */
    explicit connection(RouterT& router, attachment_type& attachment, socket_type socket)
        : _router(router), 
          _attachment(attachment),
          _socket(std::move(socket)),
          _strand(_socket.get_executor()),
          _lambda(*this),
          _time(boost::posix_time::second_clock::local_time()),
          _responded(false)
          {}
    ~connection(){
        // std::cout << "destructing connection" << std::endl;
    }
    /**
     * start the read loop
     */
    void run(){
        do_read();
    }
    void do_read(){
        _req = {};
        http::async_read(_socket, _buffer, _req, boost::asio::bind_executor(_strand, std::bind(&self_type::on_read, std::enable_shared_from_this<connection<RouterT, AttachmentT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
    }
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred){
        boost::ignore_unused(bytes_transferred);
        if(ec == http::error::end_of_stream){
            return do_close();
        }
        if(bytes_transferred == 0 || ec == boost::asio::error::connection_reset){
            return;
        }
        boost::asio::ip::tcp::endpoint remote = _socket.remote_endpoint(ec);
        if(ec){
            _attachment << udho::logging::messages::formatted::error("connection", "Unexpected error while retrieving socket remote endpoint %1%") % ec;
            return do_close();
        }
        _time = boost::posix_time::second_clock::local_time();
        
        std::string path;
        std::string target_str(_req.target());
        std::stringstream path_stream(target_str);
        std::getline(path_stream, path, '?');
        auto start = std::chrono::high_resolution_clock::now();
        try{
            context_type ctx(_attachment.aux(), _req, _attachment.shadow());
            ctx.attach(_attachment);
            ctx._pimpl->_respond.connect(std::bind(&self_type::respond, std::enable_shared_from_this<connection<RouterT, AttachmentT>>::shared_from_this(), std::placeholders::_1));
            int status = 0;
            do{
                if(ctx.rerouted()){
                    if(!ctx.reroutes()){
                        _attachment << udho::logging::messages::formatted::error("router", "Error expecting rerouted context but got empty stack while serving %1%") % std::string(_req.target());
                        throw exceptions::http_error(boost::beast::http::status::internal_server_error, (boost::format("Error expecting rerouted context but got empty stack while serving %1%") % std::string(_req.target())).str());
                    }
                    
                    udho::detail::route last = ctx.top();
                    path = boost::regex_replace(last._subject, boost::regex(last._pattern), ctx.alt_path());
                    _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% rerouted to %4%") % remote.address() % _req.method() % last._path % path;
                    ctx.clear();
                }
                try{
                    status = _router.serve(ctx, _req.method(), path, _lambda);
                }catch(const udho::exceptions::reroute& rerouted){
                    ctx.reroute(rerouted.alt_path());
                }
            }while(ctx.rerouted());
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> delta = end - start;
            std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(delta);
             
            if(status == -102){ // ROUTING_DEFERRED
                _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% deferred") % remote.address() % _req.method() % path;
                return;
            }
            
            if(status == 0){
                boost::filesystem::path doc_root = _attachment.aux().docroot();
                boost::filesystem::path local_path = internal::path_cat(doc_root, path);
                if(!internal::path_inside(doc_root, local_path)){
                    _attachment << udho::logging::messages::formatted::warning("router", "%1% %2% %3% access denied for %4%") % remote.address() % _req.method() % path % local_path;
                    throw exceptions::http_error(boost::beast::http::status::forbidden, (boost::format("Access denied to %1%") % local_path).str());
                }
                std::string extension = local_path.extension().string();
                std::string mime_type = _attachment.aux().config()[udho::configs::server::mime_default];
                if(!extension.empty() && extension.front() == '.'){
                    extension = extension.substr(1);
                    mime_type = _attachment.aux().config()[udho::configs::server::mimes].of(extension);
                }
                _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% looking for %4%") % remote.address() % _req.method() % path % local_path;
                boost::beast::error_code err;
                http::file_body::value_type body;
                body.open(local_path.c_str(), boost::beast::file_mode::scan, err);
                if(err == boost::system::errc::no_such_file_or_directory){
                    _attachment << udho::logging::messages::formatted::warning("router", "%1% %2% %3% not found %4% %5%μs") % remote.address() % _req.method() % path % local_path % ms.count();
                    throw exceptions::http_error(boost::beast::http::status::not_found);
                }else{
                    _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% found %4%") % remote.address() % _req.method() % path % local_path;
                }
                if(err){
                    _attachment << udho::logging::messages::formatted::warning("router", "%1% %2% %3% %4%μs") % remote.address() % _req.method() % path % ms.count();
                    throw exceptions::http_error(boost::beast::http::status::internal_server_error, (boost::format("Error %1% while reading file `%2%` from disk") % err % local_path).str());
                }
                auto const size = body.size();
                if(_req.method() == boost::beast::http::verb::head){
                    http::response<boost::beast::http::string_body> res{http::status::ok, _req.version()};
                    res.set(http::field::server, UDHO_VERSION_STRING);
                    res.set(http::field::content_type, mime_type);
                    res.content_length(size);
                    res.keep_alive(_req.keep_alive());
                    _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% %4% %5% %6%μs") % remote.address() % 200 % http::status::ok % _req.method() % path % ms.count();
                    _lambda(std::move(res));
                    return;
                }
                // Respond to GET request
                http::response<http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(http::status::ok, _req.version())};
                res.set(http::field::server, UDHO_VERSION_STRING);
                res.set(http::field::content_type, mime_type);
                res.content_length(size);
                res.keep_alive(_req.keep_alive());
                return _lambda(std::move(res));
            }else{
                http::status response = static_cast<http::status>(status);
                _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% %4% %5% %6%μs") % remote.address() % status % response % _req.method() % path % ms.count();
            }
        }catch(const exceptions::http_error& ex){
            auto res = ex.response(_req, _router);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> delta = end - start;
            std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(delta);
            _attachment << udho::logging::messages::formatted::warning("router", "%1% %2% %3% %4% %5% %6%μs") % remote.address() % (int) ex.result() % ex.result() % _req.method() % path % ms.count();
            return _lambda(std::move(res));
        }
    }
    void on_write(boost::system::error_code /*ec*/, std::size_t bytes_transferred, bool close){
        boost::ignore_unused(bytes_transferred);
        if(close){
            return do_close();
        }
        res_ = nullptr;
        do_read();
    }
    void do_close(){
        boost::system::error_code ec;
        _socket.shutdown(tcp::socket::shutdown_send, ec);
    }
    void respond(udho::defs::response_type& msg){
        std::string path;
        std::string target_str(_req.target());
        std::stringstream path_stream(target_str);
        std::getline(path_stream, path, '?');
        
        boost::posix_time::time_duration diff = boost::posix_time::second_clock::local_time() - _time;
        
        const std::lock_guard<std::mutex> lock(_respond_mutex);
        if(!_responded){
            _attachment << udho::logging::messages::formatted::info("router", "%1% %2% %3% responded after %4% delay") % _socket.remote_endpoint().address() % _req.method() % path % diff;
            _lambda(std::move(msg));
            _responded = true;
        }else{
            _attachment << udho::logging::messages::formatted::warning("router", "%1% %2% %3% discarded redundant response after %4% delay") % _socket.remote_endpoint().address() % _req.method() % path % diff;
            std::cout << msg << std::endl << "--------------------" << std::endl;
        }
    }
};

}


#endif // UDHO_CONNECTION_H
