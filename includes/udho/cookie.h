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

#ifndef UDHO_COOKIE_H
#define UDHO_COOKIE_H

#include <map>
#include <string>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/beast/http/message.hpp>

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

}

#endif // UDHO_COOKIE_H
