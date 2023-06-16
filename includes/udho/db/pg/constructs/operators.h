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

#ifndef UDHO_DB_PG_CONSTRUCTS_OPERATORS_H
#define UDHO_DB_PG_CONSTRUCTS_OPERATORS_H

#include <udho/hazo/node/tag.h>
#include <ozo/query_builder.h>
#include <udho/hazo/map/element.h>
#include <boost/hana/string.hpp>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/schema/detail.h>
#include <udho/hazo/seq/seq.h>
#include <boost/algorithm/string/replace.hpp>
#include <udho/db/pg/schema/column.h>

/**
 * @brief PostgreSQL Operators
 * @note The operator must be declared inside udho::db::pg::op namespace
 * @code
 * namespace udho{
 * namespace db{
 * namespace pg{
 * namespace op{
 *     DECLARE_OPERATOR(near);
 * }
 * }
 * }
 * }
 * @endcode
 * @note A generator has to be created using @ref GENERATE_OPERATOR
 * @ingroup pg
 */
#define DECLARE_OPERATOR(OPCODE)                                                                                                                                                                                            \
    template <typename FieldT, typename ColumnT = void>                                                                                                                                                                     \
    struct OPCODE ## _ : FieldT{                                                                                                                                                                                            \
        using valueable = std::is_void<ColumnT>;                                                                                                                                                                            \
        using FieldT::FieldT;                                                                                                                                                                                               \
        using index_type = OPCODE ## _ <typename detail::infer_index_type<FieldT>::type, ColumnT>;                                                                                                                          \
        template <template <typename> class MappingT>                                                                                                                                                                       \
        using attach = OPCODE ## _ <typename FieldT::template attach<MappingT>, ColumnT>;                                                                                                                                   \
                                                                                                                                                                                                                            \
        const static constexpr udho::hazo::element_t<OPCODE ## _ <FieldT, ColumnT>> val = udho::hazo::element_t<OPCODE ## _ <FieldT, ColumnT>>();                                                                           \
    };                                                                                                                                                                                                                      \
    template <typename FieldT, typename ColumnT>                                                                                                                                                                            \
    const udho::hazo::element_t<OPCODE ## _ <FieldT, ColumnT>> OPCODE ## _ <FieldT, ColumnT>::val;                                                                                                                          \
    template <typename FieldT>                                                                                                                                                                                              \
    using OPCODE = OPCODE ## _ <FieldT, void>                                                                                                                                                                               \
                                                                                                                                                                                                                            \


namespace udho{
namespace db{
namespace pg{
 
namespace op{

DECLARE_OPERATOR(lt);
DECLARE_OPERATOR(gt);
DECLARE_OPERATOR(lte);
DECLARE_OPERATOR(gte);
DECLARE_OPERATOR(eq);
DECLARE_OPERATOR(neq);
DECLARE_OPERATOR(is);
DECLARE_OPERATOR(is_not);
DECLARE_OPERATOR(in);
DECLARE_OPERATOR(not_in);
DECLARE_OPERATOR(like);
DECLARE_OPERATOR(not_like);

}

}
}
}

#endif // UDHO_DB_PG_CONSTRUCTS_OPERATORS_H
