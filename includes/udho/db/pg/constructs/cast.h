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

#ifndef UDHO_DB_PG_CONSTRUCTS_CAST_H
#define UDHO_DB_PG_CONSTRUCTS_CAST_H

#include <string>
#include <ozo/query_builder.h>
#include <ozo/pg/definitions.h>
#include <ozo/core/strong_typedef.h>
#include <ozo/pg/types.h>
#include <udho/db/pg/constructs/fwd.h>
#include <udho/hazo/node/fwd.h>
#include <udho/hazo/node/tag.h>

namespace udho{
namespace db{
namespace pg{
       
/**
 * @brief Casts a PostgreSQL field of one type into another and generates corresponding SQL query.
 * - subclasses from `FieldT::alter<PgType>` for receiving `PgType` values using the newly casted type.
 * - hides the `relate()` and `ozo_name()` method of the base class to inject corresponding query for PostgreSQL type casting.
 * .
 * @tparam FieldT The Postgresql field 
 * @tparam PgType The new type of that field
 * 
 * @ingroup pg
 */
template <typename FieldT, typename PgType>
struct cast: FieldT::template alter<PgType>{
    typedef typename FieldT::template alter<PgType> casted_field;
    typedef cast<typename FieldT::field_type, PgType> index_type;
    
    enum {detached = FieldT::detached};
    
    const static constexpr udho::hazo::element_t<cast<typename FieldT::field_type, PgType>> val = udho::hazo::element_t<cast<typename FieldT::field_type, PgType>>();
    
    using casted_field::casted_field;
    
    template <template <typename> class MappingT>
    using attach = cast<typename FieldT::template attach<MappingT>, PgType>;
    
    static constexpr auto key() {
        return FieldT::key();
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return "CAST("_SQL + std::move(FieldT::relate(rel)) + " as "_SQL + std::move(PgType::name()) + ")"_SQL; 
    }
    static constexpr auto ozo_name() {
        using namespace ozo::literals;
        return "CAST("_SQL + std::move(FieldT::ozo_name()) + " as "_SQL + std::move(PgType::name()) + ")"_SQL; 
    }
};
    
template <typename FieldT, typename PgType>
const udho::hazo::element_t<cast<typename FieldT::field_type, PgType>> cast<FieldT, PgType>::val;

    
}
}
}



#endif // UDHO_DB_PG_CONSTRUCTS_CAST_H
