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

#ifndef UDHO_CACHE_H
#define UDHO_CACHE_H

#include <set>
#include <map>
#include <chrono>
#include <cstdint>
#include <boost/asio/ip/address.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace udho{
namespace cache{
    
// struct peer{
//     boost::asio::ip::address _address;
//     std::uint32_t _port;
//     std::uint32_t _latency;
//     
//     peer(const boost::asio::ip::address& address, std::uint32_t port);
// };
// 
// template <typename T>
// struct item{
//     typedef std::chrono::time_point<std::chrono::system_clock> time_type;
//     
//     T _item;
//     std::size_t _order;
//     time_type _created;
//     time_type _updated;
//     
//     item(const T& v): _item(v), _created(std::chrono::system_clock::now()), _updated(_created){}
//     template <typename StreamT>
//     void write(StreamT& stream){
//         // TODO implement
//     }
//     template <typename StreamT>
//     static item<T> read(const StreamT& stream){
//         // TODO implement;
//     }
// };
// 
// template <typename T, typename KeyT=std::string>
// struct store{
//     typedef KeyT key_type;
//     typedef T value_type;
//     typedef item<value_type> item_type;
//     typedef std::map<key_type, item_type> map_type;
//     
//     std::string _name;
//     map_type _registry;
//     
//     store(const std::string& name): _name(name){}
//     void add(const key_type& key, const value_type& value){
//         item_type item(value);
//         _registry.insert(std::make_pair(key, item));
//     }
//     bool exists(const key_type& key) const{
//         return _registry.find(key) != _registry.end();
//     }
//     T& get(const key_type& key) const{
//         auto it = _registry.find(key);
//         if(it != _registry.end()){
//             return it->second;
//         }
//         // TODO throw exception
//     }
// };

template <typename KeyT>
struct master{
    typedef KeyT key_type;
    typedef std::set<key_type> set_type;
    typedef master<KeyT> self_type;
       
    master() = default;
    master(const self_type&) = delete;
    master(self_type&&) = default;
    
    bool issued(const key_type& key) const{
        return _set.find(key) != _set.cend();
    }
    void issue(const key_type& key){
        _set.insert(key);
    }
    
protected:
    set_type _set;
};

// template <typename KeyT, typename... U>
// struct master_{
//     template <typename T>
//     using pair = std::pair<T, bool>;
//     typedef boost::tuple<pair<U>...> tuple_type;
//     typedef std::map<KeyT, tuple_type> map_type;
// };

template <typename KeyT, typename T>
struct registry{
    typedef KeyT key_type;
    typedef T value_type;
    typedef std::map<KeyT, T> map_type;
    typedef registry<KeyT, T> self_type;
       
    registry() = default;
    registry(const self_type&) = delete;
    registry(self_type&&) = default;
       
    bool exists(const key_type& key) const{
        return _storage.count(key);
    }
    const value_type& at(const key_type& key) const{
        return _storage.at(key);
    }
    void insert(const key_type& key, const value_type& value){
        _storage[key] = value;
    }
    
protected:
    map_type _storage;
};

template <typename KeyT, typename... T>
struct shadow;

template <typename KeyT, typename... T>
struct store: master<KeyT>, registry<KeyT, T>...{ 
    typedef KeyT key_type;
    typedef master<KeyT> master_type;
    typedef store<KeyT, T...> self_type;
    typedef shadow<KeyT, T...> shadow_type;
      
    store() = default;
    store(const self_type&) = delete;
    store(self_type&&) = default;
        
//     using master_type::issued;
//     using master_type::issue;
    
    template <typename V>
    bool exists(const key_type& key) const{
        return master_type::issued(key) && registry<KeyT, V>::exists(key);
    }
    template <typename V>
    const V& get(const key_type& key, const V& def=V()) const{
        if(master_type::issued(key)){
            return registry<KeyT, V>::at(key);
        }else{
            return def;
        }
    }
    template <typename V>
    V& at(const key_type& key, const V& def=V()){
        if(master_type::issued(key)){
            return registry<KeyT, V>::at(key);
        }
        // TODO throw
    }
    template <typename V>
    void insert(const key_type& key, const V& value){
        registry<KeyT, V>::insert(key, value);
        if(!master_type::issued(key)){
            master_type::issue(key);
        }
    }
};

template <typename KeyT, typename T>
struct flake{
    typedef KeyT key_type;
    typedef T value_type;
    typedef registry<key_type, T> registry_type;
    typedef flake<KeyT, T> flake_type;
    typedef flake_type self_type;
   
    template <typename... X>
    flake(store<KeyT, X...>& store): _registry(store){}
    
    bool exists(const key_type& key) const{
        return _registry.exists(key);
    }
    
    const value_type& at(const key_type& key) const{
        return _registry.at(key);
    }
    
    void insert(const key_type& key, const value_type& value){
        _registry.insert(key, value);
    }
private:
    registry_type& _registry;
};

template <typename KeyT, typename... T>
struct shadow: flake<KeyT, T>...{
    typedef KeyT key_type;
    typedef store<key_type, T...> store_type;
    typedef shadow<key_type, T...> self_type;
    typedef self_type shadow_type;
    typedef master<key_type> master_type;
    
    master_type& _master;
    
    template <typename... X>
    shadow(store<key_type, X...>& store): flake<key_type, T>(store)..., _master(store){}
    
    template <typename... X>
    shadow(shadow<key_type, X...>& other): flake<key_type, T>(other)..., _master(other._master){}
    
    bool issued(const key_type& key) const{
        return _master.issued(key);
    }
    void issue(const key_type& key){
        _master.issue(key);
    }
    
    template <typename V>
    bool exists(const key_type& key) const{
        return issued(key) && flake<key_type, V>::exists(key);
    }
    template <typename V>
    const V& at(const key_type& key) const{
        return flake<key_type, V>::at(key);
    }
    template <typename V>
    const V& get(const key_type& key, const V& def=V()) const{
        if(issued(key)){
            return at<V>(key);
        }
        return def;
    }
    template <typename V>
    void insert(const key_type& key, const V& value){
        flake<key_type, V>::insert(key, value);
        if(!issued(key)){
            issue(key);
        }
    }
};

template <typename KeyT>
struct generator;

template <>
struct generator<boost::uuids::uuid>{
    boost::uuids::uuid parse(const std::string& key){
        return boost::lexical_cast<boost::uuids::uuid>(key);;
    }
    boost::uuids::uuid generate(){
        return boost::uuids::random_generator()();
    }
};

   
}
}

#endif // UDHO_CACHE_H
