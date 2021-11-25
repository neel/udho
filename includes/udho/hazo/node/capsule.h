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
namespace hazo{
    
#ifndef __DOXYGEN__

namespace detail{
    template <typename DataT, typename ValueT = DataT>
    struct _capsule{
        typedef DataT value_type;
        typedef ValueT data_type;
        template <typename ArgT>
        using is_constructible = std::integral_constant<bool, std::is_constructible<ValueT, ArgT>::value || std::is_constructible<DataT, ArgT>::value >;
        template <typename ArgT>
        using is_assignable = std::integral_constant<bool, std::is_assignable<ValueT, ArgT>::value || std::is_assignable<DataT, ArgT>::value >;

    #if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L) 
        static inline constexpr bool is_valid_v = std::is_default_constructible<DataT>::value && std::is_constructible<DataT, ValueT>::value && std::is_assignable<DataT, ValueT>::value;
        template <typename ArgT>
        static inline constexpr bool is_constructible_v = is_constructible<ArgT>::value;
        template <typename ArgT>
        static inline constexpr bool is_assignable_v = is_assignable<ArgT>::value;
    #endif
    };

    template <typename DataT>
    struct _capsule<DataT, DataT>{
        typedef DataT value_type;
        typedef DataT data_type;
        template <typename ArgT>
        using is_constructible =  std::is_constructible<DataT, ArgT>;
        template <typename ArgT>
        using is_assignable =  std::is_assignable<DataT, ArgT>;

    #if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L) 
        static inline constexpr bool is_valid_v = std::is_default_constructible<DataT>::value;
        template <typename ArgT>
        static inline constexpr bool is_constructible_v = is_constructible<ArgT>::value;
        template <typename ArgT>
        static inline constexpr bool is_assignable_v = is_assignable<ArgT>::value;
    #endif 

    };
}

/**
 * capsule for plain old types 
 * @ingroup capsule
 */
template <typename DataT>
class capsule<DataT, false>{
    DataT _data;
    public:
    /**
     * The encapsulated type is not a class. So key_type is void
     */
    typedef void key_type;
    /**
     * type of the value encapsulated by the capsule.
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef DataT value_type;
    /**
     * type of the object encapsulated by the capsule.
     * @note Same as value_type as the encapsulated type is not a class
     */
    typedef DataT data_type;
    /**
     * type used to index this capsule.
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef data_type index_type;
    
    /**
     * Default constructor
     */
    capsule() = default;
    /**
     * Copy constructor
     */
    capsule(const capsule&) = default;
    /**
     * Construct through an object of data_type
     * @param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible<data_type, ArgT>::value, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * Assign another capsule, encapsulating same type of data.
     * @param other another capsule 
     */
    capsule& operator=(const capsule& other) {
        _data = other.data();
        return *this;
    }
    /**
     * @brief 
     * 
     * @tparam ArgT 
     * @param other 
     * @return capsule& 
     */
    template <typename ArgT, std::enable_if_t<std::is_assignable<data_type, ArgT>::value, bool> = true>
    capsule& operator=(const ArgT& other) {
        _data = other.data();
        return *this;
    }
    /**
     * returns a const reference to the encapsulated data
     * @note same as value() as the encapsulated type is not a class
     */
    const data_type& data() const { return _data; }
    /**
     * returns a non-const reference to the encapsulated data
     * @note same as value() as the encapsulated type is not a class
     */
    data_type& data() { return _data; }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * @note same as data() as the encapsulated type is not a class
     */
    const value_type& value() const { return data(); }
    /**
     * returns a non-const reference to the value() of the data encapsulated in the capsule.
     * @note same as data() as the encapsulated type is not a class
     */
    value_type& value() { return data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same type of data.
     * @param other another capsule
     */
    bool operator==(const capsule& other) const { return _data == other.data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same type of data.
     * @param other another capsule
     */
    bool operator!=(const capsule& other) const { return !operator==(other); }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * @param other data
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * @param other data
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * Sets data of the capsule
     * @param d data 
     */
    void set(const data_type& d) { _data = d; }
    /**
     * Conversion operator overload to convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * @param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * @param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * capsule for C style string literal
 * @ingroup capsule
 */
template <int N>
class capsule<char[N], false>{
    std::string _data;
    public:
    /**
     * The encapsulated type is not a class. So key_type is void
     */
    typedef void key_type;
    /**
     * type of the value encapsulated by the capsule (using std::string instead of char*).
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef std::string value_type;
    /**
     * type of the object encapsulated by the capsule (using std::string instead of char*).
     * @note Same as value_type as the encapsulated type is not a class
     */
    typedef std::string data_type;
    /**
     * type used to index this capsule.
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef data_type index_type;
    
    /**
     * Default constructor
     */
    capsule() = default;
    /**
     * Construct through another capsule of data_type
     */
    capsule(const capsule&) = default;
    /**
     * Construct from C string literal
     * @param str C string literal
     */
    capsule(const char* str): _data(str){}
    /**
     * Construct through an object of data_type
     * @param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible<data_type, ArgT>::value, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * Assign another capsule, encapsulating same type of data.
     * @param other another capsule 
     */
    capsule& operator=(const capsule& other) {
        _data = other.data();
        return *this;
    }
    /**
     * returns a const reference to the encapsulated data
     * @note same as value() as the encapsulated type is not a class
     */
    const data_type& data() const { return _data; }
    /**
     * returns a non-const reference to the encapsulated data
     * @note same as value() as the encapsulated type is not a class
     */
    data_type& data() { return _data; }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * @note same as data() as the encapsulated type is not a class
     */
    const value_type& value() const { return data(); }
    /**
     * returns a non-const reference to the value() of the data encapsulated in the capsule.
     * @note same as data() as the encapsulated type is not a class
     */
    value_type& value() { return data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * @param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_convertible<data_type, ArgT>::value, bool> = true>
    bool operator==(const capsule<ArgT>& other) const { return _data == other.data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * @param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_convertible<data_type, ArgT>::value, bool> = true>
    bool operator!=(const capsule<ArgT>& other) const { return !operator==(other); }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * @param other data
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * @param other data
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * Sets data of the capsule
     * @param d data 
     */
    void set(const data_type& value) { _data = value; }
    /**
     * Conversion operator overload to convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * @param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * @param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * capsule for basic_string
 * @ingroup capsule
 */
template <typename CharT, typename Traits, typename Alloc>
class capsule<std::basic_string<CharT, Traits, Alloc>, true>{
    std::basic_string<CharT, Traits, Alloc> _data;
    public:
    /**
     * The encapsulated type is not a class. So key_type is void
     */
    typedef void key_type;
    /**
     * type of the value encapsulated by the capsule.
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef std::basic_string<CharT, Traits, Alloc> value_type;
    /**
     * type of the value encapsulated by the capsule.
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef std::basic_string<CharT, Traits, Alloc> data_type;
    /**
     * type used to index this capsule.
     * @note Same as data_type as the encapsulated type is not a class
     */
    typedef data_type index_type;
    
    /**
     * Default constructor
     */
    capsule() = default;
    /**
     * Construct through another capsule of data_type
     */
    capsule(const capsule&) = default;
    /**
     * Construct from a basic_string
     * @param str string
     */
    capsule(const data_type& str): _data(str){}
    /**
     * Construct through an object of data_type
     * @param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible<data_type, ArgT>::value, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * Assign another capsule, encapsulating same type of data.
     * @param other another capsule 
     */
    capsule& operator=(const capsule& other) {
        _data = other.data();
        return *this;
    }
    /**
     * returns a const reference to the encapsulated data
     * @note same as value()
     */
    const data_type& data() const { return _data; }
    /**
     * returns a non-const reference to the encapsulated data
     * @note same as value()
     */
    data_type& data() { return _data; }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * @note same as data()
     */
    const value_type& value() const { return data(); }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * @note same as data()
     */
    value_type& value() { return data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * @param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible<data_type, ArgT>::value, bool> = true>
    bool operator==(const capsule<ArgT>& other) const { return _data == other.data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * @param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible<data_type, ArgT>::value, bool> = true>
    bool operator!=(const capsule<ArgT>& other) const { return !operator==(other); }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * @param other data
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * @param other data
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * Sets data of the capsule
     * @param d data 
     */
    void set(const data_type& value) { _data = value; }
    /**
     * Conversion operator overload to convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * @param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * @param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * encapsulate a class which is not an element
 * @ingroup capsule
 */
template <typename DataT>
class capsule<DataT, true>: public encapsulate<DataT>{
    DataT _data;

    public:
    /**
     * type of data encapsulated within
     */
    typedef DataT data_type;
    /**
     * If the data_type has a key() method then uses that as key_type of the capsule. Otherwise void.
     */
    typedef typename encapsulate<DataT>::key_type key_type;
    /**
     * If the data_type has a value() method and a value_type datatype then uses that as value_type of the capsule. Otherwise value_type is same as data_type.
     */
    typedef typename encapsulate<DataT>::value_type value_type;
    /**
     * If data_type has an index_type datatype then uses that as index_type of the capsule. Otherwise void.
     */
    typedef typename encapsulate<DataT>::index_type index_type;
    
    static_assert(std::is_default_constructible<value_type>::value, "capsule<DataT> default constructor requires DataT::value_type to be default constructible");
    static_assert(std::is_copy_constructible<data_type>::value, "capsule<DataT> constructor requires DataT to be copy constructible");
    static_assert(std::is_constructible<data_type, value_type>::value, "capsule<DataT> default constructor requires DataT to be constructible through DataT::value_type");

    /**
     * Default constructor
     */
    capsule(): _data(value_type()) {}
    /**
     * Copy constructor
     */
    capsule(const capsule&) = default;
    /**
     * Construct with data
     * @param d data to be encapsulated
     */
    capsule(const data_type& d): _data(d){}
    /**
     * Construct through an object of data_type
     * @param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible<data_type, ArgT>::value, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * assign another capsule encapsulating the same type
     */
    capsule& operator=(const capsule& other) = default;
    /**
     * Get the data encapsulated within
     */
    const data_type& data() const { return _data; }
    /**
     * Get the data encapsulated within
     */
    data_type& data() { return _data; }
    /**
     * Get the value of the data encapsulated within
     */
    value_type& value() { return encapsulate<DataT>::value(data()); }
    /**
     * Get the value of the data encapsulated within
     */
    const value_type& value() const { return encapsulate<DataT>::value(data()); }
    /**
     * Compare with another capsule encapsulating the same type of data
     */
    bool operator==(const capsule& other) const { return _data == other._data; }
    /**
     * Compare with another capsule encapsulating the same type of data
     */
    bool operator!=(const capsule& other) const { return !operator==(other); }
    /**
     * Compare with a data of data_type
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Compare with a data of data_type
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * set data of the capsule
     * @param d data
     */
    void set(const data_type& d) { _data = d; }
    /**
     * Convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Apply a function on the data inside
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Apply a function on the data inside
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

#else
/**
 * @brief encapsulate an object of type DataT that is to be included in a node (internally used for seq and map)
 * - Expects `DataT` to be default constructible and copy constructible.
 * - Type `capsule<DataT>::data_type` is an alias of DataT
 * - The method `capsule<DataT>::data()` can be used to get the underlying `DataT` object.
 * - The method `capsule<DataT>::value()` can be used to get `DataT::value()` if `DataT` provides a `value_type` and `value()` method.
 * - If `DataT` provides a value_type and a `value()` method (preferrably a pair of const and non-const overloads) 
 *   - Then `capsule<DataT>::value_type` is an alias of DataT::value_type
 *   - Otherwise `capsule<DataT>::value_type` is an alias of `capsule<DataT>::data_type`
 *   .
 * - If `DataT` provides a static `key()` method
 *   - Then return type of `DataT::key()` is used as `capsule<DataT>::key_type` expecting `DataT::key()` returns compile time unique types for each item
 *   - Otherwise `capsule<DataT>::key_type` is an alias of void
 *   .
 * .
 * @tparam DataT data type 
 * @ingroup hazo
 */
template <typename DataT>
struct capsule<DataT>: private encapsulate<DataT>{
    /**
     * @brief type of data encapsulated within
     */
    typedef DataT data_type;
    /**
     * @brief A locally unique type associated with the type
     * - If the DataT provides a key() method 
     *   - then key_type of the capsule is same as the return type of DataT::key()
     *   . 
     * - Otherwise void
     * .
     * Generally a compile time string is associated as key e.g. boost::hana::string
     */
    typedef typename encapsulate<DataT>::key_type key_type;
    /**
     * @brief Type of value the compsule is containing
     * - If the DataT provides a value() method and a value_type typedef 
     *   - then uses that as value_type of the capsule
     *   .
     * - Otherwise data_type
     * .
     * @see data_type
     */
    typedef typename encapsulate<DataT>::value_type value_type;
    /**
     * @brief A locally unique type associated with the type
     * - If DataT provides an index_type datatype 
     *   - then uses that as index_type of the capsule
     *   .
     * - Otherwise void
     * .
     */
    typedef typename encapsulate<DataT>::index_type index_type;
    
    data_type _data;
    
    /**
     * @brief Default constructor
     */
    capsule(): _data(value_type()){};
    /**
     * @brief Copy constructor
     */
    capsule(const self_type&) = default;
    /**
     * @brief Construct with data
     * @param d data to be encapsulated
     */
    capsule(const data_type& d): _data(d){}
    /**
     * @brief assign another capsule encapsulating the same type
     */
    self_type& operator=(const self_type& other) = default;
    /**
     * @brief Get the data encapsulated within
     */
    const data_type& data() const { return _data; }
    /**
     * @brief Get the data encapsulated within
     */
    data_type& data() { return _data; }
    /**
     * @brief Get the value of the data encapsulated within
     */
    const value_type& value() const { return data().value(); }
    /**
     * @brief Get the value of the data encapsulated within
     */
    value_type& value() { return data().value(); }
    /**
     * @brief Compare with another capsule encapsulating the same type of data
     */
    bool operator==(const self_type& other) const { return _data == other._data; }
    /**
     * @brief Compare with another capsule encapsulating the same type of data
     */
    bool operator!=(const self_type& other) const { return !operator==(other); }
    /**
     * @brief Compare with a data of data_type
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * @brief Compare with a data of data_type
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * @brief set data of the capsule
     * @param d data
     */
    void set(const data_type& d) { _data = d; }
    /**
     * @brief Convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * @brief Apply a function on the data inside
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * @brief Apply a function on the data inside
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};
#endif

}
}

#endif // UDHO_HAZO_CAPSULE_H
