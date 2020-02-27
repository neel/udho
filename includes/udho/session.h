#ifndef SESSION_H
#define SESSION_H

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <udho/req.h>
#include "logging.h"
#include "page.h"

namespace udho{

namespace internal{
    boost::beast::string_view mime_type(boost::beast::string_view path);
    std::string path_cat(boost::beast::string_view base, boost::beast::string_view path);
}
    
using tcp = boost::asio::ip::tcp; 
namespace http = boost::beast::http;

/**
 * Stateful HTTP Session
 */
template <typename RouterT, typename AttachmentT>
class session : public std::enable_shared_from_this<session<RouterT, AttachmentT>>{    
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket_type;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket_type;
#endif
    
    RouterT& _router;
    AttachmentT& _attachment;
    typedef session<RouterT, AttachmentT> self_type;
    typedef AttachmentT attachment_type;
    typedef udho::req<http::request<http::string_body>, AttachmentT> req_type;
    
    struct send_lambda{
        self_type& self_;

        explicit send_lambda(self_type& self): self_(self){}

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
    std::shared_ptr<std::string const> _doc_root;
    http::request<http::string_body> _req;
    std::shared_ptr<void> res_;
    send_lambda _lambda;
  public:
    /**
     * session constructor
     * @param router router
     * @param socket TCP socket
     * @param doc_root document root to serve static contents
     */
    explicit session(RouterT& router, attachment_type& attachment, socket_type socket, std::shared_ptr<std::string const> const& doc_root): _router(router), _attachment(attachment), _socket(std::move(socket)), _strand(_socket.get_executor()), _doc_root(doc_root), _lambda(*this){}
    /**
     * start the read loop
     */
    void run(){
        do_read();
    }
    void do_read(){
        _req = {};
        http::async_read(_socket, _buffer, _req, boost::asio::bind_executor(_strand, std::bind(&self_type::on_read, std::enable_shared_from_this<session<RouterT, AttachmentT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
    }
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred){
        boost::ignore_unused(bytes_transferred);
        if(ec == http::error::end_of_stream)
            return do_close();
        std::string path;
        std::stringstream path_stream(_req.target().to_string());
        std::getline(path_stream, path, '?');
        auto start = std::chrono::high_resolution_clock::now();
        try{
            auto response = _router.serve(req_type(_req, _attachment), _req.method(), path, _lambda);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> delta = end - start;
            std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(delta);
            
            if(response == http::status::unknown){
                std::string local_path = internal::path_cat(*_doc_root, path);
                boost::beast::error_code err;
                http::file_body::value_type body;
                body.open(local_path.c_str(), boost::beast::file_mode::scan, err);
                if(err == boost::system::errc::no_such_file_or_directory){
                    _router.log(udho::logging::status::info, udho::logging::segment::router, (boost::format("%1% %2% %3% %4% %5% %6%μs") % _socket.remote_endpoint().address() % (int) response % response % _req.method() % path % ms.count()).str());
                    throw exceptions::http_error(boost::beast::http::status::not_found, path);
                }
                if(err){
                    _router.log(udho::logging::status::info, udho::logging::segment::router, (boost::format("%1% %2% %3% %4% %5% %6%μs") % _socket.remote_endpoint().address() % (int) response % response % _req.method() % path % ms.count()).str());
                    throw exceptions::http_error(boost::beast::http::status::internal_server_error, path);
                }
                auto const size = body.size();
                if(_req.method() == boost::beast::http::verb::head){
                    http::response<boost::beast::http::string_body> res{http::status::ok, _req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(http::field::content_type, internal::mime_type(local_path));
                    res.content_length(size);
                    res.keep_alive(_req.keep_alive());
                    _router.log(udho::logging::status::info, udho::logging::segment::router, (boost::format("%1% %2% %3% %4% %5% %6%μs") % _socket.remote_endpoint().address() % 200 % http::status::ok % _req.method() % path % ms.count()).str());
                    _lambda(std::move(res));
                    return;
                }
                // Respond to GET request
                http::response<http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(http::status::ok, _req.version())};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, internal::mime_type(local_path));
                res.content_length(size);
                res.keep_alive(_req.keep_alive());
                return _lambda(std::move(res));
            }else{
                _router.log(udho::logging::status::info, udho::logging::segment::router, (boost::format("%1% %2% %3% %4% %5% %6%μs") % _socket.remote_endpoint().address() % (int) response % response % _req.method() % path % ms.count()).str());
            }
        }catch(const exceptions::http_error& ex){
            auto res = ex.response(_req, _router);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> delta = end - start;
            std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(delta);
            _router.log(udho::logging::status::info, udho::logging::segment::router, (boost::format("%1% %2% %3% %4% %5% %6%μs") % _socket.remote_endpoint().address() % (int) ex.result() % ex.result() % _req.method() % path % ms.count()).str());
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
};

}


#endif // SESSION_H
