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

#ifndef UDHO_ACTIVITIES_DB_PG_SCHEMA_COLUMN_H
#define UDHO_ACTIVITIES_DB_PG_SCHEMA_COLUMN_H

#include <ozo/query_builder.h>
#include <udho/db/pg/schema/fwd.h>
#include <udho/db/pg/schema/detail.h>

namespace udho{
namespace db{
namespace pg{
    
template <typename FieldT, typename RelationT>
struct column: FieldT{
    typedef FieldT field_type;
    typedef RelationT relation_type;
    typedef typename detail::infer_index_type<FieldT>::type index_type;
    
    enum {detached = false};
    
    template <typename AltValueT>
    using alter = column<typename FieldT::template alter<AltValueT>, RelationT>;
    
    using FieldT::FieldT;
    static constexpr decltype(auto) ozo_name() {
        return FieldT::relate(RelationT::name());
    }
};

template <typename L, typename R>
struct is_equivalent{
    enum {
        value = std::is_same<L, R>::value
    };
};

template <typename L, typename R, typename RelationT>
struct is_equivalent<L, pg::column<R, RelationT>>{
    enum {
        value = std::is_same<L, R>::value
    };
};

template <typename L, typename R, typename RelationT>
struct is_equivalent<pg::column<L, RelationT>, R>{
    enum {
        value = std::is_same<L, R>::value
    };
};

template <typename L, typename R, typename RelationT>
struct is_equivalent<pg::column<L, RelationT>, pg::column<R, RelationT>>{
    enum {
        value = std::is_same<L, R>::value
    };
};
    
}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_SCHEMA_COLUMN_H
