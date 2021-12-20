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

#ifndef UDHO_DB_PG_CONSTRUCTS_TYPES_H
#define UDHO_DB_PG_CONSTRUCTS_TYPES_H

#include <string>
#include <ozo/query_builder.h>
#include <ozo/pg/definitions.h>
#include <ozo/core/strong_typedef.h>
#include <ozo/pg/types.h>
#include <udho/db/pg/constructs/cast.h>
#include <ctti/nameof.hpp>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif 

namespace udho{
namespace db{
namespace pg{
    
namespace oz{
    OZO_STRONG_TYPEDEF(std::string, varchar)
}
    
namespace types{

#define _OZO_LITERAL_(TEXT) TEXT ## _SQL
#define _UDHO_DECLARE_PG_TYPE_NAME_(Name)       \
    struct Name{                                \
        static auto name() {                    \
            using namespace ozo::literals;      \
            return _OZO_LITERAL_(#Name);        \
        }                                       \
    }
    
namespace names{
    
_UDHO_DECLARE_PG_TYPE_NAME_(text);
_UDHO_DECLARE_PG_TYPE_NAME_(varchar);
_UDHO_DECLARE_PG_TYPE_NAME_(json);
_UDHO_DECLARE_PG_TYPE_NAME_(bigint);
_UDHO_DECLARE_PG_TYPE_NAME_(integer);
_UDHO_DECLARE_PG_TYPE_NAME_(smallint);
_UDHO_DECLARE_PG_TYPE_NAME_(bigserial);
_UDHO_DECLARE_PG_TYPE_NAME_(serial);
_UDHO_DECLARE_PG_TYPE_NAME_(smallserial);
_UDHO_DECLARE_PG_TYPE_NAME_(boolean);
_UDHO_DECLARE_PG_TYPE_NAME_(real);
_UDHO_DECLARE_PG_TYPE_NAME_(float8);
_UDHO_DECLARE_PG_TYPE_NAME_(uuid);

struct timestamp{
    static auto name() {
        using namespace ozo::literals;
        return "timestamp without time zone"_SQL;
    }
};

struct timestamp_tz{
    static auto name() {
        using namespace ozo::literals;
        return "timestamp with time zone"_SQL;
    }
};

}

/**
 * @brief A postgresql datatype.
 * @tparam ValueT C++ type
 * @tparam NameT A struct specifying a postgresql type as compile time string
 *
 * A postgresql type is defined by a C++ datatype and a postgresql type name expressed as a C++ class that provides
 * a public static function named `name()` which returns a complie time string representing the postgresql type.
 * @code 
 * struct bigint{
 *     static auto name() { 
 *         using namespace ozo::literals;
 *         return "bigint"_SQL;
 *     }
 * };
 * using bigint = type<std::int64_t, names::bigint>;
 * @endcode 
 * @ingroup pg
 */
template <typename ValueT, typename NameT>
struct type{
    using value_type = ValueT;
    using val = value_type;
    static auto name() {
        return NameT::name();
    }
};

/**
 * @{
 * @ingroup pg
 */
using bigint      = type<ozo::pg::bigint, names::bigint>;               ///< Postgresql Type bigint
using integer     = type<ozo::pg::int4, names::integer>;                ///< Postgresql Type integer
using smallint    = type<ozo::pg::int2, names::smallint>;               ///< Postgresql Type smallint
using bigserial   = type<ozo::pg::bigint, names::bigserial>;            ///< Postgresql Type bigserial
using serial      = type<ozo::pg::int4, names::serial>;                 ///< Postgresql Type serial
using smallserial = type<ozo::pg::int2, names::smallserial>;            ///< Postgresql Type smallserial
using real        = type<ozo::pg::float4, names::real>;                 ///< Postgresql Type real
using float8      = type<ozo::pg::float8, names::float8>;               ///< Postgresql Type float8
using varchar     = type<udho::db::pg::oz::varchar, names::varchar>;    ///< Postgresql Type varchar
using boolean     = type<bool, names::boolean>;                         ///< Postgresql Type boolean
using text        = type<std::string, names::text>;                     ///< Postgresql Type text
using timestamp   = type<ozo::pg::timestamp, names::timestamp>;         ///< Postgresql Type timestamp
using uuid        = type<ozo::pg::uuid, names::uuid>;                   ///< Postgresql Type uuid
#ifdef __DOXYGEN__
using json        = type<implementation-defined, names::json>;          ///< Postgresql Type json @warning If nlohmann json is unavailable then std::string is used to store the json data. Otherwise nlohmann json is used
#endif 
/**
 * @}
 */

#ifndef __DOXYGEN__

#ifdef WITH_JSON_NLOHMANN
using json        = type<nlohmann::json, names::json>;
#else 
using json        = type<ozo::pg::json, names::json>;
#endif 

#endif // __DOXYGEN__

}
 
using bigint      = types::bigint;
using integer     = types::integer;
using smallint    = types::smallint;
using bigserial   = types::bigserial;
using serial      = types::serial;
using smallserial = types::smallserial;
using real        = types::real;
using float8      = types::float8;
using varchar     = types::varchar;
using boolean     = types::boolean;
using text        = types::text;
using timestamp   = types::timestamp;
using json        = types::json;
using uuid        = types::uuid;
 
}

}
}

OZO_PG_BIND_TYPE(udho::db::pg::oz::varchar, "varchar")

#endif // UDHO_DB_PG_CONSTRUCTS_TYPES_H
