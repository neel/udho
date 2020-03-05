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
#include <iostream>
#include <boost/optional.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <udho/util.h>
#include <udho/cache.h>
#include <boost/signals2/signal.hpp>

namespace udho{
    
/**
 * logged stateful
 */
template <typename LoggerT=void, typename CacheT=void>
struct attachment: LoggerT, CacheT{
    typedef attachment<LoggerT, CacheT> self_type;
    typedef LoggerT logger_type;
    typedef CacheT  cache_type;
    
    attachment(LoggerT& logger): LoggerT(logger){}
};

/**
 * logged stateless
 */
template <typename LoggerT>
struct attachment<LoggerT, void>: LoggerT{
    typedef attachment<LoggerT, void> self_type;
    typedef LoggerT logger_type;
    
    attachment(LoggerT& logger): LoggerT(logger){}
};

/**
 * quiet stateful
 */
template <typename CacheT>
struct attachment<void, CacheT>: CacheT{
    typedef attachment<void, CacheT> self_type;
    typedef void logger_type;
    typedef CacheT  cache_type;
    
    attachment(){}
    template <typename... U>
    void log(U...){}
};

/**
 * quiet stateless
 */
template <>
struct attachment<void, void>{
    typedef attachment<void, void> self_type;
    typedef void logger_type;
    typedef void cache_type;
    
    attachment(){}
    template <typename... U>
    void log(U...){}
};

   
// https://github.com/cmakified/cgicc/blob/master/cgicc/HTTPCookie.h
// https://github.com/cmakified/cgicc/blob/master/cgicc/HTTPCookie.cpp
template <typename ValueT>
struct cookie_{
    typedef cookie_<ValueT> self_type;
    
    std::string _name;
    ValueT _value;
    bool _removed;
    boost::optional<std::string> _comment;
    boost::optional<std::string> _domain;
    boost::optional<std::string>   _path;
    boost::optional<unsigned long> _age;
    boost::optional<bool>          _secure;
    
    cookie_(const std::string& name): _name(name), _removed(false){}
    cookie_(const std::string& name, const ValueT& value): _name(name), _value(value), _removed(false){}
    
    self_type& path(const std::string& p){
        _path = p;
        return *this;
    }
    std::string path() const{
        return !!_path ? *_path : std::string();
    }
    
    self_type& domain(const std::string& d){
        _domain = d;
        return *this;
    }
    std::string domain() const{
        return !!_domain ? *_domain : std::string();
    }
    
    self_type& age(unsigned long age){
        _age = age;
        return *this;
    }
    unsigned long age() const{
        return !!_age ? *_age : 0;
    }
    
    template <typename StreamT>
    StreamT& render(StreamT& stream) const{
        stream << _name << '=' << boost::lexical_cast<std::string>(_value);
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

template <typename ValueT>
udho::cookie_<ValueT> cookie(const std::string& name, const ValueT& v){
    return udho::cookie_<ValueT>(name, v);
}

namespace detail{

template <typename RequestT>
struct form_{
    typedef RequestT request_type;
    typedef typename request_type::body_type::value_type body_type;
    typedef std::map<std::string, std::string> fields_map_type;
    
    const request_type& _request;
    body_type _body;
    fields_map_type _fields;
    
    form_(const request_type& request): _request(request), _body(request.body()){
        if(_request[boost::beast::http::field::content_type].find("application/x-www-form-urlencoded") != std::string::npos){
            parse_urlencoded();
        }else if(_request[boost::beast::http::field::content_type].find("multipart/form-data") != std::string::npos){
            parse_multipart();
        }
    }
    void parse_urlencoded(){
        std::vector<std::string> fields;
        boost::split(fields, _body, boost::is_any_of("&"));
        for(const std::string& field: fields){
            auto pos = field.find("=");
            std::string key = boost::algorithm::trim_copy(field.substr(0, pos));
            std::string val = boost::algorithm::trim_copy(field.substr(pos+1));
            _fields.insert(std::make_pair(udho::util::urldecode(key), udho::util::urldecode(val)));
        }
    }
    void parse_multipart(){
        
    }
    bool has(const std::string& name) const{
        return _fields.find(name) != _fields.end();
    }
    template <typename V>
    V field(const std::string& name) const{
        return boost::lexical_cast<V>(_fields.at(name));
    }
    fields_map_type::size_type count() const{
        return _fields.size();
    }
};

// https://www.boost.org/doc/libs/1_72_0/libs/iterator/doc/iterator_facade.html#tutorial-example
// https://www.boost.org/doc/libs/1_72_0/libs/iterator/doc/iterator_adaptor.html#base-parameters
template <typename RequestT>
struct form_iterator: boost::iterator_facade<form_iterator<RequestT>, form_<RequestT>, boost::forward_traversal_tag>{
    typedef form_<RequestT> form_type;
    
    form_type& _form;
    
    explicit form_iterator(form_type& form): _form(form){}
    
    private:
//     friend class boost::iterator_core_access;
//     void increment() { m_node = m_node->next(); }
//     bool equal(node_iterator const& other) const{
//         return this->m_node == other.m_node;
//     }
//     node_base& dereference() const { return *m_node; }
};
    
template <typename RequestT>
struct cookies_{
    typedef RequestT request_type;
    typedef boost::beast::http::header<true> headers_type;
    typedef std::map<std::string, std::string> cookie_jar_type;
    
    const request_type& _request;
    headers_type&       _headers;
    cookie_jar_type     _jar;
    
    cookies_(const request_type& request, headers_type& headers): _request(request), _headers(headers){
        collect();
    }
    void collect(){
        if(_request.count(boost::beast::http::field::cookie)){
            std::string cookies_str(_request[boost::beast::http::field::cookie]);
            std::vector<std::string> cookies;
            boost::split(cookies, cookies_str, boost::is_any_of(";"));
            for(const std::string& cookie: cookies){
                auto pos = cookie.find("=");
                std::string key = boost::algorithm::trim_copy(cookie.substr(0, pos));
                std::string val = boost::algorithm::trim_copy(cookie.substr(pos+1));
                _jar.insert(std::make_pair(key, val));
            }
        }
    }
    template <typename V>
    void add(const cookie_<V>& c){
        _headers.insert(boost::beast::http::field::set_cookie, c.to_string());
    }
    template <typename V>
    void add(const std::string& key, const V& value){
        add(udho::cookie_<V>(key, value));
    }
    bool exists(const std::string& key) const{
        return _jar.count(key);
    }
    template <typename V>
    V get(const std::string& key) const{
        if(exists(key)){
            return boost::lexical_cast<V>(_jar.at(key));
        }else{
            return V();
        }
    }
};

template <typename RequestT, typename V>
cookies_<RequestT>& operator<<(cookies_<RequestT>& cookies, const udho::cookie_<V>& cookie){
    cookies.add(cookie);
    return cookies;
}
    
template <typename RequestT, typename AttachmentT>
struct session_{
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
    typedef boost::beast::http::header<true> headers_type;
    typedef typename AttachmentT::key_type session_key;
    typedef udho::detail::cookies_<RequestT> cookies_type;
    typedef std::map<std::string, std::string> cookie_jar_type;
    
    const request_type& _request;
    attachment_type&    _attachment;
    cookies_type&       _cookies;
    std::string         _sessid;
    cookie_jar_type     _jar;
    session_key         _id;
    bool                _returning;
    
    session_(const request_type& request, attachment_type& attachment, cookies_type& cookies): _request(request), _attachment(attachment), _cookies(cookies), _sessid(_attachment._sessid), _returning(false){
        identify();
    }
    void identify(){
        if(_cookies.exists(_sessid)){
            session_key id = _cookies.template get<session_key>(_sessid);
            if(!_attachment.issued(id)){
                _id = attachment_type::generate();
                _attachment.issue(_id);
                _cookies.add(_sessid, _id);
                _returning = false;
            }else{
                _id = id;
                _returning = true;
            }
        }else{
            _id = attachment_type::generate();
            _attachment.issue(_id);
            _cookies.add(_sessid, _id);
            _returning = false;
        }
    }
    const session_key& id() const{
        return _id;
    }
    bool returning() const{
        return _returning;
    }
    template <typename V>
    bool exists() const{
        return _attachment.template exists<V>(_id);
    }
    template <typename V>
    const V& get() const{
        return _attachment.template get<V>(_id);
    }
    template <typename V>
    V& at(){
        return _attachment.template at<V>(_id);
    }
    template <typename V>
    void set(const V& value){
        _attachment.template insert<V>(_id, value);
    }
};

template <typename RequestT, typename LoggerT>
struct session_<RequestT, udho::attachment<LoggerT, void>>{
    template <typename... U>
    session_(U...){}
};

template <typename RequestT, typename AttachmentT, typename T>
session_<RequestT, AttachmentT>& operator<<(session_<RequestT, AttachmentT>& session, const T& data){
    session.template set<T>(data);
    return session;
}

template <typename RequestT, typename AttachmentT, typename T>
const session_<RequestT, AttachmentT>& operator>>(const session_<RequestT, AttachmentT>& session, T& data){
    data = session.template get<T>();
    return session;
}
    
template <typename RequestT, typename AttachmentT, bool linked=true>
struct context_impl{
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
    typedef context_impl<request_type, attachment_type, linked> self_type;
    typedef boost::beast::http::header<true> headers_type;
    typedef udho::detail::form_<RequestT> form_type;
    typedef udho::detail::cookies_<RequestT> cookies_type;
    typedef udho::detail::session_<RequestT, AttachmentT> sess_type;
    
    request_type     _request;
    attachment_type& _attachment;
    form_type        _form;
    headers_type     _headers;
    cookies_type     _cookies;
    sess_type        _sess;
    
    context_impl(attachment_type& attachment): _attachment(attachment), _form(request), _cookies(request, _headers), _sess(request, _attachment, _cookies){}
    context_impl(const RequestT& request, attachment_type& attachment): _request(request), _attachment(attachment), _form(request), _cookies(request, _headers), _sess(request, _attachment, _cookies){}
    context_impl(const self_type&) = delete;
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
};

template <typename RequestT, typename AttachmentT>
struct context_impl<RequestT, AttachmentT, false>{
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
    typedef context_impl<request_type, attachment_type, false> self_type;
    typedef boost::beast::http::header<true> headers_type;
    typedef udho::detail::form_<RequestT> form_type;
    typedef udho::detail::cookies_<RequestT> cookies_type;
    typedef udho::detail::session_<RequestT, AttachmentT> sess_type;
    
    request_type     _request;
    attachment_type _attachment;
    form_type        _form;
    headers_type     _headers;
    cookies_type     _cookies;
    sess_type        _sess;
    
    context_impl(): _form(request), _cookies(request, _headers), _sess(request, _attachment, _cookies){}
    context_impl(const RequestT& request): _request(request), _form(request), _cookies(request, _headers), _sess(request, _attachment, _cookies){}
    context_impl(const self_type&) = delete;
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
};

}

/**
 * @todo write docs
 */
template <typename RequestT, typename AttachmentT>
struct context{
    typedef RequestT request_type;
    typedef AttachmentT attachment_type;
    typedef context<request_type, attachment_type> self_type;
    typedef detail::context_impl<request_type, attachment_type> impl_type;
    typedef boost::shared_ptr<impl_type> pimple_type;
    typedef udho::detail::form_<RequestT> form_type;
    typedef udho::detail::cookies_<RequestT> cookies_type;
    typedef udho::detail::session_<RequestT, AttachmentT> sess_type;
    typedef typename attachment_type::logger_type logger_type;
    
    pimple_type _pimpl;
        
    context(attachment_type& attachment): _pimpl(new impl_type(attachment)){}
    context(const RequestT& request, attachment_type& attachment): _pimpl(new impl_type(request, attachment)){}
    context(const self_type& other): _pimpl(other._pimpl){}
    
    const request_type& request() const{return _pimpl->request();}
    
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    form_type& form(){
        return _pimpl->_form;
    }
    sess_type& session(){
        return _pimpl->_sess;
    }
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    
    operator request_type() const{
        return _pimpl->request();
    }
//     operator context<RequestT, udho::attachment<logger_type, void>>() const{
//         typedef context<RequestT, udho::attachment<logger_type, void>> other_type;
//         other_type other();
//         return other;
//     }
};

template <typename RequestT, typename ShadowT>
struct session_data{
    typedef RequestT request_type;
    typedef udho::detail::cookies_<request_type> cookies_type;
    typedef ShadowT shadow_type;
    typedef typename shadow_type::key_type key_type;
    typedef session_data<request_type, shadow_type> self_type;
    
    cookies_type& _cookies;
    shadow_type&  _shadow;
    key_type      _id;
    bool          _returning;
};

template <typename RequestT, typename KeyT, typename... T>
struct context_data{
    typedef RequestT request_type;
    typedef KeyT key_type;
    typedef udho::cache::shadow<key_type, T...> shadow_type;
    typedef context<request_type, shadow_type> self_type;
    typedef udho::detail::form_<request_type> form_type;
    typedef udho::detail::cookies_<request_type> cookies_type;
    typedef boost::beast::http::header<true> headers_type;
    typedef session_data<request_type, shadow_type> session_type;
    
    const request_type& _request;
    shadow_type _shadow;
    headers_type _headers;
    session_type _session;
    
    boost::signals2::signal<void (const std::string&)> logged;
    
};

}

#endif // UDHO_CONTEXT_H
