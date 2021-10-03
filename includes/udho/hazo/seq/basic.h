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


#ifndef UDHO_HAZO_SEQ_BASIC_H
#define UDHO_HAZO_SEQ_BASIC_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/seq/fwd.h>
#include <udho/hazo/seq/tag.h>
#include <udho/hazo/seq/helpers.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/operations/flatten.h>

namespace udho{
namespace hazo{
    
template <typename Policy, typename H, typename... X>
struct basic_seq: node<H, basic_seq<Policy, X...>>{
    typedef node<H, basic_seq<Policy, X...>> node_type;
    
    typedef seq_proxy<Policy, H, X...> proxy;
    
    using hana_tag = udho_hazo_seq_tag<Policy, 1+sizeof...(X)>;
    
    template <typename... U>
    using exclude = typename operations::exclude<basic_seq<Policy, H, X...>, U...>::type;
    template <typename... U>
    using extend = typename operations::append<basic_seq<Policy, H, X...>, U...>::type;
    template <template <typename...> class ContainerT>
    using translate = ContainerT<H, X...>;
    template <typename T>
    using contains = typename node_type::types::template exists<T>;
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    template <template <typename> class F>
    using transform = basic_seq<Policy, F<H>, F<X>...>;
    
    using node_type::node_type;
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        const_call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        call_helper<Policy, node_type, typename build_indices<1+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
};

template <typename Policy, typename H>
struct basic_seq<Policy, H>: node<H, void>{
    typedef node<H, void> node_type;
    
    typedef seq_proxy<Policy, H> proxy;
    
    using hana_tag = udho_hazo_seq_tag<Policy, 1>;
    
    template <typename... U>
    using exclude = typename operations::exclude<basic_seq<Policy, H>, U...>::type;
    template <typename... U>
    using extend = typename operations::append<basic_seq<Policy, H>, U...>::type;
    template <template <typename...> class ContainerT>
    using translate = ContainerT<H>;
    template <typename T>
    using contains = typename node_type::types::template exists<T>;
    template <typename KeyT>
    using has = typename node_type::types::template has<KeyT>;
    template <template <typename> class F>
    using transform = basic_seq<Policy, F<H>>;
    
    using node_type::node_type;
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        const_call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
};

template <typename... X>
using basic_seq_d = basic_seq<by_data, X...>;
template <typename... X>
using basic_seq_v = basic_seq<by_value, X...>;
    
}
}

#endif // UDHO_HAZO_SEQ_BASIC_H
