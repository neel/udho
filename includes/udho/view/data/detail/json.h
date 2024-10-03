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

#ifndef UDHO_VIEW_DATA_DETAIL_JSON_H
#define UDHO_VIEW_DATA_DETAIL_JSON_H

#include <udho/view/data/fwd.h>
#include <udho/view/data/type.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/detail/traits.h>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

namespace detail{

#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    struct to_json_f{
        to_json_f(nlohmann::json& root, const DataT& data): _root(root), _data(data) {}

        template <typename PolicyT, typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<PolicyT>, KeyT, ValueT>& nvp){
            _root.push_back({nvp.name(), udho::view::data::to_json(nvp.value().get(_data))});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<policies::function, KeyT, ValueT>&){ return false; }

        nlohmann::json& _root;
        const DataT& _data;
    };

    template <typename DataT>
    struct from_json_f{
        from_json_f(const nlohmann::json& root, DataT& data): _root(root), _data(data) {}

        template <typename PolicyT, typename KeyT, typename ValueT, typename std::enable_if<std::is_same_v<PolicyT, udho::view::data::policies::writable> || std::is_same_v<PolicyT, udho::view::data::policies::functional>, int>::type* =  nullptr>
        bool operator()(nvp<udho::view::data::policies::property<PolicyT>, KeyT, ValueT>& nvp){
            using result_type = typename ValueT::result_type;
            result_type v;
            udho::view::data::from_json(v, _root[nvp.name()]);
            nvp.value().set(_data, v);
            return true;
        }


        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>&){ return false; }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<policies::function, KeyT, ValueT>&){ return false; }

        const nlohmann::json& _root;
        DataT& _data;
    };

    template <typename ClassT, std::enable_if_t<!has_metatype<ClassT>::value, int> = 0 >
    nlohmann::json to_json_internal(const ClassT& data){
        return nlohmann::json(data);
    }

    template <class ClassT, std::enable_if_t<has_metatype<ClassT>::value, int> = 0 >
    nlohmann::json to_json_internal(const ClassT& data){
        auto meta = metatype(udho::view::data::type<ClassT>{});
        return meta.members().json(data);
    }

    template <class ClassT, std::enable_if_t<has_metatype<ClassT>::value, int> = 0 >
    nlohmann::json to_json_internal(const std::vector<ClassT>& data) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& d : data) {
            nlohmann::json d_json = to_json(d);
            std::string dumped = d_json.dump();
            j.push_back(d_json);
        }

        return j;
    }

    template <typename ClassT, std::enable_if_t<!has_metatype<ClassT>::value, int> = 0 >
    void from_json_internal(ClassT& data, const nlohmann::json& json){
        data = json;
    }

    template <class ClassT, std::enable_if_t<has_metatype<ClassT>::value, int> = 0 >
    void from_json_internal(ClassT& data, const nlohmann::json& json){
        auto meta = metatype(udho::view::data::type<ClassT>{});
        return meta.members().json(data, json);
    }

    template <class ClassT, std::enable_if_t<has_metatype<ClassT>::value, int> = 0 >
    void from_json_internal(std::vector<ClassT>& data, const nlohmann::json& json) {
        for(const nlohmann::json& j: json){
            ClassT obj;
            from_json_internal(obj, j);
            data.push_back(obj);
        }
    }

#endif

}

}
}
}

#endif // UDHO_VIEW_DATA_DETAIL_JSON_H
