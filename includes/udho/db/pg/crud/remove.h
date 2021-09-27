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

#ifndef UDHO_ACTIVITIES_DB_PG_CRUD_REMOVE_H
#define UDHO_ACTIVITIES_DB_PG_CRUD_REMOVE_H

#include <udho/db/pg/activities/activity.h>
#include <udho/db/pg/schema.h>
#include <udho/db/common.h>
#include <udho/db/pg/crud/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * remove returning none 
 * modifiable pg::schema<Fields...> for where query
 */
template <typename... Fields>
struct basic_remove{
    
    using with_type = pg::schema<Fields...>;
    
    template <typename RelationT>
    struct generators{
        using from_type   = pg::generators::from<RelationT>;
        using where_type  = pg::generators::where<with_type>;
        
        from_type   from;
        where_type  where;
        
        generators(const with_type& with): where(with){}
        auto operator()() {
            using namespace ozo::literals;
            return "delete "_SQL + std::move(from()) + " "_SQL + std::move(where());
        }
    };
    
    template <typename DerivedT>
    struct activity: pg::activity<DerivedT, db::none>, with_type{
        typedef pg::activity<DerivedT, db::none> activity_type;
        
        template <typename CollectorT, typename... Args>
        activity(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, Args&&... args): 
            activity_type(collector, pool, io), 
            with_type(std::forward<Args>(args)...)
            {}
        template <typename ContextT, typename... T, typename... Args>
        activity(pg::controller<ContextT, T...>& ctrl, Args&&... args): activity(ctrl.data(), ctrl.pool(), ctrl.io(), std::forward<Args>(args)...){}
        
        with_type& with() { return static_cast<with_type&>(*this); }
        
        using activity_type::operator();
    };
};
    
template <typename... Fields>
using remove = basic_remove<Fields...>;

}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_CRUD_REMOVE_H
