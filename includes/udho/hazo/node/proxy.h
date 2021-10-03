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
#include <udho/hazo/node/basic.h>

namespace udho{
namespace hazo{
    
namespace detail{
template <typename BeforeT, typename ExpectedT, typename NextT>
struct counter{
    enum {value = BeforeT::template count<NextT>::value + std::is_same_v<ExpectedT, NextT>};
};
  
template <typename PreviousT = void, typename NextT = void>
struct before{
    typedef NextT next_type;
    
    template <typename T>
    using count = counter<PreviousT, NextT, T>;
};

template <typename ExpectedT, typename NextT>
struct counter<before<>, ExpectedT, NextT>{
    enum {value = std::is_same_v<ExpectedT, NextT>};
};

template <>
struct before<void, void>{
    typedef void next_type;
    
    template <typename T>
    using count = counter<before<>, void, T>;
};

template <typename NextT>
struct before<before<>, NextT>{
    typedef NextT next_type;
    
    template <typename T>
    using count = counter<before<>, NextT, T>;
};

template <typename ValueT, int Index>
struct group{
    enum {index = Index};
    typedef ValueT type;
    typedef capsule<type> capsule_type;
    typedef typename capsule_type::key_type key_type;
    typedef typename capsule_type::data_type data_type;
    typedef typename capsule_type::value_type value_type;
    
    template <typename HeadT, typename TailT>
    group(node<HeadT, TailT>& n): _capsule(n.template capsule_at<type, Index-1>()){}
    
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
    typedef node_proxy<detail::before<BeforeT, H>, Rest...> tail_type;
    typedef detail::group<H, detail::before<BeforeT, H>::template count<H>::value> group_type;
    typedef typename group_type::key_type key_type;
    typedef typename group_type::data_type data_type;
    typedef typename group_type::value_type value_type;
    
    enum {depth = tail_type::depth +1};
    
    group_type _group;
    
    template <typename T>
    using count = typename node_proxy<detail::before<BeforeT, H>, Rest...>::template count<T>;
    
    template <typename HeadT, typename TailT>
    node_proxy(node<HeadT, TailT>& n): tail_type(n), _group(n){}
    
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
    node_proxy(node<HeadT, TailT>&){}
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
    
}
}

#endif // UDHO_HAZO_NODE_PROXY_H
