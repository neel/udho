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

#ifndef UDHO_FOLDING_DETAIL_HELPER_CALL_H
#define UDHO_FOLDING_DETAIL_HELPER_CALL_H

#include <utility>
#include <type_traits>
#include <udho/folding/detail/indices.h>
#include <udho/folding/detail/extraction_helper.h>
#include <udho/folding/node/fwd.h>

namespace udho{
namespace util{
namespace folding{

template <typename Policy, typename LevelT, typename Indecies>
struct const_call_helper;

template <typename Policy, typename HeadT, typename TailT, std::size_t... Is>
struct const_call_helper<Policy, node<HeadT, TailT>, indices<Is...>>{
    typedef node<HeadT, TailT> node_type;
    
    const node_type& _node;
    const_call_helper(const node_type& l): _node(l){}
    
    template <typename FunctionT>
    decltype(auto) apply(FunctionT&& f){
        return f(const_extraction_helper<Policy, node_type, Is>::apply(_node)...);
    }
};

template <typename Policy, typename LevelT, typename Indecies>
struct call_helper;

template <typename Policy, typename HeadT, typename TailT, std::size_t... Is>
struct call_helper<Policy, node<HeadT, TailT>, indices<Is...>>{
    typedef node<HeadT, TailT> node_type;
    
    node_type& _node;
    call_helper(node_type& l): _node(l){}
    
    template <typename FunctionT>
    decltype(auto) apply(FunctionT&& f){
        return std::forward<FunctionT>(f)(extraction_helper<Policy, node_type, Is>::apply(_node)...);
    }
};


}
}
}

#endif // UDHO_FOLDING_DETAIL_HELPER_CALL_H
