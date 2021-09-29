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

#ifndef UDHO_DB_PG_SCHEMA_DEFS_H
#define UDHO_DB_PG_SCHEMA_DEFS_H

#include <udho/hazo/node/tag.h>
#include <ozo/query_builder.h>
#include <udho/hazo/map/element.h>
#include <boost/hana/string.hpp>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/constructs/operators.h>
#include <udho/db/pg/constructs/functions.h>
#include <udho/db/pg/schema/fwd.h>
#include <udho/pretty/type.h>
#include <boost/algorithm/string/replace.hpp>

namespace udho{
namespace db{
namespace pg{
    
namespace detail{
    
template <typename ValueT>
struct extract_value_type{
    typedef ValueT type;
};

template <typename ValueT, typename NameT>
struct extract_value_type<udho::db::pg::types::type<ValueT, NameT>>{
    typedef ValueT type;
};

template <typename IdentifierT, typename LookupT>
struct attached{
    typedef pg::column<IdentifierT, LookupT> type;
};

template <typename IdentifierT>
struct attached<IdentifierT, void>{
    typedef IdentifierT type;
};
    
template <typename FieldT, typename ValueT>
struct field_lhs{
    typedef FieldT field_type;
    
    enum {detached = true};
    
    template <typename PgType>
    using cast = pg::cast<FieldT, PgType>;
    using text = cast<pg::types::text>;
    using bigint = cast<pg::types::bigint>;
    using varchar = cast<pg::types::varchar>;
    using timestamp = cast<pg::types::timestamp>;
       
    template <typename AltFieldT>
    using as = pg::alias<FieldT, AltFieldT>;
    
    template <template <typename> class MappingT>
    using lookup = MappingT<FieldT>;
    
    template <template <typename> class MappingT>
    using attach = typename attached<FieldT, lookup<MappingT>>::type;
    
    typedef op::eq<FieldT>       eq;
    typedef op::neq<FieldT>      neq;
    typedef op::lt<FieldT>       lt;
    typedef op::gt<FieldT>       gt;
    typedef op::lte<FieldT>      lte;
    typedef op::gte<FieldT>      gte;
    typedef op::like<FieldT>     like;
    typedef op::not_like<FieldT> not_like;
    typedef op::is<FieldT>       is;
    typedef op::is_not<FieldT>   is_not;
    typedef op::in<FieldT>       in;
    typedef op::not_in<FieldT>   not_in;
    
    typedef pg::count<FieldT>    count;
    typedef pg::min<FieldT>      min;
    typedef pg::max<FieldT>      max;
    typedef pg::avg<FieldT>      avg;
    typedef pg::sum<FieldT>      sum;
    
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return std::move(rel) + "."_SQL + std::move(FieldT::ozo_name());
    }
    static constexpr auto unqualified_name(){
        return FieldT::ozo_name();
    }
    
    bool null() const {return !(static_cast<const FieldT*>(this)->initialized());}
    void null(bool flag) { static_cast<FieldT*>(this)->uninitialize(flag);}
};
    
}
    
}
}
}

// #define PG_NULLABLE(Name)                                                    
// template <typename T>                                                        
// struct ::ozo::is_nullable<Name ## _<T>>: ::std::true_type {}

#define OZO_LITERAL(TEXT) TEXT ## _SQL
#define PG_ELEMENT(Name, Type, ...)                                          \
template <typename T>                                                        \
struct Name ## _                                                             \
    : udho::util::hazo::element<                                             \
        Name ## _ <T>,                                                       \
        typename udho::db::pg::detail::extract_value_type<T>::type,          \
        udho::db::pg::detail::field_lhs,                                     \
        ##__VA_ARGS__                                                        \
    >{                                                                       \
    using base = udho::util::hazo::element<                                  \
        Name ## _ <T>,                                                       \
        typename udho::db::pg::detail::extract_value_type<T>::type,          \
        udho::db::pg::detail::field_lhs,                                     \
        ##__VA_ARGS__                                                        \
    >;                                                                       \
    using base::base;                                                        \
    using base::operator=;                                                   \
    template <typename X>                                                    \
    using alter = Name ## _ <X>;                                             \
    static constexpr auto key() {                                            \
        return BOOST_HANA_STRING(#Name);                                     \
    }                                                                        \
    static constexpr auto ozo_name() {                                       \
        using namespace ozo::literals;                                       \
        return OZO_LITERAL(#Name);                                           \
    }                                                                        \
    static std::string pretty(){                                             \
        udho::pretty::printer printer;                                   \
        printer.substitute<T>();                                             \
        using pretty_type = udho::pretty::type<Name ## _<T>, false>;     \
        std::string pretty = pretty_type::name(printer);                     \
        if(std::is_same<T, Type>::value){                                    \
            std::string field = #Name;                                       \
            std::size_t pos = pretty.find(field+"_<");                       \
            return pretty.substr(0, pos+field.length());                     \
        }                                                                    \
        return pretty;                                                       \
    }                                                                        \
};                                                                           \
using Name = Name ## _ <Type>

#define PG_ELEMENT_CAST(Name, Type, PgType, ...)                             \
struct Name: udho::util::hazo::element<Name, Type, udho::db::pg::detail::field_lhs, ##__VA_ARGS__>{         \
    using element::element;                                                  \
    static constexpr auto key() {                                            \
        return BOOST_HANA_STRING(#Name);                                     \
    }                                                                        \
    static constexpr decltype(auto) ozo_name() {                             \
        using namespace ozo::literals;                                       \
        return OZO_LITERAL(#Name);                                           \
    }                                                                        \
    static constexpr decltype(auto) ozo_qualified_name() {                   \
        using namespace ozo::literals;                                       \
        return OZO_LITERAL(#Name);                                           \
    }                                                                        \
    template <typename StrT>                                                 \
    auto cast(StrT str) const{                                               \
        using namespace ozo::literals;                                       \
        return OZO_LITERAL("CAST(")                                          \
            + std::move(str) + OZO_LITERAL(" as ") + OZO_LITERAL(#PgType)    \
            + OZO_LITERAL(")");                                              \
    }                                                                        \
}

#define PG_NAME(Name)                   \
    static auto name(){                 \
        using namespace ozo::literals;  \
        return OZO_LITERAL(#Name);      \
    }

#endif // UDHO_DB_PG_SCHEMA_DEFS_H
