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

    member_variable(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    private:
        member_type _member;
};

template <typename Class, typename T>
struct const_member_function<T (Class::*)() const>{
    using member_type = T (Class::*)() const;
    using result_type = T;
    using class_type  = Class;

    const_member_function(member_type&& member): _member(std::move(member)) {}
    member_type operator*() { return _member; }

    private:
        member_type _member;
};

template <typename Class, typename T, typename Res>
struct member_function<Res (Class::*)(T)>{
    using member_type = Res (Class::*)(T);
    using result_type = Res;
    using class_type  = Class;

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

template <typename Res, typename Class, typename T>
struct wrapper1<Res (Class::*)(T)>: member_function<Res (Class::*)(T)> {
    using member_function<Res (Class::*)(T)>::member_function;
};

template <typename Class, typename T>
struct wrapper1<T (Class::*)() const>: const_member_function<T (Class::*)() const> {
    using const_member_function<T (Class::*)() const>::const_member_function;
};

template <typename U, typename V>
struct wrapper2;

template <typename Class, typename U, typename V, typename Res>
struct wrapper2<U (Class::*)() const, Res (Class::*)(V)>: getter_value<U (Class::*)() const>, setter_value<Res (Class::*)(V)> {
    wrapper2(U (Class::*u)() const, Res (Class::*v)(V))
        : getter_value<U (Class::*)() const>    (std::move(u)),
          setter_value<Res (Class::*)(V)>       (std::move(v)) {}
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

template <typename PolicyT, typename KeyT, typename... X>
struct nvp;

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

        template <typename PolicyT, typename KeyT, typename ValueT>
        decltype(auto) operator()(nvp<PolicyT, KeyT, ValueT>& nvp){
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
        template <typename OtherPolicyT, typename OtherKeyT, typename ValueT>
        bool operator()(const nvp<OtherPolicyT, OtherKeyT, ValueT>& nvp){
            return false;
        }

        KeyT _key;
        std::size_t _count;
    };

    struct match_all{
        template <typename PolicyT, typename KeyT, typename ValueT>
        bool operator()(nvp<PolicyT, KeyT, ValueT>& nvp){
            return true;
        }
    };
}

template <typename AssociativeT>
struct metatype;

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

    metatype<associative<X, Xs...>> as(const std::string& name){
        return metatype<associative<X, Xs...>>{name, std::move(*this)};
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

template <typename... Xs>
struct metatype<associative<Xs...>>{
    using members_type = associative<Xs...>;

    metatype(const std::string& name, members_type&& members): _name(name), _members(std::move(members)) {}

    const std::string& name() const { return _name; }
    members_type& members() { return _members; }

    template <typename Function>
    std::size_t operator()(Function&& f){
        return _members.template apply<Function>(std::forward<Function>(f));
    }

    private:
        std::string  _name;
        members_type _members;
};

template <typename Class>
struct type{};

// template <class X>
// struct foreign;

template <class ClassT>
auto prototype(udho::view::data::type<ClassT>){
    static_assert("prototype method not overloaded");
}

// template <typename X>
// struct view{
//
// };

}
}
}

#endif // UDHO_VIEW_SCOPE_H
