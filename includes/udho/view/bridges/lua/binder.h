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

#ifndef UDHO_VIEW_BRIDGES_LUA_BINDER_H
#define UDHO_VIEW_BRIDGES_LUA_BINDER_H

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <sol/sol.hpp>
#include <udho/view/data/associative.h>
#include <udho/view/data/bindings.h>
#include <udho/view/bridges/fwd.h>
#include <udho/view/bridges/lua/fwd.h>
#include <udho/view/bridges/lua/state.h>
#include <boost/numeric/odeint/util/is_pair.hpp>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

namespace helper{

template <typename ResT, bool Enable = has_metatype<ResT>::value>
struct recurse{
    static void apply(detail::lua::state& state){
        using result_type = ResT;
        //udho::view::data::binder<binder, result_type>::apply(state, udho::view::data::type<result_type>{});

        using bridge_type = udho::view::data::bridges::bridge<
                detail::lua::state,
                detail::lua::compiler,
                detail::lua::script,
                detail::lua::binder
        >;

        udho::view::data::bridges::bind<bridge_type> binder{state};
        binder(udho::view::data::type<result_type>{});

    }
};

template <typename ResT>
struct recurse<ResT, false>{
    static void apply(detail::lua::state&){}
};

template <typename ResT>
struct recurse<std::vector<ResT>, false>{
    static void apply(detail::lua::state& state){
        recurse<ResT, has_metatype<ResT>::value>::apply(state);
    }
};

template <typename KeyT, typename ValueT>
struct recurse<std::map<KeyT, ValueT>, false>{
    static void apply(detail::lua::state& state){
        recurse<KeyT, has_metatype<KeyT>::value>::apply(state);
        recurse<ValueT, has_metatype<ValueT>::value>::apply(state);
    }
};

template <typename KeyT, typename ValueT>
struct recurse<std::pair<KeyT, ValueT>, false>{
    static void apply(detail::lua::state& state){
        recurse<KeyT, has_metatype<KeyT>::value>::apply(state);
        recurse<ValueT, has_metatype<ValueT>::value>::apply(state);
    }
};


template <typename Class, typename ResT>
struct writable_internal_binder{
    template <typename Usertype, typename... X>
    static void apply(Usertype& type, const std::string& name, udho::view::data::wrapper<X...>& w){
        type.set(name, *w);
    }
};
template <typename Class, typename T>
struct writable_internal_binder<Class, std::vector<T>>{
    template <typename Usertype, typename... X>
    static void apply(Usertype& type, const std::string& name, udho::view::data::wrapper<X...>& wrapper){
        type.set(name, sol::property(
            [w = *wrapper](Class& d) { return sol::as_table(std::bind(w, std::ref(d))()); }
        ));
    }
};

template <typename Class, typename K, typename T>
struct writable_internal_binder<Class, std::map<K, T>>{
    template <typename Usertype, typename... X>
    static void apply(Usertype& type, const std::string& name, udho::view::data::wrapper<X...>& wrapper){
        type.set(name, sol::property(
            [w = *wrapper](Class& d) { return sol::as_table(std::bind(w, std::ref(d))()); }
        ));
    }
};

template <typename Class, typename ResT>
struct readonly_internal_binder{
    template <typename Usertype, typename... X>
    static void apply(Usertype& type, const std::string& name, udho::view::data::wrapper<X...>& w){
        type.set(name, sol::readonly(*w));
    }
};
template <typename Class, typename T>
struct readonly_internal_binder<Class, std::vector<T>>{
    template <typename Usertype, typename... X>
    static void apply(Usertype& type, const std::string& name, udho::view::data::wrapper<X...>& wrapper){
        type.set(name, sol::property(
            [w = *wrapper](Class& d) { return sol::readonly(sol::as_table(std::bind(w, std::ref(d))())); }
        ));
    }
};

template <typename Class, typename K, typename T>
struct readonly_internal_binder<Class, std::map<K, T>>{
    template <typename Usertype, typename... X>
    static void apply(Usertype& type, const std::string& name, udho::view::data::wrapper<X...>& wrapper){
        type.set(name, sol::property(
            [w = *wrapper](Class& d) { return sol::readonly(sol::as_table(std::bind(w, std::ref(d))())); }
        ));
    }
};

struct internal_index_binder {
    template<typename Usertype, typename T>
    static void apply(detail::lua::state& state, Usertype& type, udho::view::data::wrapper<T>& wrapper) {
        using wrapper_type = udho::view::data::wrapper<T>;
        using result_type  = typename wrapper_type::result_type;
        using class_type   = typename wrapper_type::class_type;
        using args_type    = typename wrapper_type::args_type;
        using key_type     = typename std::tuple_element<0, args_type>::type;

        static_assert(std::tuple_size<args_type>::value == 1);

        apply_impl<Usertype, key_type, class_type>(type, *wrapper, typename std::is_integral<key_type>::type());
    }

    template<typename Usertype, typename U, typename V>
    static void apply(detail::lua::state& state, Usertype& type, udho::view::data::wrapper<U, V>& wrapper) {
        using wrapper_type = udho::view::data::wrapper<U, V>;
        using result_type  = typename wrapper_type::result_type;
        using class_type   = typename wrapper_type::class_type;
        using key_type     = typename wrapper_type::key_type;
        using value_type   = typename wrapper_type::value_type;

        apply_impl2<Usertype>(type, wrapper, typename std::is_integral<key_type>::type());
    }

    private:

        template<typename Usertype, typename KeyT, typename Class, typename MemberT>
        static void apply_impl(Usertype& type, MemberT func, std::true_type) {
            type[sol::meta_function::index] = [func](Class& self, KeyT key) {
                return std::apply(func, std::make_tuple(self, key - 1));
            };
        }

        template<typename Usertype, typename KeyT, typename Class, typename MemberT>
        static void apply_impl(Usertype& type, MemberT func, std::false_type) {
            type[sol::meta_function::index] = [func](Class& self, KeyT key) {
                return std::apply(func, std::make_tuple(self, key));
            };
        }

        template<typename Usertype, typename U, typename V>
        static void apply_impl2(Usertype& type, udho::view::data::wrapper<U, V>& wrapper, std::true_type) {
            using wrapper_type = udho::view::data::wrapper<U, V>;
            using result_type  = typename wrapper_type::result_type;
            using class_type   = typename wrapper_type::class_type;
            using key_type     = typename wrapper_type::key_type;
            using value_type   = typename wrapper_type::value_type;

            type[sol::meta_function::index] = [w = wrapper](class_type& self, key_type key) mutable {
                return w.get(self, key -1);
            };
            type[sol::meta_function::new_index] = [w = wrapper](class_type& self, key_type key, value_type value) mutable {
                return w.set(self, key -1, value);
            };
        }

        template<typename Usertype, typename KeyT, typename Class, typename U, typename V>
        static void apply_impl2(Usertype& type, udho::view::data::wrapper<U, V>& wrapper, std::true_type) {
            using wrapper_type = udho::view::data::wrapper<U, V>;
            using result_type  = typename wrapper_type::result_type;
            using class_type   = typename wrapper_type::class_type;
            using key_type     = typename wrapper_type::key_type;
            using value_type   = typename wrapper_type::value_type;

            type[sol::meta_function::index] = [w = wrapper](class_type& self, key_type key) mutable {
                return w.get(self, key);
            };
            type[sol::meta_function::new_index] = [w = wrapper](class_type& self, key_type key, value_type value) mutable {
                return w.set(self, key, value);
            };
        }
};

struct internal_iter_binder {
    template<typename Usertype, typename U>
    static void apply(detail::lua::state& state, Usertype& type, udho::view::data::wrapper<U, U>& wrapper) {
        using wrapper_type  = udho::view::data::wrapper<U, U>;
        using value_type    = typename wrapper_type::value_type;
        using class_type    = typename wrapper_type::class_type;
        using iterator_type = typename wrapper_type::iterator_type;

        helper::recurse<value_type>::apply(state);
        apply_impl(type, wrapper, typename boost::numeric::odeint::is_pair<value_type>::type{});
    }

    template <typename Usertype, typename U>
    static void apply_impl(Usertype& type, udho::view::data::wrapper<U, U>& wrapper, std::false_type){
        using wrapper_type  = udho::view::data::wrapper<U, U>;
        using value_type    = typename wrapper_type::value_type;
        using class_type    = typename wrapper_type::class_type;
        using iterator_type = typename wrapper_type::iterator_type;

        type["ipairs"] = [w = wrapper](class_type& d) mutable {
            std::size_t i = 0;
            iterator_type it = w.begin(d), end = w.end(d);

            return sol::as_function([i, it, end](sol::this_state ts) mutable -> std::tuple<sol::object, sol::object> {
                if (it != end) {
                    auto v = *it;

                    auto key    = sol::make_object(ts, i + 1);
                    auto value  = sol::make_object(ts, v);

                    ++it;
                    ++i;

                    return std::make_tuple(key, value);
                } else {
                    return std::make_tuple(sol::make_object(ts, sol::nil), sol::make_object(ts, sol::nil));
                }
            });
        };
    }

    template <typename Usertype, typename U>
    static void apply_impl(Usertype& type, udho::view::data::wrapper<U, U>& wrapper, std::true_type){
        using wrapper_type  = udho::view::data::wrapper<U, U>;
        using value_type    = typename wrapper_type::value_type;
        using class_type    = typename wrapper_type::class_type;
        using iterator_type = typename wrapper_type::iterator_type;

        type["pairs"] = [w = wrapper](class_type& d) mutable {
            iterator_type it = w.begin(d), end = w.end(d);

            return sol::as_function([it, end](sol::this_state ts) mutable -> std::tuple<sol::object, sol::object> {
                if (it != end) {
                    auto k = it->first;
                    auto v = it->second;

                    auto key    = sol::make_object(ts, k);
                    auto value  = sol::make_object(ts, v);

                    ++it;

                    return std::make_tuple(key, value);
                } else {
                    return std::make_tuple(sol::make_object(ts, sol::nil), sol::make_object(ts, sol::nil));
                }
            });
        };
    }
};

}

template <typename X>
struct binder{
    using user_type = sol::usertype<X>;

    binder(detail::lua::state& state, const std::string& name): _state(state), _type(state._udho.new_usertype<X>(name)) {}

    template <typename KeyT, typename T>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, udho::view::data::wrapper<T>>& nvp){
        using result_type = typename udho::view::data::wrapper<T>::result_type;
        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding mutable property: " << nvp.name() << std::endl;

        auto& w = nvp.value();
        helper::writable_internal_binder<X, result_type>::apply(_type, nvp.name(), w);
        return *this;
    }
    template <typename KeyT, typename T>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, udho::view::data::wrapper<T>>& nvp){
        using result_type = typename udho::view::data::wrapper<T>::result_type;
        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding immutable property: " << nvp.name() << std::endl;

        auto& w = nvp.value();
        helper::readonly_internal_binder<X, result_type>::apply(_type, nvp.name(), w);
        return *this;
    }
    template <typename KeyT, typename U, typename V>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, udho::view::data::wrapper<U, V>>& nvp){
        using result_type = typename udho::view::data::wrapper<U, V>::result_type;
        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding functional property: " << nvp.name() << std::endl;

        auto& w = nvp.value();
        _type.set(nvp.name(), sol::property(
            *static_cast<udho::view::data::getter_value<U>&>(w),
            *static_cast<udho::view::data::setter_value<V>&>(w)
        ));
        return *this;
    }
    template <typename KeyT, typename U>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, udho::view::data::wrapper<U>>& nvp){
        using result_type = typename udho::view::data::wrapper<U>::result_type;
        using class_type  = typename udho::view::data::wrapper<U>::class_type;
        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding functional property: " << nvp.name() << std::endl;

        auto& w = nvp.value();
        _type.set(nvp.name(), sol::property([callback = *w](const class_type& d){
            result_type res = std::bind(callback, std::ref(d))();
            return res;
        }));
        return *this;
    }
    template <typename KeyT, typename T>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::function, KeyT, udho::view::data::wrapper<T>>& nvp){
        using result_type = typename udho::view::data::wrapper<T>::result_type;
        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding function: " << nvp.name() << std::endl;

        auto& w = nvp.value();
        _type.set(nvp.name(), *w);
        return *this;
    }


    template <typename KeyT, typename T>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::index<false>, KeyT, udho::view::data::wrapper<T>>& nvp){
        using result_type = typename udho::view::data::wrapper<T>::result_type;

        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding function: " << nvp.name() << std::endl;

        auto& wrapper = nvp.value();
        helper::internal_index_binder::apply(_state, _type, wrapper);
        return *this;
    }
    template <typename KeyT, typename U, typename V>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::index<true>, KeyT, udho::view::data::wrapper<U, V>>& nvp){
        using result_type = typename udho::view::data::wrapper<U, V>::result_type;
        using class_type  = typename udho::view::data::wrapper<U, V>::class_type;
        using args_type   = typename udho::view::data::wrapper<U, V>::args_type;
        using key_type    = typename std::tuple_element<0, args_type>::type;

        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding function: " << nvp.name() << std::endl;

        auto& wrapper = nvp.value();
        helper::internal_index_binder::apply(_state, _type, wrapper);
        return *this;
    }

    template <typename KeyT, typename U, typename V>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::iterable, KeyT, udho::view::data::wrapper<U, V>>& nvp){
        using begin_type    = typename udho::view::data::wrapper<U, V>::begin_type;
        using end_type      = typename udho::view::data::wrapper<U, V>::end_type;
        using iterator_type = typename udho::view::data::wrapper<U, V>::iterator_type;
        using value_type    = typename udho::view::data::wrapper<U, V>::value_type;

        helper::recurse<value_type>::apply(_state);

        std::cout << "lua binding function: " << nvp.name() << std::endl;

        auto& wrapper = nvp.value();
        helper::internal_iter_binder::apply(_state, _type, wrapper);
        return *this;
    }

    private:
        detail::lua::state& _state;
        user_type _type;
};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_BINDER_H
