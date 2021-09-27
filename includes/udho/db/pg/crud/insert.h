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

#ifndef UDHO_ACTIVITIES_DB_PG_CRUD_INSERT_H
#define UDHO_ACTIVITIES_DB_PG_CRUD_INSERT_H

#include <udho/db/pg/activities/activity.h>
#include <udho/db/common/result.h>
#include <udho/db/common/none.h>
#include <udho/db/pg/generators/fwd.h>
#include <udho/db/pg/crud/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * insert returning one pg::schema<Fields...> (e.g. the record just inserted)
 * modifiable pg::schema<Fields...> for assignments using [] 
 * no where query
 */
template <typename SchemaT>
struct basic_insert{
    
    typedef SchemaT schema_type;
    
    template <typename RelationT>
    struct generators{
        using into_type      = pg::generators::into<RelationT>;
        using keys_type      = pg::generators::keys<schema_type>;
        using values_type    = pg::generators::values<schema_type>;
        
        into_type      into;
        keys_type      keys;
        values_type    values;
        
        generators(const schema_type& schema): keys(schema), values(schema){}
        auto operator()() {
            using namespace ozo::literals;
            return "insert "_SQL + std::move(into()) + " "_SQL + std::move(keys()) + " "_SQL + std::move(values());
        }
    };
    
    template <typename DerivedT>
    struct activity: pg::activity<DerivedT, db::none>, schema_type{
        typedef pg::activity<DerivedT, db::none> activity_type;
        
        template <typename CollectorT, typename... Args>
        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): 
            activity_type(collector, pool, io), 
            schema_type(std::forward<Args>(args)...)
            {}
        template <typename ContextT, typename... T, typename... Args>
        activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(args)...){}
        
        schema_type& schema() { return static_cast<schema_type&>(*this); }
        
        using activity_type::operator();

    };
    
    /**
     * update returning one pg::schema<F...> (e.g. the record just updated)
     * modifiable pg::schema<Fields...> for assignments using [] 
     * no where query
     */
    template <typename... F>
    struct returning{
        
        using returning_schema_type = pg::schema<F...>;
        
        template <typename RelationT>
        struct generators{
            using into_type      = pg::generators::into<RelationT>;
            using keys_type      = pg::generators::keys<schema_type>;
            using values_type    = pg::generators::values<schema_type>;
            using returning_type = pg::generators::returning<returning_schema_type>;
            
            into_type      into;
            keys_type      keys;
            values_type    values;
            returning_type returning;
            
            generators(const schema_type& schema): keys(schema), values(schema){}
            auto operator()() {
                using namespace ozo::literals;
                return "insert "_SQL + std::move(into()) + " "_SQL + std::move(keys()) + " "_SQL + std::move(values()) + " "_SQL + std::move(returning());
            }
        };
        
        template <typename DerivedT>
        struct activity: pg::activity<DerivedT, db::result<returning_schema_type>>, schema_type{
            typedef pg::activity<DerivedT, db::result<returning_schema_type>> activity_type;
            
            template <typename CollectorT, typename... Args>
            activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): 
                activity_type(collector, pool, io), 
                schema_type(std::forward<Args>(args)...)
                {}
            template <typename ContextT, typename... T, typename... Args>
            activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(args)...){}
            
            schema_type& schema() { return static_cast<schema_type&>(*this); }
            
            using activity_type::operator();

        };
    };
};

template <typename SchemaT>
using insert = basic_insert<SchemaT>;
    
}
}
}


#endif // UDHO_ACTIVITIES_DB_PG_CRUD_INSERT_H
