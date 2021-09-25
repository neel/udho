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

#ifndef UDHO_ACTIVITIES_DB_PG_SCHEMA_RELATION_H
#define UDHO_ACTIVITIES_DB_PG_SCHEMA_RELATION_H

#include <udho/db/pg/schema.h>
#include <udho/db/pg/schema/column.h>
#include <udho/db/pg/schema/readonly.h>
#include <udho/db/pg/crud/builder.h>
#include <udho/db/pg/crud/from.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * \code
 * namespace student{
 * 
 * PG_ELEMENT(id,    std::int64_t)
 * PG_ELEMENT(name,  std::string)
 * PG_ELEMENT(grade, std::int64_t)
 * PG_ELEMENT(marks, std::int64_t)
 * 
 * struct crud: pg::relation<crud, id, name, grade, marks>{
 *      static auto name(){
 *          using namespace ozo::literals;
 *          return "student"_SQL;
 *      }
 * };
 * 
 * }
 * 
 * namespace students{
 * 
 * using by_id = student::crud
 *       ::retrieve
 *       ::by<student::id>
 *       ::apply;
 * 
 * using by_grade = student::crud
 *       ::fetch
 *       ::by<student::grade>
 *       ::limit<5>
 *       ::apply;
 * 
 * using best_by_grade = student::crud
 *       ::fetch
 *       ::by<student::grade, student::marks::gte>
 *       ::descending<student::marks>
 *       ::limit<5>
 *       ::apply;
 * 
 * }
 * 
 * \endcode
 */
template <typename RelationT, typename... Fields>
struct relation{
    typedef RelationT relation_type;
    typedef pg::builder<pg::from<relation_type>> builder_type;
    
    template <typename FieldT>
    using column = pg::column<FieldT, RelationT>;
    using schema = pg::schema<column<Fields>...>;
    using many = pg::many<column<Fields>...>;
    using one = pg::one<column<Fields>...>;
    using all = schema;
    // commented out fields because this name may conflict with a user created fields namespace
    // using fields = pg::schema<Fields...>;
    
    static auto name(){
        return RelationT::name();
    }
    
    using readonly = pg::readonly<>;
};
    
}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_SCHEMA_RELATION_H
