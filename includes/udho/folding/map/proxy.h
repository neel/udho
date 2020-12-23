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


#ifndef UDHO_FOLDING_MAP_PROXY_H
#define UDHO_FOLDING_MAP_PROXY_H

#include <udho/folding/node/proxy.h>
#include <udho/folding/map/tag.h>
#include <udho/folding/map/helpers.h>

namespace udho{
namespace util{
namespace folding{
    
template <typename Policy, typename... X>
struct map_proxy: proxy<X...>{
    typedef proxy<X...> node_type;
    
    using hana_tag = udho_folding_map_tag<Policy, X...>;
    
    using node_type::node_type;

    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        call_helper<Policy, node_type, typename build_indices<sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        const_call_helper<Policy, node_type, typename build_indices<sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    
    template <typename ElementT>
    decltype(auto) operator[](const element_t<ElementT>& e){
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
    }
    template <typename KeyT>
    decltype(auto) operator[](const KeyT& k){
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
    }
    template <typename ElementT>
    decltype(auto) operator[](const element_t<ElementT>& e) const {
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<ElementT>(e));
    }
    template <typename KeyT>
    decltype(auto) operator[](const KeyT& k) const {
        return map_by_key_helper<Policy>::apply(node_type::template operator[]<KeyT>(k));
    }

};
    
template <typename... X>
using map_proxy_d = map_proxy<by_data, X...>;
template <typename... X>
using map_proxy_v = map_proxy<by_value, X...>;

}
}
}

#endif // UDHO_FOLDING_MAP_PROXY_H