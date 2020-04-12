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
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

namespace udho{
    
struct declaration{
    std::string _id;
    std::string _ref;
    std::size_t _depth;
    
    declaration(const std::string& key, std::size_t depth);
    bool operator<(const declaration& decl) const;
};
  
template <typename GroupT>
struct lookup_table{
    typedef GroupT group_type;
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
    
    group_type&         _group;
    storage_type        _storage;
    named_storage_type& _storage_named;
    depth_storage_type& _storage_depth;
    std::size_t         _level;
    
    lookup_table(GroupT& group): _group(group), _storage_named(_storage.get<1>()), _storage_depth(_storage.get<2>()), _level(0){}
    bool valid(const std::string& key) const{
        auto colon  = key.find_first_of(':');
        auto period = key.find_first_of('.');
        return std::min(colon, period) == std::string::npos;
    }
    std::string lookup(const std::string& key){
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
    std::string eval(const std::string& key){
        return _group[lookup(key)];
    }
    std::vector<std::string> keys(const std::string& key){
        return _group.keys(key);
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
    void list(){
        for(const udho::declaration& decl: _storage_named){
            std::cout << decl._id << " " << decl._ref << " -> " << decl._depth << std::endl;
        }
    }
};

template <typename GroupT>
lookup_table<GroupT> scope(GroupT& group){
    return lookup_table<GroupT>(group);
}

}

#endif // UDHO_SCOPE_H
