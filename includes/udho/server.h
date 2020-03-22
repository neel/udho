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
#include <udho/defs.h>
#include <udho/attachment.h>

namespace udho{

namespace http = boost::beast::http;
    
/**
 * @todo write docs
 */
template <typename LoggerT=void, typename CacheT=void>
struct server{
    typedef LoggerT logger_type;
    typedef CacheT cache_type;
    typedef udho::attachment<logger_type, cache_type> attachment_type;
    typedef server<logger_type, cache_type> self_type;
    
    template <typename RequestT>
    using context_type = udho::context<RequestT, attachment_type>;
    typedef udho::defs::request_type http_request_type;
    typedef http_request_type request_type;
    
    boost::asio::io_service& _io;
    attachment_type _attachment;
    
    server(boost::asio::io_service& io, logger_type& logger): _io(io), _attachment(logger){}
    server(const self_type&) = delete;
    server(self_type&& other) = default;
    template <typename RouterT>
    void serve(RouterT&& router, int port=9198, std::string doc_root=""){
#ifdef WITH_ICU
        _attachment << udho::logging::messages::formatted::info("server", "server (-with-icu) started on port %1%") % port;
#else
        _attachment << udho::logging::messages::formatted::info("server", "server started on port %1%") % port;
#endif
//         _attachment.log(udho::logging::status::info, udho::logging::segment::server, "server started");
        router.template listen<attachment_type>(_io, _attachment, port, doc_root);
    }
};

template <typename CacheT>
struct server<void, CacheT>{
    typedef CacheT cache_type;
    typedef udho::attachment<void, cache_type> attachment_type;
    typedef server<void, cache_type> self_type;
    
    template <typename RequestT>
    using context_type = udho::context<RequestT, attachment_type>;
    typedef udho::defs::request_type http_request_type;
    typedef context_type<http_request_type> context;
    typedef http_request_type request_type;
    
    boost::asio::io_service& _io;
    attachment_type _attachment;
    
    server(boost::asio::io_service& io): _io(io){}
    server(const self_type&) = delete;
    server(self_type&& other) = default;
    template <typename RouterT>
    void serve(RouterT&& router, int port=9198, std::string doc_root=""){
#ifdef WITH_ICU
        _attachment << udho::logging::messages::formatted::info("server", "server (-with-icu) started on port %1%") % port;
#else
        _attachment << udho::logging::messages::formatted::info("server", "server started on port %1%") % port;
#endif
        router.template listen<attachment_type>(_io, _attachment, port, doc_root);
    }
};

namespace servers{

namespace stateless{
    template <typename LoggerT>
    using logged = server<LoggerT, void>;
    using quiet  = server<void, void>;
}

template <typename... T>
struct stateful{
    typedef udho::cache::store<boost::uuids::uuid, T...> cache_type;
    
    template <typename LoggerT>
    using logged    = server<LoggerT, cache_type>;
    using ostreamed = logged<udho::loggers::ostream>;
    using quiet     = server<void, cache_type>;
};

namespace quiet{
    template <typename... T>
    using stateful  = server<void, udho::cache::store<boost::uuids::uuid, T...>>;
    using stateless = server<void, void>;
}

template <typename T>
struct logged{
    typedef T logger_type;
    
    template <typename... U>
    using stateful  = server<logger_type, udho::cache::store<boost::uuids::uuid, U...>>;
    using stateless = server<logger_type, void>;
};

namespace ostreamed{
    template <typename CacheT>
    struct ostreamed_helper{
        typedef server<udho::loggers::ostream, CacheT> server_type;
        typedef typename server_type::logger_type logger_type;
        typedef typename server_type::cache_type cache_type;
        typedef typename server_type::attachment_type attachment_type;
        typedef typename server_type::http_request_type request_type;
        
        udho::loggers::ostream _logger;
        server_type _server;
        
        ostreamed_helper(boost::asio::io_service& io, udho::loggers::ostream::stream_type& stream): _logger(stream), _server(io, _logger){}
        template <typename RouterT>
        void serve(RouterT&& router, int port=9198, std::string doc_root=""){
            _server.template serve(router, port, doc_root);
        }
    };
    
    template <typename... U>
    using stateful  = ostreamed_helper<udho::cache::store<boost::uuids::uuid, U...>>;
    using stateless = ostreamed_helper<void>;
}
    
}

}

#endif // UDHO_SERVER_H
