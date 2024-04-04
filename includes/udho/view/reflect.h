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

#ifndef UDHO_VIEW_REFLECT_H
#define UDHO_VIEW_REFLECT_H

#include <string>
#include <utility>
#include <type_traits>

namespace udho{
namespace view{
namespace data{

template <typename T, typename NameT = void, typename Enable = void>
struct value_wrapper;

template <typename T, typename NameT>
struct value_wrapper<T, NameT, std::enable_if_t<std::is_reference_v<T> && std::is_arithmetic_v<std::remove_reference_t<T>> > >{
    using name_type = NameT;
    using value_type = T;
    using reference_type = std::add_lvalue_reference_t<T>;
    using const_reference_type = std::add_const_t<std::add_lvalue_reference_t<T>>;

    value_wrapper(reference_type value): _value(value) {}

    name_type name() const {
        const static name_type name;
        return name;
    }

    reference_type value() { return _value; }
    const_reference_type value() const { return _value; }

    value_type operator*() const { return value(); }

    template <typename ValueT>
    value_wrapper& operator=(ValueT&& v){
        _value = std::forward<ValueT>(v);
        return *this;
    }

    template <typename SomeNameT>
    value_wrapper<T&, SomeNameT> as(SomeNameT&&) { return value_wrapper<T&, SomeNameT>{_value}; }

    private:
        reference_type _value;
};

template <typename T>
struct reflection;

template <typename T>
class specialized{
    template <class Arg, class Dummy = decltype(reflection<Arg>{})>
    static constexpr bool exists(int) { return true; }
    template <class Arg>
    static constexpr bool exists(char) { return false; }
    public:
        static constexpr bool value = exists<T>(42);
};

template <typename T, bool Specialized>
struct reflection_internal;

template <typename T>
struct reflection_internal<T, false>{
    static auto reflect(T& v){
        return value_wrapper<T&, void>{v};
    }
};

template <typename T>
struct reflection_internal<T, true>{
    static auto reflect(T& v){
        reflection<T> rt;
        return rt(v);
    }
};

template <typename T>
auto reflect_value(T& v){
    return reflection_internal<T, specialized<T>::value>::reflect(v);
}

template <typename T>
auto reflect(T& v){
    return reflect_value(v);
}

template <typename NameT, typename T>
auto reflect(NameT&& name, T& v){
    return reflect(v).as(std::forward<NameT>(name));
}

}
}
}

#endif // UDHO_VIEW_REFLECT_H
