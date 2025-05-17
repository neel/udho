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
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/beast/http/message.hpp>
#include <udho/utils/encoding.h>
#include <udho/cookies/cookie.h>

namespace udho{

namespace cookies{

template <typename RequestT>
struct jar{
    typedef RequestT request_type;
    typedef boost::beast::http::header<true> headers_type;
    typedef std::map<std::string, std::string> cookie_jar_type;

    const request_type& _request;
    headers_type&       _headers;
    cookie_jar_type     _jar;

    jar(const jar&) = delete;
    jar& operator=(const jar&) = delete;

    jar(const request_type& request, headers_type& headers): _request(request), _headers(headers){
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
    void add(const udho::cookies::cookie<V>& c){
        _headers.insert(boost::beast::http::field::set_cookie, c.to_string());
    }
    template <typename V>
    void add(const std::string& key, const V& value){
        add(udho::cookies::cookie<V>(key, value));
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
udho::cookies::jar<RequestT>& operator<<(udho::cookies::jar<RequestT>& cookies, const udho::cookies::cookie<V>& cookie){
    cookies.add(cookie);
    return cookies;
}

}

}

#endif // UDHO_COOKIE_H
