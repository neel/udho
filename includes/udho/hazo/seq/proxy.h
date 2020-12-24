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


#ifndef UDHO_HAZO_SEQ_PROXY_H
#define UDHO_HAZO_SEQ_PROXY_H

#include <udho/hazo/node/proxy.h>
#include <udho/hazo/seq/tag.h>
#include <udho/hazo/seq/helpers.h>
#include <udho/hazo/seq/seq.h>

namespace udho{
namespace util{
namespace hazo{
    
template <typename Policy, typename... X>
struct seq_proxy: proxy<X...>{
    typedef proxy<X...> node_type;
    typedef seq_proxy<Policy, X...> self_type;
    
    using hana_tag = udho_hazo_seq_tag<Policy, sizeof...(X)>;
    
    seq_proxy() = delete;
    seq_proxy(const self_type&) = default;
    template <typename OtherPolicy, typename... Rest>
    seq_proxy(seq<OtherPolicy, Rest...>& actual): node_type(static_cast<typename seq<OtherPolicy, Rest...>::node_type&>(actual)){}
    
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        const_call_helper<Policy, node_type, typename build_indices<sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        call_helper<Policy, node_type, typename build_indices<sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
};

template <typename... X>
using seq_proxy_d = seq_proxy<by_data, X...>;
template <typename... X>
using seq_proxy_v = seq_proxy<by_value, X...>;
    
}
}
}

#endif // UDHO_HAZO_SEQ_PROXY_H
