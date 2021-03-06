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

#ifndef UDHO_SCOPE_H
#define UDHO_SCOPE_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <udho/access.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

namespace udho{
    
struct declaration{
    std::string _id;
    std::string _ref;
    std::size_t _depth;
    
    inline declaration(const std::string& key, std::size_t depth): _id(key), _depth(depth){}
    inline bool operator<(const declaration& decl) const{
        return _id < decl._id;
    }
};
  
/**
 * Accepts a prepared object that can be queried through string keys.
 * Maintains a lookup table which contains aliases of such keys. So 
 * that the keys can be with both the real name as well as through the 
 * alias names. An unsigned integer depth value associated with every
 * aliases. Based on which all aliases of some depth can be cleared.
 * The <tt>up</tt> and <tt>down</tt> increases and decreases the
 * value of depth. <tt>add</tt> will add an alias with the current depth.
 * It can be used in a scoped environment where the entering to and 
 * leaving from an inner scope will call the down and up functions
 * respectively. Calling up will clear the aliases set in the depth below
 */
template <typename DataT>
struct lookup_table;

template <typename DataT>
struct lookup_table<udho::prepared<DataT>>{
    typedef udho::prepared<DataT> prepared_type;
    typedef boost::multi_index_container<
        declaration,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::identity<declaration>>,
            boost::multi_index::ordered_unique<boost::multi_index::member<declaration, std::string, &declaration::_id>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::member<declaration, std::size_t, &declaration::_depth>>
        > 
    > storage_type;
    typedef typename storage_type::nth_index<1>::type named_storage_type;
    typedef typename storage_type::nth_index<2>::type depth_storage_type;
    
    const prepared_type&  _prepared;
    storage_type          _storage;
    named_storage_type&   _storage_named;
    depth_storage_type&   _storage_depth;
    std::size_t           _level;
    
    lookup_table(const prepared_type& p): _prepared(p), _storage_named(_storage.get<1>()), _storage_depth(_storage.get<2>()), _level(0){}
    bool valid(const std::string& key) const{
        auto colon  = key.find_first_of(':');
        auto period = key.find_first_of('.');
        return std::min(colon, period) == std::string::npos;
    }
    std::string lookup(const std::string& key) const{
        std::string khead, ktail;
        auto colon  = key.find_first_of(':');
        auto period = key.find_first_of('.');
        auto sep    = std::min(colon, period);
        if(sep != std::string::npos){
            khead = key.substr(0, sep);
            ktail = key.substr(sep);
        }else{
            khead = key;
        }
        auto it = _storage_named.find(khead);
        if(it != _storage_named.end()){
            khead = it->_ref;
            return khead + ktail;
        }else{
            return key;
        }
    }
    bool add(const std::string& key, const std::string& ref, std::size_t depth){
        if(!valid(key)){
            return false;
        }
        auto it = _storage_named.find(key);
        if(it == _storage_named.end()){
            declaration decl(key, depth);
            std::string uri = lookup(ref);
            decl._ref = uri;
            _storage_named.insert(decl);
            return true;
        }
        return false;
    }
    std::size_t clear(std::size_t depth){
        std::size_t removed = _storage_depth.erase(depth);
        return removed;
    }
    std::string eval(const std::string& key) const{
        return _prepared[lookup(key)];
    }
    std::string operator[](const std::string& key) const{
        return eval(key);
    }
    template <typename T>
    T extract(const std::string& key) const{
        return _prepared.template extract<T>(lookup(key));
    }
    template <typename T>
    T parse(const std::string& key) const{
        return _prepared.template parse<T>(lookup(key));
    }
    std::vector<std::string> keys(const std::string& key) const{
        return _prepared.keys(key);
    }
    std::size_t count(const std::string& key) const{
        return _prepared.count(key);
    }
    std::size_t down(){
        return ++_level;
    }
    std::size_t up(){
        clear(_level);
        return --_level;
    }
    bool add(const std::string& key, const std::string& ref){
        return add(key, ref, _level);
    }
};

template <typename U, typename V>
struct lookup_table<udho::prepared_group<U, V>>{
    typedef udho::prepared_group<U, V> prepared_type;
    typedef boost::multi_index_container<
        declaration,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::identity<declaration>>,
            boost::multi_index::ordered_unique<boost::multi_index::member<declaration, std::string, &declaration::_id>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::member<declaration, std::size_t, &declaration::_depth>>
        > 
    > storage_type;
    typedef typename storage_type::nth_index<1>::type named_storage_type;
    typedef typename storage_type::nth_index<2>::type depth_storage_type;
    
    const prepared_type&  _prepared;
    storage_type          _storage;
    named_storage_type&   _storage_named;
    depth_storage_type&   _storage_depth;
    std::size_t           _level;
    
    lookup_table(const prepared_type& p): _prepared(p), _storage_named(_storage.get<1>()), _storage_depth(_storage.get<2>()), _level(0){}
    bool valid(const std::string& key) const{
        auto colon  = key.find_first_of(':');
        auto period = key.find_first_of('.');
        return std::min(colon, period) == std::string::npos;
    }
    std::string lookup(const std::string& key) const{
        std::string khead, ktail;
        auto colon  = key.find_first_of(':');
        auto period = key.find_first_of('.');
        auto sep    = std::min(colon, period);
        if(sep != std::string::npos){
            khead = key.substr(0, sep);
            ktail = key.substr(sep);
        }else{
            khead = key;
        }
        auto it = _storage_named.find(khead);
        if(it != _storage_named.end()){
            khead = it->_ref;
            return khead + ktail;
        }else{
            return key;
        }
    }
    bool add(const std::string& key, const std::string& ref, std::size_t depth){
        if(!valid(key)){
            return false;
        }
        auto it = _storage_named.find(key);
        if(it == _storage_named.end()){
            declaration decl(key, depth);
            std::string uri = lookup(ref);
            decl._ref = uri;
            _storage_named.insert(decl);
            return true;
        }
        return false;
    }
    std::size_t clear(std::size_t depth){
        std::size_t removed = _storage_depth.erase(depth);
        return removed;
    }
    std::string eval(const std::string& key) const{
        return _prepared[lookup(key)];
    }
    std::string operator[](const std::string& key) const{
        return eval(key);
    }
    template <typename T>
    T extract(const std::string& key) const{
        return _prepared.template extract<T>(lookup(key));
    }
    template <typename T>
    T parse(const std::string& key) const{
        return _prepared.template parse<T>(lookup(key));
    }
    std::vector<std::string> keys(const std::string& key) const{
        return _prepared.keys(key);
    }
    std::size_t count(const std::string& key) const{
        return _prepared.count(key);
    }
    std::size_t down(){
        return ++_level;
    }
    std::size_t up(){
        clear(_level);
        return --_level;
    }
    bool add(const std::string& key, const std::string& ref){
        return add(key, ref, _level);
    }
};

/**
 * returns a table of strings mapped with value returning objects
 * GroupT must be an instance of udho::prepare<T> which can be obtained by using udho::data(const T&)
 */
template <typename GroupT>
lookup_table<GroupT> scope(const GroupT& p){
    return lookup_table<GroupT>(p);
}

}

#endif // UDHO_SCOPE_H
