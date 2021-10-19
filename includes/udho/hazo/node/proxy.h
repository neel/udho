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

#ifndef UDHO_HAZO_NODE_PROXY_H
#define UDHO_HAZO_NODE_PROXY_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/fwd.h>
#include <udho/hazo/node/tag.h>
#include <udho/hazo/node/capsule.h>
#include <udho/hazo/node/meta.h>

namespace udho{
namespace hazo{
    
#ifndef __DOXYGEN__

namespace detail{

    /**
     * @brief counts the number of occurences of NextT
     * 
     * @tparam BeforeT 
     * @tparam ExpectedT 
     * @tparam NextT 
     */
    template <typename BeforeT, typename ExpectedT, typename NextT>
    struct counter{
        enum {value = BeforeT::template count<NextT>::value + std::is_same_v<ExpectedT, NextT>};
    };
    
    template <typename PreviousT = void, typename NextT = void>
    struct before{
        template <typename T>
        using count = counter<PreviousT, NextT, T>;
    };

    template <typename ExpectedT, typename NextT>
    struct counter<before<>, ExpectedT, NextT>{
        enum {value = std::is_same_v<ExpectedT, NextT>};
    };

    template <>
    struct before<void, void>{
        template <typename T>
        using count = counter<before<>, void, T>;
    };

    template <typename NextT>
    struct before<before<>, NextT>{
        template <typename T>
        using count = counter<before<>, NextT, T>;
    };

    template <typename T, int Index>
    struct group{
        enum {index = Index};
        typedef capsule<T> capsule_type;
        typedef typename capsule_type::key_type key_type;
        typedef typename capsule_type::data_type data_type;
        typedef typename capsule_type::value_type value_type;
        
        template <typename HeadT, typename TailT>
        group(basic_node<HeadT, TailT>& node): _capsule(node.template capsule_at<T, Index-1>()){}
        template <int OtherIndex>
        group(group<T, OtherIndex>& other): _capsule(other._capsule) {}
        
        capsule_type& _capsule;
        
        data_type& data() { return _capsule.data(); }
        const data_type& data() const { return _capsule.data(); }
        value_type& value() { return _capsule.value(); }
        const value_type& value() const { return _capsule.value(); }
        
        template <typename FunctionT>
        void call(FunctionT& f) const{
            _capsule.call(f);
        }
        
        template <typename StreamT>
        StreamT& write(StreamT& stream) const{
            stream << _capsule << ", " ;
            return stream;
        }
    };

}
/**
 * \code
 * node_proxy<
 *      before<>, 
 *      A, B, C, D, E
 *  >
 *  : node_proxy<
 *          before<before<>, A>, 
 *          B, C, D, E
 *      >
 *      : node_proxy<
 *              before<before<before<>, A>, B>, 
 *              C, D, E
 *          >
 *          : node_proxy<
 *                  before<before<before<before<>, A>, B>, C>, 
 *                  D, E
 *              >
 *              : node_proxy<
 *                      before<before<before<before<before<>, A>, B>, C>, D>, 
 *                      E
 *                  >
 *                  : node_proxy<
 *                          before<before<before<before<before<before<>, A>, B>, C>, D>, E>
 *                      >
 * \endcode
 * \ingroup node
 */
template <typename BeforeT, typename H = void, typename... Rest>
struct node_proxy: private node_proxy<detail::before<BeforeT, H>, Rest...> {
    typedef H head_type;
    typedef detail::before<BeforeT, H> before_type;
    typedef node_proxy<before_type, Rest...> tail_type;
    typedef detail::group<H, before_type::template count<H>::value> group_type;
    typedef typename group_type::key_type key_type;
    typedef typename group_type::data_type data_type;
    typedef typename group_type::value_type value_type;
    
    enum {depth = tail_type::depth +1};
    
    group_type _group;
    
    template <typename T>
    using count = typename node_proxy<before_type, Rest...>::template count<T>;

    template <typename HeadT, typename TailT>
    node_proxy(basic_node<HeadT, TailT>& n): tail_type(n), _group(n){}

    template <typename XBeforeT, typename... T>
    node_proxy(node_proxy<XBeforeT, T...>& p): tail_type(p), _group(p.template group_of<H, before_type::template count<H>::value>()) {}

    template <typename T>
    bool exists() const {
        return std::is_same_v<T, H> || tail_type::template exists<T>();
    }

    template <typename T, int Count, std::enable_if_t<std::is_same_v<T, H> && group_type::index == Count, bool> = true>
    group_type& group_of(){ return _group; }
    template <typename T, int Count, std::enable_if_t<!std::is_same_v<T, H> || group_type::index != Count, bool> = true>
    auto& group_of(){ return tail_type::template group_of<T, Count>(); }
    
    data_type& data() { return _group.data(); }
    const data_type& data() const { return _group.data(); }
    value_type& value() { return _group.value(); }
    const value_type& value() const { return _group.value(); }
    
    tail_type& tail() { return static_cast<tail_type&>(*this); }
    const tail_type& tail() const { return static_cast<const tail_type&>(*this); }
    
    // { data<T, N>() const
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same_v<T, data_type>, bool> = true>
    const data_type& data() const{ return data(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same_v<T, data_type>, bool> = true>
    const auto& data() const{ return tail_type::template data<T, N>(); }
    template <typename T, int N, std::enable_if_t<N != 0, bool> = true>
    decltype(auto) data() const{ return tail_type::template data<T, N-1>(); }
    // }
    
    // { data<T, N>()
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same_v<T, data_type>, bool> = true>
    data_type& data() { return data(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same_v<T, data_type>, bool> = true>
    auto& data() { return tail_type::template data<T, N>(); }
    template <typename T, int N, std::enable_if_t<N != 0, bool> = true>
    decltype(auto) data() { return tail_type::template data<T, N-1>(); }
    // }
    
    // { value<T, N>() const
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same_v<T, data_type>, bool> = true>
    const value_type& value() const{ return value(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same_v<T, data_type>, bool> = true>
    const auto& value() const{ return tail_type::template value<T, N>(); }
    template <typename T, int N, std::enable_if_t<N != 0, bool> = true>
    decltype(auto) value() const{ return tail_type::template value<T, N-1>(); }
    // }
    
    // { value<T, N>()
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same_v<T, data_type>, bool> = true>
    value_type& value() { return value(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same_v<T, data_type>, bool> = true>
    auto& value() { return tail_type::template value<T, N>(); }
    template <typename T, int N, std::enable_if_t<N != 0, bool> = true>
    decltype(auto) value() { return tail_type::template value<T, N-1>(); }
    // }
    
    // { data<N>()
    template <int N, std::enable_if_t<N == 0, bool> = true>
    data_type& data() { return data(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    auto& data() { return tail_type::template data<N-1>();  }
    // }
    
    // { data<N>() const
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const data_type& data() const { return data(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    const auto& data() const { return tail_type::template data<N-1>();  }
    // }
    
    // { value<N>()
    template <int N, std::enable_if_t<N == 0, bool> = true>
    value_type& value() { return value(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    auto& value() { return tail_type::template value<N-1>();  }
    // }
    
    // { value<N>() const
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const value_type& value() const { return value(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    const auto& value() const { return tail_type::template value<N-1>();  }
    // }
    // { data<KeyT>(const KeyT&)
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && std::is_same_v<KeyT, key_type>, bool> = true>
    data_type& data(const KeyT&){ return data(); }
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && !std::is_same_v<KeyT, key_type>, bool> = true>
    auto& data(const KeyT& k){ return tail_type::template data<KeyT>(k); }
    // }
    
    // { data<KeyT>(const KeyT&) const
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && std::is_same_v<KeyT, key_type>, bool> = true>
    const data_type& data(const KeyT&) const { return data(); }
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && !std::is_same_v<KeyT, key_type>, bool> = true>
    const auto& data(const KeyT& k) const { return tail_type::template data<KeyT>(k); }
    // }
    
    // { value<KeyT>(const KeyT&)
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && std::is_same_v<KeyT, key_type>, bool> = true>
    value_type& value(const KeyT&){ return value(); }
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && !std::is_same_v<KeyT, key_type>, bool> = true>
    auto& value(const KeyT& k){ return tail_type::template value<KeyT>(k); }
    // }
    
    // { value<KeyT>(const KeyT&) const
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && std::is_same_v<KeyT, key_type>, bool> = true>
    const value_type& value(const KeyT&) const { return value(); }
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && !std::is_same_v<KeyT, key_type>, bool> = true>
    const auto& value(const KeyT& k) const { return tail_type::template value<KeyT>(k); }
    // }
    
    // { element<ElementT>(const element_t<ElementT>&)
    template <typename ElementT, std::enable_if_t<std::is_same_v<ElementT, data_type>, bool> = true>
    data_type& element(const element_t<ElementT>&) { return data(); }
    template <typename ElementT, std::enable_if_t<!std::is_same_v<ElementT, data_type>, bool> = true>
    decltype(auto) element(const element_t<ElementT>& e) { return tail().template element<ElementT>(e); }
    // }
    
    // { element<ElementT>(const element_t<ElementT>&) const
    template <typename ElementT, std::enable_if_t<std::is_same_v<ElementT, data_type>, bool> = true>
    const data_type& element(const element_t<ElementT>&) const { return data(); }
    template <typename ElementT, std::enable_if_t<!std::is_same_v<ElementT, data_type>, bool> = true>
    decltype(auto) element(const element_t<ElementT>& e) const { return tail().template element<ElementT>(e); }
    // }
    
    template <typename ElementT>
    decltype(auto) operator[](const element_t<ElementT>& e){ return element<ElementT>(e); }
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && std::is_same_v<KeyT, key_type>, bool> = true>
    data_type& operator[](const KeyT& k){ return data<KeyT>(k); }
    template <typename K, std::enable_if_t<!std::is_void_v<key_type> && !std::is_same_v<K, key_type>, bool> = true>
    decltype(auto) operator[](const K& k){ return tail().template operator[]<K>(k); }
    
    template <typename ElementT>
    decltype(auto) operator[](const element_t<ElementT>& e) const { return element<ElementT>(e); }
    template <typename KeyT, std::enable_if_t<!std::is_void_v<key_type> && std::is_same_v<KeyT, key_type>, bool> = true>
    data_type& operator[](const KeyT& k) const { return data<KeyT>(k); }
    template <typename K, std::enable_if_t<!std::is_void_v<key_type> && !std::is_same_v<K, key_type>, bool> = true>
    decltype(auto) operator[](const K& k) const { return tail().template operator[]<K>(k); }
    
    const tail_type& next(data_type& var) const { var = data(); return tail(); } 
    template <typename T, std::enable_if_t<!std::is_same_v<data_type, value_type> && std::is_convertible_v<value_type, T>, bool> = true>
    const tail_type& next(T& var) const { var = value(); return tail(); } 
    
    template <typename FunctionT>
    void visit(FunctionT&& f) const{
        _group.call(std::forward<FunctionT>(f));
        tail_type::visit(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    void visit(FunctionT&& f){
        _group.call(std::forward<FunctionT>(f));
        tail_type::visit(std::forward<FunctionT>(f));
    }
       
    template <typename FunctionT, typename InitialT>
    auto accumulate(FunctionT&& f, InitialT&& initial) const {
        return std::forward<FunctionT>(f)(data(), tail_type::accumulate(std::forward<FunctionT>(f), std::forward<InitialT>(initial)));
    }
    template <typename FunctionT>
    auto accumulate(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(data(), tail_type::accumulate(std::forward<FunctionT>(f)));
    }
    template <typename FunctionT, typename InitialT>
    auto decorate(FunctionT&& f, InitialT&& initial) const {
        return std::forward<FunctionT>(f).finish(accumulate(std::forward<FunctionT>(f), std::forward<InitialT>(initial)));
    }
    template <typename FunctionT>
    auto decorate(FunctionT&& f) const{
        return std::forward<FunctionT>(f).finish(accumulate(std::forward<FunctionT>(f)));
    }
};

template <typename BeforeT>
struct node_proxy<BeforeT, void>{
    typedef BeforeT before_type;
    enum {depth = 0};
    template <typename T>
    using count = typename BeforeT::template count<T>;
    
    template <typename HeadT, typename TailT>
    node_proxy(basic_node<HeadT, TailT>&){}

    template <typename XBeforeT, typename... T>
    node_proxy(node_proxy<XBeforeT, T...>& p) {}

    template <typename T>
    bool exists() const {
        return false;
    }
};

template <typename... T>
decltype(auto) operator>>(const node_proxy<T...>& proxy, typename node_proxy<T...>::head_type& var){
    return proxy.next(var);
}

template <typename... T, typename V, std::enable_if_t<!std::is_same_v<typename node_proxy<T...>::data_type, typename node_proxy<T...>::value_type> && std::is_convertible_v<typename node_proxy<T...>::value_type, V>, bool> = true>
decltype(auto) operator>>(const node_proxy<T...>& proxy, V& var){
    return proxy.next(var);
}

template <typename... T>
using proxy = node_proxy<detail::before<>, T...>;
    
#else

/**
 * @brief transparent proxy of a hazo node
 * 
 * @tparam T...
 * @ingroup hazo 
 */
template <typename... T>
struct proxy{
    /**
     * @brief type assistance through meta_node
     */
    typedef typename node<T...>::types types;
    /**
     * @brief Construct a new proxy object from a reference of the actual node object
     * 
     * @param original_node 
     */
    proxy(node<T...>& original_node);
    /**
     * @brief get data of a basic_node
     * @return data_type&
     */
    data_type& data();
    /**
     * @brief get data of a basic_node
     * @return const data_type& 
     */
    const data_type& data() const;
    /**
     * @brief Returns the N'th item that is identified by index_type T in the basic_node chain (if an item X in basic_node does not provide an index_type then X is considered as its index_type)
     * @code
     * hazo::node<int, double, int, std::string> some_node(...);
     * hazo::proxy<int, double, int, std::string> h(some_node);
     * h.data<int>(); // First int (first item in the node chain)
     * h.data<int, 1>(); // Second int (third item in the node chain)
     * @endcode 
     * @tparam T Data type
     * @tparam N Relative position
     * @return const T& 
     */
    template <typename T, int N = 0>
    const T& data() const;
    /**
     * @brief Returns the N'th item that is identified by index_type T in the basic_node chain (if an item X in basic_node does not provide an index_type then X is considered as its index_type)
     * @code
     * hazo::node<int, double, int, std::string> some_node(...);
     * hazo::proxy<int, double, int, std::string> h(some_node);
     * h.data<int>(); // First int (first item in the node chain)
     * h.data<int, 1>(); // Second int (third item in the node chain)
     * @endcode 
     * @tparam T Data type
     * @tparam N Relative position
     * @return typename T&
     */
    template <typename T, int N = 0>
    T& data() const;
    /**
     * @brief Returns the N'th item in the basic_node chain
     * @code
     * hazo::node<int, double, int, std::string> h(1, 3.14, 2, "Hello");
     * h.data<0>(); // 1
     * h.data<1>(); // 3.14
     * h.data<3>(); // Hello
     * @endcode 
     * @tparam N 
     * @return types::template data_at<N>&
     */
    template <int N>
    typename types::template data_at<N>& data();
    /**
     * @brief Returns the N'th item in the basic_node chain
     * @code
     * hazo::node<int, double, int, std::string> h(1, 3.14, 2, "Hello");
     * h.data<0>(); // 1
     * h.data<1>(); // 3.14
     * h.data<3>(); // Hello
     * @endcode 
     * @tparam N 
     * @return const typename types::template data_at<N>&
     */
    template <int N>
    const typename types::template data_at<N>& data() const;
    /**
     * @brief Given a key get the get the data that is associated with that key.
     * Assuming the key() methods of the items in the basic_node chain return values of compile time distingutible unique types
     * 
     * @tparam KeyT 
     * @param k 
     * @return types::template data_for<KeyT>& 
     */
    template <typename KeyT>
    typename types::template data_for<KeyT>& data(const KeyT& k);
    /**
     * @brief Given a key get the get the data that is associated with that key.
     * Assuming the key() methods of the items in the basic_node chain return values of compile time distingutible unique types
     * 
     * @tparam KeyT 
     * @param k 
     * @return types::template data_for<KeyT>& 
     */
    template <typename KeyT>
    const typename types::template data_for<KeyT>& data(const KeyT& k) const;

    /**
     * @brief get value of a basic_node
     * @return value_type&
     */
    value_type& value();
    /**
     * @brief Get value of a basic_node
     * @return const value_type& 
     */
    const value_type& value() const;
    /**
     * @brief Returns the value of N'th item that is identified by index_type T in the basic_node chain (if an item X in basic_node does not provide an index_type then X is considered as its index_type)
     * @code
     * hazo::node<int, double, int, std::string> h;
     * h.value<int>(); // First int (first item in the basic_node chain)
     * h.value<int, 1>(); // Second int (third item in the basic_node chain)
     * @endcode 
     * @tparam T Index type (if an item X in basic_node does not provide an index_type then X is considered as its index_type)
     * @tparam N Relative position
     * @return const T& 
     */
    template <typename T, int N = 0>
    const T& value() const;
    /**
     * @brief Returns the value of N'th item that is identified by index_type T in the basic_node chain (if an item X in basic_node does not provide an index_type then X is considered as its index_type)
     * @code
     * hazo::node<int, double, int, std::string> h;
     * h.value<int>(); // First int (first item in the basic_node chain)
     * h.value<int, 1>(); // Second int (third item in the basic_node chain)
     * @endcode 
     * @tparam T Index type (if an item X in basic_node does not provide an index_type then X is considered as its index_type)
     * @tparam N Relative position
     * @return const T& 
     */
    template <typename T, int N = 0>
    T& value() const;
    /**
     * @brief Returns value of the N'th item in the basic_node chain
     * @code
     * hazo::node<int, double, int, std::string> h(1, 3.14, 2, "Hello");
     * h.value<0>(); // 1
     * h.value<1>(); // 3.14
     * h.value<3>(); // Hello
     * @endcode 
     * @tparam N 
     * @return types::template value_at<N>&
     */
    template <int N>
    typename types::template value_at<N>& value();
    /**
     * @brief Returns value of the N'th item in the basic_node chain
     * @code
     * hazo::node<int, double, int, std::string> h(1, 3.14, 2, "Hello");
     * h.value<0>(); // 1
     * h.value<1>(); // 3.14
     * h.value<3>(); // Hello
     * @endcode 
     * @tparam N 
     * @return types::template value_at<N>&
     */
    template <int N>
    const typename types::template value_at<N>& value() const;
    /**
     * @brief Given a key get the get the value that is associated with that key.
     * Assuming the key() methods of the items in the basic_node chain return values of compile time distingutible unique types
     * 
     * @tparam KeyT 
     * @param k 
     * @return types::template value_for<KeyT>& 
     */
    template <typename KeyT>
    typename types::template value_for<KeyT>& value(const KeyT& k);
    /**
     * @brief Given a key get the get the value that is associated with that key.
     * Assuming the key() methods of the items in the basic_node chain return values of compile time distingutible unique types
     * 
     * @tparam KeyT 
     * @param k 
     * @return types::template value_for<KeyT>& 
     */
    template <typename KeyT>
    const typename types::template value_for<KeyT>& value(const KeyT& k) const;

    /**
     * @brief Get the data of the basic_node identified by element handle
     * 
     * @tparam ElementT 
     * @return data_type& 
     */
    template <typename ElementT>
    typename types::template data_of<ElementT>& element(const element_t<ElementT>&);
    /**
     * @brief Get the data of the basic_node identified by element handle
     * 
     * @tparam ElementT 
     * @return data_type& 
     */
    template <typename ElementT>
    const typename types::template data_of<ElementT>& element(const element_t<ElementT>&);

    /**
     * @brief finds data of an basic_node by element handle
     */
    template <typename ElementT>
    typename types::template data_of<ElementT>& operator[](const element_t<ElementT>& e);
    /**
     * @brief finds data of an basic_node by element handle
     */
    template <typename ElementT>
    const typename types::template data_of<ElementT>& operator[](const element_t<ElementT>& e) const;
};

#endif // __DOXYGEN__

}
}

#endif // UDHO_HAZO_NODE_PROXY_H
