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

#ifndef UDHO_DB_PG_GENERATORS_PARTS_FROM_H
#define UDHO_DB_PG_GENERATORS_PARTS_FROM_H

#include <ozo/query_builder.h>
#include <udho/db/pg/crud/join.h>
#include <udho/db/pg/generators/fwd.h>
#include <udho/db/pg/generators/join.h>

namespace udho{
namespace db{
namespace pg{
    
namespace generators{

/**
 * @addtogroup generators
 * @ingroup pg
 * @{
 */

/**
 * from table part of the query
 */
template <typename RelationT>
struct from<pg::from<RelationT>>{
    
    auto operator()(){
        return apply();
    }
    
    static auto apply(){
        using namespace ozo::literals;
        
        return "from "_SQL + std::move(RelationT::name());
    }
};

/**
 * from table [inner join other on table.field = other.field]* part of the query
 */
template <typename JoinType, typename FromRelationT, typename RelationT, typename FieldL, typename FieldR, typename PreviousJoin>
struct from<basic_join_on<JoinType, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin>>{
    typedef basic_join_on<JoinType, FromRelationT, RelationT, FieldL, FieldR, PreviousJoin> basic_join_type;
    
    auto operator()(){
        return apply();
    }
    
    static auto apply(){
        using namespace ozo::literals;
        
        return from<pg::from<typename basic_join_type::source>>::apply() + " "_SQL + join<typename basic_join_type::type>::apply();
    }
};

/**
 * @}
 */
    
}

}
}
}

#endif // UDHO_DB_PG_GENERATORS_PARTS_FROM_H
