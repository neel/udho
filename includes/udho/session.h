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
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include "page.h"

namespace bya{
namespace ka{

namespace internal{
    boost::beast::string_view mime_type(boost::beast::string_view path);
    std::string path_cat(boost::beast::string_view base, boost::beast::string_view path);
}
    
using tcp = boost::asio::ip::tcp; 
namespace http = boost::beast::http;

template <typename RouterT>
class session : public std::enable_shared_from_this<session<RouterT>>{
    RouterT& _router;
    typedef session<RouterT> self_type;
    struct send_lambda{
        self_type& self_;

        explicit send_lambda(self_type& self): self_(self){}

//         void operator()(const nlohmann::json& json) const{
//             std::string results_str = json.dump();
//             boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok};
//             res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
//             res.set(http::field::content_type,   "application/json");
//             res.set(http::field::content_length, results_str.size());
//             res.body() = results_str;
//             res.prepare_payload();
//         }
        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const {
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));
            self_.res_ = sp;

            http::async_write(self_._socket, *sp, boost::asio::bind_executor(self_._strand, std::bind(&self_type::on_write, self_.shared_from_this(), std::placeholders::_1, std::placeholders::_2, sp->need_eof())));
        }
    };

    tcp::socket _socket;
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
    boost::beast::flat_buffer _buffer;
    std::shared_ptr<std::string const> _doc_root;
    http::request<http::string_body> _req;
    std::shared_ptr<void> res_;
    send_lambda _lambda;
  public:
    explicit session(RouterT& router, tcp::socket socket, std::shared_ptr<std::string const> const& doc_root): _router(router), _socket(std::move(socket)), _strand(_socket.get_executor()), _doc_root(doc_root), _lambda(*this){}
    void run(){
        do_read();
    }
    void do_read(){
        _req = {};
        http::async_read(_socket, _buffer, _req, boost::asio::bind_executor(_strand, std::bind(&self_type::on_read, std::enable_shared_from_this<session<RouterT>>::shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
    }
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred){
        boost::ignore_unused(bytes_transferred);
        if(ec == http::error::end_of_stream)
            return do_close();
        std::string path;
        std::stringstream path_stream(_req.target().to_string());
        std::getline(path_stream, path, '?');
        auto response = _router.serve(_req, _req.method(), path, _lambda);
        if(response == http::status::unknown){
            std::string local_path = internal::path_cat(*_doc_root, path);
            boost::beast::error_code ec;
            http::file_body::value_type body;
            body.open(local_path.c_str(), boost::beast::file_mode::scan, ec);
            if(ec == boost::system::errc::no_such_file_or_directory){
                auto res = bya::ka::page::not_found(_req, "Not Found");
                _lambda(std::move(res));
                return;
            }
            if(ec){
                auto res = bya::ka::page::internal_error(_req, "Unknown Error Occured");
                _lambda(std::move(res));
                return;
            }
            auto const size = body.size();
            if(_req.method() == boost::beast::http::verb::head){
                http::response<boost::beast::http::string_body> res{http::status::ok, _req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, internal::mime_type(local_path));
                res.content_length(size);
                res.keep_alive(_req.keep_alive());
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
        }
    }
    void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close){
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
}


#endif // SESSION_H
