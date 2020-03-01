/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_SERVER_H
#define UDHO_SERVER_H

#include <udho/context.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <udho/logging.h>
#include <udho/session.h>

namespace udho{

namespace http = boost::beast::http;
    
/**
 * @todo write docs
 */
template <typename AttachmentT>
struct server{
    template <typename RequestT>
    using context_type = udho::context<RequestT, AttachmentT>;

    typedef AttachmentT attachment_type;
    typedef server<attachment_type> self_type;
    typedef http::request<http::string_body> http_request_type;
    typedef context_type<http_request_type> context;
    
    boost::asio::io_service& _io;
    attachment_type& _attachment;
    
    server(boost::asio::io_service& io, attachment_type& attachment): _io(io), _attachment(attachment){}
    server(const self_type&) = delete;
    server(self_type&& other) = default;
    template <typename RouterT>
    void serve(RouterT&& router, int port=9198, std::string doc_root=""){
        _attachment.log(udho::logging::status::info, udho::logging::segment::server, "server started");
        router.template listen<attachment_type>(_io, _attachment, port, doc_root);
    }
};

template <typename LoggerT>
struct server<udho::attachment<LoggerT>>{
    template <typename RequestT>
    using context_type = udho::context<RequestT, udho::attachment<LoggerT>>;

    typedef udho::attachment<LoggerT> attachment_type;
    typedef server<attachment_type> self_type;
    typedef http::request<http::string_body> http_request_type;
    typedef context_type<http_request_type> context;
    
    boost::asio::io_service& _io;
    attachment_type _attachment;
    
    server(boost::asio::io_service& io, LoggerT& logger): _io(io), _attachment(logger){}
    server(const self_type&) = delete;
    server(self_type&& other) = default;
    template <typename RouterT>
    void serve(RouterT&& router, int port=9198, std::string doc_root=""){
        _attachment.log(udho::logging::status::info, udho::logging::segment::server, "server started");
        router.template listen<attachment_type>(_io, _attachment, port, doc_root);
    }
};

template <>
struct server<void>{
    template <typename RequestT>
    using context_type = udho::context<RequestT, void>;

    typedef void attachment_type;
    typedef server<attachment_type> self_type;
    typedef http::request<http::string_body> http_request_type;
    typedef context_type<http_request_type> context;
    
    boost::asio::io_service& _io;
    
    server(boost::asio::io_service& io): _io(io){}
    server(const self_type&) = delete;
    server(self_type&& other) = default;
    template <typename RouterT>
    void serve(RouterT&& router, int port=9198, std::string doc_root=""){
        router.template listen<attachment_type>(_io, port, doc_root);
    }
};

namespace servers{
template <typename LoggerT=udho::loggers::ostream, typename... T>
struct logging: server<udho::attachment<LoggerT, udho::session<T...>>>{
    typedef server<udho::attachment<LoggerT, udho::session<T...>>> base_type;
    
    using base_type::base_type;
};

template <typename... T>
struct logging<udho::loggers::ostream, T...>{
    template <typename RequestT>
    using context_type = udho::context<RequestT, udho::attachment<udho::loggers::ostream, udho::session<T...>>>;

    typedef udho::attachment<udho::loggers::ostream, udho::session<T...>> attachment_type;
    typedef logging<udho::loggers::ostream, T...> self_type;
    typedef http::request<http::string_body> http_request_type;
    typedef context_type<http_request_type> context;
    
    boost::asio::io_service& _io;
    udho::loggers::ostream _logger;
    attachment_type _attachment;
    
    logging(boost::asio::io_service& io, std::ostream& stream): _io(io), _logger(stream), _attachment(_logger){}
    logging(const self_type&) = delete;
    logging(self_type&& other) = default;
    template <typename RouterT>
    void serve(RouterT&& router, int port=9198, std::string doc_root=""){
        _attachment.log(udho::logging::status::info, udho::logging::segment::server, "server started");
        router.template listen<attachment_type>(_io, _attachment, port, doc_root);
    }
};

typedef logging<> logged;
typedef server<void> quiet;
    
}

}

#endif // UDHO_SERVER_H
