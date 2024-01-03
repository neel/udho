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

#ifndef UDHO_HAZO_NODE_NODE_H
#define UDHO_HAZO_NODE_NODE_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/capsule.h>
#include <udho/hazo/node/meta.h>
#include <udho/hazo/node/tag.h>

namespace udho{
namespace hazo{

#ifndef __DOXYGEN__

namespace detail{

    template <typename NodeT, typename ArgT, typename... T>
    struct node_is_constructible{
        using type = std::integral_constant<bool, std::is_constructible<typename NodeT::data_type, ArgT>::value && node_is_constructible<typename NodeT::tail_type, T...>::type::value>;
    };

    template <typename NodeT, typename ArgT>
    struct node_is_constructible<NodeT, ArgT>{
        using type = std::is_constructible<typename NodeT::data_type, ArgT>;
    };

    template <typename ArgT>
    struct node_is_constructible<void, ArgT>{
        using type = std::false_type;
    };

}

/**
 * @brief A non terminal node in the chain of nodes. 
 * A node internally uses `capsule` to store the data inside the node.
 * @tparam HeadT type of the value in a node
 * @tparam TailT type of the tail (void for terminal  node)
 * @ingroup node
 */
template <typename HeadT, typename TailT>
struct basic_node: private TailT /*basic_node<typename TailT::data_type, typename TailT::tail_type>*/{
    /**
     * tail of the node
     */
    // typedef basic_node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    typedef TailT tail_type;
    /**
     * type assistance through basic_node
     */
    typedef typename meta_node<HeadT, TailT>::types types;
    /**
     * capsule type for the node
     */
    typedef capsule<HeadT> capsule_type;
    /**
     * data type for the node
     * @see capsule
     */
    typedef typename capsule_type::data_type data_type;
    /**
     * data type for the node
     * @see capsule
     */
    typedef typename capsule_type::value_type value_type;
    /**
     * index type for the node
     * @see capsule
     */
    typedef typename capsule_type::index_type index_type;
    /**
     * key type for the node
     * @see capsule
     */
    typedef typename capsule_type::key_type key_type;

    typedef basic_node<HeadT, TailT> self_type;

    // template <typename OtherNode>
    // using concat_type = basic_node<HeadT, typename TailT::template concat_type<OtherNode>>;
    
    enum { depth = tail_type::depth +1 };

    template <typename ArgT, typename... T>
    using is_constructible = typename detail::node_is_constructible<basic_node, ArgT, T...>::type;
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L) 
    template <typename ArgT, typename... T>
    static inline constexpr bool is_constructible_v = is_constructible<ArgT, T...>::value;
#endif 

    /// @name Constructor 
    /// @{
    /**
     * Default Constructor
     */
    basic_node() = default;
    /**
     * Construct a node with a value of the current node and values for all or some of the nodes in the tail
     * @param v value of the current node
     * @param ts ... values of the nodes in the tail
     */
    template <typename ArgT, typename... T, std::enable_if_t<sizeof...(T) <= depth && is_constructible<ArgT, T...>::value, bool> = true>
    basic_node(const ArgT& v, const T&... ts):  tail_type(ts...), _capsule(v) {}
    /**
     * Copy constructor to construct from another node of same type
     * @param other another noode of same type
     */
    basic_node(const self_type& other) = default;
    /**
     * Construct from a node having different head and tail
     * @param other another noode of different type
     */
    template <typename OtherHeadT, typename OtherTailT, std::enable_if_t<!std::is_same<self_type, basic_node<OtherHeadT, OtherTailT>>::value, bool> = true>
    basic_node(const basic_node<OtherHeadT, OtherTailT>& other): tail_type(other) { _capsule.set(other.template data<index_type>()); }

    /**
     * Construct from a node having different head and tail
     * @param other another noode of different type
     */
    template <typename OtherTailT, typename LeafT, std::enable_if_t<!std::is_same<self_type, basic_node<HeadT, OtherTailT>>::value && !std::is_void<OtherTailT>::value, bool> = true>
    basic_node(const basic_node<HeadT, OtherTailT>& l, const LeafT& r): tail_type(l.tail(), r), _capsule(l.front()) { }

    template <typename LeafT, std::enable_if_t<!std::is_same<self_type, basic_node<HeadT, void>>::value, bool> = true>
    basic_node(const basic_node<HeadT, void>& l, const LeafT& r): tail_type(r), _capsule(l.front()) { }
    /// @}
    
    // template <typename OtherNode>
    // concat_type<OtherNode> concat_(const OtherNode& x){ return concat_type<OtherNode>{_capsule, tail().concat_(x)}; }

    /**
     * Front of the chain of nodes
     * @return the capsule of the current node
     * @{
     */
    capsule_type& front() { return _capsule; }
    const capsule_type& front() const { return _capsule; }
    /// @}
    
    /**
     * tail of the node
     * @return the tail of the current node
     * @{
     */
    tail_type& tail() { return static_cast<tail_type&>(*this); }
    const tail_type& tail() const { return static_cast<const tail_type&>(*this); }
    /// @}
    
    /**
     * data of the node
     * @{
     */
    data_type& data() { return _capsule.data(); }
    const data_type& data() const { return _capsule.data(); }
    /// @}
    
    /**
     * value of the node
     * @{
     */
    value_type& value() { return _capsule.value(); }
    const value_type& value() const { return _capsule.value(); }
    /// @}
    
    /**
     * Checks whether there exists any node identical to the provided type
     * @tparam T type to match against all other node's index_type
     */
    template <typename T>
    bool exists() const{ return std::is_same<index_type, T>::value || tail_type::template exists<T>(); }
    
    /// @name Comparison
    /// @{
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth != depth || !std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool operator==(const OtherNodeT&) const { return false; }
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth == depth && std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool operator==(const OtherNodeT& other) const {
        return _capsule == other._capsule && tail_type::operator==(other.tail());
    }
    template <typename OtherNodeT>
    bool operator!=(const OtherNodeT& other) const{ return !(*this == other); }
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth != depth || !std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool less(const OtherNodeT&) const { return false; }
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth == depth && std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool less(const OtherNodeT& other) const {
        return _capsule < other._capsule && tail_type::less(other.tail());
    }
    /// @}
    
    /**
     * assign another node of the same type
     */
    self_type& operator=(const self_type& other) { 
        _capsule = other._capsule; 
        tail_type::operator=(other.tail());
        return *this;
    }
    /**
     * Given a value v of type T, finds a node in the chain with matching index_type and sets the value to v.
     * If all is set to true then sets values of all such nodes to v. Otherwise only sets the value for the first such node and skips the rest.
     * @param v value 
     * @param all is set to true then sets all value of type T to v
     * @name set
     * @{
     */
    template <typename T>
    bool set(const T& v, bool all = false){
        bool success = false;
        if(std::is_same<index_type, T>::value){
            _capsule.set(v);
            success = true;
        }
        if(all) 
            return success || tail_type::template set<T>(v, all);
        else
            return success && tail_type::template set<T>(v, all);
    }
    

    template <int N, typename T, std::enable_if_t<N == 0, bool> = true>
    bool set(const T& v){
        _capsule.set(v);
        return true;
    }
    template <int N, typename T, std::enable_if_t<N != 0, bool> = true>
    bool set(const T& v){
        return tail_type::template set<N-1, T>(v);
    }
    /// @}
    
    /// @name capsule_at
    /// Get the N'th capsule of type T (index_type) @{
    // { capsule<T, N>() const
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    const capsule_type& capsule_at() const{ return _capsule; }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same<T, index_type>::value, bool> = true>
    const typename types::template capsule_of<T, N>& capsule_at() const{ return tail_type::template capsule_at<T, N>(); }
    template <typename T, int N, int = 0, std::enable_if_t<N != 0 && !std::is_same<T, index_type>::value, bool> = true>
    const typename types::template capsule_of<T, N>& capsule_at() const { return tail_type::template capsule_at<T, N>(); }
    template <typename T, int N, int = 0, char = 0, std::enable_if_t<N != 0 && std::is_same<T, index_type>::value, bool> = true>
    const typename types::template capsule_of<T, N>& capsule_at() const { return tail_type::template capsule_at<T, N-1>(); }
    // }
    
    // { capsule<T, N>() 
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    capsule_type& capsule_at() { return _capsule; }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same<T, index_type>::value, bool> = true>
    typename types::template capsule_of<T, N>& capsule_at() { return tail_type::template capsule_at<T, N>(); }
    template <typename T, int N, int = 0, std::enable_if_t<N != 0 && !std::is_same<T, index_type>::value, bool> = true>
    typename types::template capsule_of<T, N>& capsule_at() { return tail_type::template capsule_at<T, N>(); }
    template <typename T, int N, int = 0, char = 0, std::enable_if_t<N != 0 && std::is_same<T, index_type>::value, bool> = true>
    typename types::template capsule_of<T, N>& capsule_at() { return tail_type::template capsule_at<T, N-1>(); }
    // }
    /// @}

    /// @name data
    /// Get the data of the N'th of type T (index_type) @{
    // { data<T, N>() const
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    const data_type& data() const{ return data(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same<T, index_type>::value, bool> = true>
    const typename types::template data_of<T, N>& data() const{ return tail_type::template data<T, N>(); }
    template <typename T, int N, int = 0, std::enable_if_t<N != 0 && !std::is_same<T, index_type>::value, bool> = true>
    const typename types::template data_of<T, N>& data() const { return tail_type::template data<T, N>(); }
    template <typename T, int N, int = 0, char = 0, std::enable_if_t<N != 0 && std::is_same<T, index_type>::value, bool> = true>
    const typename types::template data_of<T, N>& data() const { return tail_type::template data<T, N-1>(); }
    // }
    
    // { data<T, N>()
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    data_type& data() { return data(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same<T, index_type>::value, bool> = true>
    typename types::template data_of<T, N>& data() { return tail_type::template data<T, N>(); }
    template <typename T, int N, int = 0, std::enable_if_t<N != 0 && !std::is_same<T, index_type>::value, bool> = true>
    typename types::template data_of<T, N>& data() { return tail_type::template data<T, N>(); }
    template <typename T, int N, int = 0, char = 0, std::enable_if_t<N != 0 && std::is_same<T, index_type>::value, bool> = true>
    typename types::template data_of<T, N>& data() { return tail_type::template data<T, N-1>(); }
    // }
    
    // { data<N>()
    /**
     * Get the data of the N'th node
     */
    template <int N, std::enable_if_t<N == 0, bool> = true>
    data_type& data() { return data(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    typename types::template data_at<N>& data() { return tail_type::template data<N-1>();  }
    // }
    
    // { data<N>() const
    /**
     * Get the data of the N'th node
     */
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const data_type& data() const { return data(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    const typename types::template data_at<N>& data() const { return tail_type::template data<N-1>();  }
    // }
    
    // { data<KeyT>(const KeyT&)
    /**
     * Get the data of the node identified by the key KeyT
     */
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    data_type& data(const KeyT&){ return data(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value, bool> = true>
    typename types::template data_for<KeyT>& data(const KeyT& k){ return tail_type::template data<KeyT>(k); }
    // }
    
    // { data<KeyT>(const KeyT&) const
    /**
     * Get the data of the node identified by the key KeyT
     */
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    const data_type& data(const KeyT&) const { return data(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value, bool> = true>
    const typename types::template data_for<KeyT>& data(const KeyT& k) const { return tail_type::template data<KeyT>(k); }
    // }
    
    /// @}
    
    /// @name value
    /// Get the value of the N'th of type T (index_type) @{
    // { value<T, N>() const
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    const value_type& value() const{ return value(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same<T, index_type>::value, bool> = true>
    const typename types::template value_of<T, N>& value() const{ return tail_type::template value<T, N>(); }
    template <typename T, int N, int = 0, std::enable_if_t<N != 0 && !std::is_same<T, index_type>::value, bool> = true>
    const typename types::template value_of<T, N>& value() const { return tail_type::template value<T, N>(); }
    template <typename T, int N, int = 0, char = 0, std::enable_if_t<N != 0 && std::is_same<T, index_type>::value, bool> = true>
    const typename types::template value_of<T, N>& value() const { return tail_type::template value<T, N-1>(); }
    // }
    
    // { value<T, N>()
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    value_type& value() { return value(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 && !std::is_same<T, index_type>::value, bool> = true>
    typename types::template value_of<T, N>& value() { return tail_type::template value<T, N>(); }
    template <typename T, int N, int = 0, std::enable_if_t<N != 0 && !std::is_same<T, index_type>::value, bool> = true>
    typename types::template value_of<T, N>& value() { return tail_type::template value<T, N>(); }
    template <typename T, int N, int = 0, char = 0, std::enable_if_t<N != 0 && std::is_same<T, index_type>::value, bool> = true>
    typename types::template value_of<T, N>& value() { return tail_type::template value<T, N-1>(); }
    // }

    // { value<N>()
    /**
     * Get the value of the N'th node
     */
    template <int N, std::enable_if_t<N == 0, bool> = true>
    value_type& value() { return value(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    typename types::template value_at<N>& value() { return tail_type::template value<N-1>();  }
    // }
    
    // { value<N>() const
    /**
     * Get the value of the N'th node
     */
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const value_type& value() const { return value(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    const typename types::template value_at<N>& value() const { return tail_type::template value<N-1>();  }
    // }
    
    // { value<KeyT>(const KeyT&)
    /**
     * Get the value of the node identified by the key KeyT
     */
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    value_type& value(const KeyT&){ return value(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value, bool> = true>
    typename types::template value_for<KeyT>& value(const KeyT& k){ return tail_type::template value<KeyT>(k); }
    // }
    
    // { value<KeyT>(const KeyT&) const
    /**
     * Get the value of the node identified by the key KeyT
     */
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    const value_type& value(const KeyT&) const { return value(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && !std::is_same<KeyT, key_type>::value, bool> = true>
    const typename types::template value_for<KeyT>& value(const KeyT& k) const { return tail_type::template value<KeyT>(k); }
    // }
    
    /// @}
    
    /// @name tail_at
    /// Get the tail of the N'th node @{
    template <int N, std::enable_if_t<N == 0, bool> = true>
    tail_type& tail_at() { return tail(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    typename types::template tail_at<N>& tail_at() { return tail_type::template tail_at<N>(); }
    
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const tail_type& tail_at() const { return tail(); }
    template <int N, std::enable_if_t<N != 0, bool> = true>
    typename types::template tail_at<N>& tail_at() const { return tail_type::template tail_at<N>(); }
    /// @}
    
    /// @name element
    /// Get the data of the node identified by element handle @{
    // { element<ElementT>(const element_t<ElementT>&)
    template <typename ElementT, std::enable_if_t<std::is_same<ElementT, index_type>::value, bool> = true>
    data_type& element(const element_t<ElementT>&) { return data(); }
    template <typename ElementT, std::enable_if_t<!std::is_same<ElementT, index_type>::value, bool> = true>
    typename types::template data_of<ElementT>& element(const element_t<ElementT>& e) { return tail().template element<ElementT>(e); }
    // }
    
    // { element<ElementT>(const element_t<ElementT>&) const
    template <typename ElementT, std::enable_if_t<std::is_same<ElementT, index_type>::value, bool> = true>
    const data_type& element(const element_t<ElementT>&) const { return data(); }
    template <typename ElementT, std::enable_if_t<!std::is_same<ElementT, index_type>::value, bool> = true>
    const typename types::template data_of<ElementT>& element(const element_t<ElementT>& e) const { return tail().template element<ElementT>(e); }
    // }
    /// @}
    
    /// @name operator[]
    /// @{
    /**
     * finds data of an node by element handle
     */
    template <typename ElementT>
    typename types::template data_of<ElementT>& operator[](const element_t<ElementT>& e){ return element<ElementT>(e); }
    /**
     * finds data of an node by element handle
     */
    template <typename ElementT>
    const typename types::template data_of<ElementT>& operator[](const element_t<ElementT>& e) const { return element<ElementT>(e); }
    
    /**
     * Finds data of an element by key
     */
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    data_type& operator[](const KeyT& k){ return data<KeyT>(k); }
    /**
     * Finds data of an element by key
     */
    template <typename K, std::enable_if_t<std::is_void<key_type>::value || !std::is_same<K, key_type>::value, bool> = true>
    auto& operator[](const K& k){ return tail().template operator[]<K>(k); } 
    /**
     * Finds data of an element by key
     */
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    const data_type& operator[](const KeyT& k) const { return data<KeyT>(k); }
    /**
     * Finds data of an element by key
     */
    template <typename K, std::enable_if_t<std::is_void<key_type>::value || !std::is_same<K, key_type>::value, bool> = true>
    const auto& operator[](const K& k) const { return tail().template operator[]<K>(k); }
    /// @} 
    
    /**
     * @name next
     * get the current data in input var and return the tail
     * @param var[out] 
     * @{
     */
    const tail_type& next(data_type& var) const { var = data(); return tail(); } 
    template <typename T, std::enable_if_t<!std::is_same<data_type, value_type>::value && std::is_convertible<value_type, T>::value, bool> = true>
    const tail_type& next(T& var) const { var = value(); return tail(); } 
    /// @}
        
    /**
     * print the chain 
     * @warning all data in the chain has to be printable
     */
    template <typename StreamT>
    StreamT& write(StreamT& stream) const{
        stream << _capsule << ", " ;
        tail_type::template write<StreamT>(stream);
        return stream;
    }
    
    /**
     * @name visit
     * Apply a function over all elements in the chain of nodes
     * @{
     */
    template <typename FunctionT>
    void visit(FunctionT&& f) const{
        _capsule.call(std::forward<FunctionT>(f));
        tail_type::visit(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    void visit(FunctionT&& f){
        _capsule.call(std::forward<FunctionT>(f));
        tail_type::visit(std::forward<FunctionT>(f));
    }
    /// @}
       
    /**
     * @name accumulate
     * accumulate a function f over the chain e.g. f(a, f(b, f(c, initial)))
     * @{
     */
    template <typename FunctionT, typename InitialT>
    auto accumulate(FunctionT&& f, InitialT&& initial) const {
        return std::forward<FunctionT>(f)(data(), tail_type::accumulate(std::forward<FunctionT>(f), std::forward<InitialT>(initial)));
    }
    /**
     * accumulate a function f over the chain e.g. f(a, f(b, f(c)))
     */
    template <typename FunctionT>
    auto accumulate(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(data(), tail_type::accumulate(std::forward<FunctionT>(f)));
    }
    /// @}
    
    /**
     * @name decorate 
     * @{
     */
    template <typename FunctionT, typename InitialT>
    auto decorate(FunctionT&& f, InitialT&& initial) const {
        return std::forward<FunctionT>(f).finish(accumulate(std::forward<FunctionT>(f), std::forward<InitialT>(initial)));
    }
    template <typename FunctionT>
    auto decorate(FunctionT&& f) const{
        return std::forward<FunctionT>(f).finish(accumulate(std::forward<FunctionT>(f)));
    }
    /// @}
    
    capsule_type _capsule;
};

/**
 * A terminal node in the chain of nodes
 * @tparam HeadT type of the value in a node
 * @ingroup node
 */
template <typename HeadT>
struct basic_node<HeadT, void>{
    /**
     * A terminal node has no tail
     */
    typedef void tail_type;
    /**
     * type assistance 
     */
    typedef typename meta_node<HeadT>::types types;
    /**
     * Capsule for the node
     */
    typedef capsule<HeadT> capsule_type;
    /**
     * Key type of the node
     */
    typedef typename capsule_type::key_type key_type;
    /**
     * Data type of the node
     */
    typedef typename capsule_type::data_type data_type;
    /**
     * Value type of the node
     */
    typedef typename capsule_type::value_type value_type;
    /**
     * Index type of the node
     */
    typedef typename capsule_type::index_type index_type;
    typedef basic_node<HeadT, void> self_type;

    // template <typename OtherNode>
    // using concat_type = basic_node<HeadT, OtherNode>;
    
    enum { 
        /**
         * depth of the node
         */
        depth = 0 
    };

    template <typename ArgT>
    using is_constructible = typename detail::node_is_constructible<basic_node, ArgT>::type;
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L) 
    template <typename ArgT>
    static inline constexpr bool is_constructible_v = is_constructible<ArgT>::value;
#endif 

    /**
     * @name Constructors
     * @{
     */
    /**
     * Default constructor
     */
    basic_node(): _capsule(){}
    /**
     * Construct the node with any value convertible to either the date_type or the value_type of the node
     */
    template <typename ArgT, std::enable_if_t<is_constructible<ArgT>::value, bool> = true>
    basic_node(const ArgT& v): _capsule(v) {}
    /**
     * Default copy constructor
     */
    basic_node(const self_type& other) = default;
    /**
     * Construct from a node of different type
     */
    template <typename OtherHeadT, typename OtherTailT, std::enable_if_t<!std::is_same<self_type, basic_node<OtherHeadT, OtherTailT>>::value, bool> = true>
    basic_node(const basic_node<OtherHeadT, OtherTailT>& other) { _capsule.set(other.template data<HeadT>()); }
    /**
     * @}
     */
    
    // template <typename OtherNode>
    // concat_type<OtherNode> concat_(const OtherNode& x){ return concat_type<OtherNode>{_capsule, x}; }

    /**
     * Capsule in the front of the chain (the only capsule for the terminal node)
     */
    capsule_type& front() { return _capsule; }
    /**
     * Capsule in the front of the chain (the only capsule for the terminal node)
     */
    const capsule_type& front() const { return _capsule; }
    /**
     * Data in the front of the chain (the only capsule for the terminal node)
     */
    const data_type& data() const { return _capsule.data(); }
    /**
     * Data in the front of the chain (the only capsule for the terminal node)
     */
    data_type& data() { return _capsule.data(); }
    /**
     * Value in the front of the chain (the only capsule for the terminal node)
     */
    const value_type& value() const { return _capsule.value(); }
    /**
     * Value in the front of the chain (the only capsule for the terminal node)
     */
    value_type& value() { return _capsule.value(); }
    
    /**
     * @name Comparison
     * @{
     */
    /**
     * Compare with another node of different depth
     * @param other The other node to compare with
     * @return always false
     */
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth != depth || !std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool operator==(const OtherNodeT&) const { return false; }
    /**
     * Compare with another node of same depth
     * @param other The other node to compare with
     * @return the result of capsule comparator
     */
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth == depth && std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool operator==(const OtherNodeT& other) const {
        return _capsule == other._capsule;
    }
    /**
     * Not equals comparator to compare with another node 
     * @param other The other node to compare
     */
    template <typename OtherNodeT>
    constexpr bool operator!=(const OtherNodeT& other) const{
        return !(*this == other);
    }   
    /**
     * Less than comparator specialized for nodes with different depth 
     * @param other The other node to compare
     * @return false always
     */
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth != depth || !std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool less(const OtherNodeT&) const { return false; }
    /**
     * Less than Comparator specialized for nodes with same depth
     * @param other The other node to compare
     */
    template <typename OtherNodeT, std::enable_if_t<OtherNodeT::depth == depth && std::is_same<typename OtherNodeT::data_type, data_type>::value, bool> = true>
    bool less(const OtherNodeT& other) const {
        return _capsule < other._capsule;
    }
    /// @}
    
    /**
     * Checks whether any node of type T exists
     * @tparam T Type searched
     */
    template <typename T>
    bool exists() const{ return std::is_same<HeadT, T>::value; }
    
    /**
     * Assignment operator to assign another node of same type
     * @param other The other node
     */
    self_type& operator=(const self_type& other) { 
        _capsule = other._capsule;
        return *this;
    }
    
    /**
     * Set value of the node that matches with the input type
     * @tparam T type of the input
     * @param v input of type T
     */
    template <typename T>
    bool set(const T& v, bool){
        if(std::is_same<HeadT, T>::value){
            _capsule.template set<T>(v);
            return true;
        }
        return false;
    }
    /**
     * Set the value for the N'th node having data_type T
     * @tparam N has to be 0 (for the terminal node)
     * @tparam T type of the input
     * @param v input of type T
     */
    template <int N, typename T, std::enable_if_t<N == 0 && std::is_same<T, data_type>::value, bool> = true>
    bool set(const T& v){
        _capsule.set(v);
        return true;
    }
    
    /**
     * @name capsule_at
     * Get the N'th capsule of type T (index_type)
     * @{
     */
    // { capsule<T, N>() const
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    const capsule_type& capsule_at() const{ return _capsule; }
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    capsule_type& capsule_at() { return _capsule; }
    // }
    /// @}
    
    /**
     * @name data
     * Get the data of the N'th node of type T (index_type)
     * @{
     */
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    const data_type& data() const{ return data(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    data_type& data() { return data(); }
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const data_type& data() const { return data(); }
    template <int N, std::enable_if_t<N == 0, bool> = true>
    data_type& data() { return data(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    const data_type& data(const KeyT&) const{ return data(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    data_type& data(const KeyT&){ return data(); }
    /// @}
    
    /**
     * @name value
     * Get the value of the N'th node of type T (index_type)
     * @{
     */
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    const value_type& value() const{ return value(); }
    template <typename T, int N = 0, std::enable_if_t<N == 0 &&  std::is_same<T, index_type>::value, bool> = true>
    value_type& value() { return value(); }
    template <int N, std::enable_if_t<N == 0, bool> = true>
    const value_type& value() const { return value(); }
    template <int N, std::enable_if_t<N == 0, bool> = true>
    value_type& value() { return value(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    const value_type& value(const KeyT&) const{ return value(); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    value_type& value(const KeyT&){ return value(); }
    /// @}
    
    /**
     * @name value
     * Get the data of the node that matches with the given element handle
     * @{
     */
    template <typename ElementT, std::enable_if_t<std::is_same<ElementT, index_type>::value, bool> = true>
    data_type& element(const element_t<ElementT>&) { return data(); }
    template <typename ElementT, std::enable_if_t<std::is_same<ElementT, index_type>::value, bool> = true>
    const data_type& element(const element_t<ElementT>&) const { return data(); }
    /// @}
    
    /**
     * @name operator[] 
     * Get the data of the node that matches with either the given element handle or with the key of that node
     * @{
     */
    template <typename ElementT>
    data_type& operator[](const element_t<ElementT>& e){ return element<ElementT>(e); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    data_type& operator[](const KeyT& k){ return data<KeyT>(k); }
    
    template <typename ElementT>
    const data_type& operator[](const element_t<ElementT>& e) const { return element<ElementT>(e); }
    template <typename KeyT, std::enable_if_t<!std::is_void<key_type>::value && std::is_same<KeyT, key_type>::value, bool> = true>
    const data_type& operator[](const KeyT& k) const { return data<KeyT>(k); }
    /// @}
    
    /**
     * get the current data in input var and return the tail
     * @param var[out] 
     * @{
     */
    void next(data_type& var) const { var = data(); } 
    template <typename T, std::enable_if_t<!std::is_same<data_type, value_type>::value && std::is_convertible<value_type, T>::value, bool> = true>
    void next(T& var) const { var = value(); } 
    /// @}
    
    template <typename StreamT>
    StreamT& write(StreamT& stream) const{
        stream << _capsule ;
        return stream;
    }
    
    /**
     * Apply a function over all elements in the chain of nodes
     * @{
     */
    template <typename FunctionT>
    void visit(FunctionT&& f) const{
        _capsule.call(std::forward<FunctionT>(f));
    }
    template <typename FunctionT>
    void visit(FunctionT&& f){
        _capsule.call(std::forward<FunctionT>(f));
    }
    /// @}
    
    template <typename FunctionT>
    void operator()(FunctionT&& f){
        std::forward<FunctionT>(f)(data());
    }
    template <typename FunctionT, typename InitialT>
    auto accumulate(FunctionT&& f, InitialT&& initial) const {
        return std::forward<FunctionT>(f)(data(), std::forward<InitialT>(initial));
    }
    template <typename FunctionT>
    auto accumulate(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(data());
    }
    template <typename FunctionT, typename InitialT>
    auto decorate(FunctionT&& f, InitialT&& initial) const {
        return std::forward<FunctionT>(f).finish(accumulate(std::forward<FunctionT>(f), std::forward<InitialT>(initial)));
    }
    template <typename FunctionT>
    auto decorate(FunctionT&& f) const{
        return std::forward<FunctionT>(f).finish(accumulate(std::forward<FunctionT>(f)));
    }
    
    capsule_type _capsule;
};

namespace detail {
    template <typename H, typename... T>
    struct basic_node_builder{
        using type = basic_node<H, typename basic_node_builder<T...>::type>;
    };
    template <typename H>
    struct basic_node_builder<H>{
        using type = basic_node<H, void>;
    };
    /**
    * @brief builds a chain of basic node with the given type
    * 
    * @tparam H 
    * @tparam T... 
    */
    template <typename H, typename... T>
    struct node_{
        typedef typename detail::basic_node_builder<H, T...>::type type;
    };
}

/**
 * @brief build a chain of nodes
 * 
 * @tparam T...
 */
template <typename... T>
using node = typename detail::node_<T...>::type;

template <typename HeadT, typename TailT>
decltype(auto) operator>>(const basic_node<HeadT, TailT>& node, HeadT& var){
    return node.next(var);
}
template <typename... T, typename V, std::enable_if_t<!std::is_same<typename basic_node<T...>::data_type, typename basic_node<T...>::value_type>::value && std::is_convertible<typename basic_node<T...>::value_type, V>::value, bool> = true>
decltype(auto) operator>>(const basic_node<T...>& node, V& var){
    return node.next(var);
}

#else

/**
 * @brief A basic_node serves as the basic building block behind hazo seq and map
 * - Head of the basic_node has to be default constructible and copy constrictible
 * - Instead of storing the Head directly basic_node wraps it inside a capsule @see capsule
 * @note use @ref node instead of using basic_node directly
 * @tparam HeadT Head Type
 * @tparam TailT another basic_node or void
 * @see node
 * @ingroup hazo
 */
template <typename HeadT, typename TailT>
struct basic_node{
    /**
     * @brief tail of the basic_node
     */
    typedef basic_node<typename TailT::data_type, typename TailT::tail_type> tail_type;
    /**
     * @brief type assistance through meta_node
     */
    typedef typename meta_node<HeadT, TailT>::types types;
    /**
     * @brief capsule type for the basic_node
     */
    typedef capsule<HeadT> capsule_type;
    /**
     * @brief data type for the basic_node
     * @see capsule
     */
    typedef typename capsule_type::data_type data_type;
    /**
     * @brief data type for the basic_node
     * @see capsule
     */
    typedef typename capsule_type::value_type value_type;
    /**
     * @brief index type for the basic_node
     * @see capsule
     */
    typedef typename capsule_type::index_type index_type;
    /**
     * @brief key type for the basic_node
     * @see capsule
     */
    typedef typename capsule_type::key_type key_type;

    typedef basic_node<HeadT, TailT> self_type;
    
    enum { depth = tail_type::depth +1 };

    template <typename ArgT, typename... T>
    using is_constructible = detail::node_is_constructible<basic_node, ArgT, T...>;
    template <typename ArgT, typename... T>
    static inline constexpr bool is_constructible_v = is_constructible<ArgT, T...>::type::value;

    /**
     * @brief Default Constructor
     */
    basic_node() = default;
    /**
     * @brief Construct a basic_node with a value of the current basic_node and values for all or some of the nodes in the tail.
     * It is expected that the data_type's of nodes in the tail are constructible through the values provided. 
     * @param v value of the current basic_node
     * @param ts ... values of the nodes in the tail
     */
    template <typename ArgT, typename... T>
    basic_node(const ArgT& v, const T&... ts);
    /**
     * @brief Copy constructor to construct from another basic_node of same type
     * @param other another noode of same type
     */
    basic_node(const self_type& other) = default;
    /**
     * @brief Construct from a basic_node having different head and tail
     * @param other another noode of different type
     */
    template <typename OtherHeadT, typename OtherTailT>
    basic_node(const basic_node<OtherHeadT, OtherTailT>& other): tail_type(other);
    
    /**
     * @brief Front of the chain of nodes
     * @return the capsule of the current basic_node
     */
    capsule_type& front();
    /**
     * @brief Front of the chain of nodes
     * @return the capsule of the current basic_node
     */
    const capsule_type& front() const;
    /**
     * @brief Get the N'th capsule of type T (index_type) 
     * 
     * @tparam T 
     * @tparam N 
     * @return const capsule_type& 
     */
    template <typename T, int N = 0>
    const capsule_type& capsule_at() const;
    /**
     * @brief Get the N'th capsule of type T (index_type) 
     * 
     * @tparam T 
     * @tparam N 
     * @return const capsule_type& 
     */
    template <typename T, int N = 0>
    capsule_type& capsule_at();
    
    /**
     * @brief tail of the basic_node
     * @return the tail of the current basic_node
     */
    tail_type& tail();
    /**
     * @brief tail of the basic_node
     * @return the tail of the current basic_node
     */
    const tail_type& tail() const;
    /**
     * @brief Get the tail of the N'th basic_node
     * 
     * @tparam N 
     * @return types::template tail_at<N>&
     */
    template <int N>
    typename types::template tail_at<N>& tail_at();
    /**
     * @brief Get the tail of the N'th basic_node
     * 
     * @tparam N 
     * @return types::template tail_at<N>&
     */
    template <int N>
    const typename types::template tail_at<N>& tail_at();

    /**
     * @brief Checks whether there exists any basic_node identical to the provided type
     * @tparam T type to match against all other basic_node's index_type
     */
    template <typename T>
    bool exists() const;

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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h;
     * h.data<int>(); // First int (first item in the basic_node chain)
     * h.data<int, 1>(); // Second int (third item in the basic_node chain)
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h(1, 3.14, 2, "Hello");
     * h.data<int>(); // 1
     * h.data<int, 1>(); // 2
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h(1, 3.14, 2, "Hello");
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h(1, 3.14, 2, "Hello");
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h;
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h;
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h(1, 3.14, 2, "Hello");
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
     * hazo::basic_node<int, hazo::basic_node<double, hazo::basic_node<int, hazo::basic_node<std::string>>>> h(1, 3.14, 2, "Hello");
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
     * @brief Compare with another basic_node having same depth and same data_type
     * If the other basic_node has different level of depth or it has a different data_type then returns false
     * @return bool
     */
    template <typename OtherNodeT>
    bool operator==(const OtherNodeT& other) const;
    /**
     * @brief Compare with another basic_node
     * @return bool
     */
    template <typename OtherNodeT>
    bool operator!=(const OtherNodeT& other) const;
    /**
     * @brief Less that comparator
     * 
     * @tparam OtherNodeT 
     * @param other 
     * @return true 
     * @return false 
     */
    template <typename OtherNodeT>
    bool less(const OtherNodeT& other) const;
    

    /**
     * @brief assign another basic_node of the same type
     */
    self_type& operator=(const self_type& other);
    /**
     * @brief Given a value v of type T, finds a basic_node in the chain with matching index_type and sets the value to v.
     * If all is set to true then sets values of all such nodes to v. Otherwise only sets the value for the first such basic_node and skips the rest.
     * @param v value 
     * @param all is set to true then sets all value of type T to v
     */
    template <typename T>
    bool set(const T& v, bool all = false);
    /**
     * @brief Set value of the basic_node at N'th depth
     * 
     * @tparam N 
     * @tparam T 
     * @param v 
     * @return true 
     * @return false 
     */
    template <int N, typename T>
    bool set(const T& v);

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
    
    /**
     * Finds data of an element by key
     */
    template <typename KeyT>
    auto& operator[](const KeyT& k);
    /**
     * Finds data of an element by key
     */
    template <typename KeyT>
    const auto& operator[](const KeyT& k) const;

    
    /**
     * @brief get the current data in input var and return the tail
     * @tparam T Either data_type or convertible to value_type
     * @param var[out] 
     */
    template <typename T, std::enable_if_t<!std::is_same_v<data_type, value_type> && std::is_convertible_v<value_type, T>, bool> = true>
    const tail_type& next(T& var) const;
        
    /**
     * print the chain 
     * @warning all data in the chain has to be printable
     */
    template <typename StreamT>
    StreamT& write(StreamT& stream) const;
    
    /**
     * @brief Apply a function over all elements in the chain of nodes
     */
    template <typename FunctionT>
    void visit(FunctionT&& f) const;
    /**
     * @brief Apply a function over all elements in the chain of nodes
     */
    template <typename FunctionT>
    void visit(FunctionT&& f);
       
    /**
     * @brief accumulate a function f over the chain e.g. `f(a, f(b, f(c, initial)))`
     */
    template <typename FunctionT, typename InitialT>
    auto accumulate(FunctionT&& f, InitialT&& initial) const;
    /**
     * @brief accumulate a function f over the chain e.g. `f(a, f(b, f(c)))`
     */
    template <typename FunctionT>
    auto accumulate(FunctionT&& f) const;
    
    /**
     * @brief decorate 
     */
    template <typename FunctionT, typename InitialT>
    auto decorate(FunctionT&& f, InitialT&& initial) const;
    /**
     * @brief decorate 
     */
    template <typename FunctionT>
    auto decorate(FunctionT&& f) const;
};

/**
 * @brief build a chain of nodes with given types
 * @tparam T...
 * @ingroup hazo
 * @see basic_node
 */
template <typename... T>
using node = typename detail::node_<T...>::type;

#endif // __DOXYGEN__
    
}
}

#endif // UDHO_HAZO_NODE_NODE_H
