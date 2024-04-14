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

#ifndef UDHO_VIEW_SCOPE_H
#define UDHO_VIEW_SCOPE_H

#include <string>
#include <utility>
#include <type_traits>
// #include <udho/hazo/seq/seq.h>

namespace udho{
namespace view{
namespace data{

namespace detail{

/**
 * Value traits define how to expose these values to the foreign language.
 * A value can be scalar, range, vector, function, dict
 * A scalar value can be composed of const or non-const reference to a variable or constant expression or a pair of getter and setter.
 * A range is composed of a pair of itarators. These itarators can be passed by value or by function that returns an iterator.
 * A vector is a standard stl container.
 * A function is a C++ function.
 */
enum class value_category{ unknown, scalar, range, vector, function, dict };
enum class value_method{ unknown, constant, reference, functional };

template <value_category Category, value_method Method>
struct value_trait{
    constexpr static const value_category category = Category;
    constexpr static const value_method   method   = Method;
};

template <typename T, typename Enable = void>
struct value_wrapper{
    using type = void;
};

template <typename T>
struct value_wrapper<T, std::enable_if_t<
                            !std::is_reference_v<T> &&
                            !std::is_pointer_v<T> &&
                            !std::is_function_v<T> &&
                            !std::is_array_v<T> && (
                                std::is_arithmetic_v<T>
                            )
>>
{
    using type                  = T;
    using reference_type        = type &;
    using const_reference_type  = const type &;
    using trait                 = value_trait<value_category::scalar, value_method::constant>;

    value_wrapper(T&& v): _v(std::move(v)) {}
    const_reference_type v() const { return _v; }
    const_reference_type operator*() const { return v(); }
    private:
        T _v;
};

template <typename T>
struct value_wrapper<T, std::enable_if_t<
                             std::is_reference_v<T> &&
                            !std::is_function_v<T> &&
                            !std::is_pointer_v<T> && (
                                std::is_array_v<std::remove_reference_t<T>> &&
                                std::is_same_v<std::remove_const_t<std::remove_extent_t<std::remove_reference_t<T>>>, char>
                            )
                        >>
{
    using type                  = T;
    using reference_type        = type &;
    using const_reference_type  = const type &;
    using trait                 = value_trait<value_category::scalar, value_method::constant>;

    value_wrapper(T&& v): _v(std::move(v)) {}
    const_reference_type v() const { return _v; }
    const_reference_type operator*() const { return v(); }
    private:
        T _v;
};

template <typename T>
struct value_wrapper<T, std::enable_if_t<
                             std::is_reference_v<T> &&
                            !std::is_function_v<std::remove_reference_t<T>> &&
                             std::is_const_v<std::remove_reference_t<T>> &&
                            !std::is_array_v<std::remove_reference_t<T>>
                        >>
{
    using type                  = std::remove_reference_t<std::remove_const_t<T>>;
    using reference_type        = type &;
    using const_reference_type  = const type &;
    using trait                 = value_trait<value_category::scalar, value_method::reference>;

    value_wrapper(T v): _v(v) {}
    const_reference_type v() const { return _v; }
    const_reference_type operator*() const { return v(); }
    private:
        T _v;
};

template <typename T>
struct value_wrapper<T, std::enable_if_t<
                             std::is_reference_v<T> &&
                            !std::is_function_v<std::remove_reference_t<T>> &&
                            !std::is_const_v<std::remove_reference_t<T>> &&
                            !std::is_array_v<std::remove_reference_t<T>>
                        >>
{
    using type                  = std::remove_reference_t<T>;
    using reference_type        = type &;
    using const_reference_type  = const type &;
    using trait                 = value_trait<value_category::scalar, value_method::reference>;

    value_wrapper(T v): _v(v) {}
    const_reference_type v() const { return _v; }
    reference_type v() { return _v; }
    const_reference_type operator*() const { return v(); }
    reference_type operator*() { return v(); }
    private:
        T _v;
};

template <typename T>
struct value_wrapper<T, std::enable_if_t< (
                                std::is_reference_v<T> &&
                                std::is_function_v<std::remove_reference_t<T>>
                            ) || (
                                std::is_pointer_v<T> &&
                                std::is_function_v<std::remove_pointer_t<T>>
                            )
                        >>
{
    using type  = T;
    using reference_type        = type &;
    using const_reference_type  = const type &;
    using trait = value_trait<value_category::scalar, value_method::functional>;

    value_wrapper(T&& v): _v(std::move(v)) {}
    const_reference_type v() const { std::cout << "SEE ME" << std::endl; return _v; }
    const_reference_type operator*() const { return v(); }
    private:
        T _v;
};

template <typename T>
value_wrapper<T> wrap(T&& v){
    return value_wrapper<T>{std::forward<T>(v)};
}

template <typename T, typename = void>
struct is_wrappable : std::false_type {};

template <typename T>
struct is_wrappable<T, std::enable_if_t<!std::is_void<typename value_wrapper<T>::type>::value>> : std::true_type {};

}

template <typename K, typename T, typename Enable = void>
struct nvp;

template <typename K, typename T>
struct nvp<K, T, std::enable_if_t< std::is_const_v<std::remove_reference_t<T>> > >{
    static_assert(detail::is_wrappable<T>::value);

    using name_type             = K;
    using wrapper_type          = detail::value_wrapper<T>;
    using value_type            = typename wrapper_type::type;
    using reference_type        = typename wrapper_type::reference_type;
    using const_reference_type  = typename wrapper_type::const_reference_type;

    nvp(name_type&& name, T&& v): _name(std::move(name)), _value(std::forward<T>(v)) {}
    const name_type& name() const { return _name; }

    const_reference_type operator*() const { return *_value; }

    friend std::ostream& operator<< (std::ostream& stream, const nvp& nvp) {
        stream << nvp._name << ": " << *nvp._value;
        return stream;
    }
    private:
        name_type  _name;
        wrapper_type _value;
};

template <typename K, typename T>
struct nvp<K, T, std::enable_if_t< !std::is_const_v<std::remove_reference_t<T>> > >{
    static_assert(detail::is_wrappable<T>::value);

    using name_type             = K;
    using wrapper_type          = detail::value_wrapper<T>;
    using value_type            = typename wrapper_type::type;
    using reference_type        = typename wrapper_type::reference_type;
    using const_reference_type  = typename wrapper_type::const_reference_type;

    nvp(name_type&& name, T&& v): _name(std::move(name)), _value(std::forward<T>(v)) {}
    const name_type& name() const { return _name; }
    const_reference_type operator*() const { return *_value; }
    reference_type operator*(){ return *_value; }

    friend std::ostream& operator<< (std::ostream& stream, const nvp& nvp) {
        stream << nvp._name << ": " << *nvp._value;
        return stream;
    }
    private:
        name_type _name;
        wrapper_type  _value;
};


/**
 * @brief make name value pair of a reference (or a ...)
 * @tparam name string name
 * @tparam v value
 *
 */
template <typename K, typename T>
nvp<K, T> make_nvp(K&& name, T&& v){
    return nvp<K, T>(std::move(name), std::forward<T>(v));
}


// template <typename LK, typename LT, typename RK, typename RT>
// auto operator,(nvp<LK, LT>&& left, nvp<RK, RT>&& right){
//     return udho::hazo::make_seq_d(std::move(left), std::move(right));
// }
//
// template <typename... Args, typename RK, typename RT>
// auto operator,(udho::hazo::basic_seq<udho::hazo::by_data, Args...>&& left, nvp<RK, RT>&& right){
//     using lhs_type = udho::hazo::basic_seq<udho::hazo::by_data, Args...>;
//     using rhs_type = nvp<RK, RT>;
//     return typename lhs_type::template extend<rhs_type>(left, std::move(right));
// }


namespace detail{

template <typename Head = void, typename Tail = void>
struct associative;

template <typename KeyT, typename ValueT, typename Tail>
struct associative<nvp<KeyT, ValueT>, Tail>{
    using head_type = nvp<KeyT, ValueT>;
    using tail_type = Tail;

    associative(head_type&& head, tail_type&& tail): _head(std::move(head)), _tail(std::move(tail)) {}

    template <typename Function, typename MatchT>
    std::size_t invoke(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
        }
        return _tail.invoke(std::forward<Function>(f), std::forward<MatchT>(match), count +1);
    }

    private:
        head_type _head;
        tail_type _tail;
};

template <typename KeyT, typename ValueT>
struct associative<nvp<KeyT, ValueT>, void>{
    using head_type = nvp<KeyT, ValueT>;
    using tail_type = void;

    associative(head_type&& head): _head(std::move(head)) {}

    template <typename Function, typename MatchT>
    std::size_t invoke(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
        }
        return count+1;
    }

    private:
        head_type _head;
};

template <>
struct associative<void, void>{
    using head_type = void;
    using tail_type = void;

    template <typename Function, typename MatchT>
    std::size_t invoke(Function&&, MatchT&&, std::size_t count = 0){
        return count;
    }

};

}

namespace detail{
    template <typename Function>
    struct extractor_f{
        extractor_f(Function&& f): _f(std::move(f)) {}

        template <typename KeyT, typename ValueT>
        void operator()(nvp<KeyT, ValueT>& nvp){
            return _f(nvp);
        }

        Function _f;
    };
    template <typename KeyT, bool Once = false>
    struct match_f{
        match_f(KeyT&& key): _key(std::move(key)), _count(0) {}
        template <typename ValueT>
        bool operator()(const nvp<KeyT, ValueT>& nvp){
            if(Once && _count > 1){
                return false;
            }

            bool res = nvp.name() == _key;
            _count = _count + res;
            return res;
        }
        template <typename OtherKeyT, typename ValueT>
        bool operator()(const nvp<OtherKeyT, ValueT>& nvp){
            return false;
        }

        KeyT _key;
        std::size_t _count;
    };

    struct match_all{
        template <typename KeyT, typename ValueT>
        bool operator()(const nvp<KeyT, ValueT>&){
            return true;
        }
    };
}

template <typename X = void, typename... Xs>
struct associative: detail::associative<X, associative<Xs...>> {
    associative(X&& x, Xs&&... xs): base(std::move(x), associative<Xs...>(std::forward<Xs>(xs)...)) {}
    template <typename KeyT, typename Function>
    std::size_t apply(KeyT&& key, Function&& f){
        return base::invoke(detail::extractor_f<Function>{std::forward<Function>(f)}, detail::match_f<KeyT>{std::forward<KeyT>(key)});
    }
    template <typename KeyT, typename Function>
    std::size_t apply_once(KeyT&& key, Function&& f){
        return base::invoke(detail::extractor_f<Function>{std::forward<Function>(f)}, detail::match_f<KeyT, true>{std::forward<KeyT>(key)});
    }
    template <typename Function>
    std::size_t apply(Function&& f){
        return base::invoke(detail::extractor_f<Function>{std::forward<Function>(f)}, detail::match_all{});
    }
    private:
        using base = detail::associative<X, associative<Xs...>>;
};

template <>
struct associative<void>: detail::associative<void, void> {};

}
}
}

#endif // UDHO_VIEW_SCOPE_H
