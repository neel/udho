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

#ifndef UDHO_VIEW_INTEROP_H
#define UDHO_VIEW_INTEROP_H

#include <string>

namespace udho{
namespace view{

struct python{};


namespace data{

template <typename Bridge>
struct bridge;

template <typename Bridge, typename T>
struct foreign;

template <>
struct bridge<udho::view::python>{};

namespace bridges{

    using python = udho::view::data::bridge<udho::view::python>;

}

template <typename Bridge>
struct generator;

template <typename Bridge, typename T>
struct reflection;

}

namespace detail{

template <typename Bridge, typename T>
class specialized{
    template <class Arg, class Dummy = decltype(udho::view::data::reflection<Bridge, Arg>{})>
    static constexpr bool exists(int) { return true; }
    template <class Arg>
    static constexpr bool exists(char) { return false; }
    public:
        static constexpr bool value = exists<T>(42);
};

template <typename Bridge, typename T, bool Specialized = specialized<Bridge, T>::value >
struct reflect_internal;

template <typename Bridge, typename T>
struct reflect_internal<Bridge, T, true>{
    static decltype(auto) reflect(T&& value) {
        udho::view::data::reflection<Bridge, T> reflection;
        return reflection(std::forward<T>(value));
    }
};

template <typename Bridge, typename T>
struct reflect_internal<Bridge, T, false>{
    static decltype(auto) reflect(T&& value) {
        // Not specialized. Send it directly to the bridge
        using wrapper_type = udho::view::data::foreign<Bridge, T>;
        return wrapper_type{std::forward<T>(value)};
    }
};

template <typename Bridge, typename T>
struct resolver{
    using value_type   = T;

    resolver(value_type v): _value(v) {}

    template <typename Binder>
    decltype(auto) resolve(Binder& binder) const {
        auto reflected = reflect(udho::view::data::bridge<Bridge>{}, std::forward<T>(_value));
        return binder(reflected);
    }

    value_type   _value;
};

template <typename Bridge, typename T>
struct binding{

    using value_type   = T;

    binding(value_type v): _value(v) {}

    template <typename Binder>
    decltype(auto) resolve(Binder& binder, const std::string& name) {
        // binds (_name, _value) with the binder and return an appropriate value
        // `decltype(auto)` is used here as a placeholder. However it can vary depending
        // on which library is being used to generate bindings.
        using resolver_type = resolver<Bridge, T>;

        resolver_type resolver(_value);
        return resolver.resolve(binder);
    }

    value_type _value;
};

/**
 * `named_binding` does not need Bridge template argument, because the name part is common
 * for all foreign language and the ultimate bindings are provided by the binding template
 */
template <typename BindingT>
struct named_binding{
    using binding_type = BindingT;

    named_binding(const std::string& name, binding_type&& binding): _name(std::move(name)), _binding(std::move(binding)) {}

    template <typename Binder>
    decltype(auto) resolve(Binder& binder) { return _binding.resolve(binder, _name); }

    const std::string& name() const { return _name; }

    std::string  _name;
    binding_type _binding;
};

}

namespace data{

template <typename Bridge, typename T>
decltype(auto) reflect(udho::view::data::bridge<Bridge> bridge, const std::string& name, T&& value){
    detail::binding<Bridge, T> binding{std::forward<T>(value)};
    return detail::named_binding<detail::binding<Bridge, T>>{name, std::move(binding)};
}

template <typename Bridge, typename T>
decltype(auto) reflect(udho::view::data::bridge<Bridge> bridge, T&& value){
    return detail::reflect_internal<Bridge, T>::reflect(std::forward<T>(value));
}

}


}
}

#endif // UDHO_VIEW_INTEROP_H
