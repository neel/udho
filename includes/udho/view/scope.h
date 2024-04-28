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
#include <udho/hazo/seq/seq.h>

namespace udho{
namespace view{
namespace data{


namespace traits{

template<class T>
struct is_numeric : std::integral_constant<bool, std::is_arithmetic_v<std::remove_reference_t<T>>> {};
template<class T>
struct is_string  : std::integral_constant<bool,
   (std::is_reference_v<T> && std::is_array_v<std::remove_reference_t<T>>   && std::is_same_v<std::remove_const_t<std::remove_extent_t<std::remove_reference_t<T>>>,  char>) ||
   (std::is_reference_v<T> && std::is_pointer_v<std::remove_reference_t<T>> && std::is_same_v<std::remove_const_t<std::remove_extent_t<std::remove_pointer_t<std::remove_reference_t<T>>>>,  char>) ||
   (std::is_same_v<std::remove_const_t<std::remove_reference_t<T>>, std::string>)
> {};
template<class T>
struct is_plain:    std::integral_constant<bool, is_numeric<T>::value || is_string<T>::value > {};
template <class T>
struct is_linked:   std::integral_constant<bool,
    (std::is_reference_v<T> && !std::is_array_v<std::remove_reference_t<T>> && is_plain<std::remove_reference_t<T>>::value)  ||
    (std::is_pointer_v<std::remove_reference_t<T>> && std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>, char>)
> {};
template <class T>
struct is_function:   std::integral_constant<bool,
    (std::is_reference_v<T> && std::is_function_v<std::remove_reference_t<T>>)  ||
    (std::is_pointer_v<T>   && std::is_function_v<std::remove_pointer_t<T>>)
> {};
template<class T>
struct is_mutable:    std::integral_constant<bool, is_linked<T>::value && !std::is_const_v<std::remove_reference_t<T>> > {};

template<class T>
inline constexpr bool is_numeric_v = is_numeric<T>::value;

template<class T>
inline constexpr bool is_string_v = is_string<T>::value;

template<class T>
inline constexpr bool is_plain_v = is_plain<T>::value;

template<class T>
inline constexpr bool is_linked_v = is_linked<T>::value;

template<class T>
inline constexpr bool is_function_v = is_function<T>::value;

template<class T>
inline constexpr bool is_mutable_v = is_mutable<T>::value;

}

template <typename T>
struct immutable_value{
    using value_type  = T;
    using result_type = std::add_const_t<std::add_lvalue_reference_t<T>>;
    using const_result_type = result_type;

    static constexpr const bool assignable = false;
    static constexpr const bool getter = false;
    static constexpr const bool setter = false;

    immutable_value(value_type v): _value(v) {}
    const_result_type operator*() const { return _value; }

    private:
        value_type _value;
};

template <typename T>
struct mutable_value{
    using value_type  = T;
    using result_type = std::add_lvalue_reference_t<T>;
    using const_result_type = std::add_const_t<result_type>;

    static constexpr const bool assignable = true;
    static constexpr const bool getter = false;
    static constexpr const bool setter = false;

    mutable_value(T& v): _value(v) {}
    const_result_type operator*() const { return _value; }
    result_type operator*(){ return _value; }

    private:
        value_type _value;
};

template <typename T>
struct getter_value{
    using callback_type  = T;

    static constexpr const bool assignable = false;
    static constexpr const bool getter = true;
    static constexpr const bool setter = false;

    getter_value(T&& v): _callback(std::move(v)) {}
    callback_type operator*() const { return _callback; }

    private:
        callback_type _callback;
};

template <typename T>
struct setter_value{
    using callback_type  = T;

    static constexpr const bool assignable = true;
    static constexpr const bool getter = false;
    static constexpr const bool setter = true;

    setter_value(T&& v): _callback(std::move(v)) {}
    callback_type operator*() const { return _callback; }

    private:
        callback_type _callback;
};


template <typename T, typename Enable = void>
struct wrapper1;

template <typename T>
struct wrapper1<T, std::enable_if_t<
                        ((std::is_reference_v<T> && std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>>) || !std::is_reference_v<T>) &&
                        traits::is_plain<T>::value
                >> : immutable_value<T> {
                    using immutable_value<T>::immutable_value;
                };

template <typename T>
struct wrapper1<T, std::enable_if_t<
                        !std::is_pointer_v<std::remove_reference_t<T>> && traits::is_mutable<T>::value
                >> : mutable_value<T> {
                    using mutable_value<T>::mutable_value;
                };

template <typename T>
struct wrapper1<T, std::enable_if_t<
                        traits::is_function<T>::value
                >>: getter_value<T> {
                     using getter_value<T>::getter_value;
                };

template <typename U, typename V, typename Enable = void>
struct wrapper2;

template <typename U, typename V>
struct wrapper2<U, V, std::enable_if_t<
                        traits::is_function<U>::value && traits::is_function<V>::value
                >> : getter_value<U>, setter_value<V> {
                    wrapper2(U&& u, V&& v): getter_value<U>(std::move(u)), setter_value<V>(std::move(v)) {}
                };

template <typename... X>
struct wrapper;

template <typename T>
struct wrapper<T>: wrapper1<T>{
    using wrapper1<T>::wrapper1;
};

template <typename U, typename V>
struct wrapper<U, V>: wrapper2<U, V>{
    using wrapper2<U, V>::wrapper2;
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

template <typename PolicyT, typename KeyT, typename... X>
struct nvp;

namespace policies{
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
    wrapper_type& wrapper() { return _wrapper; }

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

template <typename PolicyT, typename KeyT, typename ValueT, typename Tail>
struct associative<nvp<PolicyT, KeyT, ValueT>, Tail>{
    using head_type = nvp<PolicyT, KeyT, ValueT>;
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

template <typename PolicyT, typename KeyT, typename ValueT>
struct associative<nvp<PolicyT, KeyT, ValueT>, void>{
    using head_type = nvp<PolicyT, KeyT, ValueT>;
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

template <typename... Xs>
associative<Xs...> assoc(Xs&&... xs){
    return associative<Xs...>{std::forward<Xs>(xs)...};
}


}
}
}

#endif // UDHO_VIEW_SCOPE_H
