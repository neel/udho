/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_DB_PG_DECORATORS_DEFINITIONS_H
#define UDHO_DB_PG_DECORATORS_DEFINITIONS_H

#include <utility>
#include <type_traits>
#include <ozo/query_builder.h>
#include <udho/db/pg/decorators/logical.h>
#include <udho/db/pg/decorators/comma.h>
#include <udho/db/pg/decorators/traits.h>
#include <udho/db/pg/schema/constraints.h>

namespace udho{
namespace db{
namespace pg{

namespace decorators{

template <typename FieldT, bool has_default>
struct extract_default_value{
    using h_default_value = pg::constants::empty;
    static constexpr auto value() {
        return h_default_value();
    }
};

template <typename FieldT>
struct extract_default_value<FieldT, true>{
    using h_default_value = typename FieldT::field_type::default_value;
    static constexpr auto value() {
        return pg::constants::defaultv() + pg::constants::space() + h_default_value();
    }
};

template <typename FieldT, bool has_reference>
struct extract_reference{
    static constexpr auto str() {
        using namespace ozo::literals;
        return ""_SQL;
    }
};

template <typename FieldT>
struct extract_reference<FieldT, true>{
    static constexpr auto str() {
        using namespace ozo::literals;
        return "references "_SQL + FieldT::field_type::foreign_ref::str();
    }
};


template <typename FieldT>
struct field_constraints{
    using h_not_null = typename std::conditional<
        pg::constraints::has::not_null<typename FieldT::field_type>::value,
            pg::constants::not_null,
            pg::constants::empty
    >::type;
    using h_unique = typename std::conditional<
        pg::constraints::has::unique<typename FieldT::field_type>::value,
            pg::constants::unique,
            pg::constants::empty
    >::type;
    using h_default_value = extract_default_value<FieldT, pg::constraints::has::default_value<typename FieldT::field_type>::value>;
    using h_references    = extract_reference<FieldT, pg::constraints::has::references<typename FieldT::field_type>::value>;

    static constexpr auto not_null(){
        return ozo::make_query_builder(boost::hana::make_tuple(ozo::make_query_text(h_not_null())));
    }

    static constexpr auto unique(){
        return ozo::make_query_builder(boost::hana::make_tuple(ozo::make_query_text(h_unique())));
    }

    static constexpr auto default_value(){
        return ozo::make_query_builder(boost::hana::make_tuple(ozo::make_query_text(h_default_value::value())));
    }

    static constexpr auto references(){
        return h_references::str();
    }
};

/**
 * @todo write docs
 */
struct basic_definitions: private traits::fields::unqualified{
    template <typename... Args>
    basic_definitions(Args&&... args): traits::fields::unqualified(std::forward<Args>(args)...){}

    template <typename FieldT>
    decltype(auto) operator()(const FieldT& f){
        using namespace ozo::literals;
        return "    "_SQL + traits::fields::unqualified::apply(f)   + " "_SQL + FieldT::field_type::pg_data_type::name()  + " "_SQL
          + field_constraints<FieldT>::not_null()      + " "_SQL
          + field_constraints<FieldT>::unique()        + " "_SQL
          + field_constraints<FieldT>::default_value() + " "_SQL
          + field_constraints<FieldT>::references();
    }
    template <typename FieldT, typename ResultT>
    decltype(auto) operator()(const FieldT& f, ResultT res){
        using namespace ozo::literals;
        return pg::decorators::newline(""_SQL, std::move(operator()(f)), std::move(res));
    }
    template <typename ResultT>
    auto finish(ResultT res){
        return res;
    }
};

template <template <bool...> class Enabler, typename... Fields>
struct selected_definitions: private traits::fields::unqualified{
    template <typename SubjectT>
    using enable  = typename std::enable_if<Enabler<pg::is_equivalent<SubjectT, Fields>::value...>::value>;
    template <typename SubjectT>
    using disable = typename std::enable_if<!Enabler<pg::is_equivalent<SubjectT, Fields>::value...>::value>;

    template <typename... Args>
    selected_definitions(Args&&... args): traits::fields::unqualified(std::forward<Args>(args)...){}

    template <typename FieldT, typename = typename enable<FieldT>::type>
    auto operator()(const FieldT& f){
        using namespace ozo::literals;
        return "    "_SQL + traits::fields::unqualified::apply(f)   + " "_SQL + FieldT::field_type::pg_data_type::name()  + " "_SQL
          + field_constraints<FieldT>::not_null()      + " "_SQL
          + field_constraints<FieldT>::unique()        + " "_SQL
          + field_constraints<FieldT>::default_value() + " "_SQL
          + field_constraints<FieldT>::references();
    }
    template <typename FieldT, typename ResultT, typename = typename enable<FieldT>::type>
    auto operator()(const FieldT& f, ResultT res){
        using namespace ozo::literals;
        return pg::decorators::newline(""_SQL, std::move(operator()(f)), std::move(res));
    }
    template <typename FieldT, int = 0, typename = typename disable<FieldT>::type>
    decltype(auto) operator()(const FieldT& /*f*/){
        using namespace ozo::literals;
        return ""_SQL;
    }
    template <typename FieldT, typename ResultT, int = 0, typename = typename disable<FieldT>::type>
    decltype(auto) operator()(const FieldT& /*f*/, ResultT res){
        return res;
    }
    template <typename ResultT>
    auto finish(ResultT res){
        return res;
    }
};

template <typename... Fields>
using definitions_only = selected_definitions<logical_or, Fields...>;

template <typename... Fields>
using definitions_except = selected_definitions<logical_nor, Fields...>;

struct definitions: basic_definitions{
    /**
     * @brief include only a subset of fields
     * @tparam Fields... Subset of fields to include
     */
    template <typename... Fields>
    using only = definitions_only<Fields...>;
    /**
     * @brief include only a subset of fields
     * @tparam Fields... Subset of fields to exclude
     */
    template <typename... Fields>
    using except = definitions_except<Fields...>;
};

}

}
}
}

#endif // UDHO_DB_PG_DECORATORS_DEFINITIONS_H
