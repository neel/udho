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

#ifndef UDHO_FORMS_CONSTRAINTS_H
#define UDHO_FORMS_CONSTRAINTS_H

/**
 * @file
 * @brief Reusable value, string-length, membership, and byte-classification checks.
 *
 * Validators return a Boolean result and expose a configurable failure message.
 */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <set>
#include <string>
#include <type_traits>

namespace udho{
namespace forms{

namespace constraints{

/// @brief Common failure-message storage and fluent setters for a validator.
template <typename DerivedT>
struct basic_constrain{
    /// Concrete validator type returned by fluent setters.
    typedef DerivedT derived_type;

    /// @brief Store the message to report when validation fails.
    basic_constrain(const std::string& m): _message(m){}
    /// @brief Return this object as its concrete validator type.
    derived_type& self() { return static_cast<derived_type&>(*this); }
    /// @brief Replace the failure message and return this validator.
    derived_type& error(const std::string& m) { _message = m; return self(); }
    /// @brief Replace the failure message and return this validator.
    derived_type& message(const std::string& m) { _message = m; return self(); }
    /// @brief Return the configured failure message.
    const std::string& message() const { return _message; }
    private:
        std::string _message;
};

/// @brief Check a value against an inclusive lower bound; strings use byte length.
template <typename T>
struct gte: basic_constrain<gte<T>>{
    typedef basic_constrain<gte<T>> base; ///< Failure-message support for this validator.

    T _value; ///< Stored comparison value.

    /// @brief Store the lower bound and optional failure message.
    gte(const T& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input is greater than or equal to the bound.
    bool operator()(const T& input) const{ return input >= _value; }
};
/// @brief Check a value against an exclusive lower bound; strings use byte length.
template <typename T>
struct gt: basic_constrain<gt<T>>{
    typedef basic_constrain<gt<T>> base; ///< Failure-message support for this validator.

    T _value; ///< Stored comparison value.

    /// @brief Store the lower bound and optional failure message.
    gt(const T& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input is greater than the bound.
    bool operator()(const T& input) const{ return input > _value; }
};
/// @brief Check a value against an inclusive upper bound; strings use byte length.
template <typename T>
struct lte: basic_constrain<lte<T>>{
    typedef basic_constrain<lte<T>> base; ///< Failure-message support for this validator.

    T _value; ///< Stored comparison value.

    /// @brief Store the upper bound and optional failure message.
    lte(const T& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input is less than or equal to the bound.
    bool operator()(const T& input) const{ return input <= _value; }
};
/// @brief Check a value against an exclusive upper bound; strings use byte length.
template <typename T>
struct lt: basic_constrain<lt<T>>{
    typedef basic_constrain<lt<T>> base; ///< Failure-message support for this validator.

    T _value; ///< Stored comparison value.

    /// @brief Store the upper bound and optional failure message.
    lt(const T& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input is less than the bound.
    bool operator()(const T& input) const{ return input < _value; }
};
/// @brief Check value equality, including complete string contents.
template <typename T>
struct eq: basic_constrain<eq<T>>{
    typedef basic_constrain<eq<T>> base; ///< Failure-message support for this validator.

    T _value; ///< Stored comparison value.

    /// @brief Store the expected value and optional failure message.
    eq(const T& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input equals the expected value.
    bool operator()(const T& input) const{ return input == _value; }
};

/// @brief Check value inequality, including complete string contents.
template <typename T>
struct neq: basic_constrain<neq<T>>{
    typedef basic_constrain<neq<T>> base; ///< Failure-message support for this validator.

    T _value; ///< Stored comparison value.

    /// @brief Store the excluded value and optional failure message.
    neq(const T& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input differs from the excluded value.
    bool operator()(const T& input) const{ return input != _value; }
};

/**
 * @brief Check membership in a set of allowed values.
 *
 * In the variadic value constructor, every string argument is a choice.
 * Supply a message for string choices through the iterator-range or
 * initializer-list constructor, or call message() on the constructed validator.
 */
template <typename T>
struct in: basic_constrain<in<T>>{
    typedef basic_constrain<in<T>> base; ///< Failure-message support for this validator.

    std::set<T> _values; ///< Distinct allowed choices.

    /// @brief Copy choices from [begin, end) and store the supplied failure message.
    template <typename It, typename = typename std::iterator_traits<It>::iterator_category,
              std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<It>::value_type, T>, int> = 0>
    in(const std::string& message, It begin, It end): base(message){
        std::copy(begin, end, std::inserter(_values, _values.begin()));
    }
    /// @brief Store a message followed by choices; this overload excludes string values.
    template <typename... U, std::enable_if_t<!std::is_same_v<T, std::string> && (std::is_convertible_v<U, T> && ...), int> = 0>
    in(const std::string& message, const U&... args): base(message){
        std::initializer_list<T> values{static_cast<T>(args)...};
        std::copy(values.begin(), values.end(), std::inserter(_values, _values.begin()));
    }
    /// @brief Store the supplied choices with an empty failure message.
    template <typename... U, std::enable_if_t<(std::is_convertible_v<U, T> && ...), int> = 0>
    in(const U&... args): base(""){
        std::initializer_list<T> values{static_cast<T>(args)...};
        std::copy(values.begin(), values.end(), std::inserter(_values, _values.begin()));
    }
    /// @brief Store initializer-list choices and an optional failure message.
    in(std::initializer_list<T> values, const std::string& message = ""): base(message), _values(values){}
    /// @brief Return whether input occurs among the allowed choices.
    bool operator()(const T& input) const{
        return _values.count(input);
    }
};

/// @brief Require at least the specified number of bytes in a string.
template <>
struct gte<std::string>: basic_constrain<gte<std::string>>{
    typedef basic_constrain<gte<std::string>> base; ///< Failure-message support for this validator.

    std::size_t _value; ///< Stored string byte-length bound.

    /// @brief Store the minimum byte length and optional failure message.
    gte(const std::size_t& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input.size() meets the inclusive minimum.
    bool operator()(const std::string& input) const{ return input.size() >= _value; }
};

/// @brief Require more than the specified number of bytes in a string.
template <>
struct gt<std::string>: basic_constrain<gt<std::string>>{
    typedef basic_constrain<gt<std::string>> base; ///< Failure-message support for this validator.

    std::size_t _value; ///< Stored string byte-length bound.

    /// @brief Store the exclusive minimum byte length and optional failure message.
    gt(const std::size_t& value, const std::string& message = ""): base(message), _value(value) {}
    /// @brief Return whether input.size() exceeds the minimum.
    bool operator()(const std::string& input) const{ return input.size() > _value; }
};
/// @brief Require at most the specified number of bytes in a string.
template <>
struct lte<std::string>: basic_constrain<lte<std::string>>{
    typedef basic_constrain<lte<std::string>> base; ///< Failure-message support for this validator.

    std::size_t _value; ///< Stored string byte-length bound.

    /// @brief Store the maximum byte length and optional failure message.
    lte(const std::size_t& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input.size() meets the inclusive maximum.
    bool operator()(const std::string& input) const{ return input.size() <= _value; }
};
/// @brief Require fewer than the specified number of bytes in a string.
template <>
struct lt<std::string>: basic_constrain<lt<std::string>>{
    typedef basic_constrain<lt<std::string>> base; ///< Failure-message support for this validator.

    std::size_t _value; ///< Stored string byte-length bound.

    /// @brief Store the exclusive maximum byte length and optional failure message.
    lt(const std::size_t& value, const std::string& message = ""): base(message), _value(value){}
    /// @brief Return whether input.size() is below the maximum.
    bool operator()(const std::string& input) const{ return input.size() < _value; }
};

/**
 * @brief Require every byte to pass std::isdigit; empty strings pass.
 * @note Uses C character classification without decoding Unicode.
 */
struct all_digits: basic_constrain<all_digits>{
    typedef basic_constrain<all_digits> base; ///< Failure-message support for this validator.

    /// @brief Store an optional failure message.
    all_digits(std::string message = ""): base(message){}
    /// @brief Return whether all unsigned bytes are classified as digits.
    inline bool operator()(const std::string& value) const{
        for(auto it = value.begin(); it != value.end(); ++it){
            char c = *it;
            if(!std::isdigit(static_cast<unsigned char>(c))){
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief Reject bytes classified as whitespace by std::isspace; empty strings pass.
 * @note Uses the current C locale's character classification without decoding Unicode.
 */
struct no_space: basic_constrain<no_space>{
    typedef basic_constrain<no_space> base; ///< Failure-message support for this validator.

    /// @brief Store an optional failure message.
    no_space(std::string message = ""): base(message){}
    /// @brief Return whether no unsigned byte is classified as whitespace.
    inline bool operator()(const std::string& value) const{
        for(auto it = value.begin(); it != value.end(); ++it){
            char c = *it;
            if(std::isspace(static_cast<unsigned char>(c))){
                return false;
            }
        }
        return true;
    }
};

/// @brief Inclusive minimum value, or minimum byte length for strings.
template <typename T>
using min = gte<T>;
/// @brief Inclusive maximum value, or maximum byte length for strings.
template <typename T>
using max = lte<T>;

/// Inclusive minimum string byte length.
typedef gte<std::string>    length_gte;
/// Exclusive minimum string byte length.
typedef gt<std::string>     length_gt;
/// Inclusive maximum string byte length.
typedef lte<std::string>    length_lte;
/// Exclusive maximum string byte length.
typedef lt<std::string>     length_lt;
/// Alias for the inclusive minimum string byte length.
typedef length_gte          length_min;
/// Alias for the inclusive maximum string byte length.
typedef length_lte          length_max;

/// @brief Require an exact string byte length.
struct length_eq: basic_constrain<length_eq>{
    typedef basic_constrain<length_eq> base; ///< Failure-message support for this validator.

    std::size_t _length; ///< Required string byte length.

    /// @brief Store the required byte length and optional failure message.
    length_eq(std::size_t length, std::string message = ""): base(message), _length(length){}
    /// @brief Return whether value.length() equals the required byte length.
    inline bool operator()(const std::string& value) const{
        return value.length() == _length;
    }
};

}

}
}

#endif // UDHO_FORMS_CONSTRAINTS_H
