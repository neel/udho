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
#include <udho/logging.h>
#include <boost/signals2/signal.hpp>
#include <udho/defs.h>

namespace udho{
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
    
template <typename RequestT, typename ShadowT>
struct session_{
    typedef RequestT request_type;
    typedef udho::detail::cookies_<request_type> cookies_type;
    typedef ShadowT shadow_type;
    typedef typename shadow_type::key_type key_type;
    typedef session_<request_type, shadow_type> self_type;
    typedef udho::cache::generator<key_type> generator_type;
    
    cookies_type&  _cookies;
    shadow_type    _shadow;
    std::string    _sessid;
    bool           _returning;
    bool           _identified;
    key_type       _id;
    generator_type _generator;
    
    session_(cookies_type& cookies, shadow_type& shadow): _cookies(cookies), _shadow(shadow), _sessid("UDHOSESSID"), _returning(false), _identified(false){
        identify();
    }
    template <typename... T>
    session_(session_<request_type, udho::cache::shadow<key_type, T...>>& other): _cookies(other._cookies), _shadow(other._shadow), _sessid(other._sessid), _returning(other._returning), _identified(other._identified), _id(other._id), _generator(other._generator){}
//     template <typename... T>
//     session_(session_<request_type, udho::cache::store<key_type, T...>>& other): _cookies(other._cookies), _shadow(other._shadow), _sessid(other._sessid), _returning(other._returning), _identified(other._identified), _id(other._id), _generator(other._generator){}
    void identify(){
        if(!_identified){
            if(_cookies.exists(_sessid)){
                key_type id = _cookies.template get<key_type>(_sessid);
                if(!_shadow.issued(id)){
                    _id = _generator.generate();
                    _shadow.issue(_id);
                    _cookies.add(_sessid, _id);
                    _returning = false;
                }else{
                    _id = id;
                    _returning = true;
                }
            }else{
                _id = _generator.generate();
                _shadow.issue(_id);
                _cookies.add(_sessid, _id);
                _returning = false;
            }
        }
    }
    const key_type& id() const{
        return _id;
    }
    bool returning() const{
        return _returning;
    }
    template <typename V>
    bool exists() const{
        return _shadow.template exists<V>(_id);
    }
    template <typename V>
    const V& get() const{
        return _shadow.template get<V>(_id);
    }
    template <typename V>
    V& at(){
        return _shadow.template at<V>(_id);
    }
    template <typename V>
    void set(const V& value){
        _shadow.template insert<V>(_id, value);
    }
//     template <typename V>
//     V& operator V(){
//         return at<V>(_id);
//     }
//     template <typename V>
//     const V& operator V() const{
//         return get<V>(_id);
//     }
    
};

template <typename RequestT>
struct session_<RequestT, void>{
    template <typename... U>
    session_(U...){}
};

template <typename RequestT, typename ShadowT, typename T>
session_<RequestT, ShadowT>& operator<<(session_<RequestT, ShadowT>& session, const T& data){
    session.template set<T>(data);
    return session;
}

template <typename RequestT, typename ShadowT, typename T>
const session_<RequestT, ShadowT>& operator>>(const session_<RequestT, ShadowT>& session, T& data){
    data = session.template get<T>();
    return session;
}
 
template <typename RequestT>
struct context_impl{
    typedef RequestT request_type;
    typedef context_impl<request_type> self_type;
    typedef udho::detail::form_<request_type> form_type;
    typedef udho::detail::cookies_<request_type> cookies_type;
    typedef boost::beast::http::header<true> headers_type;
    
    const request_type& _request;
    form_type    _form;
    headers_type _headers;
    cookies_type _cookies;
    
    boost::signals2::signal<void (const udho::logging::abstract_message&)> logged;
    
    context_impl(const request_type& request): _request(request), _form(request), _cookies(request, _headers){}
    context_impl(const self_type& other) = delete;
    const request_type& request() const{return _request;}
    cookies_type& cookies(){
        return _cookies;
    }
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
    void log(const udho::logging::abstract_message& msg){
        logged(msg);
    }
    
};
 
// template <typename RequestT>
// struct context_impl{
//     typedef RequestT request_type;
//     typedef ShadowT shadow_type;
//     typedef typename shadow_type::key_type key_type;
//     typedef context_impl<request_type, shadow_type> self_type;
//     typedef udho::detail::form_<request_type> form_type;
//     typedef udho::detail::cookies_<request_type> cookies_type;
//     typedef boost::beast::http::header<true> headers_type;
//     typedef session_<request_type, shadow_type> session_type;
//     
//     const request_type& _request;
//     shadow_type _shadow;
//     form_type    _form;
//     headers_type& _headers;
//     cookies_type _cookies;
//     session_type _session;
//     
//     boost::signals2::signal<void (const udho::logging::abstract_message&)> logged;
//     
//     template <typename... V>
//     context_impl(const request_type& request, headers_type& headers, udho::cache::store<key_type, V...>& store): _request(request), _headers(headers), _shadow(store), _form(request), _cookies(request, _headers), _session(_cookies, _shadow){}
//     template <typename... V>
//     context_impl(const request_type& request, headers_type& headers, udho::cache::shadow<key_type, V...>& shadow): _request(request), _headers(headers), _shadow(shadow), _form(request), _cookies(request, _headers), _session(_cookies, _shadow){}
//     context_impl(const self_type& other) = delete;
//     const request_type& request() const{return _request;}
//     template<class Body, class Fields>
//     void patch(boost::beast::http::message<false, Body, Fields>& res) const{
//         for(const auto& header: _headers){
//             if(header.name() != boost::beast::http::field::set_cookie){
//                 res.set(header.name(), header.value());
//             }
//         }
//         res.erase(boost::beast::http::field::set_cookie);
//         for(const auto& header: _headers){
//             if(header.name() == boost::beast::http::field::set_cookie){
//                 res.insert(header.name(), header.value());
//             }
//         }
//     }
//     void log(const udho::logging::abstract_message& msg){
//         logged(msg);
//     }
//     
// };

// template <typename RequestT>
// struct context_impl<RequestT, void>{
//     typedef RequestT request_type;
//     typedef void shadow_type;
//     typedef context_impl<request_type, shadow_type> self_type;
//     typedef udho::detail::form_<request_type> form_type;
//     typedef udho::detail::cookies_<request_type> cookies_type;
//     typedef boost::beast::http::header<true> headers_type;
//     typedef session_<request_type, shadow_type> session_type;
//     
//     const request_type& _request;
//     form_type    _form;
//     headers_type& _headers;
//     cookies_type _cookies;
//     session_type _session;
//     
//     boost::signals2::signal<void (const udho::logging::abstract_message&)> logged;
//     
//     context_impl(const request_type& request, headers_type& headers): _request(request), _headers(headers), _form(request), _cookies(request, _headers){}
//     context_impl(const self_type& other) = delete;
//     const request_type& request() const{return _request;}
//     template<class Body, class Fields>
//     void patch(boost::beast::http::message<false, Body, Fields>& res) const{
//         for(const auto& header: _headers){
//             if(header.name() != boost::beast::http::field::set_cookie){
//                 res.set(header.name(), header.value());
//             }
//         }
//         res.erase(boost::beast::http::field::set_cookie);
//         for(const auto& header: _headers){
//             if(header.name() == boost::beast::http::field::set_cookie){
//                 res.insert(header.name(), header.value());
//             }
//         }
//     }
//     void log(const udho::logging::abstract_message& msg){
//         logged(msg);
//     }
//     
// };

}

/**
 * @todo write docs
 */
template <typename RequestT, typename ShadowT>
struct context{
    typedef RequestT request_type;
    typedef ShadowT shadow_type;
    typedef typename shadow_type::key_type key_type;
    typedef context<request_type, shadow_type> self_type;
    typedef detail::context_impl<request_type> impl_type;
    typedef boost::shared_ptr<impl_type> pimple_type;
    typedef udho::detail::form_<RequestT> form_type;
    typedef udho::detail::cookies_<RequestT> cookies_type;
    typedef detail::session_<request_type, shadow_type> session_type;
    
    pimple_type _pimpl;
    session_type _session;
        
    template <typename... V>
    context(udho::cache::store<key_type, V...>& store): _pimpl(new impl_type(request_type())), _session(_pimpl->cookies(), store){}
    template <typename... V>
    context(const RequestT& request, udho::cache::shadow<key_type, V...>& shadow): _pimpl(new impl_type(request)), _session(_pimpl->cookies(), shadow){}
    template <typename OtherShadowT>
    context(context<RequestT, OtherShadowT>& other): _pimpl(other._pimpl), _session(other._session){}
    
//     context(const self_type& other): _pimpl(other._pimpl){}
    
    const request_type& request() const{return _pimpl->request();}
    
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    form_type& form(){
        return _pimpl->_form;
    }
    session_type& session(){
        return _session;
    }
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    
    operator request_type() const{
        return _pimpl->request();
    }
};

template <typename RequestT>
struct context<RequestT, void>{
    typedef RequestT request_type;
    typedef void shadow_type;
    typedef context<request_type, void> self_type;
    typedef detail::context_impl<request_type> impl_type;
    typedef boost::shared_ptr<impl_type> pimple_type;
    typedef udho::detail::form_<RequestT> form_type;
    typedef udho::detail::cookies_<RequestT> cookies_type;
    
    pimple_type _pimpl;
        
//     template <typename C>
//     context(C&): _pimpl(new impl_type(request_type())){}
    template <typename C>
    context(const RequestT& request, C&): _pimpl(new impl_type(request)){}
    template <typename ShadowT>
    context(context<RequestT, ShadowT>& other): _pimpl(other._pimpl){}
//     context(const self_type& other): _pimpl(other._pimpl){}
    
    const request_type& request() const{return _pimpl->request();}
    
    template<class Body, class Fields>
    void patch(boost::beast::http::message<false, Body, Fields>& res) const{
        _pimpl->patch(res);
    }
    form_type& form(){
        return _pimpl->_form;
    }
    cookies_type& cookies(){
        return _pimpl->_cookies;
    }
    
    operator request_type() const{
        return _pimpl->request();
    }
};

namespace contexts{
    using stateless = context<udho::defs::request_type, void>;
    template <typename... T>
    using stateful = context<udho::defs::request_type, udho::cache::shadow<udho::defs::session_key_type, T...>>;
}

}

#endif // UDHO_CONTEXT_H
