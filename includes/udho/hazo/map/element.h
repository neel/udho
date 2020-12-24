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

#ifndef UDHO_HAZO_MAP_ELEMENT_H
#define UDHO_HAZO_MAP_ELEMENT_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/tag.h>

namespace udho{
namespace util{
namespace hazo{
    
template <typename DerivedT, typename ValueT, template<class, typename> class... Mixins>
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
    bool operator==(const value_type& v) const { return _value == v; }
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

template <typename DerivedT, typename ValueT, template<class, typename> class... Mixins>
const element_t<DerivedT> element<DerivedT, ValueT, Mixins...>::val;

#define HAZO_ELEMENT(Name, Type, mixins...)                              \
struct Name: udho::util::hazo::element<Name , Type , ## mixins>{         \
    using element::element;                                              \
    static constexpr auto key() {                                        \
        return val;                                                      \
    }                                                                    \
};

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


}
}
}

#endif // UDHO_HAZO_MAP_ELEMENT_H
