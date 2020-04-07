/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_ACCESS_H
#define UDHO_ACCESS_H

#include <string>
#include <type_traits>
#include <udho/util.h>
#include <boost/lexical_cast.hpp>

namespace udho{
       
namespace detail{
   
template <typename F>
struct association{
    typedef F callback_type;
    typedef typename F::result_type result_type;
    
    std::string   _key;
    callback_type _callback;
    
    association(const std::string& key, callback_type callback): _key(key), _callback(callback){}
    auto call() const{
        return _callback();
    }
    auto call(){
        return _callback();
    }
    bool matched(const std::string& key) const{
        return _key == key;
    }
    template <typename C>
    bool invoke(const std::string& key, C cb){
        if(matched(key)){
            cb(call());
            return true;
        }
        return false;
    }
    template <typename C>
    bool invoke(const std::string& key, C cb) const{
        if(matched(key)){
            cb(call());
            return true;
        }
        return false;
    }
};

template <typename ValueT>
struct association_value_extractor{
    ValueT _value;
    bool   _success;
    
    association_value_extractor(): _success(false){}
    void operator()(ValueT value){
        _value = value;
        _success = true;
    }
    template <typename T>
    typename std::enable_if<!std::is_same<ValueT, T>::value>::type operator()(const T& /*value*/){}
    ValueT value() const{
        return _value;
    }
    void clear(){
        _value = ValueT();
        _success = false;
    }
};

template<typename S, typename T>
class is_streamable{
    template<typename SS, typename TT>
    static auto test(int) -> decltype( std::declval<SS&>() << std::declval<TT>(), std::true_type() );

    template<typename, typename>
    static auto test(...) -> std::false_type;

  public:
    static const bool value = decltype(test<S,T>(0))::value;
};

template <typename ValueT>
struct association_lexical_extractor{
    ValueT _value;
    bool   _success;
    
    association_lexical_extractor(): _success(false){}
    void operator()(ValueT value){
        _value = value;
        _success = true;
    }
    template <typename T>
    typename std::enable_if<!std::is_same<ValueT, T>::value && is_streamable<std::stringstream, T>::value>::type operator()(const T& value){
        _value = boost::lexical_cast<ValueT>(value);
        _success = true;
    }
    template <typename T>
    typename std::enable_if<!std::is_same<ValueT, T>::value && !is_streamable<std::stringstream, T>::value>::type operator()(const T& value){}
    ValueT value() const{
        return _value;
    }
    void clear(){
        _value = ValueT();
        _success = false;
    }
};

// template <typename GroupT>
// struct association_accessibility{
//     std::string _key;
//     GroupT     _group;
//     
//     association_accessibility(const std::string& key, const GroupT& group): _key(key), _group(group){}
//     template <typename T>
//     T as(){
//         return _group.at(_key, T());
//     }
//     template <typename T>
//     T as() const{
//         return _group.at(_key, T());
//     }
//     auto operator[](const std::string& key) const{
//         return _group[key];
//     }
//     auto operator[](const std::string& key){
//         return _group[key];
//     }
// };

/**
 * \code
 * association_group<X, void> | Y                                             => association_group<Y, association_group<X, void>>
 * association_group<Y, association_group<X, void>> | Z                       => association_group<Z, association_group<Y, association_group<X, void>>>
 * association_group<Z, association_group<Y, association_group<X, void>>> | T => association_group<T, association_group<Z, association_group<Y, association_group<X, void>>>>
 * \endcode
 */
template <typename U, typename V=void>
struct association_group{
    typedef U head_type;
    typedef V tail_type;
    typedef typename head_type::result_type result_type;
    typedef association_group<U, V> self_type;

    head_type _head;
    tail_type _tail;
    
    association_group(const head_type& head, const tail_type& tail): _head(head), _tail(tail){}
    bool matched(const std::string& key) const{
        return _head.matched(key);
    }
    bool exists(const std::string& key) const{
        return matched(key);
    }
    auto call() const{
        return _head.call();
    }
    auto call(){
        return _head.call();
    }
};

template <typename U>
struct association_group<U, void>{
    typedef U head_type;
    typedef void tail_type;
    typedef typename head_type::result_type result_type;
    typedef association_group<U, void> self_type;

    head_type _head;
    
    association_group(const head_type& head): _head(head){}
    bool matched(const std::string& key) const{
        return _head.matched(key);
    }
    bool exists(const std::string& key) const{
        return matched(key);
    }
    auto call() const{
        return _head.call();
    }
    auto call(){
        return _head.call();
    }
};

template <typename U, typename V, typename F>
association_group<association<F>, association_group<U, V>> operator|(const association_group<U, V>& nonterminal, const association<F>& terminal){
    return association_group<association<F>, association_group<U, V>>(terminal, nonterminal);
}

template <typename U>
struct is_prepared{
    template <typename V>
    static typename V::prepared_type test(int);
    template <typename>
    static void test(...);
    enum {value = !std::is_void<decltype(test<U>(0))>::value};
};

template <typename GroupT, bool IsPrepared=is_prepared<typename GroupT::head_type::result_type>::value>
struct association_group_visitor;

template <typename U, typename V>
association_group_visitor<association_group<U, V>> visit(const association_group<U, V>& nonterminal){
    return association_group_visitor<association_group<U, V>>(nonterminal);
}

template <typename U>
struct association_group_visitor<association_group<U, void>, false>{
    typedef association_group<U, void> group_type;
    const group_type& _group;
    
    association_group_visitor(const group_type& group): _group(group){}
    template <typename CallbackT>
    bool find(CallbackT& callback, const std::string& key){
        std::string khead, ktail;
        auto dot = key.find_first_of('.');
        if(dot != std::string::npos){
            // TODO exception
            return false;
        }else{
            khead = key;
            if(_group.matched(khead)){
                callback(_group.call());
                return true;
            }
        }
        return false;
    }
};

template <typename U, typename V>
struct association_group_visitor<association_group<U, V>, false>{
    typedef association_group<U, V> group_type;
    typedef V tail_type;
    typedef association_group_visitor<tail_type> tail_visitor_type;
    const group_type& _group;
    tail_visitor_type _tail_visitor;
    
    association_group_visitor(const group_type& group): _group(group), _tail_visitor(group._tail){}
    template <typename CallbackT>
    bool find(CallbackT& callback, const std::string& key){
        std::string khead, ktail;
        auto dot = key.find_first_of('.');
        if(dot != std::string::npos){
            // TODO exception
            return false;
        }else{
            khead = key;
            if(_group.matched(khead)){
                callback(_group.call());
                return true;
            }else{
                return _tail_visitor.find(callback, key);
            }
        }
        return false;
    }
};

template <typename U, typename V>
struct association_group_visitor<association_group<U, V>, true>{
    typedef association_group<U, V> group_type;
    typedef typename group_type::result_type result_type;
    typedef V tail_type;
    typedef association_group_visitor<tail_type> tail_visitor_type;
    const group_type& _group;
    tail_visitor_type _tail_visitor;
    
    association_group_visitor(const group_type& group): _group(group), _tail_visitor(group._tail){}
    template <typename CallbackT>
    bool find(CallbackT& callback, const std::string& key){
        std::string khead, ktail;
        auto dot = key.find_first_of('.');
        if(dot != std::string::npos){
            khead = key.substr(0, dot);
            ktail = key.substr(dot+1);
            if(_group.matched(khead)){
                result_type result = _group.call();
                auto index = result.index();
                auto visitor = udho::detail::visit(index);
                return visitor.find(callback, ktail);
            }
        }else{
            khead = key;
            if(_group.matched(khead)){
                callback(_group.call());
                return true;
            }else{
                return _tail_visitor.find(callback, key);
            }
        }
        return false;
    }
};


struct association_leaf{
    typedef void result_type;
    bool matched(const std::string& /*key*/) const{
        return false;
    }
    bool call() const{return false;}
    template <typename C>
    bool invoke(const std::string& /*key*/, C /*cb*/) const{
        return false;
    }
};

}

struct assoc: detail::association_group<detail::association_leaf, void>{
    assoc(): detail::association_group<detail::association_leaf, void>(detail::association_leaf()){}
};

template <typename F>
detail::association<F> associate(const std::string& key, F callback){
    return detail::association<F>(key, callback);
}

template <typename DerivedT>
struct prepare{
    typedef DerivedT prepared_type;
    
    template <typename F>
    auto var(const std::string& key, F f){
        return udho::associate(key, internal::member(f, static_cast<DerivedT*>(this)));
    }
    template <typename F>
    auto var(const std::string& key, F f) const{
        return udho::associate(key, internal::member(f, static_cast<const DerivedT*>(this)));
    }
    template <typename F>
    auto fn(const std::string& key, F f){
        return udho::associate(key, internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    template <typename F>
    auto fn(const std::string& key, F f) const{
        return udho::associate(key, internal::reduced(f, static_cast<const DerivedT*>(this)));
    }
    auto index(){
        DerivedT& obj = static_cast<DerivedT&>(*this);
        return obj.dict(udho::assoc());
    }
    auto index() const{
        const DerivedT& obj = static_cast<const DerivedT&>(*this);
        return obj.dict(udho::assoc());
    }
};

}

#endif // UDHO_ACCESS_H
