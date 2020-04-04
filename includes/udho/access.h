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
struct association_visitor{
    ValueT _value;
    
    association_visitor(ValueT def=ValueT()): _value(def){}
    void operator()(ValueT value){
        std::cout << "found " << value << std::endl;
        _value = value;
    }
    template <typename T>
    typename std::enable_if<!std::is_same<ValueT, T>::value>::type operator()(const T& value){
        std::cout << "lost " << value << std::endl;
    }
    ValueT value() const{
        return _value;
    }
};

template <typename GroupT>
struct association_accessibility{
    std::string _key;
    GroupT&     _group;
    
    association_accessibility(const std::string& key, GroupT& group): _key(key), _group(group){}
    template <typename T>
    T as(){
        return _group.at(_key, T());
    }
};

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
    template <typename C>
    bool invoke(const std::string& key, C& cb) const{
        if(matched(key)){
            cb(call());
            return true;
        }else{
            return _tail.template invoke<C>(key, cb);
        }
        return false;
    }
    template <typename C>
    bool invoke(const std::string& key, C& cb){
        if(matched(key)){
            cb(call());
            return true;
        }else{
            return _tail.template invoke<C>(key, cb);
        }
        return false;
    }
    template <typename ValueT>
    ValueT at(const std::string& key, ValueT def) const {       
        association_visitor<ValueT> visitor(def);
        invoke(key, visitor);
        return visitor.value();
    }
    template <typename ValueT>
    ValueT at(const std::string& key, ValueT def){
        association_visitor<ValueT> visitor(def);
        invoke(key, visitor);
        return visitor.value();
    }
    association_accessibility<self_type> operator[](const std::string& key) const{
        return association_accessibility<self_type>(key, *this);
    }
    association_accessibility<self_type> operator[](const std::string& key){
        return association_accessibility<self_type>(key, *this);
    }
};

template <typename U>
struct association_group<U, void>{
    typedef U head_type;
    typedef void tail_type;
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
    template <typename C>
    bool invoke(const std::string& key, C& cb) const{
        if(matched(key)){
            cb(call());
            return true;
        }
        return false;
    }
    template <typename C>
    bool invoke(const std::string& key, C& cb){
        if(matched(key)){
            cb(call());
            return true;
        }
        return false;
    }
    template <typename ValueT>
    ValueT at(const std::string& key, ValueT def) const {
        association_visitor<ValueT> visitor(def);
        invoke(key, visitor);
        return visitor.value();
    }
    template <typename ValueT>
    ValueT at(const std::string& key, ValueT def){
        association_visitor<ValueT> visitor(def);
        invoke(key, visitor);
        return visitor.value();
    }
    association_accessibility<self_type> operator[](const std::string& key) const{
        return association_accessibility<self_type>(key, *this);
    }
    association_accessibility<self_type> operator[](const std::string& key){
        return association_accessibility<self_type>(key, *this);
    }
};

struct association_leaf{
    bool matched(const std::string& /*key*/) const{
        return false;
    }
    bool call() const{return false;}
    template <typename C>
    bool invoke(const std::string& /*key*/, C /*cb*/) const{
        return false;
    }
};

template <typename U, typename V, typename F>
association_group<association<F>, association_group<U, V>> operator|(const association_group<U, V>& nonterminal, const association<F>& terminal){
    return association_group<association<F>, association_group<U, V>>(terminal, nonterminal);
}

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
    auto operator[](const std::string& key) const{
        auto idx = index();
        return idx[key];
    }
    auto operator[](const std::string& key){
        auto idx = index();
        return idx[key];
    }
};


/**
 * @todo write docs
 */
// template <typename T>
// class access{
//     const T& _target;
//     
//     auto value(const std::string& key){
//         _target.dict()
//     }
// };

}

#endif // UDHO_ACCESS_H
