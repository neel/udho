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

#ifndef UDHO_HAZO_NODE_META_H
#define UDHO_HAZO_NODE_META_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/capsule.h>

namespace udho{
namespace hazo{
    
#ifndef __DOXYGEN__

/**
 * @brief basic types for a non-terminal node 
 * @tparam HeadT type of the data inside this node
 * @tparam TailT tail of the current node
 * basic_node provide type assistance for different operations on the node.
 * @ingroup node
 */
template <typename HeadT, typename TailT = void>
struct meta_node{
    /**
     * tail type
     */
    typedef meta_node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    /**
     * capsule type
     */
    typedef capsule<HeadT> capsule_type;
    /**
     * key type
     */
    typedef typename capsule_type::key_type key_type;
    /**
     * data type
     */
    typedef typename capsule_type::data_type data_type;
    /**
     * value type
     */
    typedef typename capsule_type::value_type value_type;
    /**
     * index type
     */
    typedef typename capsule_type::index_type index_type;
    
    /**
     * types
     */
    struct types{
        /**
         * type of the tail at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using tail_at = typename std::conditional<N == 0, tail_type, typename tail_type::types::template tail_at<N-1>>::type;
        /**
         * type of the capsule at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using capsule_at = typename std::conditional<N == 0, capsule_type, typename tail_type::types::template capsule_at<N-1>>::type;
        /**
         * type of the data inside the capsule at N'th level of depth 
         * @tparam N level of depth
         */
        template <int N>
        using data_at = typename capsule_at<N>::data_type;
        /**
         * value_type of the data inside the capsule at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using value_at = typename capsule_at<N>::value_type;
        /**
         * type of capsule used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using capsule_of = typename std::conditional<std::is_same<T, index_type>::value && N == 0, 
                capsule_type, 
                typename tail_type::types::template capsule_of<T, N - std::is_same<T, index_type>::value>
            >::type;
        /**
         * type of data used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using data_of = typename capsule_of<T, N>::data_type;
        /**
         * value_type of data used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using value_of = typename capsule_of<T, N>::value_type;
        /**
         * type of data used for the node identified by the key KeyT.
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        using data_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
                data_type,
                typename tail_type::types::template data_for<KeyT>
            >::type;
        /**
         * value_type of data used for the node identified by the key KeyT.
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        using value_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
                value_type,
                typename tail_type::types::template value_for<KeyT>
            >::type;
            
        /**
         * A chain of all index_types in the chain of nodes
         */
        using indices = meta_node<index_type, typename tail_type::types::indices>;
        /**
         * A chain of all key_type's in the chain of nodes
         */
        using keys = meta_node<key_type, typename tail_type::types::keys>;
            
        /**
         * Checks whether there exists a node encapsulating data with index_type T
         * @tparam T index_type searched for
         */
        template <typename T>
        struct exists{
            enum { 
                /**
                 * true if the chain of nodes contains a node with index_type T
                 */
                value = std::is_same<T, index_type>::value || tail_type::types::template exists<T>::value 
            };
        };
        
        /**
         * Checks whether there exists a node with key_type provided by the KeyT
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        struct has{
            enum { 
                /**
                 * true if the chain of nodes contains a node with key_type KeyT
                 */
                value = (!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value) || tail_type::types::template has<KeyT>::value 
            };
        };
    };
};

/**
 * @brief basic types for the terminal node
 * @tparam HeadT type of the data inside this node
 * basic_node provide type assistance for different operations on the node.
 * @ingroup node
 */
template <typename HeadT>
struct meta_node<HeadT, void>{
    /**
     * The terminal node has no tail, hence set to void.
     */
    typedef void tail_type;
    /**
     * type of capsule for the current node
     */
    typedef capsule<HeadT> capsule_type;
    /**
     * key type for the current node
     */
    typedef typename capsule_type::key_type key_type;
    /**
     * data type for the current node
     */
    typedef typename capsule_type::data_type data_type;
    /**
     * value_type for the current node
     */
    typedef typename capsule_type::value_type value_type;
    /**
     * index_type for the current node
     */
    typedef typename capsule_type::index_type index_type;
    
    /**
     * types
     */
    struct types{
        /**
         * type of the tail at N'th level of depth (void as thsi is the terminal node)
         * @tparam N level of depth
         */
        template <int N>
        using tail_at = void;
        /**
         * type of the capsule at N'th level of depth (void unless N is 0)
         * @tparam N level of depth
         */
        template <int N>
        using capsule_at = typename std::conditional<N == 0, capsule_type, void>::type;
        /**
         * type of the data inside the capsule at N'th level of depth 
         * @tparam N level of depth
         */
        template <int N>
        using data_at = typename capsule_at<N>::data_type;
        /**
         * value_type of the data inside the capsule at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using value_at = typename capsule_at<N>::value_type;
        /**
         * type of capsule used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using capsule_of = typename std::conditional<std::is_same<T, index_type>::value && N == 0, capsule_type, void>::type;
        /**
         * type of data used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using data_of = typename capsule_of<T, N>::data_type;
        /**
         * value_type of data used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using value_of = typename capsule_of<T, N>::value_type;
        /**
         * type of data used for the node identified by the key KeyT.
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        using data_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
            data_type,
            void
            >::type;
        /**
         * value_type of data used for the node identified by the key KeyT.
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        using value_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
            value_type,
            void
            >::type;
            
        /**
         * A chain of all index_types in the chain of nodes
         */
        using indices = meta_node<index_type, void>;
        /**
         * A chain of all key_type's in the chain of nodes
         */
        using keys = meta_node<key_type, void>;
            
        /**
         * Checks whether there exists a node encapsulating data with index_type T
         * @tparam T index_type searched for
         */
        template <typename T>
        struct exists{
            enum { 
                /**
                 * true if the chain of nodes contains a node with index_type T
                 */
                value = std::is_same<T, index_type>::value 
            };
        };
        /**
         * Checks whether there exists a node with key_type provided by the KeyT
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        struct has{
            enum { 
                /**
                 * true if the chain of nodes contains a node with key_type KeyT
                 */
                value = !std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value
            };
        };
    };
};

#else 

/**
 * @brief provides type assistance for hazo node
 * @tparam HeadT type of the data inside this node
 * @tparam TailT tail of the current node
 * @ingroup hazo
 */
template <typename HeadT, typename TailT = void>
struct meta_node{
    /**
     * @brief tail type
     */
    typedef meta_node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    /**
     * @brief capsule type
     */
    typedef capsule<HeadT> capsule_type;
    /**
     * @brief key type
     */
    typedef typename capsule_type::key_type key_type;
    /**
     * @brief data type
     */
    typedef typename capsule_type::data_type data_type;
    /**
     * @brief value type
     */
    typedef typename capsule_type::value_type value_type;
    /**
     * @brief index type
     */
    typedef typename capsule_type::index_type index_type;
    
    /**
     * @brief types
     */
    struct types{
        /**
         * @brief type of the tail at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using tail_at = typename std::conditional<N == 0, tail_type, typename tail_type::types::template tail_at<N-1>>;
        /**
         * @brief type of the capsule at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using capsule_at = typename std::conditional<N == 0, capsule_type, typename tail_type::types::template capsule_at<N-1>>::type;
        /**
         * @brief type of the data inside the capsule at N'th level of depth 
         * @tparam N level of depth
         */
        template <int N>
        using data_at = typename capsule_at<N>::data_type;
        /**
         * @brief value_type of the data inside the capsule at N'th level of depth
         * @tparam N level of depth
         */
        template <int N>
        using value_at = typename capsule_at<N>::value_type;
        /**
         * @brief type of capsule used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using capsule_of = typename std::conditional<std::is_same<T, index_type>::value && N == 0, 
                capsule_type, 
                typename tail_type::types::template capsule_of<T, N - std::is_same<T, index_type>::value>
            >::type;
        /**
         * @brief type of data used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using data_of = typename capsule_of<T, N>::data_type;
        /**
         * @brief value_type of data used for N'th instance of index T in the chain of nodes.
         * @tparam T index_type searched for
         * @tparam N defaults to 0
         */
        template <typename T, int N = 0>
        using value_of = typename capsule_of<T, N>::value_type;
        /**
         * @brief type of data used for the node identified by the key KeyT.
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        using data_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
                data_type,
                typename tail_type::types::template data_for<KeyT>
            >::type;
        /**
         * @brief value_type of data used for the node identified by the key KeyT.
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        using value_for = typename std::conditional<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value,
                value_type,
                typename tail_type::types::template value_for<KeyT>
            >::type;
            
        /**
         * @brief A chain of all index_types in the chain of nodes
         */
        using indices = meta_node<index_type, typename tail_type::types::indices>;
        /**
         * @brief A chain of all key_type's in the chain of nodes
         */
        using keys = meta_node<key_type, typename tail_type::types::keys>;
            
        /**
         * @brief Checks whether there exists a node encapsulating data with index_type T
         * @tparam T index_type searched for
         */
        template <typename T>
        struct exists{
            enum { 
                /**
                 * @brief true if the chain of nodes contains a node with index_type T
                 */
                value = std::is_same<T, index_type>::value || tail_type::types::template exists<T>::value 
            };
        };
        
        /**
         * @brief Checks whether there exists a node with key_type provided by the KeyT
         * @tparam KeyT key_type searched for
         */
        template <typename KeyT>
        struct has{
            enum { 
                /**
                 * @brief true if the chain of nodes contains a node with key_type KeyT
                 */
                value = (!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value) || tail_type::types::template has<KeyT>::value 
            };
        };
    };
};

#endif // __DOXYGEN__

}
}


#endif // UDHO_HAZO_NODE_META_H
