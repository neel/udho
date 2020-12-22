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

#ifndef UDHO_FOLDING_NODE_H
#define UDHO_FOLDING_NODE_H

#include <utility>
#include <type_traits>
#include <udho/folding/node/capsule.h>
#include <udho/folding/node/tag.h>

namespace udho{
namespace util{
namespace folding{

/**
 * node<A, node<B, node<C>, node<D, void>>>
 */
template <typename HeadT, typename TailT>
struct node: private node<typename TailT::data_type, typename TailT::tail_type>{
    typedef node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    typedef capsule<HeadT> capsule_type;
    typedef typename capsule_type::key_type key_type;
    typedef typename capsule_type::data_type data_type;
    typedef typename capsule_type::value_type value_type;
    typedef node<HeadT, TailT> self_type;
    
    enum { depth = tail_type::depth +1 };
    
    using tail_type::tail_type;
    template <typename ArgT, typename = typename std::enable_if<std::is_convertible<ArgT, value_type>::value || (!std::is_same<value_type, data_type>::value && std::is_convertible<ArgT, data_type>::value)>::type>
    node(const ArgT& h): _capsule(h) {}
    template <typename ArgT, typename... T, typename = typename std::enable_if<std::is_convertible<ArgT, value_type>::value || (!std::is_same<value_type, data_type>::value && std::is_convertible<ArgT, data_type>::value)>::type>
    node(const ArgT& h, const T&... ts):  tail_type(ts...), _capsule(h) {}
    template <typename OtherHeadT, typename OtherTailT, typename = typename std::enable_if<!std::is_same<node<HeadT, TailT>, node<OtherHeadT, OtherTailT>>::value>::type>
    node(const node<OtherHeadT, OtherTailT>& other): tail_type(other) { _capsule.set(other.template data<data_type>()); }
    
    capsule_type& front() { return _capsule; }
    const capsule_type& front() const { return _capsule; }
    
    tail_type& tail() { return static_cast<tail_type&>(*this); }
    const tail_type& tail() const { return static_cast<const tail_type&>(*this); }
    
    data_type& data() { return _capsule.data(); }
    const data_type& data() const { return _capsule.data(); }
    
    value_type& value() { return _capsule.value(); }
    const value_type& value() const { return _capsule.value(); }
    
    template <typename T>
    bool exists() const{ return std::is_same<data_type, T>::value || tail_type::template exists<T>(); }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::data_type, data_type>::value, bool>::type operator==(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::data_type, data_type>::value, bool>::type operator==(const LevelT& other) const {
        return _capsule == other._capsule && tail_type::operator==(other.tail());
    }
    template <typename LevelT>
    constexpr bool operator!=(const LevelT& other) const{ return !(*this == other); }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::data_type, data_type>::value, bool>::type less(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::data_type, data_type>::value, bool>::type less(const LevelT& other) const {
        return _capsule < other._capsule && tail_type::less(other.tail());
    }
    
    self_type& operator=(const self_type& other) { 
        _capsule = other._capsule; 
        tail_type::operator=(other.tail());
        return *this;
    }
    template <typename T>
    bool set(const T& v, bool all = false){
        bool success = false;
        if(std::is_same<data_type, T>::value){
            _capsule.set(v);
            success = true;
        }
        if(all) 
            return success || tail_type::template set<T>(v, all);
        else
            return success && tail_type::template set<T>(v, all);
    }
    
    template <int N, typename T>
    const typename std::enable_if<N == 0, bool>::type set(const T& v){
        _capsule.set(v);
        return true;
    }
    template <int N, typename T>
    const typename std::enable_if<N != 0, bool>::type set(const T& v){
        return tail_type::template set<N-1, T>(v);
    }
    
    // { data<T, N>() const
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    const data_type& data() const{ return data(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 && !std::is_same<T, data_type>::value>::type>
    const auto& data() const{ return tail_type::template data<T, N>(); }
    template <typename T, int N, typename = typename std::enable_if<N != 0>::type>
    decltype(auto) data() const{ return tail_type::template data<T, N-1>(); }
    // }
    
    // { data<T, N>()
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    data_type& data() { return data(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 && !std::is_same<T, data_type>::value>::type>
    auto& data() { return tail_type::template data<T, N>(); }
    template <typename T, int N, typename = typename std::enable_if<N != 0>::type>
    decltype(auto) data() { return tail_type::template data<T, N-1>(); }
    // }
    
    // { value<T, N>() const
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    const value_type& value() const{ return value(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 && !std::is_same<T, data_type>::value>::type>
    const auto& value() const{ return tail_type::template value<T, N>(); }
    template <typename T, int N, typename = typename std::enable_if<N != 0>::type>
    decltype(auto) value() const{ return tail_type::template value<T, N-1>(); }
    // }
    
    // { value<T, N>()
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    value_type& value() { return value(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 && !std::is_same<T, data_type>::value>::type>
    auto& value() { return tail_type::template value<T, N>(); }
    template <typename T, int N, typename = typename std::enable_if<N != 0>::type>
    decltype(auto) value() { return tail_type::template value<T, N-1>(); }
    // }
       
    template <int N, typename = typename std::enable_if<N == 0>::type>
    data_type& data() { return data(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& data() { return tail_type::template data<N-1>();  }
    
    template <int N, typename = typename std::enable_if<N == 0>::type>
    const data_type& data() const { return data(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    const auto& data() const { return tail_type::template data<N-1>();  }
    
    
    template <int N, typename = typename std::enable_if<N == 0>::type>
    const value_type& value() const { return value(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    const auto& value() const { return tail_type::template value<N-1>();  }
    
    template <int N, typename = typename std::enable_if<N == 0>::type>
    value_type& value() { return value(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& value() { return tail_type::template value<N-1>();  }
    
    
    template <int N, typename = typename std::enable_if<N == 0, tail_type>::type>
    tail_type& tail_at() { return tail(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& tail_at() { return tail_type::template tail_at<N>(); }
    
    template <int N, typename = typename std::enable_if<N == 0, tail_type>::type>
    const tail_type& tail_at() const { return tail(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& tail_at() const { return tail_type::template tail_at<N>(); }
        
        
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    data_type& data(const KeyT&){ return data(); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value>::type>
    auto& data(const KeyT& k){ return tail_type::template data<KeyT>(k); }
    
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    const data_type& data(const KeyT&) const { return data(); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value>::type>
    const auto& data(const KeyT& k) const { return tail_type::template data<KeyT>(k); }
    
    
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    value_type& value(const KeyT&){ return value(); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value>::type>
    auto& value(const KeyT& k){ return tail_type::template value<KeyT>(k); }
    
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    const value_type& value(const KeyT&) const { return value(); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value>::type>
    const auto& value(const KeyT& k) const { return tail_type::template value<KeyT>(k); }
    
    
    template <typename ElementT, typename = typename std::enable_if<std::is_same<ElementT, data_type>::value>::type>
    data_type& element(const element_t<ElementT>&) { return data(); }
    template <typename ElementT, typename = typename std::enable_if<!std::is_same<ElementT, data_type>::value>::type>
    decltype(auto) element(const element_t<ElementT>& e) { return tail().template element<ElementT>(e); }
    
    template <typename ElementT, typename = typename std::enable_if<std::is_same<ElementT, data_type>::value>::type>
    const data_type& element(const element_t<ElementT>&) const { return data(); }
    template <typename ElementT, typename = typename std::enable_if<!std::is_same<ElementT, data_type>::value>::type>
    decltype(auto) element(const element_t<ElementT>& e) const { return tail().template element<ElementT>(e); }
    
    
    template <typename ElementT>
    decltype(auto) operator[](const element_t<ElementT>& e){ return element<ElementT>(e); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    data_type& operator[](const KeyT& k){ return data<KeyT>(k); }
    template <typename K, typename = typename std::enable_if<!std::is_void<key_type>::value && !std::is_same<K, key_type>::value>::type>
    decltype(auto) operator[](const K& k){ return tail().template operator[]<K>(k); }
    
    template <typename ElementT>
    decltype(auto) operator[](const element_t<ElementT>& e) const { return element<ElementT>(e); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    data_type& operator[](const KeyT& k) const { return data<KeyT>(k); }
    template <typename K, typename = typename std::enable_if<!std::is_void<key_type>::value && !std::is_same<K, key_type>::value>::type>
    decltype(auto) operator[](const K& k) const { return tail().template operator[]<K>(k); }
        
    const tail_type& next(data_type& var) const { var = data(); return tail(); } 
    template <typename T, typename = typename std::enable_if<!std::is_same<data_type, value_type>::value && std::is_convertible<value_type, T>::value, T>::type>
    const tail_type& next(T& var) const { var = value(); return tail(); } 
        
    template <typename StreamT>
    StreamT& write(StreamT& stream) const{
        stream << _capsule << ", " ;
        tail_type::template write<StreamT>(stream);
        return stream;
    }
    template <typename FunctionT>
    void visit(FunctionT& f) const{
        _capsule.call(f);
        tail_type::visit(f);
    }
    template <typename FunctionT>
    void visit(FunctionT& f){
        _capsule.call(f);
        tail_type::visit(f);
    }
       
    template <typename FunctionT, typename InitialT>
    auto accumulate(FunctionT f, InitialT initial) const {
        return f(data(), tail_type::accumulate(f, initial));
    }
    template <typename FunctionT>
    auto accumulate(FunctionT f) const {
        return f(data(), tail_type::accumulate(f));
    }
    template <typename FunctionT, typename InitialT>
    auto decorate(FunctionT f, InitialT initial) const {
        return f.finish(accumulate(f, initial));
    }
    template <typename FunctionT>
    auto decorate(FunctionT f) const{
        return f.finish(accumulate(f));
    }
    
    capsule_type _capsule;
};

/**
 * folding_node<D, void>
 */
template <typename HeadT>
struct node<HeadT, void>{
    typedef void tail_type;
    typedef capsule<HeadT> capsule_type;
    typedef typename capsule_type::key_type key_type;
    typedef typename capsule_type::data_type data_type;
    typedef typename capsule_type::value_type value_type;
    typedef node<HeadT, void> self_type;
    
    enum { depth = 0 };
    
    node(): _capsule(){}
    template <typename ArgT, typename = typename std::enable_if<std::is_convertible<ArgT, value_type>::value || (!std::is_same<value_type, data_type>::value && std::is_convertible<ArgT, data_type>::value)>::type>
    node(const ArgT& h): _capsule(h) {}
    template <typename OtherHeadT, typename OtherTailT, typename = typename std::enable_if<!std::is_same<node<HeadT, void>, node<OtherHeadT, OtherTailT>>::value>::type>
    node(const node<OtherHeadT, OtherTailT>& other) { _capsule.set(other.template data<HeadT>()); }
    
    capsule_type& front() { return _capsule; }
    const capsule_type& front() const { return _capsule; }
    const data_type& data() const { return _capsule.data(); }
    data_type& data() { return _capsule.data(); }
    const value_type& value() const { return _capsule.value(); }
    value_type& value() { return _capsule.value(); }
    
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::data_type, data_type>::value, bool>::type operator==(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::data_type, data_type>::value, bool>::type operator==(const LevelT& other) const {
        return _capsule == other._capsule;
    }
    template <typename LevelT>
    constexpr bool operator!=(const LevelT& other) const{
        return !(*this == other);
    }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth != depth || !std::is_same<typename LevelT::data_type, data_type>::value, bool>::type less(const LevelT&) const { return false; }
    template <typename LevelT>
    constexpr typename std::enable_if<LevelT::depth == depth && std::is_same<typename LevelT::data_type, data_type>::value, bool>::type less(const LevelT& other) const {
        return _capsule < other._capsule;
    }
    
    template <typename T>
    bool exists() const{ return std::is_same<HeadT, T>::value; }
    
    self_type& operator=(const self_type& other) { 
        _capsule = other._capsule;
        return *this;
    }
    template <typename T>
    bool set(const T& v, bool){
        if(std::is_same<HeadT, T>::value){
            _capsule.template set<T>(v);
            return true;
        }
        return false;
    }
    
    template <int N, typename T>
    const typename std::enable_if<N == 0 && std::is_same<T, data_type>::value, bool>::type set(const T& v){
        _capsule.set(v);
        return true;
    }
    
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    const data_type& data() const{ return data(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    data_type& data() { return data(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    const value_type& value() const{ return value(); }
    template <typename T, int N = 0, typename = typename std::enable_if<N == 0 &&  std::is_same<T, data_type>::value>::type>
    value_type& value() { return value(); }
    
    template <int N, typename = typename std::enable_if<N == 0, data_type>::type>
    const data_type& data() const { return data(); }
    template <int N, typename = typename std::enable_if<N == 0, data_type>::type>
    data_type& data() { return data(); }
    
    template <int N, typename = typename std::enable_if<N == 0, value_type>::type>
    const value_type& value() const { return value(); }
    template <int N, typename = typename std::enable_if<N == 0, value_type>::type>
    value_type& value() { return value(); }

    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    const data_type& data(const KeyT&) const{ return data(); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    data_type& data(const KeyT&){ return data(); }
    
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    const value_type& value(const KeyT&) const{ return value(); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    value_type& value(const KeyT&){ return value(); }
    
    template <typename ElementT, typename = typename std::enable_if<std::is_same<ElementT, data_type>::value>::type>
    data_type& element(const element_t<ElementT>&) { return data(); }
    template <typename ElementT, typename = typename std::enable_if<std::is_same<ElementT, data_type>::value>::type>
    const data_type& element(const element_t<ElementT>&) const { return data(); }
    
    template <typename ElementT>
    data_type& operator[](const element_t<ElementT>& e){ return element<ElementT>(e); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    data_type& operator[](const KeyT& k){ return data<KeyT>(k); }
    
    template <typename ElementT>
    const data_type& operator[](const element_t<ElementT>& e) const { return element<ElementT>(e); }
    template <typename KeyT, typename = typename std::enable_if<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value>::type>
    const data_type& operator[](const KeyT& k) const { return data<KeyT>(k); }
    
    void next(data_type& var) const { var = data(); } 
    template <typename T, typename = typename std::enable_if<!std::is_same<data_type, value_type>::value && std::is_convertible<value_type, T>::value, T>::type>
    void next(T& var) const { var = value(); } 
    
    template <typename StreamT>
    StreamT& write(StreamT& stream) const{
        stream << _capsule ;
        return stream;
    }
    template <typename FunctionT>
    void visit(FunctionT& f) const{
        _capsule.call(f);
    }
    template <typename FunctionT>
    void visit(FunctionT& f){
        _capsule.call(f);
    }
    template <typename FunctionT>
    void operator()(FunctionT&& f){
        f(data());
    }
    template <typename FunctionT, typename InitialT>
    auto accumulate(FunctionT f, InitialT initial) const {
        return f(data(), initial);
    }
    template <typename FunctionT>
    auto accumulate(FunctionT f) const {
        return f(data());
    }
    template <typename FunctionT, typename InitialT>
    auto decorate(FunctionT f, InitialT initial) const {
        return f.finish(accumulate(f, initial));
    }
    template <typename FunctionT>
    auto decorate(FunctionT f) const{
        return f.finish(accumulate(f));
    }
    
    capsule_type _capsule;
};

template <typename HeadT, typename TailT>
decltype(auto) operator>>(const node<HeadT, TailT>& node, HeadT& var){
    return node.next(var);
}
template <typename... T, typename V, typename = typename std::enable_if<!std::is_same<typename node<T...>::data_type, typename node<T...>::value_type>::value && std::is_convertible<typename node<T...>::value_type, V>::value>::type>
decltype(auto) operator>>(const node<T...>& node, V& var){
    return node.next(var);
}
    
}
}
}

#endif // UDHO_FOLDING_NODE_H
