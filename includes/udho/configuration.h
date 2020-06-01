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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>

/**
 * typedef udho::configuration<udho::config::server, udho::config::router, udho::config::logging, udho::config::view> configuration;
 * configuration config;
 * config[udho::configs::document_root] = "/path/to/document/root";
 */

namespace udho{
/**
 * @todo write docs
 */
template <typename... T>
class configuration{
    
};

template <typename K, typename C>
struct proxy{
    typedef K key_type;
    typedef C config_type;
    typedef decltype(C().get(K())) value_type;
    typedef proxy<K, C> proxy_type;
    
    config_type& _config;
    
    proxy(config_type& conf): _config(conf){}
    value_type value() const{
        return _config.get(K());
    }
    operator value_type() const{
        return value();
    }
    template <typename V>
    proxy_type& operator=(V value){
        _config.set(K(), value);
        return *this;
    }
};

template <typename T>
struct config: T{
    typedef config<T> self_type;    
        
    template <typename K>
    auto operator[](const K& /*key*/) const{
        return T::get(K());
    }
    template <typename K>
    proxy<K, self_type> operator[](const K& /*key*/){
        return proxy<K, self_type>(*this);
    }
};

template <typename K, typename C>
std::ostream& operator<<(std::ostream& stream, const proxy<K, C>& p){
    stream << p.value();
    return stream;
}


namespace configs{
template <typename T = void>
struct server_{
    const static struct port_t{} port;
    const static struct document_root_t{} document_root;
    const static struct template_root_t{} template_root;
    
    unsigned _port;
    std::string _document_root;
    std::string _template_root;
    
    inline void set(port_t, unsigned v){_port = v;}
    inline unsigned get(port_t) const{return _port;}
    
    inline void set(document_root_t, std::string v){_document_root = v;}
    inline std::string get(document_root_t) const{return _document_root;}
    
    inline void set(template_root_t, std::string v){_template_root = v;}
    inline std::string get(template_root_t) const{return _template_root;}
};

template <typename T> const typename server_<T>::port_t server_<T>::port;
template <typename T> const typename server_<T>::document_root_t server_<T>::document_root;
template <typename T> const typename server_<T>::template_root_t server_<T>::template_root;

typedef server_<> server;

}



}

#endif // CONFIGURATION_H
