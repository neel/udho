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


#ifndef UDHO_HAZO_SEQ_SEQ_H
#define UDHO_HAZO_SEQ_SEQ_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/seq/fwd.h>
#include <udho/hazo/seq/tag.h>
#include <udho/hazo/seq/helpers.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/detail/monoid.h>

namespace udho{
namespace util{
namespace hazo{

template <typename H>
using monoid_seq = detail::monoid<seq, H>;

/**
 * seq<A, B, C, D>: node<A, seq<B, C, D>>                                                        -> node<A, node<B, node<C, node<D, void>>>>     -> capsule<A>  depth 3
 *                          seq<B, C, D> : node<B, seq<C, D>>                                    -> node<B, node<C, node<D, void>>>              -> capsule<B>  depth 2
 *                                                 seq<C, D>: node<C, seq<D>>                    -> node<C, node<D, void>>                       -> capsule<C>  depth 1
 *                                                                     seq<D>: node<D, void>     -> node<D, void>                                -> capsule<D>  depth 0
 * 
 * \code
    seq<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    
    std::cout << vec.get<int>() << std::endl;
    std::cout << vec.get<std::string>() << std::endl;
    std::cout << vec.get<double>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.get<0>() << std::endl;
    std::cout << vec.get<1>() << std::endl;
    std::cout << vec.get<2>() << std::endl;
    std::cout << vec.get<3>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.set<0>(43) << std::endl;
    std::cout << vec.set<1>("World") << std::endl;
    std::cout << vec.set<2>(6.28) << std::endl;
    std::cout << vec.set<3>(42) << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.get<0>() << std::endl;
    std::cout << vec.get<1>() << std::endl;
    std::cout << vec.get<2>() << std::endl;
    std::cout << vec.get<3>() << std::endl;
 * \endcode
 */
template <typename Policy, typename H, typename... X>
struct seq: node<typename monoid_seq<H>::head, typename monoid_seq<H>::template extend<Policy, X...>>{
    typedef node<typename monoid_seq<H>::head, typename monoid_seq<H>::template extend<Policy, X...>> node_type;
    
    typedef seq_proxy<Policy, H, X...> proxy;
    
    using hana_tag = udho_hazo_seq_tag<Policy, 1+sizeof...(X)>;
    
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
struct seq<Policy, H>: node<typename monoid_seq<H>::head, typename monoid_seq<H>::rest>{
    typedef node<typename monoid_seq<H>::head, typename monoid_seq<H>::rest> node_type;
    
    typedef seq_proxy<Policy, H> proxy;
    
    using hana_tag = udho_hazo_seq_tag<Policy, 1>;
    
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
using seq_d = seq<by_data, X...>;
template <typename... X>
using seq_v = seq<by_value, X...>;

template <typename Policy, typename... X>
seq<Policy, X...> make_seq(const X&... xs){
    return seq<Policy, X...>(xs...);
}

template <typename... X>
seq_d<X...> make_seq_d(const X&... xs){
    return seq_d<X...>(xs...);
}

template <typename... X>
seq_v<X...> make_seq_v(const X&... xs){
    return seq_v<X...>(xs...);
}


}
}
}

#endif // UDHO_HAZO_SEQ_SEQ_H
