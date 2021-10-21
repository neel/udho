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

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/node/tag.h>

namespace udho{
namespace hazo{
    
/**
 * @brief An element is an object identifiable by a key defined by DerivedT::key() and contains a value of type ValueT.
 * - It is expected that the DerivedT class must have a key() method which returns a key that is compile time distinguishable from other keys e.g. compile time string.
 * - An element is uninitialized by default. Whenever a value is set it gets initialized.
 * - An element element has a name which is obtained by converting key().c_str() to std::string. 
 * .
 * Following is an example usage of the element directly
 * @code {.cpp}
 * struct label: udho::hazo::element<label, std::string>{
 *   using element::element;
 *   using element::operator=;
 * };
 * struct height: udho::hazo::element<height, std::size_t>{
 *   using element::element;
 *   using element::operator=;
 * };
 * 
 * label l1, l2("good");
 * l1 = l2;
 * l1 = "okay";
 * height h1, h2(42);
 * h1 = h2;
 * h1 = 24;
 * @endcode
 * However it is mostly used through two macros @ref HAZO_ELEMENT and @ref HAZO_ELEMENT_HANA
 * @tparam DerivedT The class that provides the key() method
 * @tparam ValueT The type of value the element intends to contain
 * @tparam Mixins ... extend features of the element.
 * @see HAZO_ELEMENT, HAZO_ELEMENT_HANA
 * @ingroup hazo
 */
template <typename DerivedT, typename ValueT, template<class, typename> class... Mixins>
struct element: Mixins<DerivedT, ValueT>...{
    /**
     * Derived type
     */
    typedef DerivedT derived_type;
    /**
     * Value type
     */
    typedef ValueT value_type;
    typedef element<DerivedT, ValueT, Mixins...> self_type;
    /**
     * element type
     */
    typedef self_type element_type;
    /**
     * alter the value type and produce a new element type 
     */
    template <typename AltValueT>
    using alter = element<DerivedT, AltValueT, Mixins...>;
    
    /**
     * element handle
     */
    const static constexpr element_t<derived_type> val = element_t<derived_type>();
    
    /**
     * Construct an element
     * @{
     */
    element(): _value(value_type()), _initialized(false){}
    element(const value_type& v): _value(v), _initialized(true){}
    element(const self_type& other) = default;
    /// @}
    
    /**
     * returns the key used to identify the element
     * @{
     */
    static constexpr auto key() { return DerivedT::key(); }
    /**
     * returns the key as std::string for runtime usage
     */
    std::string name() const { return std::string(key().c_str()); }
    /// @}
    
    /**
     * Assign a value to the element
     * @{
     */
    template <typename V, std::enable_if_t<std::is_assignable<std::add_lvalue_reference_t<value_type>, V>::value, bool> = true>
    self_type& operator=(const V& v) { 
        _value = v; 
        _initialized = true;
        return *this; 
    }
    self_type& operator=(const self_type& other) = default;
    /// @}
    
    /**
     * Get the value of the element
     * @{
     */
    value_type& value() { return _value; }
    const value_type& value() const { return _value; }
    /// @}
    
    /**
     * Checks whether the element has been initialized with a value or not 
     * @{
     */
    bool initialized() const { return _initialized; }
    void uninitialize(bool flag = true) {
        _initialized = !flag;
        if(flag){
            _value = value_type();
        }
    }
    /// @}
    
    /**
     * Compare with a value of value_type
     * @param v value_type
     * @{
     */
    bool operator==(const value_type& v) const { return _initialized && _value == v; }
    /**
     * Compare with an element of same type
     * @param other element_type
     */
    bool operator==(const self_type& other) const { return _initialized == other._initialized && _value == other._value; }
    bool operator!=(const self_type& other) const { return !operator==(other); }
    /// @}
    
    /**
     * Set value of the element only if the type of value matches with value type and name matches with the name of the element
     * @param n name 
     * @param v value
     * @{
     */
    template<typename V, std::enable_if_t<std::is_assignable_v<value_type, V>, bool> = true>
    void set(const std::string& n, const V& v){
        if(n == name()){
            _value = v;
            _initialized = true;
        }
    }
    template<typename V, std::enable_if_t<!std::is_assignable_v<value_type, V>, bool> = true>
    void set(const std::string&, const V&){}
    /// @}
    
    private:
        value_type _value;
        bool _initialized;
};

template <typename DerivedT, typename ValueT, template<class, typename> class... Mixins>
const element_t<DerivedT> element<DerivedT, ValueT, Mixins...>::val;

/**
 * @brief Define and element that has a key() method which returns compile time distinguishable element_t
 * @code 
 * HAZO_HANA(first_name, std::string);
 * first_name f("Neel");
 * @endcode 
 * @param Name Name of the element
 * @param Type Type of the element
 * @ingroup hazo
 */
#define HAZO_ELEMENT(Name, Type, ...)                                   \
    struct Name: udho::hazo::element<Name , Type , ##__VA_ARGS__>{      \
        using element::element;                                         \
        using element::operator=;                                       \
        static constexpr auto key() {                                   \
            return val;                                                 \
        }                                                               \
    }

#ifndef __DOXYGEN__

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

#endif // __DOXYGEN__

}
}

#endif // UDHO_HAZO_MAP_ELEMENT_H
