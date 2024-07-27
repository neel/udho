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

#ifndef UDHO_VIEW_METATYPE_H
#define UDHO_VIEW_METATYPE_H

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/seq/seq.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/detail.h>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

namespace detail{

template <typename Head = void, typename Tail = void>
struct associative;

template <typename PolicyT, typename KeyT, typename ValueT, typename Tail>
struct associative<nvp<PolicyT, KeyT, ValueT>, Tail>{
    using head_type = nvp<PolicyT, KeyT, ValueT>;
    using tail_type = Tail;

    associative(head_type&& head, tail_type&& tail): _head(std::move(head)), _tail(std::move(tail)) {}

    template <typename Function>
    std::size_t apply_(Function& f, std::size_t count = 0){
        bool res = f(_head);
        if(!res){
            return _tail.apply_(f, count +1);
        } else {
            return count;
        }

    }

    template <typename Function, typename MatchT>
    std::size_t invoke(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
        }
        return _tail.invoke(std::forward<Function>(f), std::forward<MatchT>(match), count +1);
    }

    template <typename MatchT>
    std::size_t find_recursive(MatchT& match, std::size_t count = 0){
        if(match(_head)){
            return count;
        }
        return _tail.find_recursive(match, count +1);
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

    template <typename Function>
    std::size_t apply_(Function& f, std::size_t count = 0){
        bool res = f(_head);
        if(!res){
            return count;
        } else {
            return count +1;
        }
    }

    template <typename Function, typename MatchT>
    std::size_t invoke(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            return count;
        }
        return count+1;
    }

    template <typename MatchT>
    std::size_t find_recursive(MatchT& match, std::size_t count = 0){
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

    template <typename Function>
    std::size_t apply_(Function& f, std::size_t count = 0){
        return count;
    }

    template <typename Function, typename MatchT>
    std::size_t invoke(Function&&, MatchT&&, std::size_t count = 0){
        return count;
    }

    template <typename MatchT>
    std::size_t find_recursive(MatchT& match, std::size_t count = 0){
        return count;
    }

};


}

template <typename AssociativeT>
struct metatype;

template <typename X, typename... Xs>
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

    template <typename Ret, typename DataT, typename KeyT>
    std::size_t get(DataT& data, KeyT&& key, Ret& ret){
        return apply_once(std::forward<KeyT>(key), detail::getter_f<Ret, DataT>(ret, data));
    }
    template <typename Ret, typename DataT, typename KeyT, typename... Args>
    std::size_t call(DataT& data, KeyT&& key, Ret& ret, Args&&... args){
        return apply_once(std::forward<KeyT>(key), detail::caller_f<Ret, DataT, Args...>(ret, data, std::forward<Args>(args)...));
    }

    template <typename Ret, typename DataT, typename IteratorT>
    std::size_t getx(DataT& data, IteratorT begin, IteratorT end){
        if(std::distance(begin, end) > 1){
            return getx(data, begin +1, end);
        }
    }

#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    nlohmann::json json(const DataT& data) {
        nlohmann::json root = nlohmann::json::object();
        detail::to_json_f jsonifier{root, data};
        base::invoke(jsonifier, detail::match_all{});
        return root;
    }

    template <typename DataT>
    void json(DataT& data, const nlohmann::json& json) {
        detail::from_json_f jsonifier{json, data};
        base::invoke(jsonifier, detail::match_all{});
    }

#endif

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
    const members_type& members() const { return _members; }

    template <typename Function>
    std::size_t operator()(Function&& f){
        return _members.template apply<Function>(std::forward<Function>(f));
    }

    private:
        std::string  _name;
        members_type _members;
};

template <template<class> class BinderT, typename Class>
struct binder;

template <typename StateT, typename T>
struct bindings{

    template <template<class> class BinderT, typename Class>
    friend struct binder;

    static bool exists() { return _exists; }
    private:
        static bool _exists;
};

template <typename DerivedT, typename T>
bool bindings<DerivedT, T>::_exists = false;

template <template<class> class BinderT, typename ClassT>
struct binder{
    template <typename StateT>
    static void apply(StateT& state, udho::view::data::type<ClassT> type){
        if(!udho::view::data::bindings<StateT, ClassT>::exists()){
            auto meta = prototype(type);
            std::cout << "udho::view::data::binder: binding " << meta.name() << std::endl;
            BinderT<ClassT> binder(state, meta.name());
            meta.members().apply(std::move(binder));

            bindings<StateT, ClassT>::_exists = true;
        }
    }
};

#ifdef WITH_JSON_NLOHMANN

template <typename ClassT, std::enable_if_t<!has_prototype<ClassT>::value, int> = 0 >
nlohmann::json to_json_internal(const ClassT& data){
    return nlohmann::json(data);
}

template <class ClassT, std::enable_if_t<has_prototype<ClassT>::value, int> = 0 >
nlohmann::json to_json_internal(const ClassT& data){
    auto meta = prototype(udho::view::data::type<ClassT>{});
    return meta.members().json(data);
}

template <class ClassT, std::enable_if_t<has_prototype<ClassT>::value, int> = 0 >
nlohmann::json to_json_internal(const std::vector<ClassT>& data) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& d : data) {
        j.push_back(to_json(d));
    }
    return j;
}

/**
 * @brief Convert a C++ data object to json assuming the prototype function has been overloaded for it.
 */
template <class ClassT>
nlohmann::json to_json(const ClassT& data){
    return to_json_internal(data);
}

/**
 * @brief Loads a C++ data object from json assuming the prototype function has been overloaded for it.
 */
template <class ClassT>
void from_json(ClassT& data, const nlohmann::json& json){
    auto meta = prototype(udho::view::data::type<ClassT>{});
    return meta.members().json(data, json);
}

#endif

}
}
}

#endif // UDHO_VIEW_METATYPE_H
