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

#ifndef UDHO_SESSION_H
#define UDHO_SESSION_H

#include <string>
#include <udho/cache.h>
#include <udho/cookie.h>
#include <udho/configuration.h>

namespace udho{
    
template <typename RequestT, typename ShadowT>
struct session_{
    typedef RequestT request_type;
    typedef udho::cookies_<request_type> cookies_type;
    typedef ShadowT shadow_type;
    typedef typename shadow_type::key_type key_type;
    typedef session_<request_type, shadow_type> self_type;
    typedef udho::cache::generator<key_type> generator_type;
    typedef udho::config<udho::configs::session> session_config_type;
    
    const session_config_type& _config;
    cookies_type&  _cookies;
    shadow_type    _shadow;
    bool           _returning;
    bool           _identified;
    key_type       _id;
    generator_type _generator;
    
    session_(cookies_type& cookies, shadow_type& shadow, const session_config_type& config): _config(config), _cookies(cookies), _shadow(shadow), _returning(false), _identified(false){
        identify();
    }
    template <typename... T>
    session_(session_<request_type, udho::cache::shadow<key_type, T...>>& other): _config(other.config()), _cookies(other._cookies), _shadow(other._shadow), _returning(other._returning), _identified(other._identified), _id(other._id), _generator(other._generator){}
    void identify(){
        if(!_identified){
            if(_cookies.exists(sessid())){
                key_type id = _cookies.template get<key_type>(sessid());
                if(!_shadow.issued(id)){
                    _id = _generator.generate();
                    _shadow.issue(_id);
                    _cookies.add(sessid(), _id);
                    _returning = false;
                }else{
                    _id = id;
                    _returning = true;
                }
            }else{
                _id = _generator.generate();
                _shadow.issue(_id);
                _cookies.add(sessid(), _id);
                _returning = false;
            }
        }
    }
    const session_config_type& config() const{
        return _config;
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
        _shadow.template set<V>(_id, value);
    }
    std::size_t size() const{
        return _shadow.size();
    }
    bool remove(){
        return _shadow.remove(_id);
    }
    template <typename V>
    bool remove(){
        return _shadow.template remove<V>(_id);
    }
    boost::posix_time::ptime created() const{
        return _shadow.created(_id);
    }
    boost::posix_time::ptime updated() const{
        return _shadow.updated(_id);
    }
    boost::posix_time::time_duration age() const{
        return _shadow.age(_id);
    }
    boost::posix_time::time_duration idle() const{
        return _shadow.idle(_id);
    }
    template <typename V>
    boost::posix_time::ptime created() const{
        return _shadow.template created<V>(_id);
    }
    template <typename V>
    boost::posix_time::ptime updated() const{
        return _shadow.template updated<V>(_id);
    }
    template <typename V>
    boost::posix_time::time_duration age() const{
        return _shadow.template age<V>(_id);
    }
    template <typename V>
    boost::posix_time::time_duration idle() const{
        return _shadow.template idle<V>(_id);
    }
    std::string sessid() const{
        return _config[udho::configs::session::id];
    }
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
    
}

#endif // UDHO_SESSION_H
