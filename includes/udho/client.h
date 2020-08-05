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

#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace udho{
    
/**
 * @todo write docs
 */
template <typename CallbackT>
struct https_client_connection: public std::enable_shared_from_this<https_client_connection<CallbackT>>{
    typedef boost::enable_shared_from_this<https_client_connection<CallbackT>> base;
    typedef https_client_connection<CallbackT> self_type;
    
    CallbackT _callback;
    boost::asio::ip::tcp::resolver resolver;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream;
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::empty_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    
    explicit https_client_connection(CallbackT cb, boost::asio::executor ex, boost::asio::ssl::context& ctx): _callback(cb), resolver(ex), stream(ex, ctx){}
    void run(const char* host, const char* port, const char* target, int version){
        if(!SSL_set_tlsext_host_name(stream.native_handle(), host)){
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cout << ec.message() << std::endl;
            return;
        }
        req.version(version);
        req.method(boost::beast::http::verb::get);
        req.target(target);
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver.async_resolve(host, port, boost::beast::bind_front_handler(&self_type::on_resolve, base::shared_from_this()));
    }
    void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results){
        if(ec) std::cout << "failed to resolve" << std::endl;
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        boost::beast::get_lowest_layer(stream).async_connect(results, boost::beast::bind_front_handler(&self_type::on_connect, base::shared_from_this()));
    }
    void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type){
        if(ec) std::cout << "failed to connect" << std::endl;
        stream.async_handshake(boost::asio::ssl::stream_base::client, boost::beast::bind_front_handler(&self_type::on_handshake, base::shared_from_this()));
    }
    void on_handshake(boost::beast::error_code ec) {
        if(ec) std::cout << "failed to handshake" << std::endl;
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        boost::beast::http::async_write(stream, req, boost::beast::bind_front_handler(&self_type::on_write, base::shared_from_this()));
    }
    void on_write(boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
        if(ec) std::cout << "failed to write" << std::endl;
        boost::beast::http::async_read(stream, buffer, res, boost::beast::bind_front_handler(&self_type::on_read, base::shared_from_this()));
    }
    void on_read(boost::beast::error_code ec, std::size_t /*bytes_transferred*/){
        if(ec) std::cout << "failed to read" << std::endl;
        std::cout << res << std::endl;
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        stream.async_shutdown(boost::beast::bind_front_handler(&self_type::on_shutdown, base::shared_from_this()));
        
    }
    void on_shutdown(boost::beast::error_code ec) {
        if(ec == boost::asio::error::eof){
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if(ec) std::cout << "failed to shutdown" << std::endl;
    }
};

template <typename CallbackT>
boost::shared_ptr<https_client_connection<CallbackT>> connect_https(boost::asio::io_context& io, CallbackT cb){
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    ctx.set_verify_mode(boost::asio::ssl::context::verify_peer);
    boost::certify::enable_native_https_server_verification(ctx);

    return std::make_shared<https_client_connection<CallbackT>>(boost::asio::make_strand(io), ctx);
}

}

#endif // UDHO_CLIENT_H
