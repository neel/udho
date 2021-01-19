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

#ifndef UDHO_HAZO_CAPSULE_H
#define UDHO_HAZO_CAPSULE_H

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/node/fwd.h>
#include <udho/hazo/node/encapsulate.h>

namespace udho{
namespace util{
namespace hazo{
    
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
    
    capsule() = default;
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
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
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
    
    capsule() = default;
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
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
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
    
    capsule() = default;
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
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * encapsulate a class which is not an element
 */
template <typename DataT>
struct capsule<DataT, true>: encapsulate<DataT>{
    typedef DataT data_type;
    typedef typename encapsulate<DataT>::key_type key_type;
    typedef typename encapsulate<DataT>::value_type value_type;
    typedef capsule<DataT, true> self_type;
    
    data_type _data;
    
    capsule(): _data(value_type()){};
    capsule(const self_type&) = default;
    capsule(const data_type& h): _data(h){}
    self_type& operator=(const self_type& other) = default;
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
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};
    
}
}
}

#endif // UDHO_HAZO_CAPSULE_H
