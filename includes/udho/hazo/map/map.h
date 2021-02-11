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

#ifndef UDHO_HAZO_MAP_MAP_H
#define UDHO_HAZO_MAP_MAP_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/map/helpers.h>
#include <udho/hazo/map/tag.h>
#include <udho/hazo/map/fwd.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/detail/monoid.h>
#include <udho/hazo/detail/fwd.h>
#include <udho/hazo/detail/operations.h>
#include <udho/hazo/map/basic.h>
#include <udho/hazo/map/operations.h>

namespace udho{
namespace util{
namespace hazo{

template <typename... T>
using map_d = typename operations::flatten<basic_map_d, T...>::type;

template <typename... T>
using map_v = typename operations::flatten<basic_map_v, T...>::type;
    
// template <typename Policy, typename... X, typename... T>
// struct extend<map<Policy, X...>, T...>{
//     using type = map<Policy, T..., X...>;
// };
// 
// template <typename Policy, typename... T>
// struct extend<map<Policy, void>, T...>{
//     using type = map<Policy, T...>;
// };
// 
// template <typename Policy, typename H, typename... X, typename U>
// struct remove<map<Policy, H, X...>, U>{
//     enum { 
//         matched = std::is_same<H, U>::value
//     };
//     using tail = map<Policy, X...>;
//     using type = typename std::conditional<matched, 
//         tail,
//         typename extend<typename remove<tail, U>::type, H>::type
//     >::type;
// };
// 
// template <typename Policy, typename H, typename U>
// struct remove<map<Policy, H>, U>{
//     enum { 
//         matched = std::is_same<H, U>::value
//     };
//     using type = typename std::conditional<matched, 
//         map<Policy, void>,
//         map<Policy, H>
//     >::type;
// };
// 
// template <typename H>
// using monoid_map = detail::monoid<map, H>;
//     
// template <typename Policy, typename H, typename... X>
// struct map: node<typename monoid_map<H>::head, typename monoid_map<H>::template extend<Policy, X...>>{
//     typedef node<typename monoid_map<H>::head, typename monoid_map<H>::template extend<Policy, X...>> node_type;
//     
//     typedef map_proxy<Policy, H, X...> proxy;
//     
//     template <typename... U>
//     using exclude = typename exclude<map<Policy, H, X...>, U...>::type;
//     template <typename... U>
//     using extend = typename extend<map<Policy, H, X...>, U...>::type;
//     
//     using hana_tag = udho_hazo_map_tag<Policy, H, X...>;
//     
//     using node_type::node_type;
//     map(const H& h, const X&... xs): node<H, map<Policy, X...>>(h, xs...){}
//     template <typename... Y, typename = typename std::enable_if<!std::is_same<map<Policy, H, X...>, map<Policy, Y...>>::value>::type>
//     map(const map<Policy, Y...>& other): node_type(static_cast<const typename map<Policy, Y...>::node_type&>(other)) {}
//     template <typename FunctionT>
//     decltype(auto) unpack(FunctionT&& f) const{
//         call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
//         return helper.apply(std::forward<FunctionT>(f));
//     }
//     template <typename FunctionT>
//     decltype(auto) unpack(FunctionT&& f){
//         const_call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
//         return helper.apply(std::forward<FunctionT>(f));
//     }
//     
//     template <typename ElementT>
//     using contains = typename node_type::types::template exists<ElementT>;
//     template <typename KeyT>
//     using has = typename node_type::types::template has<KeyT>;
//     
//     template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
//     decltype(auto) operator[](const element_t<ElementT>& e){
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
//     }
//     template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
//     decltype(auto) operator[](const KeyT& k){
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
//     }
//     template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
//     decltype(auto) operator[](const element_t<ElementT>& e) const {
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
//     }
//     template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
//     decltype(auto) operator[](const KeyT& k) const {
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
//     }
// 
// };
// 
// template <typename Policy, typename H>
// struct map<Policy, H>: node<typename monoid_map<H>::head, typename monoid_map<H>::rest>{
//     typedef node<typename monoid_map<H>::head, typename monoid_map<H>::rest> node_type;
//     
//     typedef map_proxy<Policy, H> proxy;
//     
//     using hana_tag = udho_hazo_map_tag<Policy, H>;
//     
//     template <typename... U>
//     using exclude = typename exclude<map<Policy, H>, U...>::type;
//     template <typename... U>
//     using extend = typename extend<map<Policy, H>, U...>::type;
//     
//     using node_type::node_type;
//     map(const H& h): node<H, void>(h){}
//     template <typename... Y, typename = typename std::enable_if<!std::is_same<map<Policy, H>, map<Policy, Y...>>::value>::type>
//     map(const map<Policy, Y...>& other): node_type(static_cast<const typename map<Policy, Y...>::node_type&>(other)) {}
//     
//     template <typename ElementT>
//     using contains = typename node_type::types::template exists<ElementT>;
//     template <typename KeyT>
//     using has = typename node_type::types::template has<KeyT>;
//     
//     template <typename FunctionT>
//     decltype(auto) unpack(FunctionT&& f) const{
//         call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
//         return helper.apply(std::forward<FunctionT>(f));
//     }
//     template <typename FunctionT>
//     decltype(auto) unpack(FunctionT&& f) {
//         call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
//         return helper.apply(std::forward<FunctionT>(f));
//     }
//     template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
//     decltype(auto) operator[](const element_t<ElementT>& e){
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
//     }
//     template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
//     decltype(auto) operator[](const KeyT& k){
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
//     }
//     template <typename ElementT, typename = typename std::enable_if<contains<ElementT>::value && !has<ElementT>::value>::type>
//     decltype(auto) operator[](const element_t<ElementT>& e) const {
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
//     }
//     template <typename KeyT, typename = typename std::enable_if<!contains<KeyT>::value && has<KeyT>::value>::type>
//     decltype(auto) operator[](const KeyT& k) const {
//         return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
//     }
// };
// 
// template <typename P, typename... T, typename V>
// decltype(auto) operator>>(const map<P, T...>& node, V& var){
//     return operator>>(static_cast<const typename map<P, T...>::node_type&>(node), var);
// }
// 
// template <typename... X>
// using map_d = map<by_data, X...>;
// template <typename... X>
// using map_v = map<by_value, X...>;
// 
// template <typename Policy, typename... X>
// map<Policy, X...> make_map(const X&... xs){
//     return map<Policy, X...>(xs...);
// }
// 
// template <typename... X>
// map_d<X...> make_map_v(const X&... xs){
//     return map_d<X...>(xs...);
// }
// 
// template <typename... X>
// map_v<X...> make_map_v(const X&... xs){
//     return map_v<X...>(xs...);
// }

}
}
}

#endif // UDHO_HAZO_MAP_MAP_H
