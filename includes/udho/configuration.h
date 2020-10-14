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

#include <map>
#include <string>
#include <boost/filesystem/path.hpp>

namespace udho{

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

/**
 * typedef udho::configuration<udho::config::server, udho::config::router, udho::config::logging, udho::config::view> configuration;
 * configuration config;
 * config[udho::configs::server::document_root] = "/path/to/document/root";
 */
template <typename... T>
struct configuration: udho::config<T>...{
    template <typename K>
    auto operator[](const K& k) const{
        return udho::config<typename K::component>::operator[](k);
    }
    template <typename K>
    auto operator[](const K& k){
        return udho::config<typename K::component>::operator[](k);
    }
};

namespace configs{
template <typename T = void>
struct server_{
    typedef std::map<std::string, std::string> mime_map;
    
    const static struct document_root_t{
        typedef server_<T> component;
    } document_root;
    const static struct template_root_t{
        typedef server_<T> component;
    } template_root;
    const static struct mime_default_t{
        typedef server_<T> component;
    } mime_default;
    const static struct mimes_t{
        typedef server_<T> component;
    } mimes;
    
    boost::filesystem::path _document_root;
    boost::filesystem::path _template_root;
    std::string _mime_default;
    mime_map    _mimes;
    
    
    server_(): _mime_default("application/octet-stream"){
        _mimes.insert(std::make_pair("htm",     "text/html"));
        _mimes.insert(std::make_pair("html",    "text/html"));
        _mimes.insert(std::make_pair("xhtm",    "text/html"));
        _mimes.insert(std::make_pair("xhtml",   "text/html"));
        _mimes.insert(std::make_pair("txt",     "text/plain"));
        _mimes.insert(std::make_pair("js",      "application/javascript"));
        _mimes.insert(std::make_pair("css",     "application/css"));
        _mimes.insert(std::make_pair("json",    "application/json"));
        _mimes.insert(std::make_pair("geojson", "application/json"));
        _mimes.insert(std::make_pair("xml",     "application/xml"));
        _mimes.insert(std::make_pair("swf",     "application/x-shockwave-flash"));
        _mimes.insert(std::make_pair("flv",     "video/x-flv"));
        _mimes.insert(std::make_pair("png",     "image/png"));
        _mimes.insert(std::make_pair("jpe",     "image/jpeg"));
        _mimes.insert(std::make_pair("jpeg",    "image/jpeg"));
        _mimes.insert(std::make_pair("jpg",     "image/jpeg"));
        _mimes.insert(std::make_pair("gif",     "image/gif"));
        _mimes.insert(std::make_pair("bmp",     "image/bmp"));
        _mimes.insert(std::make_pair("ico",     "image/x-icon"));
        _mimes.insert(std::make_pair("tiff",    "image/tiff"));
        _mimes.insert(std::make_pair("tif",     "image/tiff"));
        _mimes.insert(std::make_pair("svg",     "image/svg+xml"));
        _mimes.insert(std::make_pair("svgz",    "image/svg+xml"));
    }
        
    void set(document_root_t, const boost::filesystem::path& v){_document_root = v;}
    boost::filesystem::path get(document_root_t) const{return _document_root;}
    
    void set(template_root_t, const boost::filesystem::path& v){_template_root = v;}
    boost::filesystem::path get(template_root_t) const{return _template_root;}
    
    void set(mime_default_t, std::string v){_mime_default = v;}
    std::string get(mime_default_t) const{return _mime_default;}

    const mime_map& get(mimes_t) const{return _mimes;}
    std::string mime(const std::string& extension) const{
        return _mimes.at(extension);
    }
    void mime(const std::string& extension, const std::string& type){
        _mimes[extension] = type;
    }
};

template <typename T> const typename server_<T>::document_root_t server_<T>::document_root;
template <typename T> const typename server_<T>::template_root_t server_<T>::template_root;
template <typename T> const typename server_<T>::mime_default_t  server_<T>::mime_default;
template <typename T> const typename server_<T>::mimes_t         server_<T>::mimes;

typedef server_<> server;

}

template <>
struct config<configs::server_<>>: configs::server_<>{
    typedef config<configs::server_<>> self_type;    
        
    template <typename K>
    auto operator[](const K& /*key*/) const{
        return configs::server_<>::get(K());
    }
    template <typename K>
    proxy<K, self_type> operator[](const K& /*key*/){
        return proxy<K, self_type>(*this);
    }
    void mime(const std::string& extension, const std::string& type){
         configs::server_<>::mime(extension, type);
    }
};

template <>
struct proxy<configs::server_<>::mimes_t, config<configs::server_<>>>{
    typedef configs::server_<>::mimes_t key_type;
    typedef config<configs::server_<>> config_type;
    typedef decltype(config_type().get(key_type())) value_type;
    typedef proxy<key_type, config_type> proxy_type;
    
    config_type& _config;
    
    proxy(config_type& conf): _config(conf){}
    value_type value() const{
        return _config.get(key_type());
    }
    operator value_type() const{
        return value();
    }
    template <typename V>
    proxy_type& operator=(V value){
        _config.set(key_type(), value);
        return *this;
    }
    std::string at(const std::string& key) const{
        auto map = value();
        if(map.count(key)){
            return map.at(key);
        }
        return _config[configs::server::mime_default];
    }
    std::string of(const std::string& key) const{
        return at(key);
    }
    void set(const std::string& ext, const std::string& mime){
        _config.mime(ext, mime);
    }
};

namespace configs{
template <typename T = void>
struct session_{
    enum class format{
        none, binary, json, xml
    };
    
    const static struct persistent_t{
        typedef session_<T> component;
    } persistent;
    const static struct serialization_t{
        typedef session_<T> component;
    } serialization;
    const static struct path_t{
        typedef session_<T> component;
    } path;
    const static struct id_t{
        typedef session_<T> component;
    } id;
    
    bool        _persistent;
    format      _serialization;
    boost::filesystem::path _path;
    std::string _id;
    
    session_(): _persistent(false), _serialization(format::none), _id("UDHOSESSID"){}
    
    void set(persistent_t, bool v){_persistent = v;}
    unsigned get(persistent_t) const{return _persistent;}
    
    void set(serialization_t, format v){_serialization = v;}
    format get(serialization_t) const{return _serialization;}
    
    void set(path_t, const boost::filesystem::path& p){_path = p;}
    boost::filesystem::path get(path_t) const{return _path;}
    
    void set(id_t, std::string v){_id = v;}
    std::string get(id_t) const{return _id;}
};

template <typename T> const typename session_<T>::persistent_t session_<T>::persistent;
template <typename T> const typename session_<T>::serialization_t session_<T>::serialization;
template <typename T> const typename session_<T>::path_t session_<T>::path;
template <typename T> const typename session_<T>::id_t session_<T>::id;

typedef session_<> session;
}

namespace configs{
template <typename T = void>
struct router_{

};

typedef router_<> router;
}

namespace configs{
template <typename T = void>
struct logger_{

};

typedef logger_<> logger;
}

namespace configs{
template <typename T = void>
struct form_{

};

typedef form_<> form;
}

typedef udho::configuration<udho::configs::server, udho::configs::session, udho::configs::router, udho::configs::logger, udho::configs::form> configuration_type;

}

#endif // CONFIGURATION_H
