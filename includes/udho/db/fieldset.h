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

#ifndef WEE_ACTIVITY_DB_FIELDSET_H
#define WEE_ACTIVITY_DB_FIELDSET_H

#include <string>
#include <type_traits>

namespace udho{
namespace db{
    
/**
 * \code
 * struct first_name: column<first_name, std::string>{
 *      first_name(): column<first_name, std::string>("first_name"){}
 * };
 * \endcode
 */
template <typename DerivedT, typename T>
struct field{
    typedef field<DerivedT, T> self_type;
    typedef self_type field_type;
    
    struct handle{
        typedef self_type field_type;
    };
    
    const static constexpr handle value = handle();
    
    typedef DerivedT derived_type;
    typedef T value_type;
    
    field(const std::string& name, const value_type& v = value_type()): _name(name), _value(v){}
    const value_type& get() const { return _value; }
    void set(const value_type& v) { _value = v; }
    self_type& self(const handle&) { return *this; }
    template<typename ValueT>
    typename std::enable_if<std::is_convertible<ValueT, value_type>::value, void>::type set(const std::string& name, const ValueT& v){
        if(name == _name){
            _value = v;
        }
    }
    template<typename ValueT>
    typename std::enable_if<!std::is_convertible<ValueT, value_type>::value, void>::type set(const std::string&, const ValueT&){}
    private:
        std::string _name;
        value_type _value;
    
};

template <typename DerivedT, typename T> 
const typename field<DerivedT, T>::handle field<DerivedT, T>::value;

#define DB_FIELD(name, type)                                                \
    struct name: udho::db::field<name, type> {                                     \
        name(const typename field::value_type& v = typename field::value_type())  \
            : field(#name, v){}                                                   \
    };

template <typename StreamT, typename DerivedT, typename T>
StreamT& operator<<(StreamT& stream, const field<DerivedT, T>& col){
    stream << col.get();
    return stream;
}


template <typename FieldT>
struct field_proxy;

template <typename DerivedT, typename T>
struct field_proxy<field<DerivedT, T>>{
    typedef field<DerivedT, T> field_type;
    
    field_type& _field;
    
    field_proxy(field_type& field): _field(field){}
    field_proxy<field_type>& operator=(const typename field_type::value_type& v){
        _field.set(v);
        return *this;
    }
    typename field_type::value_type get() const{
        return _field.get();
    }
    operator typename field_type::value_type() const {
        return get();
    }
    typename field_type::value_type operator*() const{
        return get();
    }
};

template <typename StreamT, typename DerivedT, typename T>
StreamT& operator<<(StreamT& stream, const field_proxy<field<DerivedT, T>>& fp){
    stream << fp.get();
    return stream;
}


namespace detail{
    template <typename CFieldT>
    struct visitor_field_setter{
        visitor_field_setter(const CFieldT& cf): _cf(cf){}
        
        template <typename FieldT>
        void operator()(FieldT& f) const{
            f.set(_cf.name(), _cf.value());
        }
        
        const CFieldT& _cf;
    };
}

/**
 * \code
 * fieldset<first_name, last_name, age> fs;
 * 
 * fs[first_name::value] = "Neel";
 * fs[last_name::value] = "Basu";
 * fs[age::value] = 32;
 * 
 * std::cout << fs[first_name::value] << std::endl;
 * std::cout << *fs[last_name::value] << std::endl;
 * std::cout << fs[age::value] << std::endl;
 * \endcode
 */
template <typename... Fields>
struct fieldset: Fields...{
    typedef fieldset<Fields...> self_type;
    
    fieldset() = default;
    template <typename... Args>
    fieldset(const Args&... args): Fields(args)...{}
    
    template <typename HandleT>
    field_proxy<typename HandleT::field_type> field(const HandleT& h){ 
        typedef typename HandleT::field_type field_type;
        return field_proxy<field_type>(field_type::self(h)); 
    }
    
    template <typename HandleT>
    const typename HandleT::field_type::value_type& cfield(const HandleT&) const {
        typedef typename HandleT::field_type field_type;
        return field_type::get(); 
    }
    
    template <typename HandleT>
    const typename HandleT::field_type::value_type& field(const HandleT& h) const {
         return field<HandleT>(h); 
    }
    
    template <typename HandleT>
    field_proxy<typename HandleT::field_type> operator[](const HandleT& h) { return field<HandleT>(h); }
    
    template <typename HandleT>
    const typename HandleT::field_type::value_type& operator[](const HandleT& h) const { return field<HandleT>(h); }
    
    template <typename CFieldT>
    void set(const CFieldT& cf){
        // https://stackoverflow.com/a/26106059/256007
        using discard = int[];
        (void)discard{0, (Fields::set(cf.name(), cf.value()), 1)...};
    }
    
    self_type& self() { return *this; }
    const self_type& self() const { return *this; }
};

template <typename CFieldT, typename... Fields>
fieldset<Fields...>& operator<<(fieldset<Fields...>& fs, const CFieldT& cf){
    fs.set(cf);
    return fs;
}
    
}
}

#endif // WEE_ACTIVITY_DB_FIELDSET_H
