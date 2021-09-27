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

#ifndef UDHO_DB_PG_GENERATORS_PARTS_JOIN_H
#define UDHO_DB_PG_GENERATORS_PARTS_JOIN_H

#include <ozo/query_builder.h>
#include <udho/db/pg/crud/fwd.h>
#include <udho/db/pg/crud/join.h>
#include <udho/db/pg/generators/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
namespace generators{

template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR>
struct join<joined<JoinType, RelationL, RelationR, FieldL, FieldR>>{    
    static auto apply(){
        using namespace ozo::literals;
        return std::move(JoinType::keyword()) + " "_SQL 
                + std::move(RelationR::name()) + " on "_SQL 
                + std::move(RelationL::name()) + "."_SQL + std::move(FieldL::ozo_name()) + " = "_SQL + std::move(RelationR::name()) + "."_SQL + std::move(FieldR::ozo_name());
    }
};

template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR>
struct join<join_clause<joined<JoinType, RelationL, RelationR, FieldL, FieldR>, void>>{
    typedef join<joined<JoinType, RelationL, RelationR, FieldL, FieldR>> head;

    static auto apply(){
        using namespace ozo::literals;
        return std::move(head::apply()) + " "_SQL;
    }
};

template <typename JoinType, typename RelationL, typename RelationR, typename FieldL, typename FieldR, typename CurrentJoin, typename RestJoin>
struct join<join_clause<joined<JoinType, RelationL, RelationR, FieldL, FieldR>, join_clause<CurrentJoin, RestJoin>>>{
    typedef join<joined<JoinType, RelationL, RelationR, FieldL, FieldR>> head;
    typedef join<join_clause<CurrentJoin, RestJoin>> tail;
    
    static auto apply(){
        using namespace ozo::literals;
        return std::move(tail::apply()) + " "_SQL + std::move(head::apply());
    }
};
    
}

}
}
}

#endif // UDHO_DB_PG_GENERATORS_PARTS_JOIN_H
