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
    
#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace detail{
    template <typename DataT, typename ValueT = DataT>
    struct _capsule{
        typedef DataT value_type;
        typedef ValueT data_type;
        static inline constexpr bool is_valid_v = std::is_default_constructible_v<DataT> && std::is_constructible_v<DataT, ValueT> && std::is_assignable_v<DataT, ValueT>;
        template <typename ArgT>
        using is_constructible = std::integral_constant<bool, std::is_constructible_v<ValueT, ArgT> || std::is_constructible_v<DataT, ArgT> >;
        template <typename ArgT>
        static inline constexpr bool is_constructible_v = is_constructible<ArgT>::value;
        template <typename ArgT>
        using is_assignable = std::integral_constant<bool, std::is_assignable_v<ValueT, ArgT> || std::is_assignable_v<DataT, ArgT> >;
        template <typename ArgT>
        static inline constexpr bool is_assignable_v = is_assignable<ArgT>::value;
    };
    template <typename DataT>
    struct _capsule<DataT, DataT>{
        typedef DataT value_type;
        typedef DataT data_type;
        static inline constexpr bool is_valid_v = std::is_default_constructible_v<DataT>;
        template <typename ArgT>
        using is_constructible =  std::is_constructible<DataT, ArgT>;
        template <typename ArgT>
        static inline constexpr bool is_constructible_v = is_constructible<ArgT>::value;
        template <typename ArgT>
        using is_assignable =  std::is_assignable<DataT, ArgT>;
        template <typename ArgT>
        static inline constexpr bool is_assignable_v = is_assignable<ArgT>::value;
    };
}

/**
 * capsule for plain old types 
 * \ingroup capsule
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
     * \note Same as data_type as the encapsulated type is not a class
     */
    typedef DataT value_type;
    /**
     * type of the object encapsulated by the capsule.
     * \note Same as value_type as the encapsulated type is not a class
     */
    typedef DataT data_type;
    /**
     * type used to index this capsule.
     * \note Same as data_type as the encapsulated type is not a class
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
     * \param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible_v<data_type, ArgT>, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * Assign another capsule, encapsulating same type of data.
     * \param other another capsule 
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
    template <typename ArgT, std::enable_if_t<std::is_assignable_v<data_type, ArgT>, bool> = true>
    capsule& operator=(const ArgT& other) {
        _data = other.data();
        return *this;
    }
    /**
     * returns a const reference to the encapsulated data
     * \note same as value() as the encapsulated type is not a class
     */
    const data_type& data() const { return _data; }
    /**
     * returns a non-const reference to the encapsulated data
     * \note same as value() as the encapsulated type is not a class
     */
    data_type& data() { return _data; }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * \note same as data() as the encapsulated type is not a class
     */
    const value_type& value() const { return data(); }
    /**
     * returns a non-const reference to the value() of the data encapsulated in the capsule.
     * \note same as data() as the encapsulated type is not a class
     */
    value_type& value() { return data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same type of data.
     * \param other another capsule
     */
    bool operator==(const capsule& other) const { return _data == other.data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same type of data.
     * \param other another capsule
     */
    bool operator!=(const capsule& other) const { return !operator==(other); }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * \param other data
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * \param other data
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * Sets data of the capsule
     * \param d data 
     */
    void set(const data_type& d) { _data = d; }
    /**
     * Conversion operator overload to convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * \param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * \param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * capsule for C style string literal
 * \ingroup capsule
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
     * \note Same as data_type as the encapsulated type is not a class
     */
    typedef std::string value_type;
    /**
     * type of the object encapsulated by the capsule (using std::string instead of char*).
     * \note Same as value_type as the encapsulated type is not a class
     */
    typedef std::string data_type;
    /**
     * type used to index this capsule.
     * \note Same as data_type as the encapsulated type is not a class
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
     * \param str C string literal
     */
    capsule(const char* str): _data(str){}
    /**
     * Construct through an object of data_type
     * \param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible_v<data_type, ArgT>, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * Assign another capsule, encapsulating same type of data.
     * \param other another capsule 
     */
    capsule& operator=(const capsule& other) {
        _data = other.data();
        return *this;
    }
    /**
     * returns a const reference to the encapsulated data
     * \note same as value() as the encapsulated type is not a class
     */
    const data_type& data() const { return _data; }
    /**
     * returns a non-const reference to the encapsulated data
     * \note same as value() as the encapsulated type is not a class
     */
    data_type& data() { return _data; }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * \note same as data() as the encapsulated type is not a class
     */
    const value_type& value() const { return data(); }
    /**
     * returns a non-const reference to the value() of the data encapsulated in the capsule.
     * \note same as data() as the encapsulated type is not a class
     */
    value_type& value() { return data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * \param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_convertible_v<data_type, ArgT>, bool> = true>
    bool operator==(const capsule<ArgT>& other) const { return _data == other.data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * \param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_convertible_v<data_type, ArgT>, bool> = true>
    bool operator!=(const capsule<ArgT>& other) const { return !operator==(other); }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * \param other data
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * \param other data
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * Sets data of the capsule
     * \param d data 
     */
    void set(const data_type& value) { _data = value; }
    /**
     * Conversion operator overload to convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * \param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * \param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * capsule for basic_string
 * \ingroup capsule
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
     * \note Same as data_type as the encapsulated type is not a class
     */
    typedef std::basic_string<CharT, Traits, Alloc> value_type;
    /**
     * type of the value encapsulated by the capsule.
     * \note Same as data_type as the encapsulated type is not a class
     */
    typedef std::basic_string<CharT, Traits, Alloc> data_type;
    /**
     * type used to index this capsule.
     * \note Same as data_type as the encapsulated type is not a class
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
     * \param str string
     */
    capsule(const data_type& str): _data(str){}
    /**
     * Construct through an object of data_type
     * \param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible_v<data_type, ArgT>, bool> = true>
    capsule(const ArgT& d): _data(d){}
    /**
     * Assign another capsule, encapsulating same type of data.
     * \param other another capsule 
     */
    capsule& operator=(const capsule& other) {
        _data = other.data();
        return *this;
    }
    /**
     * returns a const reference to the encapsulated data
     * \note same as value()
     */
    const data_type& data() const { return _data; }
    /**
     * returns a non-const reference to the encapsulated data
     * \note same as value()
     */
    data_type& data() { return _data; }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * \note same as data()
     */
    const value_type& value() const { return data(); }
    /**
     * returns a const reference to the value() of the data encapsulated in the capsule.
     * \note same as data()
     */
    value_type& value() { return data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * \param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible_v<data_type, ArgT>, bool> = true>
    bool operator==(const capsule<ArgT>& other) const { return _data == other.data(); }
    /**
     * Comparison operator overload to compare with another capsule, encapsulating same or convertible type of data.
     * \param other another capsule
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible_v<data_type, ArgT>, bool> = true>
    bool operator!=(const capsule<ArgT>& other) const { return !operator==(other); }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * \param other data
     */
    bool operator==(const data_type& other) const { return _data == other; }
    /**
     * Comparison operator overload to compare with an object of data_type.
     * \param other data
     */
    bool operator!=(const data_type& other) const { return !operator==(_data, other); }
    /**
     * Sets data of the capsule
     * \param d data 
     */
    void set(const data_type& value) { _data = value; }
    /**
     * Conversion operator overload to convert to data_type
     */
    operator data_type() const { return _data; }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * \param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) const {
        return std::forward<FunctionT>(f)(_data);
    }
    /**
     * Calls a function with the encapsulated data and returns the returned output of that function
     * \param f function
     */
    template <typename FunctionT>
    auto call(FunctionT&& f) {
        return std::forward<FunctionT>(f)(_data);
    }
};

/**
 * encapsulate a class which is not an element
 * \ingroup capsule
 */
template <typename DataT>
class capsule<DataT, true>: encapsulate<DataT>{
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
    
    /**
     * Default constructor
     */
    capsule(): _data(value_type()){};
    /**
     * Copy constructor
     */
    capsule(const capsule&) = default;
    /**
     * Construct with data
     * \param d data to be encapsulated
     */
    capsule(const data_type& d): _data(d){}
    /**
     * Construct through an object of data_type
     * \param d data to be encapsulated 
     */
    template <typename ArgT, std::enable_if_t<std::is_constructible_v<data_type, ArgT>, bool> = true>
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
    const value_type& value() const { return data().value(); }
    /**
     * Get the value of the data encapsulated within
     */
    value_type& value() { return data().value(); }
    /**
     * Compare with another capsule encapsulating the same type of data
     */
    bool operator==(const capsule& other) const { return _data == other._head; }
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
     * \param d data
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
 * encapsulate a class which is not an element
 * \ingroup capsule
 */
template <typename DataT>
struct capsule<DataT>{
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
     * Default constructor
     */
    capsule(): _data(value_type()){};
    /**
     * Copy constructor
     */
    capsule(const self_type&) = default;
    /**
     * Construct with data
     * \param d data to be encapsulated
     */
    capsule(const data_type& d): _data(d){}
    /**
     * assign another capsule encapsulating the same type
     */
    self_type& operator=(const self_type& other) = default;
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
    const value_type& value() const { return data().value(); }
    /**
     * Get the value of the data encapsulated within
     */
    value_type& value() { return data().value(); }
    /**
     * Compare with another capsule encapsulating the same type of data
     */
    bool operator==(const self_type& other) const { return _data == other._head; }
    /**
     * Compare with another capsule encapsulating the same type of data
     */
    bool operator!=(const self_type& other) const { return !operator==(other); }
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
     * \param d data
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
#endif

}
}
}

#endif // UDHO_HAZO_CAPSULE_H
