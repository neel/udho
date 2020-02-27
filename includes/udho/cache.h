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

#include <map>
#include <chrono>
#include <cstdint>
#include <boost/asio/ip/address.hpp>

namespace udho{
namespace cache{
    
struct peer{
    boost::asio::ip::address _address;
    std::uint32_t _port;
    std::uint32_t _latency;
    
    peer(const boost::asio::ip::address& address, std::uint32_t port);
};

template <typename T>
struct item{
    typedef std::chrono::time_point<std::chrono::system_clock> time_type;
    
    T _item;
    std::size_t _order;
    time_type _created;
    time_type _updated;
    
    item(const T& v): _item(v), _created(std::chrono::system_clock::now()), _updated(_created){}
    template <typename StreamT>
    void write(StreamT& stream){
        // TODO implement
    }
    template <typename StreamT>
    static item<T> read(const StreamT& stream){
        // TODO implement;
    }
};

template <typename T, typename KeyT=std::string>
struct store{
    typedef KeyT key_type;
    typedef T value_type;
    typedef item<value_type> item_type;
    typedef std::map<key_type, item_type> map_type;
    
    std::string _name;
    map_type _registry;
    
    store(const std::string& name): _name(name){}
    void add(const key_type& key, const value_type& value){
        item_type item(value);
        _registry.insert(std::make_pair(key, item));
    }
    bool exists(const key_type& key) const{
        return _registry.find(key) != _registry.end();
    }
    T& get(const key_type& key) const{
        auto it = _registry.find(key);
        if(it != _registry.end()){
            return it->second;
        }
        // TODO throw exception
    }
};
    
}    
}

#endif // UDHO_CACHE_H
