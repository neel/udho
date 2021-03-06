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

#ifndef UDHO_UTIL_FOLDING_H
#define UDHO_UTIL_FOLDING_H

#ifndef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#define BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#endif

#include <utility>
#include <iostream>
#include <type_traits>
#include <boost/hana.hpp>

#include <cassert>
#include <sstream>

namespace udho{
namespace util{
namespace folding{


/**
 * capsule holds data of type data_type inside 
 * some special capsules have value_type which may refer to something  
 * other than data_type depending on what the capsule is encapsulating
 * otherwise data_type and value_type are same
 */
template <typename ValueT, bool IsClass = std::is_class<ValueT>::value>
struct capsule;
    
/**
 * capsule for plain old types 
 * 
 */
template <typename DataT>
struct capsule<DataT, false>{
    typedef void key_type;
    typedef DataT value_type;
    typedef DataT data_type;
    typedef capsule<DataT, false> self_type;
    
    data_type _data;
    
    capsule(): _data(data_type()){};
    capsule(const self_type&) = default;
    capsule(const data_type& h): _data(h){}
    self_type& operator=(const self_type& other) {
        _data = other._data;
        return *this;
    }
    const data_type& data() const { return _data; }
    data_type& data() { return _data; }
    const value_type& value() const { return data(); }
    value_type& value() { return data(); }
    bool operator==(const self_type& other) const { return _data == other._data; }
    bool operator!=(const self_type& other) const { return !operator==(other); }
    bool operator==(const data_type& other) const { return _data == other; }
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    void set(const data_type& value) { _data = value; }
    operator data_type() const { return _data; }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_data);
    }
};

/**
 * capsule for string
 * 
 */
template <int N>
struct capsule<char[N], false>{
    typedef void key_type;
    typedef std::string value_type;
    typedef std::string data_type;
    typedef capsule<char[N], false> self_type;
    
    data_type _data;
    
    capsule(): _data(data_type()){};
    capsule(const self_type&) = default;
    capsule(const char* h): _data(h){}
    capsule(const std::string& h): _data(h){}
    self_type& operator=(const self_type& other) {
        _data = other._data;
        return *this;
    }
    const data_type& data() const { return _data; }
    data_type& data() { return _data; }
    const value_type& value() const { return data(); }
    value_type& value() { return data(); }
    template <typename ValueT, typename = typename std::enable_if<std::is_convertible<ValueT, data_type>::value>>
    bool operator==(const capsule<ValueT>& other) const { return _data == other._data; }
    template <typename ValueT, typename = typename std::enable_if<std::is_convertible<ValueT, data_type>::value>>
    bool operator!=(const capsule<ValueT>& other) const { return !operator==(other); }
    bool operator==(const data_type& other) const { return _data == other; }
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    void set(const data_type& value) { _data = value; }
    operator data_type() const { return _data; }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_data);
    }
};

/**
 * capsule for string
 */
template <typename CharT, typename Traits, typename Alloc>
struct capsule<std::basic_string<CharT, Traits, Alloc>, true>{
    typedef void key_type;
    typedef std::basic_string<CharT, Traits, Alloc> value_type;
    typedef std::basic_string<CharT, Traits, Alloc> data_type;
    typedef capsule<std::basic_string<CharT, Traits, Alloc>, true> self_type;
    
    data_type _data;
    
    capsule(): _data(data_type()){};
    capsule(const self_type&) = default;
    capsule(const std::basic_string<CharT, Traits, Alloc>& h): _data(h){}
    self_type& operator=(const self_type& other) {
        _data = other._data;
        return *this;
    }
    const data_type& data() const { return _data; }
    data_type& data() { return _data; }
    const value_type& value() const { return data(); }
    value_type& value() { return data(); }
    template <typename ValueT, typename = typename std::enable_if<std::is_convertible<ValueT, data_type>::value>>
    bool operator==(const capsule<ValueT>& other) const { return _data == other._data; }
    template <typename ValueT, typename = typename std::enable_if<std::is_convertible<ValueT, data_type>::value>>
    bool operator!=(const capsule<ValueT>& other) const { return !operator==(other); }
    bool operator==(const data_type& other) const { return _data == other; }
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    void set(const data_type& value) { _data = value; }
    operator data_type() const { return _data; }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_data);
    }
};

template <typename CharT, typename Traits, typename Alloc>
std::ostream& operator<<(std::ostream& stream, const capsule<std::basic_string<CharT, Traits, Alloc>, true>& c){
    stream << c.value();
    return stream;
}

// http://loungecpp.wikidot.com/tips-and-tricks%3aindices
template <std::size_t... Is>
struct indices {};

template <std::size_t N, std::size_t... Is>
struct build_indices
    : build_indices<N-1, N-1, Is...> {};

template <std::size_t... Is>
struct build_indices<0, Is...> : indices<Is...> {
    typedef indices<Is...> indices_type;
};

template <typename DerivedT>
struct element_t{
    typedef DerivedT element_type;
};

/**
 * node<A, node<B, node<C>, node<D, void>>>
 */
template <typename HeadT, typename TailT = void>
struct node: node<typename TailT::data_type, typename TailT::tail_type>{
    typedef node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    typedef capsule<HeadT> capsule_type;
    typedef typename capsule_type::key_type key_type;
    typedef typename capsule_type::data_type data_type;
    typedef typename capsule_type::value_type value_type;
    typedef node<HeadT, TailT> self_type;
    
    enum { depth = tail_type::depth +1 };
    
    template <typename ArgT>
    node(const ArgT& h): _capsule(h) {}
    template <typename ArgT, typename... T>
    node(const ArgT& h, const T&... ts):  tail_type(ts...), _capsule(h) {}
    using tail_type::tail_type;
    
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
    
    template <typename T>
    const typename std::enable_if<std::is_same<T, data_type>::value, T>::type& data() const{ return data(); }
    template <typename T>
    const typename std::enable_if<!std::is_same<T, data_type>::value, T>::type& data() const{ return tail_type::template data<T>(); }
    
    
    template <int N, typename = typename std::enable_if<N == 0>::type>
    data_type& data() { return data(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    auto& data() { return tail_type::template data<N-1>();  }
    
    template <int N, typename = typename std::enable_if<N == 0>::type>
    const data_type& data() const { return data(); }
    template <int N, typename = typename std::enable_if<N != 0>::type>
    const auto& data() const { return tail_type::template data<N-1>();  }
    
    
    template <typename T, typename = typename std::enable_if<std::is_same<T, data_type>::value, value_type>::type>
    const value_type& value() const{ return value(); }
    template <typename T, typename = typename std::enable_if<!std::is_same<T, data_type>::value, value_type>::type >
    const auto& value() const{ return tail_type::template value<T>(); }
    
    template <typename T, typename = typename std::enable_if<std::is_same<T, data_type>::value, value_type>::type>
    value_type& value() { return value(); }
    template <typename T, typename = typename std::enable_if<!std::is_same<T, data_type>::value, value_type>::type >
    auto& value() { return tail_type::template value<T>(); }
    
    
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
    template <typename ArgT>
    node(const ArgT& h): _capsule(h) {}
    
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
    
    template <typename T>
    const typename std::enable_if<std::is_same<T, data_type>::value, T>::type& data() const{ return data(); }
    template <typename T>
    typename std::enable_if<std::is_same<T, data_type>::value, T>::type& data() { return data(); }
    
    template <int N, typename = typename std::enable_if<N == 0, data_type>::type>
    const data_type& data() const { return data(); }
    template <int N, typename = typename std::enable_if<N == 0, data_type>::type>
    data_type& data() { return data(); }
    
    template <typename T>
    const typename std::enable_if<std::is_same<T, data_type>::value, value_type>::type& value() const{ return value(); }
    template <typename T>
    typename std::enable_if<std::is_same<T, value_type>::value, T>::type& value(){ return value(); }
    
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

struct by_data{};
struct by_value{};

template <typename Policy, typename LevelT, std::size_t N>
struct extraction_helper;

template <typename LevelT, std::size_t Index>
struct extraction_helper<by_data, LevelT, Index>{
    static auto& apply(LevelT& node){
        return node.template data<Index>();
    }
};

template <typename LevelT, std::size_t Index>
struct extraction_helper<by_value, LevelT, Index>{
    static auto& apply(LevelT& node){
        return node.template value<Index>();
    }
};

template <typename Policy, typename LevelT, std::size_t N>
struct const_extraction_helper;

template <typename LevelT, std::size_t Index>
struct const_extraction_helper<by_data, LevelT, Index>{
    static const auto& apply(const LevelT& node){
        return node.template data<Index>();
    }
};

template <typename LevelT, std::size_t Index>
struct const_extraction_helper<by_value, LevelT, Index>{
    static const auto& apply(const LevelT& node){
        return node.template value<Index>();
    }
};

template <typename Policy, typename LevelT, std::size_t N>
struct at_helper{
    template <typename NodeT>
    decltype(auto) operator()(NodeT& node){
        return extraction_helper<Policy, NodeT, N>::apply(node);
    }
    template <typename NodeT>
    decltype(auto) operator()(const NodeT& node) const{
        return const_extraction_helper<Policy, NodeT, N>::apply(node);
    }
};

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

template <typename Policy, int N>
struct udho_folding_seq_tag{};

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
template <typename Policy, typename H, typename T = void, typename... X>
struct seq: node<H, seq<Policy, T, X...>>{
    typedef node<H, seq<Policy, T, X...>> node_type;
    
    using hana_tag = udho_folding_seq_tag<Policy, 2+sizeof...(X)>;
    
    using node_type::node_type;
//     seq(const H& h, const T& t, const X&... xs): node<H, seq<Policy, T, X...>>(h, t, xs...){}
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        const_call_helper<Policy, node_type, typename build_indices<2+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        call_helper<Policy, node_type, typename build_indices<2+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
};

template <typename Policy, typename H>
struct seq<Policy, H, void>: node<H, void>{
    typedef node<H, void> node_type;
    
    using hana_tag = udho_folding_seq_tag<Policy, 1>;
    
    using node_type::node_type;
//     seq(const H& h): node<H, void>(h){}
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
std::ostream& operator<<(std::ostream& stream, const seq<Policy, X...>& s){
    stream << "(";
    s.write(stream);
    stream << ")";
    return stream;
}

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

namespace boost {
namespace hana {
    template <typename Policy, int N>
    struct at_impl<udho::util::folding::udho_folding_seq_tag<Policy, N>> {
        template <typename SeqT, typename I>
        static constexpr decltype(auto) apply(SeqT&& xs, I const&) {
            return udho::util::folding::extraction_helper<Policy, SeqT, I::value>::apply(std::forward<SeqT>(xs));
        }
    };
    
    template <typename Policy, int N>
    struct drop_front_impl<udho::util::folding::udho_folding_seq_tag<Policy, N>> {
        template <typename Xs, typename I>
        static constexpr decltype(auto) apply(Xs&& xs, I const&) {
            return xs.template tail_at<I::value>();
        }
    };

    template <typename Policy, int N>
    struct is_empty_impl<udho::util::folding::udho_folding_seq_tag<Policy, N>> {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return xs.depth == 1;
        }
    };
    
    template <typename Policy, int N>
    struct unpack_impl<udho::util::folding::udho_folding_seq_tag<Policy, N>> {
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return std::forward<Xs>(xs).unpack(std::forward<F>(f));
        }
    };
    
    template <typename Policy, int N>
    struct make_impl<udho::util::folding::udho_folding_seq_tag<Policy, N>> {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) {
            return udho::util::folding::seq<Policy, Args...>(std::forward<Args>(args)...);
        }
    };
    
    template <typename Policy, int N>
    struct length_impl<udho::util::folding::udho_folding_seq_tag<Policy, N>> {
        template <typename ...Xn>
        static constexpr auto apply(udho::util::folding::seq<Xn...> const&) {
            return hana::size_t<sizeof...(Xn)>{};
        }
    };
        
    template <typename Policy, int N>
    struct Sequence<udho::util::folding::udho_folding_seq_tag<Policy, N>> : std::true_type { };
}
}

namespace udho{
namespace util{
namespace folding{
    
template <typename DerivedT, typename ValueT, template<class, typename> typename... Mixins>
struct element: Mixins<DerivedT, ValueT>...{
    typedef DerivedT derived_type;
    typedef ValueT value_type;
    typedef element<DerivedT, ValueT, Mixins...> self_type;
    typedef self_type element_type;
    
    value_type _value;
    
    const static constexpr element_t<derived_type> val = element_t<derived_type>();
    
    element(const value_type& v): Mixins<DerivedT, ValueT>(*this)..., _value(v){}
    element(): _value(value_type()), Mixins<DerivedT, ValueT>(*this)...{}
    static constexpr auto key() { return DerivedT::key(); }
    std::string name() const { return std::string(key().c_str()); }
    self_type& operator=(const value_type& v) { 
        _value = v; 
        return *this; 
    }
    self_type& operator=(const self_type& other){
        _value = other._value;
        return *this;
    }
    value_type& value() { return _value; }
    const value_type& value() const { return _value; }
    bool operator==(const self_type& other) const { return _value == other._value; }
    bool operator!=(const self_type& other) const { return !operator==(other); }
    template<typename V>
    typename std::enable_if<std::is_convertible<V, value_type>::value, void>::type set(const std::string& n, const V& v){
        if(n == name()){
            _value = v;
        }
    }
    template<typename V>
    typename std::enable_if<!std::is_convertible<V, value_type>::value, void>::type set(const std::string&, const V&){}
};

template <typename DerivedT, typename ValueT, template<class, typename> typename... Mixins>
const element_t<DerivedT> element<DerivedT, ValueT, Mixins...>::val;

template < class T >
class HasMemberType_element_type{
    private:
        using Yes = char[2];
        using  No = char[1];

        struct Fallback { struct element_type { }; };
        struct Derived : T, Fallback { };

        template < class U >
        static No& test ( typename U::element_type* );
        template < typename U >
        static Yes& test ( U* );

    public:
        static constexpr bool RESULT = sizeof(test<Derived>(nullptr)) == sizeof(Yes);
};

template < class T >
struct has_member_type_element_type: public std::integral_constant<bool, HasMemberType_element_type<T>::RESULT>{ };

template <typename ValueT, bool IsElement = false>
struct encapsulation;

template <typename ValueT>
struct capsule<ValueT, true>: encapsulation<ValueT, has_member_type_element_type<ValueT>::value>{
    typedef encapsulation<ValueT, has_member_type_element_type<ValueT>::value> encapsulation_type;
    
    using encapsulation_type::encapsulation_type;
    using encapsulation_type::operator==;
    using encapsulation_type::operator!=;
    using encapsulation_type::operator=;
};

/**
 * encapsulate a class which is not an element
 */
template <typename DataT>
struct encapsulation<DataT, false>{
    typedef DataT data_type;
    typedef void key_type;
    typedef DataT value_type;
    typedef encapsulation<DataT, false> self_type;
    
    data_type _data;
    
    encapsulation(): _data(value_type()){};
    encapsulation(const self_type&) = default;
    encapsulation(const data_type& h): _data(h){}
    self_type& operator=(const self_type& other) { 
        _data = other._data; 
        return *this; 
    }
    const data_type& data() const { return _data; }
    data_type& data() { return _data; }
    const value_type& value() const { return data().value(); }
    value_type& value() { return data().value(); }
    bool operator==(const self_type& other) const { return _data == other._head; }
    bool operator!=(const self_type& other) const { return !operator==(other); }
    bool operator==(const data_type& other) const { return _data == other; }
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    void set(const data_type& value) { _data = value; }
    operator data_type() const { return _data; }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_data);
    }
};

/**
 * encapsulate a class which is an element
 */
template <typename DataT>
struct encapsulation<DataT, true>{
    typedef DataT data_type;
    typedef decltype(data_type::key()) key_type;
    typedef typename DataT::value_type value_type;
    typedef encapsulation<DataT, true> self_type;
    
    data_type _data;
    
    encapsulation(): _data(value_type()){};
    encapsulation(const self_type&) = default;
    encapsulation(const data_type& h): _data(h){}
    encapsulation(const value_type& h): _data(h){}
    self_type& operator=(const self_type& other) { 
        _data = other._data; 
        return *this; 
    }
    const data_type& data() const { return _data; }
    data_type& data() { return _data; }
    const value_type& value() const { return data().value(); }
    value_type& value() { return data().value(); }
    bool operator==(const self_type& other) const { return _data == other._head; }
    bool operator!=(const self_type& other) const { return !operator==(other); }
    bool operator==(const data_type& other) const { return _data == other; }
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    void set(const data_type& value) { _data = value; }
    void set(const value_type& value) { _data = value; }
    operator data_type() const { return _data; }
    operator value_type() const { return _data.value(); }
    static constexpr key_type key() { return data_type::key(); }
    template <typename FunctionT>
    auto call(FunctionT f) const {
        return f(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT f) {
        return f(_data);
    }
};

template <typename ValueT>
std::ostream& operator<<(std::ostream& stream, const capsule<ValueT, true>& c){
    stream << c.key().c_str() << " -> " << c.value();
    return stream;
}
template <typename ValueT>
std::ostream& operator<<(std::ostream& stream, const capsule<ValueT, false>& c){
    stream << c.value();
    return stream;
}

template <typename DerivedT, typename ValueT, template<class, typename> typename... Mixins>
std::ostream& operator<<(std::ostream& stream, const element<DerivedT, ValueT, Mixins...>& elem){
    stream << "< " << elem.key().c_str() << ": " << elem.value() << ">";
    return stream;
}

template <typename Policy, typename... T>
struct udho_folding_map_tag{};

template <typename Policy, typename LevelT, typename Indecies>
struct access_helper;

template <typename Policy, typename HeadT, typename TailT, std::size_t... Is>
struct access_helper<Policy, node<HeadT, TailT>, indices<Is...>>{
    typedef node<HeadT, TailT> node_type;
    
    constexpr auto apply(){
        node<HeadT, TailT> xs;
        return boost::hana::make_tuple(
            boost::hana::make_pair(xs.template data<Is>().key(), at_helper<Policy, node_type, Is>{})...
        );
    }
};

template <typename Policy>
struct map_by_key_helper;

template <>
struct map_by_key_helper<by_data>{
    template <typename DataT>
    static DataT& apply(DataT& d){
        return d;
    }
    template <typename DataT>
    static const DataT& apply(const DataT& d){
        return d;
    }
};
template <>
struct map_by_key_helper<by_value>{
    template <typename DataT>
    static typename DataT::value_type& apply(DataT& d){
        return d.value();
    }
    template <typename DataT>
    static const typename DataT::value_type& apply(const DataT& d){
        return d.value();
    }
};

template <typename Policy, typename H, typename T = void, typename... X>
struct map: node<H, map<Policy, T, X...>>{
    typedef node<H, map<Policy, T, X...>> node_type;
    
    using hana_tag = udho_folding_map_tag<Policy, H, T, X...>;
    
    using node_type::node_type;
    map(const H& h, const T& t, const X&... xs): node<H, map<Policy, T, X...>>(h, t, xs...){}
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        call_helper<Policy, node_type, typename build_indices<2+sizeof...(X)>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f){
        const_call_helper<Policy, node_type, typename build_indices<2+sizeof...(X)>::indices_type> helper(*this);
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

template <typename Policy, typename H>
struct map<Policy, H, void>: node<H, void>{
    typedef node<H, void> node_type;
    
    using hana_tag = udho_folding_map_tag<Policy, H>;
    
    using node_type::node_type;
    map(const H& h): node<H, void>(h){}
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) const{
        call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
        return helper.apply(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    decltype(auto) unpack(FunctionT&& f) {
        call_helper<Policy, node_type, typename build_indices<1>::indices_type> helper(*this);
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
using map_d = map<by_data, X...>;
template <typename... X>
using map_v = map<by_value, X...>;

template <typename Policy, typename... X>
map<Policy, X...> make_map(const X&... xs){
    return map<Policy, X...>(xs...);
}

template <typename... X>
map_d<X...> make_map_v(const X&... xs){
    return map_d<X...>(xs...);
}

template <typename... X>
map_v<X...> make_map_v(const X&... xs){
    return map_v<X...>(xs...);
}

template <typename... X>
std::ostream& operator<<(std::ostream& stream, const map<X...>& s){
    stream << "(";
    s.write(stream);
    stream << ")";
    return stream;
}

}
}
}

namespace boost {
namespace hana {
    template <typename Policy, typename... X>
    struct accessors_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename K>
        struct get_member {
            template <typename MapT>
            constexpr decltype(auto) operator()(MapT&& map) const {
                return map.template at<K::value>()._capsule.key();
            }
        };

        static decltype(auto) apply() {
            udho::util::folding::access_helper<Policy, typename udho::util::folding::map<Policy, X...>::node_type, typename udho::util::folding::build_indices<sizeof...(X)>::indices_type> helper;
            return helper.apply();
        }
    };

    template <typename Policy, typename... X>
    struct drop_front_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename Xs, typename N>
        static constexpr decltype(auto) apply(Xs&& xs, N const&) {
            return xs.template tail_at<N::value>();
        }
    };

    template <typename Policy, typename... X>
    struct is_empty_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return xs.depth == 1;
        }
    };
    
    template <typename Policy, typename... X>
    struct unpack_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return xs.unpack(std::forward<F>(f));
        }
    };
    
    template <typename Policy, typename... X>
    struct make_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) {
            return udho::util::folding::map<Policy, Args...>(std::forward<Args>(args)...);
        }
    };
    
    template <typename Policy, typename... X>
    struct length_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename ...Xn>
        static constexpr auto apply(udho::util::folding::map<Xn...> const&) {
            return hana::size_t<sizeof...(Xn)>{};
        }
    };
}
}

#define DEFINE_ELEMENT(Name, Type, mixins...)                               \
struct Name: udho::util::folding::element<Name , Type , ## mixins>{         \
    using element::element;                                                 \
    static constexpr auto key() {                                           \
        return BOOST_HANA_STRING(#Name);                                    \
    }                                                                       \
};

#endif // UDHO_UTIL_FOLDING_H
