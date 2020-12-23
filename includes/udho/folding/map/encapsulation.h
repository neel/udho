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

#ifndef UDHO_FOLDING_MAP_ENCAPSULATION_H
#define UDHO_FOLDING_MAP_ENCAPSULATION_H

namespace udho{
namespace util{
namespace folding{
    
template <typename ValueT, bool IsElement = false>
struct encapsulation;
    
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
    
}    
}
}

#endif // UDHO_FOLDING_MAP_ENCAPSULATION_H