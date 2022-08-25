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

#ifndef UDHO_DB_PG_GENERATORS_PARTS_OPERATORS_H
#define UDHO_DB_PG_GENERATORS_PARTS_OPERATORS_H

#include <udho/db/pg/schema/field.h>
#include <udho/db/pg/generators/fwd.h>
#include <udho/db/pg/schema/column.h>
#include <ozo/query_builder.h>
#include <udho/db/pg/schema/detail.h>

#define GENERATE_OPERATOR(OPCODE, OPSYM)                                   \
    template <typename FieldT, typename ColumnT>                           \
    struct op<pg::op:: OPCODE ## _ <FieldT, ColumnT>>{                     \
        static auto apply(){                                               \
            using namespace ozo::literals;                                 \
            return OZO_LITERAL(#OPSYM) + " "_SQL + std::move(detail::query_rhs<ColumnT>::apply()); \
        }                                                                  \
    };                                                                     \
    template <typename FieldT>                                             \
    struct op<pg::op:: OPCODE ## _ <FieldT, void>>{                        \
        static auto apply(){                                               \
            using namespace ozo::literals;                                 \
            return OZO_LITERAL(#OPSYM);                                  \
        }                                                                  \
    }                                                                      \

namespace udho{
namespace db{
namespace pg{
    
namespace generators{
    
template <typename FieldT>
struct op{
    static auto apply(){
        using namespace ozo::literals;
        return "="_SQL;
    }
};


GENERATE_OPERATOR(lt, <);
GENERATE_OPERATOR(lte, <=);
GENERATE_OPERATOR(gt, >);
GENERATE_OPERATOR(gte, >=);
GENERATE_OPERATOR(eq, =);
GENERATE_OPERATOR(neq, !=);
GENERATE_OPERATOR(is, is);
GENERATE_OPERATOR(is_not, is not);
GENERATE_OPERATOR(like, like);
GENERATE_OPERATOR(not_like, not like);
GENERATE_OPERATOR(in, in);
GENERATE_OPERATOR(not_in, not in);

template <typename FieldOPT, typename RelationT>
struct op<pg::column<FieldOPT, RelationT>>: op<FieldOPT>{};
    
}

}
}
}

#endif // UDHO_DB_PG_GENERATORS_PARTS_OPERATORS_H
