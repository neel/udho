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

#ifndef UDHO_VIEW_NVP_H
#define UDHO_VIEW_NVP_H

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/seq/seq.h>
#include <udho/view/data/fwd.h>
#include <udho/url/detail/function.h>

namespace udho{
namespace view{
namespace data{

template <typename X>
struct member_variable;

template <typename X>
struct const_member_function;

template <typename X>
struct member_function;

template <typename Class, typename T>
struct member_variable<T Class::*>{
    using member_type = T Class::*;
    using result_type = T;
    using class_type  = Class;

    constexpr static bool is_const() { return false; }

    member_variable(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    private:
        member_type _member;
};

template <typename Class, typename Res, typename... X>
struct const_member_function<Res (Class::*)(X...) const>{
    using member_type = Res (Class::*)(X...) const;
    using result_type = Res;
    using class_type  = Class;
    using function    = udho::url::detail::function_signature_<member_type>;

    constexpr static bool is_const() { return true; }

    const_member_function(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    private:
        member_type _member;
};

template <typename Class, typename Res, typename... X>
struct member_function<Res (Class::*)(X...)>{
    using member_type = Res (Class::*)(X...);
    using result_type = Res;
    using class_type  = Class;
    using function    = udho::url::detail::function_signature_<member_type>;

    constexpr static bool is_const() { return false; }

    member_function(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    private:
        member_type _member;
};

template <typename U>
struct getter_value: const_member_function<U>{
    using const_member_function<U>::const_member_function;
};

template <typename V>
struct setter_value: member_function<V>{
    using member_function<V>::member_function;
};

namespace detail{

template <typename T>
struct wrapper1;

template <typename Class, typename T>
struct wrapper1<T Class::*>: member_variable<T Class::*> {
    using member_variable<T Class::*>::member_variable;
};

template <typename Res, typename Class, typename... Args>
struct wrapper1<Res (Class::*)(Args...)>: member_function<Res (Class::*)(Args...)> {
    using member_function<Res (Class::*)(Args...)>::member_function;
};

template <typename Res, typename Class, typename... Args>
struct wrapper1<Res (Class::*)(Args...) const>: const_member_function<Res (Class::*)(Args...) const> {
    using const_member_function<Res (Class::*)(Args...) const>::const_member_function;
};

template <typename U, typename V>
struct wrapper2;

template <typename Class, typename U, typename V, typename Res>
struct wrapper2<U (Class::*)() const, Res (Class::*)(V)>: getter_value<U (Class::*)() const>, setter_value<Res (Class::*)(V)> {
    wrapper2(U (Class::*u)() const, Res (Class::*v)(V))
        : getter_value<U (Class::*)() const>    (std::move(u)),
          setter_value<Res (Class::*)(V)>       (std::move(v)) {}

    using getter_type = getter_value<U (Class::*)() const>;
    using setter_type = setter_value<Res (Class::*)(V)>;
    using result_type = U;

    getter_type& getter() { return *this; }
    setter_type& setter() { return *this; }
};

}

template <typename... X>
struct wrapper;

template <typename T>
struct wrapper<T>: detail::wrapper1<T>{
    using detail::wrapper1<T>::wrapper1;
};

template <typename U, typename V>
struct wrapper<U, V>: detail::wrapper2<U, V>{
    using detail::wrapper2<U, V>::wrapper2;
};

template <typename... X>
wrapper<X...> wrap(X&&... x){
    return wrapper<X...>{std::forward<X>(x)...};
}

namespace detail{
    template <typename T>
    class wrappable{
        template <typename Dummy = wrapper<T>>
        static constexpr bool check(int){ return true; }
        static constexpr bool check(char){ return false; }
        // static constexpr bool check(char a){ return check_foreign(a); }
        // template <typename Dummy = decltype(foreign(declval<T>()))>
        // static constexpr bool check_foreign(int){ return true; }
        // static constexpr bool check_foreign(char a){ return false; }
        static constexpr bool check() { return check(42); }
        public:
            using type = std::integral_constant<bool, check()>;
    };
}

template <typename T>
using is_wrappable = typename detail::wrappable<T>::type;

template<class T>
inline constexpr bool is_wrappable_v = is_wrappable<T>::value;

namespace policies{
    struct readonly{};
    struct writable{};
    struct functional{};

    template <typename T>
    struct property{};

    struct function{};
}

template <typename PolicyT, typename KeyT, typename... X>
struct nvp<PolicyT, KeyT, wrapper<X...>>{
    using policy_type           = PolicyT;
    using name_type             = KeyT;
    using wrapper_type          = wrapper<X...>;

    nvp(name_type&& name, wrapper_type&& wrapper): _name(std::move(name)), _wrapper(std::forward<wrapper_type>(wrapper)) {}
    const name_type& name() const { return _name; }
    wrapper_type& value() { return _wrapper; }

    private:
        name_type    _name;
        wrapper_type _wrapper;
};


/**
 * @brief make name value pair of a reference (or a ...)
 * @tparam name string name
 * @tparam v value
 *
 */
template <typename P, typename K, typename... X>
nvp<P, K, wrapper<X...>> make_nvp(P, K&& name, X&&... v){
    return nvp<P, K, wrapper<X...>>(std::move(name), wrap(std::forward<X>(v)...));
}

template <typename K, typename... X>
nvp< policies::property<policies::writable>, K, wrapper<X...> > mvar(K&& name, X&&... v){
    return make_nvp(policies::property<policies::writable>{}, std::forward<K>(name), std::forward<X>(v)...);
}

template <typename K, typename... X>
nvp< policies::property<policies::readonly>, K, wrapper<X...> > cvar(K&& name, X&&... v){
    return make_nvp(policies::property<policies::readonly>{}, std::forward<K>(name), std::forward<X>(v)...);
}

template <typename K, typename... X>
nvp< policies::property<policies::functional>, K, wrapper<X...> > fvar(K&& name, X&&... v){
    return make_nvp(policies::property<policies::functional>{}, std::forward<K>(name), std::forward<X>(v)...);
}

template <typename K, typename... X>
nvp< policies::function, K, wrapper<X...> > func(K&& name, X&&... v){
    return make_nvp(policies::function{}, std::forward<K>(name), std::forward<X>(v)...);
}


}
}
}

#endif // UDHO_VIEW_NVP_H
