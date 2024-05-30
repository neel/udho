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


#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    struct to_json_f{
        to_json_f(nlohmann::json& root, const DataT& data): _root(root), _data(data) {}

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            _root.push_back({nvp.name(), function()});
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
