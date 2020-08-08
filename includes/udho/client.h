/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_CLIENT_H
#define UDHO_CLIENT_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>
#include <udho/configuration.h>
#include <udho/form.h>
#include <udho/url.h>
#include <iostream>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

namespace udho{

namespace detail{
    
template <typename ContextT>
struct async_result{
    typedef ContextT context_type;
    typedef async_result<ContextT> self_type;
    typedef udho::config<udho::client_options> options_type;
    
    typedef boost::function<void (ContextT, const boost::beast::http::response<boost::beast::http::string_body>&)> success_callback_type;
    typedef boost::function<void (ContextT, const boost::beast::error_code&)> error_callback_type;
    typedef boost::function<void (const boost::beast::error_code&)> error_callback_type_aux;
    
    typedef boost::function<void (const boost::beast::http::response<boost::beast::http::string_body>&)> success_callback_type_aux_r;
    typedef boost::function<void (ContextT, boost::beast::http::status, const std::string&)> success_callback_type_aux_xsc;
    typedef boost::function<void (ContextT, const std::string&)> success_callback_type_aux_xc;
    typedef boost::function<void (boost::beast::http::status, const std::string&)> success_callback_type_aux_sc;
    typedef boost::function<void (const std::string&)> success_callback_type_aux_c;
    
    context_type          _ctx;
    success_callback_type _callback;
    error_callback_type   _ecallback;
    options_type          _options;
    
    async_result() = delete;
    async_result(const context_type& ctx, options_type options): _ctx(ctx), _options(options){}
    async_result(const self_type& other) = delete;
    
    template <typename KeyT, typename ValueT>
    self_type& option(KeyT key, ValueT value){
        _options[key] = value;
        return *this;
    }
    void success(const boost::beast::http::response<boost::beast::http::string_body>& res){
        if(!_callback.empty()){
            bool is_redirected = res.count(boost::beast::http::field::location);
            if(is_redirected && _options[udho::client_options::follow_redirect]){
                std::string redirected_url(res[boost::beast::http::field::location]);
                _ctx.client(_options).request(boost::beast::http::verb::get, udho::url(redirected_url)).then(_callback).failed(_ecallback);
            }else{
                _callback(_ctx, res);
            }
        }else{
            std::cout << "No callback added" << std::endl;
        }
    }
    void failure(const boost::beast::error_code& ec){
        if(!_ecallback.empty()){
            _ecallback(_ctx, ec);
        }
    }
    self_type& then(success_callback_type cb){
        _callback = cb;
        return *this;
    }
    self_type& failed(error_callback_type cb){
        _ecallback = cb;
        return *this;
    }
    self_type& error(error_callback_type_aux cb){
        return failed([cb](context_type, const boost::beast::error_code& ec) mutable {
            cb(ec);
        });
    }
    self_type& fetch(success_callback_type_aux_xsc cb){
        return then([cb, this](context_type, const boost::beast::http::response<boost::beast::http::string_body>& res) mutable -> void{
            cb(this->_ctx, res.result(), res.body());
        });
    }
    self_type& body(success_callback_type_aux_xc cb){
        return then([cb, this](context_type, const boost::beast::http::response<boost::beast::http::string_body>& res) mutable -> void{
            cb(this->_ctx, res.body());
        });
    }
    self_type& after(success_callback_type_aux_r cb){
        return then([cb](context_type, const boost::beast::http::response<boost::beast::http::string_body>& res) mutable -> void{
            cb(res);
        });
    }
    self_type& done(success_callback_type_aux_sc cb){
        return then([cb](context_type, const boost::beast::http::response<boost::beast::http::string_body>& res) mutable -> void{
            cb(res.result(), res.body());
        });
    }
    self_type& content(success_callback_type_aux_c cb){
        return then([cb](context_type, const boost::beast::http::response<boost::beast::http::string_body>& res) mutable -> void{
            cb(res.body());
        });
    }
};
    
/**
 * @todo write docs
 */
template <typename ContextT>
struct https_client_connection: public std::enable_shared_from_this<https_client_connection<ContextT>>, public async_result<ContextT>{
    typedef std::enable_shared_from_this<https_client_connection<ContextT>> base;
    typedef async_result<ContextT> result_type;
    typedef https_client_connection<ContextT> self_type;
    typedef ContextT context_type;
    typedef udho::config<udho::client_options> options_type;
    typedef boost::function<void (const std::string&)> redirector_type;
    
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::ssl_stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::empty_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    options_type _options;
    
    explicit https_client_connection(ContextT context, boost::asio::executor ex, boost::asio::ssl::context& ssl_ctx, options_type options): result_type(context, options), resolver(ex), stream(ex, ssl_ctx), _options(options){}
    void start(const udho::url& u, boost::beast::http::verb method = boost::beast::http::verb::get, int version = 11){
        std::string host   = u[url::host];
        std::string port   = std::to_string(u[url::port]);
        std::string target = u[url::target];
        
        if(!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())){
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cout << ec.message() << std::endl;
            result_type::failure(ec);
            return;
        }
        req.version(version);
        req.method(method);
        req.target(target);
        std::cout << "https request target " << target << std::endl;
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver.async_resolve(host, port, std::bind(&self_type::on_resolve, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results){
        if(ec){
            std::cout << "failed to resolve" << std::endl;
            std::cout << ec.message() << std::endl;
            result_type::failure( ec);
            return;
        }
        boost::asio::async_connect(stream.next_layer(), results.begin(), results.end(), std::bind(&self_type::on_connect, base::shared_from_this(), std::placeholders::_1));
    }
    void on_connect(boost::beast::error_code ec){
        if(ec){
            std::cout << "failed to connect" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure(ec);
            return;
        }
        stream.async_handshake(boost::asio::ssl::stream_base::client, std::bind(&self_type::on_handshake, base::shared_from_this(), std::placeholders::_1));
    }
    void on_handshake(boost::beast::error_code ec) {
        if(ec){
            std::cout << "failed to handshake" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure(ec);
            return;
        }
        boost::beast::http::async_write(stream, req, std::bind(&self_type::on_write, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_write(boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
        if(ec){
            std::cout << "failed to write" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure(ec);
            return;
        }
        boost::beast::http::async_read(stream, buffer, res, std::bind(&self_type::on_read, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_read(boost::beast::error_code ec, std::size_t /*bytes_transferred*/){
        if(ec){
            std::cout << "failed to read" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure( ec);
            return;
        }
        std::cout << res << std::endl;
        result_type::success(res);
        stream.async_shutdown(std::bind(&self_type::on_shutdown, base::shared_from_this(), std::placeholders::_1));
    }
    void on_shutdown(boost::beast::error_code ec) {
        if(ec == boost::asio::error::eof || ec == boost::asio::ssl::error::stream_truncated){
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if(ec){
            std::cout << "failed to shutdown" << std::endl;
            std::cout << ec.message() << std::endl;
            result_type::failure(ec);
            return;
        }
    }
    
    static std::shared_ptr<self_type> create(boost::asio::io_service& io, ContextT ctx, options_type options){
        boost::asio::ssl::context ssl_ctx{boost::asio::ssl::context::tlsv12_client};
        if(!options[udho::client_options::verify_certificate]){
            ssl_ctx.set_verify_mode(boost::asio::ssl::context::verify_none);
        }else{
            ssl_ctx.set_verify_mode(boost::asio::ssl::context::verify_peer);
            boost::certify::enable_native_https_server_verification(ssl_ctx);
        }
        std::shared_ptr<self_type> connection = std::make_shared<self_type>(ctx, boost::asio::make_strand(io), ssl_ctx, options);
        return connection;
    }
    result_type& result(){
        return *this;
    }
};

/**
 * @todo write docs
 */
template <typename ContextT>
struct http_client_connection: public std::enable_shared_from_this<http_client_connection<ContextT>>, public async_result<ContextT>{
    typedef std::enable_shared_from_this<http_client_connection<ContextT>> base;
    typedef async_result<ContextT> result_type;
    typedef http_client_connection<ContextT> self_type;
    typedef ContextT context_type;
    typedef udho::config<udho::client_options> options_type;
    typedef boost::function<void (const std::string&)> redirector_type;
    
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::socket socket;
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::empty_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    options_type _options;
    
    explicit http_client_connection(ContextT context, boost::asio::executor ex, options_type options): result_type(context, options), resolver(ex), socket(ex), _options(options){}
    void start(const udho::url& u, boost::beast::http::verb method = boost::beast::http::verb::get, int version = 11){
        std::string host   = u[url::host];
        std::string port   = std::to_string(u[url::port]);
        std::string target = u[url::target];
        
        req.version(version);
        req.method(method);
        req.target(target);
        std::cout << "http request target " << target << std::endl;
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver.async_resolve(host, port, std::bind(&self_type::on_resolve, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results){
        if(ec){
            std::cout << "failed to resolve" << std::endl;
            std::cout << ec.message() << std::endl;
            result_type::failure( ec);
            return;
        }
        boost::asio::async_connect(socket, results.begin(), results.end(), std::bind(&self_type::on_connect, base::shared_from_this(), std::placeholders::_1));
    }
    void on_connect(boost::beast::error_code ec){
        if(ec){
            std::cout << "failed to connect" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure(ec);
            return;
        }
        boost::beast::http::async_write(socket, req, std::bind(&self_type::on_write, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_write(boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
        if(ec){
            std::cout << "failed to write" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure(ec);
            return;
        }
        boost::beast::http::async_read(socket, buffer, res, std::bind(&self_type::on_read, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_read(boost::beast::error_code ec, std::size_t /*bytes_transferred*/){
        if(ec){
            std::cout << "failed to read" << std::endl;
            std::cout << ec << std::endl;
            result_type::failure( ec);
            return;
        }
        std::cout << res << std::endl;
        result_type::success(res);
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        on_shutdown(ec);
    }
    void on_shutdown(boost::beast::error_code ec) {
        if(ec == boost::asio::error::eof || ec == boost::asio::ssl::error::stream_truncated){
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if(ec){
            std::cout << "failed to shutdown" << std::endl;
            std::cout << ec.message() << std::endl;
            result_type::failure(ec);
            return;
        }
    }
    
    static std::shared_ptr<self_type> create(boost::asio::io_service& io, ContextT ctx, options_type options){
        std::shared_ptr<self_type> connection = std::make_shared<self_type>(ctx, boost::asio::make_strand(io), options);
        return connection;
    }
    result_type& result(){
        return *this;
    }
};
    
template <typename ContextT>
struct client_connection_wrapper{
    typedef client_connection_wrapper<ContextT> self_type;
    typedef udho::detail::async_result<ContextT> result_type;
    typedef udho::config<udho::client_options> options_type;
    
    boost::asio::io_service& _io;
    ContextT _context;
    options_type _options;
    
    explicit inline client_connection_wrapper(boost::asio::io_service& io, ContextT ctx, options_type options): _io(io), _context(ctx), _options(options){}
    result_type& request(boost::beast::http::verb method, udho::url url){
        self_type self(*this);
        
        std::string protocol = url[udho::url::protocol];
        if(protocol == "https"){
            if(!url[udho::url::port]){
                url[udho::url::port] = 443;
            }
            auto connection = udho::detail::https_client_connection<ContextT>::create(_io, _context, _options);
            connection->start(url, method);
            return connection->result();
        }else{ // assuming http
            if(!url[udho::url::port]){
                url[udho::url::port] = 80;
            }
            auto connection = udho::detail::http_client_connection<ContextT>::create(_io, _context, _options);
            connection->start(url, method);
            return connection->result();
        }
    }
    result_type& get(udho::url url){
        return request(boost::beast::http::verb::get, url);
    }
    result_type& post(udho::url url){
        return request(boost::beast::http::verb::post, url);
    }
    result_type& put(udho::url url){
        return request(boost::beast::http::verb::put, url);
    }
    
    result_type& get(const std::string& url){
        return get(udho::url(url));
    }
    result_type& post(const std::string& url){
        return post(udho::url(url));
    }
    result_type& put(const std::string& url){
        return put(udho::url(url));
    }
};

}

}

#endif // UDHO_CLIENT_H
