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

#ifndef UDHO_HAZO_NODE_BASIC_H
#define UDHO_HAZO_NODE_BASIC_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/capsule.h>

namespace udho{
namespace util{
namespace hazo{
    
template <typename HeadT, typename TailT = void>
struct basic_node{
    typedef basic_node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    typedef capsule<HeadT> capsule_type;
    typedef typename capsule_type::key_type key_type;
    typedef typename capsule_type::data_type data_type;
    typedef typename capsule_type::value_type value_type;
    typedef typename capsule_type::index_type index_type;
    
    struct types{
        template <int N>
        using tail_at = typename std::conditional<N == 0, tail_type, typename tail_type::types::template tail_at<N-1>>;
        template <int N>
        using capsule_at = typename std::conditional<N == 0, capsule_type, typename tail_type::types::template capsule_at<N-1>>::type;
        template <int N>
        using data_at = typename capsule_at<N>::data_type;
        template <int N>
        using value_at = typename capsule_at<N>::value_type;
        template <typename T, int N = 0>
        using capsule_of = typename std::conditional<std::is_same<T, index_type>::value && N == 0, 
                capsule_type, 
                typename tail_type::types::template capsule_of<T, N - std::is_same<T, index_type>::value>
            >::type;
        template <typename T, int N = 0>
        using data_of = typename capsule_of<T, N>::data_type;
        template <typename T, int N = 0>
        using value_of = typename capsule_of<T, N>::value_type;
        template <typename KeyT>
        using data_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
                data_type,
                typename tail_type::types::template data_for<KeyT>
            >::type;
        template <typename KeyT>
        using value_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
                value_type,
                typename tail_type::types::template value_for<KeyT>
            >::type;
            
        template <typename T>
        struct exists{
            enum { value = std::is_same<T, index_type>::value || tail_type::types::template exists<T>::value };
        };
        template <typename KeyT>
        struct has{
            enum { value = (!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value) || tail_type::types::template has<KeyT>::value };
        };
    };
};

template <typename HeadT>
struct basic_node<HeadT, void>{
    typedef void tail_type;
    typedef capsule<HeadT> capsule_type;
    typedef typename capsule_type::key_type key_type;
    typedef typename capsule_type::data_type data_type;
    typedef typename capsule_type::value_type value_type;
    typedef typename capsule_type::index_type index_type;
    
    struct types{
        template <int N>
        using tail_at = void;
        template <int N>
        using capsule_at = typename std::conditional<N == 0, capsule_type, void>::type;
        template <int N>
        using data_at = typename capsule_at<N>::data_type;
        template <int N>
        using value_at = typename capsule_at<N>::value_type;
        template <typename T, int N = 0>
        using capsule_of = typename std::conditional<std::is_same<T, index_type>::value && N == 0, capsule_type, void>::type;
        template <typename T, int N = 0>
        using data_of = typename capsule_of<T, N>::data_type;
        template <typename T, int N = 0>
        using value_of = typename capsule_of<T, N>::value_type;
        template <typename KeyT>
        using data_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
            data_type,
            void
            >::type;
        template <typename KeyT>
        using value_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
            value_type,
            void
            >::type;
            
        template <typename T>
        struct exists{
            enum { value = std::is_same<T, index_type>::value };
        };
        template <typename KeyT>
        struct has{
            enum { value = !std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value};
        };
    };
};


}
}
}


#endif // UDHO_HAZO_NODE_BASIC_H
