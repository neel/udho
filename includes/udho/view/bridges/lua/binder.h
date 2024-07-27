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
#include <udho/view/data/metatype.h>
#include <udho/view/bridges/lua/fwd.h>
#include <udho/view/bridges/lua/state.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

template <typename X>
struct binder;

namespace helper{

template <typename ResT, bool Enable = has_prototype<ResT>::value>
struct recurse{
    static void apply(detail::lua::state& state){
        using result_type = ResT;
        udho::view::data::binder<binder, result_type>::apply(state, udho::view::data::type<result_type>{});
    }
};

template <typename ResT>
struct recurse<ResT, false>{
    static void apply(detail::lua::state&){}
};

template <typename ResT>
struct recurse<std::vector<ResT>, false>{
    static void apply(detail::lua::state& state){
        recurse<ResT, has_prototype<ResT>::value>::apply(state);
    }
};

template <typename KeyT, typename ValueT>
struct recurse<std::map<KeyT, ValueT>, false>{
    static void apply(detail::lua::state& state){
        recurse<KeyT, has_prototype<KeyT>::value>::apply(state);
        recurse<ValueT, has_prototype<ValueT>::value>::apply(state);
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
    template <typename KeyT, typename T>
    binder& operator()(udho::view::data::nvp<udho::view::data::policies::function, KeyT, udho::view::data::wrapper<T>>& nvp){
        using result_type = typename udho::view::data::wrapper<T>::result_type;
        helper::recurse<result_type>::apply(_state);

        std::cout << "lua binding function: " << nvp.name() << std::endl;

        auto& w = nvp.value();
        _type.set(nvp.name(), *w);
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
