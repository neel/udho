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
#include <cassert>
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
    using value_type  = std::add_lvalue_reference_t<result_type>;

    constexpr static bool is_const() { return false; }

    member_variable(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    result_type& get(Class& d) { return d.*_member; }
    const result_type& get(const Class& d) const { return d.*_member; }
    template <typename V, std::enable_if_t< std::is_assignable_v<std::add_lvalue_reference_t<result_type>, V>, int> = 0>
    bool set(Class& d, const V& v) { d.*_member = v; return true; }
    template <typename V, std::enable_if_t< !std::is_assignable_v<std::add_lvalue_reference_t<result_type>, V>, int> = 0>
    bool set(Class&, const V&) { return false; }

    private:
        member_type _member;
};


template <typename Class, typename Res, typename... X>
struct const_member_function<Res (Class::*)(X...) const>{
    using member_type = Res (Class::*)(X...) const;
    using result_type = Res;
    using class_type  = Class;
    using function    = udho::url::detail::function_signature_<member_type>;
    using args_type   = typename function::decayed_arguments_type;
    using value_type  = result_type;

    constexpr static bool is_const() { return true; }

    const_member_function(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    result_type call(Class& d, const args_type& args){
        return std::apply(_member, std::tuple_cat(std::make_tuple(std::ref(d)), args));
    }

    result_type call(const Class& d, const args_type& args){
        return std::apply(_member, std::tuple_cat(std::make_tuple(std::ref(d)), args));
    }

    private:
        member_type _member;
};

template <typename Class, typename Res, typename... X>
struct member_function<Res (Class::*)(X...)>{
    using member_type = Res (Class::*)(X...);
    using result_type = Res;
    using class_type  = Class;
    using function    = udho::url::detail::function_signature_<member_type>;
    using args_type   = typename function::decayed_arguments_type;
    using value_type  = result_type;

    constexpr static bool is_const() { return false; }

    member_function(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    result_type call(Class& d, const args_type& args){
        return std::apply(_member, std::tuple_cat(std::make_tuple(std::ref(d)), args));
    }

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
    using arg_type    = V;
    using value_type  = result_type;

    getter_type& getter() { return *this; }
    setter_type& setter() { return *this; }

    typename getter_type::result_type get(Class& d) { return getter().call(d, std::tuple<>{}); }
    typename getter_type::result_type get(const Class& d) { return getter().call(d, std::tuple<>{}); }
    template <typename ValueT, std::enable_if_t< std::is_assignable_v<std::add_lvalue_reference_t<std::decay_t<V>>, ValueT>, int> = 0>
    bool set(Class& d, const ValueT& v) {
        std::tuple<ValueT> arg_tuple{v};
        setter().call(d, arg_tuple);
        return true;
    }
    template <typename ValueT, std::enable_if_t< !std::is_assignable_v<std::add_lvalue_reference_t<std::decay_t<V>>, ValueT>, int> = 0>
    bool set(Class&, const ValueT&) { return false; }
};

}


/**
 * @brief Provides a mechanism to wrap member variables and functions for runtime access.
 *
 * The `wrapper` templates facilitate the creation of wrapped member variables and functions, enabling operations such as getting, setting, and invoking functions.
 *
 * @tparam X Variadic template parameters defining the types to be wrapped.
 */
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

/**
 * @brief Constructs a wrapper object from given member variables or functions.
 *
 * @param x Variadic template parameters defining the member variables or functions to be wrapped.
 * @return A wrapper object encapsulating the provided members.
 * @tparam X The types of the member variables or functions to wrap.
 */
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

/**
 * @namespace policies
 * @brief Contains policy classes and templates for defining and checking property characteristics in a type-safe manner.
 *
 * This namespace provides a set of structures and type traits that help in defining and querying the characteristics
 * of properties, such as whether they are read-only, writable, or functional.
 */
namespace policies{
    /**
     * @struct readonly
     * @brief Indicates that a property is read-only.
     */
    struct readonly{};
    /**
     * @struct writable
     * @brief Indicates that a property can be written to.
     */
    struct writable{};
    /**
     * @struct functional
     * @brief Indicates that a property represents a functional aspect, typically allowing both read and write operations.
     */
    struct functional{};

    /**
     * @struct property
     * @brief Generic template struct for defining a property with a specific access policy.
     *
     * @tparam T The policy type determining the nature of the property (e.g., readonly, writable, functional).
     */
    template <typename T>
    struct property{};

    /**
     * @typedef rprop
     * @brief Represents a read-only property.
     */
    using rprop = property<readonly>;
    /**
     * @typedef wprop
     * @brief Represents a writable property.
     */
    using wprop = property<writable>;
    /**
     * @typedef fprop
     * @brief Represents a functional property that may involve more complex behaviors like a combination of reading and writing.
     */
    using fprop = property<functional>;


    /**
     * @typedef is_readable_property
     * @brief Determines whether a given property type is readable.
     *
     * This template uses a combination of std::is_same to evaluate if the policy type is either read-only, writable, or functional.
     *
     * @tparam PolicyT The policy type to check.
     */
    template <typename PolicyT>
    using is_readable_property = std::integral_constant<bool, std::is_same<PolicyT, rprop>::value || std::is_same_v<PolicyT, wprop> || std::is_same_v<PolicyT, fprop>>;

    /**
     * @typedef is_writable_property
     * @brief Determines whether a given property type is writable.
     *
     * This template checks if the policy type supports write operations, applicable for writable and functional properties.
     *
     * @tparam PolicyT The policy type to check.
     */
    template <typename PolicyT>
    using is_writable_property = std::integral_constant<bool, std::is_same<PolicyT, fprop>::value || std::is_same_v<PolicyT, wprop>>;

    /**
     * @var is_readable_property_v
     * @brief Helper variable template to simplify the usage of is_readable_property.
     *
     * @tparam PolicyT The policy type to check for readability.
     */
    template<typename PolicyT>
    inline constexpr bool is_readable_property_v = is_readable_property<PolicyT>::value;

    /**
     * @var is_writable_property_v
     * @brief Helper variable template to simplify the usage of is_writable_property.
     *
     * @tparam PolicyT The policy type to check for writability.
     */
    template<typename PolicyT>
    inline constexpr bool is_writable_property_v = is_writable_property<PolicyT>::value;

    /**
     * @struct function
     * @brief Represents a property that encapsulates a function.
     */
    struct function{};
}


/**
 * @struct nvp
 * @brief Represents a name-value pair where the value is a wrapped entity, governed by a specific policy.
 *
 * This template struct is used to associate a name (key) with a wrapper that encapsulates some properties, possibly of a class,
 * including member variables or functions. It is essential for creating mappable properties that can be managed or accessed at runtime.
 *
 * @tparam PolicyT The policy type determining the accessibility or behavior of the property (e.g., readonly, writable, functional).
 * @tparam KeyT The type representing the key or name associated with the property.
 * @tparam X Variadic template parameters defining the types encapsulated by the wrapper.
 */
template <typename PolicyT, typename KeyT, typename... X>
struct nvp<PolicyT, KeyT, wrapper<X...>>{
    using policy_type           = PolicyT;          ///< The policy type that dictates the accessibility or usage of the property.
    using name_type             = KeyT;             ///< The type of the key or name.
    using wrapper_type          = wrapper<X...>;    ///< The wrapper type that encapsulates the actual property.

    /**
     * @brief Constructs a name-value pair with a name and a wrapped value.
     *
     * This constructor initializes the name-value pair by moving the given name and wrapper.
     *
     * @param name The name of the property being wrapped.
     * @param wrapper The wrapper object encapsulating the property.
     */
    nvp(name_type&& name, wrapper_type&& wrapper): _name(std::move(name)), _wrapper(std::forward<wrapper_type>(wrapper)) {}
    /**
     * @brief Retrieves the name of the property.
     *
     * Provides read-only access to the name of the property encapsulated by this name-value pair.
     *
     * @return A constant reference to the name of the property.
     */
    const name_type& name() const { return _name; }
    /**
     * @brief Retrieves the wrapped value of the property.
     *
     * This function provides modifiable access to the wrapped value, allowing operations defined by the wrapper
     * to be performed directly on the wrapped property.
     *
     * @return A reference to the wrapper encapsulating the property.
     */
    wrapper_type& value() { return _wrapper; }

    private:
        name_type    _name;
        wrapper_type _wrapper;
};


/**
 * @brief Creates a name-value pair for a member variable or member function.
 *
 * This function template assists in creating a name-value pair for properties, which can then be used for named properties.
 *
 * @param name The name of the property.
 * @param v The property value, which could be a member variable or function.
 * @tparam P The policy type under which the property is being created.
 * @tparam K The type of the key or name of the property.
 * @tparam X Variadic template parameters defining the types encapsulated by the wrapper.
 * @return A name-value pair encapsulating the property.
 */
template <typename P, typename K, typename... X>
nvp<P, K, wrapper<X...>> make_nvp(P, K&& name, X&&... v){
    return nvp<P, K, wrapper<X...>>(std::move(name), wrap(std::forward<X>(v)...));
}

/**
 * @brief Convenience function to encapsulate a member variable as mutable property.
 *
 * @param name The name of the property.
 * @param v a member variable.
 * @tparam K The type of the key or name of the property.
 * @tparam X Variadic template parameters defining the types encapsulated by the wrapper.
 * @return A name-value pair with a writable policy.
 */
template <typename K, typename... X>
nvp< policies::property<policies::writable>, K, wrapper<X...> > mvar(K&& name, X&&... v){
    return make_nvp(policies::property<policies::writable>{}, std::forward<K>(name), std::forward<X>(v)...);
}

/**
 * @brief Convenience function to encapsulate a member variable as constant property.
 *
 * @param name The name of the property.
 * @param v a member variable.
 * @tparam K The type of the key or name of the property.
 * @tparam X Variadic template parameters defining the types encapsulated by the wrapper.
 * @return A name-value pair with a readonly policy.
 */
template <typename K, typename... X>
nvp< policies::property<policies::readonly>, K, wrapper<X...> > cvar(K&& name, X&&... v){
    return make_nvp(policies::property<policies::readonly>{}, std::forward<K>(name), std::forward<X>(v)...);
}

/**
 * @brief Convenience function for encapsulate a pair of getter and setter as mutable property.
 *
 * @param name The name of the property.
 * @param v a member function of a class.
 * @tparam K The type of the key or name of the property.
 * @tparam X Variadic template parameters defining the types encapsulated by the wrapper.
 * @return A name-value pair with a functional policy.
 */
template <typename K, typename... X>
nvp< policies::property<policies::functional>, K, wrapper<X...> > fvar(K&& name, X&&... v){
    return make_nvp(policies::property<policies::functional>{}, std::forward<K>(name), std::forward<X>(v)...);
}

/**
 * @brief Convenience function to encapsulate a member function.
 *
 * @param name The name of the function.
 * @param v a member function of a class.
 * @tparam K The type of the key or name of the property.
 * @tparam X Variadic template parameters defining the types encapsulated by the wrapper.
 * @return A name-value pair encapsulating the function.
 */
template <typename K, typename... X>
nvp< policies::function, K, wrapper<X...> > func(K&& name, X&&... v){
    return make_nvp(policies::function{}, std::forward<K>(name), std::forward<X>(v)...);
}


}
}
}

#endif // UDHO_VIEW_NVP_H
