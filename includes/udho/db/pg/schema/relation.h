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

#ifndef UDHO_DB_PG_SCHEMA_RELATION_H
#define UDHO_DB_PG_SCHEMA_RELATION_H

#include <udho/db/pg/schema/schema.h>
#include <udho/db/pg/schema/column.h>
#include <udho/db/pg/schema/readonly.h>
// #include <udho/db/pg/schema/primary.h>
#include <udho/db/pg/crud/builder.h>
#include <udho/db/pg/crud/from.h>
#include <udho/db/pg/crud/many.h>
#include <udho/db/pg/crud/one.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * @brief define a table by specifying its fields and name.
 * @tparam RelationT The derived Class
 * @tparam Fields... The fields in the relation.
 * 
 * Multiple relations relating to the same postgreSQL table can be defined. Such relations
 * are useful for scenarios when some queries are to be performed on a projection of a table.
 * - Multiple select operation on different prtojection of the same table.
 * - Selection on one projection and insert/update on different projection.
 * - Defining a view with additional fields that the table relation.
 * .
 * @code
 * namespace fields{
 *     PG_ELEMENT(id,                  pg::types::integer);
 *     PG_ELEMENT(first_name,          pg::types::varchar);
 *     PG_ELEMENT(last_name,           pg::types::varchar);
 *     PG_ELEMENT(email,               pg::types::varchar);
 *     PG_ELEMENT(phone,               pg::types::varchar);
 *     PG_ELEMENT(pass,                pg::types::varchar);
 *     PG_ELEMENT(designation,         pg::types::varchar);
 *     PG_ELEMENT(current_designation, pg::types::varchar);
 *     PG_ELEMENT(photo,               pg::types::text);
 *     PG_ELEMENT(rank,                pg::types::bigint);
 *     PG_ELEMENT(last_gossip,         pg::types::bigint);
 * }
 * 
 * struct table: pg::relation<table, fields::id, fields::first_name, fields::last_name, fields::email, fields::phone, fields::pass, fields::designation, fields::current_designation, fields::photo, fields::rank, fields::last_gossip>{
 *     PG_NAME(users)
 *     using readonly = pg::readonly<fields::id>;
 * };
 * 
 * struct brief: pg::relation<brief, fields::id, fields::first_name, fields::last_name, fields::phone>{
 *     PG_NAME(users)
 *     using readonly = pg::readonly<fields::id>;
 * };
 * @endcode
 * @note @ref PG_NAME macro specifies the actual PostgreSQL table name.
 * @ingroup schema
 */
template <typename RelationT, typename... Fields>
struct relation{
    typedef RelationT relation_type;
    typedef pg::builder<pg::from<relation_type>> builder_type;
    
    /**
     * @brief Get a field on this relation as column
     * Same as `pg::column<FieldT, RelationT>` where `RelationT` is a relation created 
     * using `pg::relation`
     * @tparam FieldT 
     */
    template <typename FieldT>
    using column = pg::column<FieldT, RelationT>;
    /**
     * @brief Schema containing all fields of this relation
     */
    using schema = pg::schema<column<Fields>...>;
    /**
     * @brief Resultset for retrieving 0 or more rows containing all fields of this relation
     */
    using many   = pg::many<column<Fields>...>;
    /**
     * @brief Resultset for retrieving 0 or 1 row containing all fields of this relation
     */
    using one    = pg::one<column<Fields>...>;
    /**
     * @brief Schema containing all fields of this relation
     */
    using all    = schema;
    
    /**
     * @brief Get name of the table as compile time OZO string
     */
    static auto name(){
        return RelationT::name();
    }
    
    /**
     * @brief Hide the default readonly typedef and specify the readonly fields in the derived class.
     */
    using readonly = pg::readonly<>;

    /**
     * @brief The primary key
     */
    // using primary  = pg::primary<>;
};
    
}
}
}

#endif // UDHO_DB_PG_SCHEMA_RELATION_H
