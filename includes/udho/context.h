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

#ifndef UDHO_CONTEXT_H
#define UDHO_CONTEXT_H

#include <sstream>
#include <boost/optional.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/shared_ptr.hpp>

namespace udho{

template <typename LoggerT=void, typename CacheT=void, typename... T>
struct attachment: LoggerT, CacheT, T...{
    typedef attachment<LoggerT, CacheT, T...> self_type;
    typedef LoggerT logger_type;
    typedef CacheT  cache_type;
    
    attachment(LoggerT& logger): LoggerT(logger){}
};

template <typename LoggerT>
struct attachment<LoggerT, void>: LoggerT{
    typedef attachment<LoggerT, void> self_type;
    typedef LoggerT logger_type;
    typedef void    cache_type;
    
    attachment(LoggerT& logger): LoggerT(logger){}
};
    
// https://github.com/cmakified/cgicc/blob/master/cgicc/HTTPCookie.h
// https://github.com/cmakified/cgicc/blob/master/cgicc/HTTPCookie.cpp
struct cookie{
    std::string _name;
    std::string _value;
    bool _removed;
    boost::optional<std::string> _comment;
    boost::optional<std::string> _domain;
    boost::optional<std::string>   _path;
    boost::optional<unsigned long> _age;
    boost::optional<bool>          _secure;
    
    cookie(const std::string& name): _name(name){}
    cookie(const std::string& name, const std::string& value): _name(name), _value(value){}
    
    void path(const std::string& p){
        _path = p;
    }
    std::string path() const{
        return !!_path ? *_path : std::string();
    }
    
    void domain(const std::string& d){
        _domain = d;
    }
    std::string domain() const{
        return !!_domain ? *_domain : std::string();
    }
    
    template <typename StreamT>
    StreamT& render(StreamT& stream) const{
        stream << _name << '=' << _value;
        if(!!_comment && !(*_comment).empty())
            stream << "; Comment=" << *_comment;
        if(!!_domain && !(*_domain).empty())
            stream << "; Domain=" << *_domain;
        if(_removed){
            stream << "; Expires=Fri, 01-Jan-1971 01:00:00 GMT;";
        }else if(!!_age && 0 != *_age){
            stream << "; Max-Age=" << *_age;
        }
        if(!!_path && !(*_path).empty())
            stream << "; Path=" << *_path;
        if(!!_secure && *_secure)
            stream << "; Secure";
        
        stream << "; Version=1";
        
        return stream;
    }
    std::string to_string() const{
        std::stringstream ss;
        render(ss);
        return ss.str();
    }
};

template <typename RequestT, typename AttachmentT>
struct context_impl{
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
    typedef context_impl<request_type, attachment_type> self_type;
    typedef boost::beast::http::header<true> headers_type;
    
    request_type _request;
    attachment_type& _attachment;
    headers_type _headers;
    
    context_impl(attachment_type& attachment): _attachment(attachment){}
    context_impl(const RequestT& request, attachment_type& attachment): _request(request), _attachment(attachment){}
    const request_type& request() const{return _request;}
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        for(const auto& header: _headers){
            if(header.name() != boost::beast::http::field::set_cookie){
                res.set(header.name(), header.value());
            }
        }
        res.erase(boost::beast::http::field::set_cookie);
        for(const auto& header: _headers){
            if(header.name() == boost::beast::http::field::set_cookie){
                res.insert(header.name(), header.value());
            }
        }
    }
    void add(const cookie& c){
        _headers.set(boost::beast::http::field::set_cookie, c.to_string());
    }
};

/**
 * @todo write docs
 */
template <typename RequestT, typename AttachmentT>
struct context{
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
    typedef context<request_type, attachment_type> self_type;
    typedef context_impl<request_type, attachment_type> impl_type;
    typedef boost::shared_ptr<impl_type> pimple_type;
    
    pimple_type _pimpl;
        
    context(attachment_type& attachment): _pimpl(new impl_type(attachment)){}
    context(const RequestT& request, attachment_type& attachment): _pimpl(new impl_type(request, attachment)){}
    context(const self_type& other): _pimpl(other._pimpl){}
    
    const request_type& request() const{return _pimpl->request();}
    
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    void add(const cookie& c){
        _pimpl->add(c);
    }
};

// template <typename RequestT>
// struct context<RequestT, void>: RequestT{
//     typedef context<RequestT, void> self_type;
//     typedef boost::beast::http::header<true> headers_type;
//     
//     typedef RequestT request_type;
//     typedef void attachment_type;
//     
//     headers_type _response_headers;
//         
//     context(): request_type(){}
//     context(const RequestT& request): request_type(request){}
//     self_type& operator=(const self_type& other){
//         request_type::operator=(other);
//         return *this;
//     }
//     self_type& operator=(const request_type& other){
//         request_type::operator=(other);
//         
//         return *this;
//     }
//     template<class Body, class Fields>
//     void patch(boost::beast::http::message<false, Body, Fields>& res) const{
//         for(const auto& header: _response_headers){
//             if(header.name() != boost::beast::http::field::set_cookie){
//                 res.set(header.name(), header.value());
//             }
//         }
//         res.erase(boost::beast::http::field::set_cookie);
//         for(const auto& header: _response_headers){
//             if(header.name() == boost::beast::http::field::set_cookie){
//                 res.insert(header.name(), header.value());
//             }
//         }
//     }
//     void add(const cookie& c){
//         _response_headers.set(boost::beast::http::field::set_cookie, c.to_string());
//     }
// };

}

#endif // UDHO_CONTEXT_H
