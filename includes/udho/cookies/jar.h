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

#ifndef UDHO_COOKIES_JAR_H
#define UDHO_COOKIES_JAR_H

#include <mutex>
#include <boost/tokenizer.hpp>
#include <udho/cookies/cookie.h>
#include <boost/beast/http/message.hpp>

namespace udho {
namespace cookies{

struct jar{
    using cookie_str_type = udho::cookies::cookie<std::string>;
    typedef std::map<std::string, cookie_str_type> container_type;

    jar() = default;
    jar(const jar&) = delete;
    jar& operator=(const jar&) = delete;

    template <typename Body, typename Fields>
    std::size_t apply(const boost::beast::http::request<Body, Fields>& request){
        clear();
        const std::lock_guard<std::mutex> lock(_mutex);
        std::size_t count = 0;
        if(request.count(boost::beast::http::field::cookie)){
            std::string_view cookie_header = request[boost::beast::http::field::cookie];
            boost::char_separator<char> sep(";");
            boost::tokenizer<boost::char_separator<char>> tokens(cookie_header, sep);
            for (const auto& token : tokens) {
                cookie_str_type cookie = udho::cookies::read(token);
                if (cookie.valid()) {
                    add(std::move(cookie));
                    ++count;
                }
            }
        }
        return count;
    }

    template <typename Body, typename Fields>
    std::size_t apply(boost::beast::http::response<Body, Fields>& response) const{
        const std::lock_guard<std::mutex> lock(_mutex);
        std::size_t count = 0;
        for(const auto& pair: _cookies){
            const cookie_str_type& cookie = pair.second;
            if(cookie.valid()){
                response.insert(boost::beast::http::field::set_cookie, udho::cookies::to_string(cookie));
                ++count;
            }
        }
        return count;
    }

    template <typename V>
    void add(const udho::cookies::cookie<V>& c){
        const std::lock_guard<std::mutex> lock(_mutex);
        cookie_str_type converted = c.template as<std::string>();
        _cookies[c.id()] = std::move(converted);
    }

    template <typename V>
    void add(udho::cookies::cookie<V>&& c){
        const std::lock_guard<std::mutex> lock(_mutex);
        cookie_str_type converted = c.template as<std::string>();
        _cookies[c.id()] = std::move(converted);
    }

    inline void clear() {
        const std::lock_guard<std::mutex> lock(_mutex);
        _cookies.clear();
    }

    inline bool exists(const std::string& key) const{
        const std::lock_guard<std::mutex> lock(_mutex);
        return _cookies.count(key) > 0;
    }

    inline const cookie_str_type& get(const std::string& key) const{
        const std::lock_guard<std::mutex> lock(_mutex);
        auto it = _cookies.find(key);
        if (it != _cookies.end()) {
            return it->second;
        }else{
            throw std::out_of_range{"No cookie found with name: " +key};
        }
    }

    inline const cookie_str_type& operator[](const std::string& key) const{ return get(key); }

    template <typename Body, typename Fields>
    std::size_t operator()(const boost::beast::http::request<Body, Fields>& request){ return apply(request); }

    template <typename Body, typename Fields>
    std::size_t operator()(const boost::beast::http::response<Body, Fields>& response){ return apply(response); }

    private:
    container_type      _cookies;
    mutable std::mutex  _mutex;

    template <typename V>
    friend udho::cookies::jar& operator<<(udho::cookies::jar& cookies, const udho::cookies::cookie<V>& cookie);
};

template <typename V>
udho::cookies::jar& operator<<(udho::cookies::jar& cookies, const udho::cookies::cookie<V>& cookie){
    cookies.add(cookie);
    return cookies;
}

}
}

#endif // UDHO_COOKIES_JAR_H
