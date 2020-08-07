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
#include <iostream>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
// #include <boost/certify/extensions.hpp>
// #include <boost/certify/https_verification.hpp>

namespace udho{
namespace detail{
    
template <typename T = void>
struct url_data_{
    const static struct protocol_t{
        typedef url_data_<T> component;
    } protocol;
    const static struct host_t{
        typedef url_data_<T> component;
    } host;
    const static struct port_t{
        typedef url_data_<T> component;
    } port;
    const static struct target_t{
        typedef url_data_<T> component;
    } target;
    const static struct path_t{
        typedef url_data_<T> component;
    } path;
    const static struct query_t{
        typedef url_data_<T> component;
    } query;
    
    std::string _protocol;
    std::string _host;
    std::size_t _port;
    std::string _path;
    std::string _target;
    std::string _query;
    
    url_data_(): _port(0){}
    
    void set(protocol_t, const std::string& v){_protocol = v;}
    std::string get(protocol_t) const{return _protocol;}
    
    void set(host_t, const std::string& v){_host = v;}
    std::string get(host_t) const{return _host;}
    
    void set(port_t, std::size_t v){_port = v;}
    std::size_t get(port_t) const{return _port;}
    
    void set(target_t, const std::string& v){_target = v;}
    std::string get(target_t) const{return _target;}
    
    void set(path_t, const std::string& v){_path = v;}
    std::string get(path_t) const{return _path;}
    
    void set(query_t, const std::string& v){_query = v;}
    const std::string& get(query_t) const{return _query;}
};

template <typename T> const typename url_data_<T>::protocol_t   url_data_<T>::protocol;
template <typename T> const typename url_data_<T>::host_t       url_data_<T>::host;
template <typename T> const typename url_data_<T>::port_t       url_data_<T>::port;
template <typename T> const typename url_data_<T>::target_t     url_data_<T>::target;
template <typename T> const typename url_data_<T>::path_t       url_data_<T>::path;
template <typename T> const typename url_data_<T>::query_t      url_data_<T>::query;

typedef url_data_<> url_data;
      
}

struct url: udho::configuration<detail::url_data>, udho::urlencoded_form<std::string::const_iterator>{
    explicit url(const std::string& url_str){
        parse(url_str);
    }
    private:
        inline void parse(const std::string& url){
            std::string proto, hst, prt, pth, tgt, qry;
            std::string protocol_terminal = "://";
            std::string::const_iterator protocol_end = std::search(url.begin(), url.end(), protocol_terminal.begin(), protocol_terminal.end());
            if(protocol_end != url.end()){
                std::copy(url.begin(), protocol_end, std::back_inserter(proto));
                std::advance(protocol_end, 3);
            }else{
                protocol_end = url.begin();
            }
            std::string terminals = ":/";
            std::string::const_iterator it = std::find_first_of(protocol_end, url.end(), terminals.begin(), terminals.end());
            std::copy(protocol_end, it, std::back_inserter(hst));
            if(it != url.end()){
                if(*it == ':'){
                    std::string::const_iterator slash_it = std::find(++it, url.end(), '/');
                    std::copy(it, slash_it, std::back_inserter(prt));
                    it = slash_it;
                }
                std::copy(it, url.end(), std::back_inserter(tgt));
                std::string::const_iterator query_it = std::find(it, url.end(), '?');
                std::copy(it, query_it, std::back_inserter(pth));
                std::copy(++query_it, url.end(), std::back_inserter(qry));
            }
            
            (*this)[protocol] = proto;
            (*this)[host]     = hst;
            (*this)[port]     = !prt.empty() ? std::stoul(prt) : 0;
            (*this)[path]     = !pth.empty() ? pth : std::string("/");
            (*this)[target]   = tgt;
            (*this)[query]    = qry;
            
            const std::string& qstr_ref = (*this)[query];
            
            udho::urlencoded_form<std::string::const_iterator>::parse(qstr_ref.begin(), qstr_ref.end());
        }
};

namespace detail{
/**
 * @todo write docs
 */
template <typename ContextT, typename CallbackT>
struct https_client_connection: public std::enable_shared_from_this<https_client_connection<ContextT, CallbackT>>{
    typedef std::enable_shared_from_this<https_client_connection<ContextT, CallbackT>> base;
    typedef https_client_connection<ContextT, CallbackT> self_type;
    
    ContextT _context;
    CallbackT _callback;
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream;
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::empty_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    
    explicit https_client_connection(ContextT context, boost::asio::executor ex, CallbackT cb, boost::asio::ssl::context& ssl_ctx): _context(context), _callback(cb), resolver(ex), stream(ex, ssl_ctx){}
    void start(const udho::url& u, boost::beast::http::verb method = boost::beast::http::verb::get, int version = 11){
        std::string host   = u[url::host];
        std::string port   = std::to_string(u[url::port]);
        std::string target = u[url::target];
        
        if(!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())){
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cout << ec.message() << std::endl;
            return;
        }
        req.version(version);
        req.method(method);
        req.target(target);
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver.async_resolve(host, port, std::bind(&self_type::on_resolve, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results){
        if(ec) std::cout << "failed to resolve" << std::endl;
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        boost::beast::get_lowest_layer(stream).async_connect(results, std::bind(&self_type::on_connect, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type){
        if(ec){
            std::cout << "failed to connect" << std::endl;
            std::cout << ec << std::endl;
            return;
        }
        stream.async_handshake(boost::asio::ssl::stream_base::client, std::bind(&self_type::on_handshake, base::shared_from_this(), std::placeholders::_1));
    }
    void on_handshake(boost::beast::error_code ec) {
        if(ec){
            std::cout << "failed to handshake" << std::endl;
            std::cout << ec << std::endl;
            return;
        }
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        boost::beast::http::async_write(stream, req, std::bind(&self_type::on_write, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_write(boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
        if(ec){
            std::cout << "failed to write" << std::endl;
            std::cout << ec << std::endl;
            return;
        }
        boost::beast::http::async_read(stream, buffer, res, std::bind(&self_type::on_read, base::shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    void on_read(boost::beast::error_code ec, std::size_t /*bytes_transferred*/){
        if(ec){
            std::cout << "failed to read" << std::endl;
            std::cout << ec << std::endl;
            return;
        }
        std::cout << res << std::endl;
        _callback(_context, res);
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        stream.async_shutdown(std::bind(&self_type::on_shutdown, base::shared_from_this(), std::placeholders::_1));
    }
    void on_shutdown(boost::beast::error_code ec) {
        if(ec == boost::asio::error::eof){
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if(ec){
            std::cout << "failed to shutdown" << std::endl;
            std::cout << ec << std::endl;
        }
    }
};
    
template <typename ContextT>
struct https_connection_wrapper{
    boost::asio::io_service& _io;
    ContextT _context;
    udho::url _url;
    
    explicit inline https_connection_wrapper(boost::asio::io_service& io, ContextT ctx, udho::url u): _io(io), _context(ctx), _url(u){}
    template <typename CallbackT>
    void request(boost::beast::http::verb method, CallbackT cb){
        boost::asio::ssl::context ssl_ctx{boost::asio::ssl::context::tlsv12_client};
        ssl_ctx.set_verify_mode(boost::asio::ssl::context::verify_none);
//         boost::certify::enable_native_https_server_verification(ssl_ctx);
        
        std::shared_ptr<udho::detail::https_client_connection<ContextT, CallbackT>> connection = std::make_shared<udho::detail::https_client_connection<ContextT, CallbackT>>(_context, boost::asio::make_strand(_io), cb, ssl_ctx);
        connection->start(_url, method);
    }
    template <typename CallbackT>
    void get(CallbackT cb){
        request(boost::beast::http::verb::get, cb);
    }
    template <typename CallbackT>
    void post(CallbackT cb){
        request(boost::beast::http::verb::post, cb);
    }
    template <typename CallbackT>
    void put(CallbackT cb){
        request(boost::beast::http::verb::put, cb);
    }
};

}

}

#endif // UDHO_CLIENT_H
