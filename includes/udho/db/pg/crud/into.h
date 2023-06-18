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

#ifndef UDHO_DB_PG_CRUD_INTO_H
#define UDHO_DB_PG_CRUD_INTO_H

#include <ozo/query_builder.h>
#include <udho/db/pg/crud/join.h>
#include <udho/db/pg/schema/column.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * @brief INTO query.
 * @ingroup crud
 * @tparam RelationT 
 */
template <typename RelationT>
struct into{
    using relation = RelationT;
    using schema = typename RelationT::schema;
    using builder_type = typename RelationT::builder_type;
    template <typename FieldT>
    using is_readonly = typename relation::readonly::template contains<FieldT>;
    
    template <typename FieldT>
    using data_of = typename schema::types::template data_of<FieldT>;
    
    template <typename FieldT>
    using relation_of = typename data_of<FieldT>::relation_type;
    
    template <typename SchemaT>
    struct basic_write_insert{
        using fields = typename builder_type::template insert<SchemaT>;
        
        template <typename... F>
        using returning = typename fields::template returning<F...>;
        
        template <typename DerivedT>
        using activity = typename fields::template activity<DerivedT>;
        
        using apply = typename fields::apply;
        
        template <typename... X>
        using exclude = basic_write_insert<typename SchemaT::template exclude<typename X::template attach<relation_of>...>>;
        template <typename... X>
        using include = basic_write_insert<typename SchemaT::template include<typename X::template attach<relation_of>...>>;
    };
    
    template <typename SchemaT>
    struct basic_write_update{
        using fields = typename builder_type::template update<SchemaT>;
        
        template <typename... F>
        using returning = typename fields::template returning<F...>;
        
        template <typename DerivedT>
        using activity = typename fields::template activity<DerivedT>;
        
        using apply = typename fields::apply;
        
        template <typename... X>
        using exclude = basic_write_update<typename SchemaT::template exclude<typename X::template attach<relation_of>...>>;
        template <typename... X>
        using include = basic_write_update<typename SchemaT::template include<typename X::template attach<relation_of>...>>;
        
        template <typename... Fields>
        using by = typename fields::template with<Fields...>;
    };
    
    struct insert{
        using all = basic_write_insert<schema>;
        using writables = basic_write_insert<typename schema::template exclude_if<is_readonly>>;
        template <typename... Fields>
        using only = basic_write_insert<pg::schema<Fields...>>;
    };
    
    struct update{
        using all = basic_write_update<schema>;
        using writables = basic_write_update<typename schema::template exclude_if<is_readonly>>;
        template <typename... Fields>
        using only = basic_write_update<pg::schema<Fields...>>;
    };
};
    
}
}
}

#endif // UDHO_DB_PG_CRUD_INTO_H

