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


#ifndef UDHO_DB_PG_PRETTY_H
#define UDHO_DB_PG_PRETTY_H

#include <udho/pretty/pretty.h>
#include <udho/hazo/node/basic.h>
#include <udho/db/pg/schema/basic.h>
#include <udho/db/pg/constructs/functions.h>
#include <udho/db/pg/constructs/concat.h>
#include <udho/db/pg/constructs/alias.h>
#include <udho/db/pg/constructs/cast.h>
#include <udho/db/pg/schema/detail.h>
#include <udho/db/pg/schema/readonly.h>
#include <udho/db/pg/crud/limit.h>
#include <udho/db/pg/crud/order.h>
#include <udho/db/pg/crud/many.h>
#include <udho/db/pg/crud/one.h>

UDHO_PRETTY(udho::db::pg::types::bigint);
UDHO_PRETTY(udho::db::pg::types::integer);
UDHO_PRETTY(udho::db::pg::types::smallint);
UDHO_PRETTY(udho::db::pg::types::bigserial);
UDHO_PRETTY(udho::db::pg::types::serial);
UDHO_PRETTY(udho::db::pg::types::smallserial);
UDHO_PRETTY(udho::db::pg::types::real);
UDHO_PRETTY(udho::db::pg::types::float8);
UDHO_PRETTY(udho::db::pg::types::boolean);
UDHO_PRETTY(udho::db::pg::types::varchar);
UDHO_PRETTY(udho::db::pg::types::text);
UDHO_PRETTY(udho::db::pg::types::timestamp);
UDHO_PRETTY(udho::db::pg::types::json);
UDHO_PRETTY(udho::db::pg::types::uuid);

namespace udho{
namespace pretty{
   
template <typename HeadT, typename TailT>
struct type<udho::hazo::basic_node<HeadT, TailT>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute<HeadT>();
        printer.substitute<TailT>();
        return udho::pretty::demangle<udho::hazo::basic_node<HeadT, TailT>>(printer);
    }
};

template <typename FieldT, typename RelationT>
struct type<udho::db::pg::column<FieldT, RelationT>, true>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute<FieldT>();
        return udho::pretty::demangle<udho::db::pg::column<FieldT, RelationT>>(printer);
    }
};

template <typename... Fields>
struct type<udho::db::pg::basic_schema<Fields...>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        using schema_type = udho::db::pg::basic_schema<Fields...>;
        udho::pretty::printer printer(p);
        printer.substitute_all<Fields...>();
        return udho::pretty::demangle<schema_type>(printer);
    }
};

template <typename... Fields>
struct type<udho::db::pg::many<Fields...>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute_all<Fields...>();
        return udho::pretty::demangle<udho::db::pg::many<Fields...>>(printer);
    }
};

template <typename... Fields>
struct type<udho::db::pg::one<Fields...>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute_all<Fields...>();
        return udho::pretty::demangle<udho::db::pg::one<Fields...>>(printer);
    }
};

#define UDHO_DETAIL_DB_PRETTY_PG_FUNCTION(Fn)                                                           \
    template <typename FieldT>                                                                          \
    struct type<udho::db::pg:: Fn <FieldT>, false>{                                                     \
        static std::string name([[maybe_unused]] const udho::pretty::printer& p = printer()){           \
            using index_type = typename udho::db::pg::detail::infer_index_type<FieldT>::type;           \
            if constexpr (std::is_same_v<FieldT, index_type>){                                          \
                return udho::pretty::name<FieldT>() + "::" + #Fn;                                       \
            }                                                                                           \
            return std::string("udho::db::pg::") + #Fn + "<" + udho::pretty::name<FieldT>() + ">";      \
        }                                                                                               \
    }

UDHO_DETAIL_DB_PRETTY_PG_FUNCTION(count);
UDHO_DETAIL_DB_PRETTY_PG_FUNCTION(min);
UDHO_DETAIL_DB_PRETTY_PG_FUNCTION(max);
UDHO_DETAIL_DB_PRETTY_PG_FUNCTION(avg);
UDHO_DETAIL_DB_PRETTY_PG_FUNCTION(sum);

#define UDHO_DETAIL_DB_PRETTY_PG_OP(Operator)                                                                   \
    template <typename FieldT>                                                                                  \
    struct type<udho::db::pg::op:: Operator <FieldT>, true>{                                                    \
        static std::string name([[maybe_unused]] const udho::pretty::printer& p = printer()){                   \
            udho::pretty::printer printer(p);                                                                   \
            printer.substitute<FieldT>();                                                                       \
            using index_type =  typename udho::db::pg::detail::infer_index_type<FieldT>::type;                  \
            if constexpr (std::is_same_v<FieldT, index_type>){                                                  \
                return udho::pretty::name<FieldT>() + "::" + #Operator;                                         \
            }else{                                                                                              \
                return udho::pretty::demangle<udho::db::pg::op:: Operator <FieldT>>(printer);                   \
            }                                                                                                   \
        }                                                                                                       \
    };                                                                                                          \
    template <typename FieldT>                                                                                  \
    struct type<udho::db::pg::op:: Operator <FieldT>, false>{                                                   \
        static std::string name([[maybe_unused]] const udho::pretty::printer& p = printer()){                   \
            udho::pretty::printer printer(p);                                                                   \
            printer.substitute<FieldT>();                                                                       \
            using index_type = typename udho::db::pg::detail::infer_index_type<FieldT>::type;                   \
            if constexpr (std::is_same_v<FieldT, index_type>){                                                  \
                return udho::pretty::name<FieldT>() + "::" + #Operator;                                         \
            }else{                                                                                              \
                return udho::pretty::demangle<udho::db::pg::op:: Operator <FieldT>>(printer);                   \
            }                                                                                                   \
        }                                                                                                       \
    }
   
UDHO_DETAIL_DB_PRETTY_PG_OP(neq);
UDHO_DETAIL_DB_PRETTY_PG_OP(eq);
UDHO_DETAIL_DB_PRETTY_PG_OP(lt);
UDHO_DETAIL_DB_PRETTY_PG_OP(gt);
UDHO_DETAIL_DB_PRETTY_PG_OP(lte);
UDHO_DETAIL_DB_PRETTY_PG_OP(gte);
UDHO_DETAIL_DB_PRETTY_PG_OP(not_like);
UDHO_DETAIL_DB_PRETTY_PG_OP(like);
UDHO_DETAIL_DB_PRETTY_PG_OP(is_not);
UDHO_DETAIL_DB_PRETTY_PG_OP(is);
UDHO_DETAIL_DB_PRETTY_PG_OP(not_in);
UDHO_DETAIL_DB_PRETTY_PG_OP(in);


template <char... C>
struct type<udho::db::pg::constants::string<C...>, false>{
    static std::string name([[maybe_unused]] const printer& p = printer()){
        char const str[] = { C... };
        std::string base = "udho::db::pg::constants::string<";
        for(std::size_t i = 0; i != sizeof(str); ++i){
            if(i != 0) 
                base.append(", ");
            base.push_back('\'');
            if(str[i] == '\'') 
                base.append("\\'");
            else 
                base.push_back(str[i]);
            base.push_back('\'');
        }
        base.append(">");
        return base;
    }
};

template <typename... X>
struct type<udho::db::pg::concat<X...>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute_all<X...>();
        return udho::pretty::demangle<udho::db::pg::concat<X...>>(printer);
    }
};

template <typename FieldT, typename PgType>
struct type<udho::db::pg::cast<FieldT, PgType>, true>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute<FieldT>();
        printer.substitute<PgType>();
        return printer(udho::pretty::demangle<udho::db::pg::cast<FieldT, PgType>>());
    }
};

template <typename SourceT, typename AliasT>
struct type<udho::db::pg::alias<SourceT, AliasT>, true>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute<SourceT>();
        printer.substitute<AliasT>();
        return printer(udho::pretty::demangle<udho::db::pg::alias<SourceT, AliasT>>());
    }
};

template <typename... Fields>
struct type<udho::db::pg::readonly<Fields...>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        udho::pretty::printer printer(p);
        printer.substitute_all<Fields...>();
        return udho::pretty::demangle<udho::db::pg::readonly<Fields...>>(printer);
    }
};

template <typename FieldT>
struct type<udho::db::pg::ascending<FieldT, true>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        return "udho::db::pg::ascending<" + udho::pretty::name<FieldT>() + ">";
    }
};

template <typename FieldT>
struct type<udho::db::pg::ascending<FieldT, false>, false>{
    static std::string name(const udho::pretty::printer& p = printer()){
        return "udho::db::pg::descending<" + udho::pretty::name<FieldT>() + ">";
    }
};

   
}
}
#endif // UDHO_DB_PG_PRETTY_H
