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
#include <udho/db/pg/schema/detail.h>

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
    
/**
 * @brief PostgreSQL field Mixin.
 * 
 * @tparam FieldT 
 * @tparam ValueT 
 * 
 * @ingroup schema
 */
template <typename FieldT, typename ValueT>
struct field_lhs{
    typedef FieldT field_type;
    
    enum {detached = true};

    using valueable = std::true_type;
    
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

    template <typename ColumnT>
    using eq_ = op::eq_<FieldT, ColumnT>;
    using eq  = eq_<void>;
    template <typename ColumnT>
    using neq_ = op::neq_<FieldT, ColumnT>;
    using neq  = neq_<void>;
    template <typename ColumnT>
    using lt_ = op::lt_<FieldT, ColumnT>;
    using lt  = lt_<void>;
    template <typename ColumnT>
    using gt_ = op::gt_<FieldT, ColumnT>;
    using gt  = gt_<void>;
    template <typename ColumnT>
    using lte_ = op::lte_<FieldT, ColumnT>;
    using lte  = lte_<void>;
    template <typename ColumnT>
    using gte_ = op::gte_<FieldT, ColumnT>;
    using gte  = gte_<void>;
    template <typename ColumnT>
    using like_ = op::like_<FieldT, ColumnT>;
    using like  = like_<void>;
    template <typename ColumnT>
    using not_like_ = op::not_like_<FieldT, ColumnT>;
    using not_like  = not_like_<void>;
    template <typename ColumnT>
    using is_ = op::is_<FieldT, ColumnT>;
    using is  = is_<void>;
    template <typename ColumnT>
    using is_not_ = op::is_not_<FieldT, ColumnT>;
    using is_not  = is_not_<void>;
    template <typename ColumnT>
    using in_ = op::in_<FieldT, ColumnT>;
    using in  = in_<void>;
    template <typename ColumnT>
    using not_in_ = op::not_in_<FieldT, ColumnT>;
    using not_in  = not_in_<void>;

    using is_null  = is_<pg::constants::null>;

    typedef pg::count<FieldT>    count;
    typedef pg::min<FieldT>      min;
    typedef pg::max<FieldT>      max;
    typedef pg::avg<FieldT>      avg;
    typedef pg::sum<FieldT>      sum;
    
    /**
     * @brief Relate the field with a relation and produce a column
     * Generates a "table.field" like OZO string where table is the name of the
     * relation provided.
     * 
     * @tparam RelT Relation
     * @param rel 
     * @return constexpr auto 
     */
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return std::move(rel) + "."_SQL + std::move(FieldT::ozo_name());
    }
    /**
     * @brief Name of the field irrespective of the table containing the field as column
     * Uses the OZO string returned by the `ozo_name()` method generated by the `PG_ELEMENT` macro.
     */
    static constexpr auto unqualified_name(){
        return FieldT::ozo_name();
    }
    /**
     * @brief Check whether the field is null
     */
    bool null() const {return !(static_cast<const FieldT*>(this)->initialized());}
    /**
     * @brief Set the field as null
     */
    void null(bool flag) { static_cast<FieldT*>(this)->uninitialize(flag);}
};
    
}
    
}
}
}

/**
 * @def PG_ELEMENT(Name, Type)
 * @brief Define a postgresql Field which is a subclass of @ref field_lhs
 * @param Name Name for the column
 * @param Type type of the column
 *
 * A postgresql field is declared using `PG_ELEMENT` macro as shown below.
 * @code
 * PG_ELEMENT(id,          pg::types::integer);
 * PG_ELEMENT(first_name,  pg::types::varchar);
 * PG_ELEMENT(last_name,   pg::types::varchar);
 * @endcode
 * The generated `Name` contains one `ozo_name()` method which returns the
 * field name as OZO string.
 *
 * By field uses @ref udho::db::pg::detail::field_lhs mixin
 *
 * @note The macro actually defines a template `Name_<T>` and a typedef `Name`
 *       which is an alias of `Name<Type>` by using `Type` as the template
 *       parameter `T`.
 *
 * <hr />
 *
 * @note `Name` provides a typedef `Name::alter<X>` which is an alias of `Name_<X>`
 *       where `X` is a PostgreSQL type different from `Type`. This is useful for
 *       type casting.
 *
 * @ingroup schema
 */
#define PG_ELEMENT(Name, Type, ...)                                          \
template <typename T>                                                        \
struct Name ## _                                                             \
    : udho::hazo::element<                                                   \
        Name ## _ <T>,                                                       \
        typename udho::db::pg::detail::extract_value_type<T>::type,          \
        udho::db::pg::detail::field_lhs,                                     \
        ##__VA_ARGS__                                                        \
    >{                                                                       \
    using base = udho::hazo::element<                                        \
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
        udho::pretty::printer printer;                                       \
        printer.substitute<T>();                                             \
        using pretty_type = udho::pretty::type<Name ## _<T>, false>;         \
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

/**
 * @def PG_NAME(Name)
 * @brief Specify name of a relation.
 * Used in relations created using pg::relation
 * @see pg::relation
 * 
 * @ingroup schema
 */
#define PG_NAME(Name)                   \
    static auto name(){                 \
        using namespace ozo::literals;  \
        return OZO_LITERAL(#Name);      \
    }

#define PG_ELEMENT_CAST(Name, Type, PgType, ...)                             \
struct Name: udho::hazo::element<Name, Type, udho::db::pg::detail::field_lhs, ##__VA_ARGS__>{         \
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

#endif // UDHO_DB_PG_SCHEMA_DEFS_H
