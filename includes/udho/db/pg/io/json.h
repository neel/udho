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

#ifndef UDHO_DB_PG_IO_JSON_H
#define UDHO_DB_PG_IO_JSON_H

#include <udho/db/common/results.h>
#include <udho/db/common/result.h>
#include <udho/db/pg/schema/schema.h>
#include <udho/db/pg/ozo/io.h>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/io/detail.h>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif 

#ifdef WITH_JSON_NLOHMANN
namespace nlohmann {
    template <>
    struct adl_serializer<ozo::pg::timestamp> {
        static void to_json(json& j, const ozo::pg::timestamp& input) {
            std::time_t time = std::chrono::system_clock::to_time_t(input);
            j = std::string(std::ctime(&time));
        }
    };
    
    template <>
    struct adl_serializer<udho::db::pg::oz::varchar> {
        static void to_json(json& j, const udho::db::pg::oz::varchar& input) {
            j = input.get();
        }
        static void from_json(const json& j, udho::db::pg::oz::varchar& input) {
            input = j.get<std::string>();
        }
    };
}
#endif 

namespace udho{
namespace db{
namespace pg{
   
#ifdef WITH_JSON_NLOHMANN

template <typename... Fields>
void to_json(nlohmann::json& json, const pg::basic_schema<Fields...>& schema);

template <typename... Fields>
void to_json(nlohmann::json& json, const udho::db::results<pg::basic_schema<Fields...>>& results);

template <typename... Fields>
void to_json(nlohmann::json& json, const udho::db::result<pg::basic_schema<Fields...>>& result);

namespace io{
namespace json{

struct decorator{
    template <typename FieldT>
    decltype(auto) operator()(const FieldT& f){
        nlohmann::json j;
        auto relation_name = io::detail::extract_relation_name<FieldT>::apply();
        nlohmann::json value = {
            {"relation", io::detail::compiled_str_to_str::apply(relation_name)},
            {"value", f.value()}
        };
        const char* key = f.detached ? f.key().c_str() : f.ozo_name().text().c_str();
        j[key] = value;
        return j;
    }
    template <typename FieldT, typename ResultT>
    decltype(auto) operator()(const FieldT& f, ResultT res){
        auto relation_name = io::detail::extract_relation_name<FieldT>::apply();
        nlohmann::json value = {
            {"relation", io::detail::compiled_str_to_str::apply(relation_name)},
            {"value", f.value()}
        };
        const char* key = f.detached ? f.key().c_str() : f.ozo_name().text().c_str();
        res[key] = value;
        return res;
    }
    template <typename ResultT>
    auto finish(ResultT res){
        nlohmann::json relations = nlohmann::json::object();
        for(const auto& elem: res.items()){
            std::string relation = elem.value().at("relation").template get<std::string>();
            std::string column = elem.key();
            if(!relation.empty()){
                if(!relations.contains(relation)){
                    relations[relation] = nlohmann::json::object();
                }
                std::size_t dot = column.find('.');
                std::string field = column.substr(dot == std::string::npos ? 0 : dot+1);
                relations[relation][field] = elem.value().at("value");
            }else{
                relations[column] = elem.value().at("value");
            }
        }
        return relations;
    }
};
    
template <typename SchemaT>
struct schema{
    nlohmann::json operator()(const SchemaT& schema){
        nlohmann::json json = schema.decorate(decorator{});
        return json;
    }
};

template <typename SchemaT>
struct results{
    nlohmann::json operator()(const udho::db::results<SchemaT>& results){
        nlohmann::json records = nlohmann::json::array();
        for(const auto& result: results){
            nlohmann::json row;
            to_json(row, result);
            records.push_back(row);
        }
        
        return {
            {"count", results.count()},
            {"records", records}
        };
    }
};

template <typename SchemaT>
struct result{
    nlohmann::json operator()(const udho::db::result<SchemaT>& result){
        nlohmann::json row;
        to_json(row, *result);
        return row;
    }
};

}
}

template <typename... Fields>
void to_json(nlohmann::json& json, const pg::basic_schema<Fields...>& schema){
    udho::db::pg::io::json::schema<pg::basic_schema<Fields...>> writer;
    json = writer(schema);
}

template <typename... Fields>
void to_json(nlohmann::json& json, const udho::db::results<pg::basic_schema<Fields...>>& results){
    udho::db::pg::io::json::results<pg::basic_schema<Fields...>> writer;
    json = writer(results);
}

template <typename... Fields>
void to_json(nlohmann::json& json, const udho::db::result<pg::basic_schema<Fields...>>& result){    
    udho::db::pg::io::json::result<pg::basic_schema<Fields...>> writer;
    json = writer(result);
}

#endif

}
}
}

#endif // UDHO_DB_PG_IO_JSON_H
