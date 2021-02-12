/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_HAZO_MAP_BASIC_H
#define UDHO_HAZO_MAP_BASIC_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/map/helpers.h>
#include <udho/hazo/map/tag.h>
#include <udho/hazo/map/fwd.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/operations/flatten.h>

namespace udho{
namespace util{
namespace hazo{
    
template <typename Policy, typename H, typename... X>
struct basic_map: node<H, basic_map<Policy, X...>>{
    typedef node<H, basic_map<Policy, X...>> node_type;
    
    using hana_tag = udho_hazo_map_tag<Policy, H, X...>;
    
    typedef map_proxy<Policy, H, X...> proxy;
    
    template <typename ElementT>
    using contains = typename node_type::types::template exists<ElementT>;
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    template <template <typename> class ConditionT, typename... U>
    using exclude_if = typename operations::exclude_if<basic_map<Policy, H, X...>, ConditionT, U...>::type;
    template <typename... U>
    using exclude = typename operations::exclude<basic_map<Policy, H, X...>, U...>::type;
    template <typename... U>
    using extend = typename operations::append<basic_map<Policy, H, X...>, U...>::type;
    template <template <typename...> class ContainerT>
    using transform = ContainerT<H, X...>;
    
    using node_type::node_type;
    basic_map(const H& h, const X&... xs): node<H, basic_map<Policy, X...>>(h, xs...){}
    template <typename... Y, typename = typename std::enable_if<!std::is_same<basic_map<Policy, H, X...>, basic_map<Policy, Y...>>::value>::type>
    basic_map(const basic_map<Policy, Y...>& other): node_type(static_cast<const typename basic_map<Policy, Y...>::node_type&>(other)) {}
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        const_call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    
    template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
    decltype(auto) operator[](const element_t<ElementT>& e){
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
    }
    template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
    decltype(auto) operator[](const KeyT& k){
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
    }
    template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
    decltype(auto) operator[](const element_t<ElementT>& e) const {
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
    }
    template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
    decltype(auto) operator[](const KeyT& k) const {
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
    }

};

template <typename Policy, typename H>
struct basic_map<Policy, H>: node<H, void>{
    typedef node<H, void> node_type;

    using hana_tag = udho_hazo_map_tag<Policy, H>;
    
    typedef map_proxy<Policy, H> proxy;
    
    template <typename ElementT>
    using contains = typename node_type::types::template exists<ElementT>;
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    template <template <typename> class ConditionT, typename... U>
    using exclude_if = typename operations::exclude_if<basic_map<Policy, H>, ConditionT, U...>::type;
    template <typename... U>
    using exclude = typename operations::exclude<basic_map<Policy, H>, U...>::type;
    template <typename... U>
    using extend = typename operations::append<basic_map<Policy, H>, U...>::type;
    template <template <typename...> class ContainerT>
    using transform = ContainerT<H>;
    
    using node_type::node_type;
    basic_map(const H& h): node<H, void>(h){}
    template <typename... Y, typename = typename std::enable_if<!std::is_same<basic_map<Policy, H>, basic_map<Policy, Y...>>::value>::type>
    basic_map(const basic_map<Policy, Y...>& other): node_type(static_cast<const typename basic_map<Policy, Y...>::node_type&>(other)) {}
    
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) {
        call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
    decltype(auto) operator[](const element_t<ElementT>& e){
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
    }
    template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
    decltype(auto) operator[](const KeyT& k){
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
    }
    template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
    decltype(auto) operator[](const element_t<ElementT>& e) const {
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
    }
    template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
    decltype(auto) operator[](const KeyT& k) const {
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
    }
};

template <typename P, typename... T, typename V>
decltype(auto) operator>>(const basic_map<P, T...>& node, V& var){
    return operator>>(static_cast<const typename basic_map<P, T...>::node_type&>(node), var);
}


template <typename... X>
using basic_map_d = basic_map<by_data, X...>;
template <typename... X>
using basic_map_v = basic_map<by_value, X...>;
    
}
}
}

#endif // UDHO_HAZO_MAP_BASIC_H
