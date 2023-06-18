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

#ifndef UDHO_DB_PG_CRUD_FROM_H
#define UDHO_DB_PG_CRUD_FROM_H

#include <ozo/query_builder.h>
#include <udho/db/pg/schema/column.h>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/constructs/functions.h>
#include <udho/db/pg/crud/many.h>
#include <udho/db/pg/crud/one.h>
#include <udho/db/pg/crud/fwd.h>

namespace udho{
namespace db{
namespace pg{

namespace detail{
    
/**
 * @brief Given a schema and field finds out the relation with which the given field is associated with
 * @tparam SchemaT The schema may include columns from multiple relations
 * @tparam FieldT  The field to search for
 */
template <typename SchemaT, typename FieldT, bool Exists = SchemaT::template contains<FieldT>::value>
struct relation_of_helper{
    /**
     * @brief relation with which the given field is associated with (void if not found)
     */
    using type = typename SchemaT::types::template data_of<FieldT>::relation_type;
};

template <typename SchemaT, typename FieldT>
struct relation_of_helper<SchemaT, FieldT, false>{
    using type = void ;
};
    
}
    
/**
 * @brief FROM clause, often the start point for building the query.
 * @ingroup crud
 * Given a relation defined the following 
 * @code 
 * PG_ELEMENT(id,          pg::types::integer);
 * PG_ELEMENT(first_name,  pg::types::varchar);
 * PG_ELEMENT(last_name,   pg::types::varchar);
 *
 * struct table: pg::relation<table, id, first_name, last_name>{
 *     PG_NAME(users)
 *     using readonly = pg::readonly<id>;
 * };
 * @endcode 
 * The following code generates an async activity that performs `SELECT * FROM users` SQL query and returns result asynchronously.
 * @code 
 * using all = pg::from<table>
 *     ::fetch                        // Use retrieve if expecting 0 or 1 rows in the resultset
 *     ::all                          // Specify schema of the resultset
 *     ::apply;                       // Generate Async Activity for this query
 * @endcode 
 * `::fetch` or `::retrieve` specifies how many rows are expected in the resultset. `::fetch` implies there can be 0..N rows whereas
 * `::retrieve` implies that there can be 0..1 rows in the resultset. `::retrieve` is used for queries that have a where clause 
 * that returns exactly one row if found, e.g. WHERE id=XYZ. 
 *
 * `::all` or `::only` specifies the schema of the resultset. `::all` implies that all fields in the relation has to be included in the
 * resultset e.g. SELECT * FROM users. `::only<X, Y, Z>` on the other hand specifies a subset a subset of fields that are expected to be
 * present in the resultset. 
 * @code 
 * using all = pg::from<table>
 *     ::retrieve 
 *     ::all 
 *     ::by<id>                // specify where clause
 *     ::apply; 
 * @endcode 
 * A more complicated example would look like the following. 
 * @code
 * 
 * namespace students{
 *      PG_ELEMENT(id,          pg::types::integer);
 *      PG_ELEMENT(first_name,  pg::types::varchar);
 *      PG_ELEMENT(last_name,   pg::types::varchar);
 *      
 *      struct table: pg::relation<table, id, first_name, last_name>{
 *          PG_NAME(users)
 *          using readonly = pg::readonly<id>;
 *      };
 * }
 * 
 * namespace projects{
 *      PG_ELEMENT(id,          pg::types::integer);
 *      PG_ELEMENT(student,     pg::types::integer);
 *      PG_ELEMENT(name,        pg::types::varchar);
 *      
 *      struct table: pg::relation<table, id, student, name>{
 *          PG_NAME(projects)
 *          using readonly = pg::readonly<id>;
 *      };
 * }
 * 
 * using by_id = pg::from<students::table>
 *  ::join<projects::table>::inner::on<students::id, projects::student>
 *  ::retrieve
 *  ::only<students::table::all>
 *  ::include<
 *      projects::id,
 *      projects::name
 *  >
 *  ::by<students::id>
 *  ::group_by<students::id, projects::id>
 *  ::apply;
 * @endcode 
 * @tparam FromRelationT 
 */
template <typename FromRelationT>
struct from{
    using schema = typename FromRelationT::schema;
    using builder_type = typename FromRelationT::builder_type;
    
    /**
     * @brief Join the provided Foreign Relation with the FromRelationT
     * The FromRelationT is on the right side of the resulting join
     * @tparam ForeignRelationT 
     */
    template <typename ForeignRelationT>
    using join = typename pg::attached<FromRelationT>::template join<ForeignRelationT>;

    template <typename FieldT>
    using autojoin = typename pg::attached<FromRelationT>::template autojoin<FieldT>;
    /**
     * @brief Given a field returns the column for it.
     * 
     * @tparam FieldT 
     */
    template <typename FieldT>
    using data_of = typename schema::types::template data_of<FieldT>;
    /**
     * @brief Given a field returns the relation it is associated with.
     * 
     * @tparam FieldT 
     */
    template <typename FieldT>
    using relation_of = typename detail::template relation_of_helper<schema, FieldT>::type;
    /**
     * @brief different types of select queries
     * 
     * @tparam ResultT schema to select
     */
    template <typename ResultT>
    struct basic_read{
        using fields = typename builder_type::template select<ResultT>;
        
        template <typename DerivedT>
        using activity = typename fields::template activity<DerivedT>;
        
        using apply = typename fields::apply;
        
        /**
         * @brief Ascending
         * 
         * @tparam FieldT 
         */
        template <typename FieldT>
        using descending = typename fields::template descending<typename FieldT::template attach<relation_of>>;
        /**
         * @brief Descending
         * 
         * @tparam FieldT 
         */
        template <typename FieldT>
        using ascending  = typename fields::template ascending<typename FieldT::template attach<relation_of>>;
        
        template <int Limit, int Offset = 0>
        using limit = typename fields::template limit<Limit, Offset>;

        // template <typename... GroupColumn>
        // using group = typename fields::template group<GroupColumn...>;
        template <typename... GroupColumn>
        using group = typename fields::template group<typename GroupColumn::template attach<relation_of>...>;

        
        /**
         * @brief Construct an where clause with conjunction of the fields provided
         * 
         * @tparam ByFields ... One or more fields on which the conjunction to be constructed
         */
        template <typename... ByFields>
        struct by: fields::template with<typename ByFields::template attach<relation_of>...>{
            using where = typename fields::template with<typename ByFields::template attach<relation_of>...>;
            
            template <typename FieldT>
            using descending = typename where::template descending<typename FieldT::template attach<relation_of>>;
            template <typename FieldT>
            using ascending  = typename where::template ascending<typename FieldT::template attach<relation_of>>;
            
            // template <typename... GroupColumn>
            // using group = typename where::template group<GroupColumn...>;
            template <typename... GroupColumn>
            using group = typename where::template group<typename GroupColumn::template attach<relation_of>...>;

        };
        
        template <typename... X>
        using exclude = basic_read<typename ResultT::template exclude<typename X::template attach<relation_of>...>>;
        template <typename... X>
        using include = basic_read<typename ResultT::template include<typename X::template attach<relation_of>...>>;
    };
    
    /**
     * @brief Fetch zero or more records 
     * use `::all` to specify the full schema in the resultset. User `::only` to specify a subset.
     */
    struct fetch{
        /**
         * @brief select all (*) fields 
         * 
         */
        using all = basic_read<typename FromRelationT::many>;
        /**
         * @brief Select only a subset of fields
         * 
         * @tparam OnlyFields ... One or more fields
         */
        template <typename... OnlyFields>
        using only = basic_read<pg::many<typename OnlyFields::template attach<relation_of>...>>;
    };
    
    /**
     * @brief fetch zero or one record
     * use `::all` to specify the full schema in the resultset. User `::only` to specify a subset.
     */
    struct retrieve{
        /**
         * @brief Select all (*) fields
         * 
         */
        using all = basic_read<typename FromRelationT::one>;
        /**
         * @brief Select a subset of fields
         * 
         * @tparam OnlyFields ... One or more fields
         */
        template <typename... OnlyFields>
        using only = basic_read<pg::one<typename OnlyFields::template attach<relation_of>...>>;
    };

    /**
     * @brief Remove zero or more rows
     * 
     */
    struct remove{
        template <typename... Fields>
        using by = typename builder_type::template remove<Fields...>;
    };
};
    
}
}
}

#endif // UDHO_DB_PG_CRUD_FROM_H
