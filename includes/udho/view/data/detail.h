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

#ifndef UDHO_VIEW_DATA_DETAIL_H
#define UDHO_VIEW_DATA_DETAIL_H

#include <udho/view/data/fwd.h>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

namespace detail{
    template <typename Ret, typename DataT>
    struct getter_f{
        getter_f(Ret& ret, DataT& d): _ret(ret), _data(d) {}

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<Ret&, typename V::result_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::writable>, K, V>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            _ret = function();
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<Ret&, typename V::result_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::readonly>, K, V>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            _ret = function();
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<Ret&, typename V::result_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::functional>, K, V>& nvp){
            auto wrapper  = *nvp.value().getter();
            auto function = std::bind(wrapper, _data);
            _ret = function();
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<Ret&, typename V::result_type> >::type>
        void operator()(nvp<policies::function, K, V>&){ }

        template <typename PolicyT, typename K, typename V, typename = typename std::enable_if< !std::is_assignable_v<Ret&, typename V::result_type> >::type>
        void operator()(nvp<PolicyT, K, V>&){ }

        Ret&   _ret;
        DataT& _data;
    };

    template <typename Ret, typename DataT, typename... X>
    struct caller_f{
        using provided_args_type = std::tuple<X...>;
        static constexpr std::size_t provided_args_size = std::tuple_size_v<provided_args_type>;

        caller_f(Ret& ret, DataT& d, X&&... args): _ret(ret), _data(d), _args(std::forward<X>(args)...) {}
        caller_f(Ret& ret, DataT& d, std::tuple<X...> args): _ret(ret), _data(d), _args(args) {}

        template <typename K, typename V, typename std::enable_if_t<std::is_assignable_v<Ret&, typename V::result_type>, int> = 0>
        void operator()(nvp<policies::function, K, V>& nvp) {
            call(nvp);
        }

        template <typename K, typename V, typename std::enable_if_t<!std::is_assignable_v<Ret&, typename V::result_type>, int> = 0>
        void operator()(nvp<policies::function, K, V>&) { }
        template <typename PropertyPolicy, typename K, typename V>
        void operator()(nvp<policies::property<PropertyPolicy>, K, V>&){}

        template <typename K, typename V, typename std::enable_if_t<provided_args_size <= std::tuple_size_v<typename V::function::arguments_type>, int> = 0>
        void call(nvp<policies::function, K, V>& nvp){
            static_assert(std::is_assignable_v<Ret&, typename V::result_type>);

            using required_arguments_type     = typename V::function::arguments_type;
            constexpr auto required_args_size = std::tuple_size_v<required_arguments_type>;
            static_assert(provided_args_size <= required_args_size);

            required_arguments_type required_args;
            udho::url::detail::tuple_copy(_args, required_args);

            auto wrapper  = *nvp.value();
            _ret = std::apply(wrapper, std::tuple_cat(std::make_tuple(_data), required_args));
        }

        template <typename K, typename V, typename std::enable_if_t< std::tuple_size_v<typename V::function::arguments_type> < provided_args_size, int> = 0>
        void call(nvp<policies::function, K, V>& nvp){
            static_assert(std::is_assignable_v<Ret&, typename V::result_type>);

            using required_arguments_type     = typename V::function::arguments_type;
            constexpr auto required_args_size = std::tuple_size_v<required_arguments_type>;
            static_assert(provided_args_size  > required_args_size);

            throw std::invalid_argument{udho::url::format("function {} called with more arguments than needed", nvp.name())};
        }

        Ret&             _ret;
        DataT&           _data;
        std::tuple<X...> _args;
    };

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
        template <typename PolicyT, typename ValueT>
        bool operator()(const nvp<PolicyT, KeyT, ValueT>& nvp){
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


#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    struct to_json_f{
        to_json_f(nlohmann::json& root, const DataT& data): _root(root), _data(data) {}

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            auto value    = function();
            _root.push_back({nvp.name(), udho::view::data::to_json(value)});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            _root.push_back({nvp.name(), function()});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value().getter();
            auto function = std::bind(wrapper, _data);
            _root.push_back({nvp.name(), function()});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<policies::function, KeyT, ValueT>& nvp){
            return false;
        }

        nlohmann::json& _root;
        const DataT& _data;
    };

    template <typename DataT>
    struct from_json_f{
        from_json_f(const nlohmann::json& root, DataT& data): _root(root), _data(data) {}

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            using result_type = typename ValueT::result_type;

            result_type v = _root[nvp.name()].template get<result_type>();

            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, &_data);
            function() = v;
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>& nvp){
            return false;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){
            using result_type = typename ValueT::result_type;

            result_type v = _root[nvp.name()].template get<result_type>();

            auto wrapper  = *nvp.value().setter();
            auto function = std::bind(wrapper, &_data, v);
            function();
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<policies::function, KeyT, ValueT>& nvp){
            return false;
        }

        const nlohmann::json& _root;
        DataT& _data;
    };

#endif
}

}
}
}


#endif // UDHO_VIEW_DATA_DETAIL_H
