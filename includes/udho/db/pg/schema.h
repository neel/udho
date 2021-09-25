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

#ifndef UDHO_ACTIVITIES_DB_PG_SCHEMA_H
#define UDHO_ACTIVITIES_DB_PG_SCHEMA_H

#include <udho/hazo.h>
#include <udho/db/common.h>
#include <udho/db/pg/activities/activity.h>
#include <udho/db/pg/decorators.h>
#include <udho/db/pg/generators.h>
#include <udho/db/pg/schema/schema.h>

namespace udho{
namespace db{
namespace pg{

template <typename... Fields>
struct many{
    typedef pg::schema<Fields...> schema_type;
    typedef db::results<schema_type> result_type;
    template <typename... X>
    using exclude = typename schema_type::template exclude<X...>::template translate<many>;
    template <typename... X>
    using include = typename schema_type::template extend<X...>::template translate<many>;
    template <typename RecordT>
    using record_type = db::results<RecordT>;
};
template <typename... Fields>
struct one{
    typedef pg::schema<Fields...> schema_type;
    typedef db::result<schema_type> result_type;
    template <typename... X>
    using exclude = typename schema_type::template exclude<X...>::template translate<one>;
    template <typename... X>
    using include = typename schema_type::template extend<X...>::template translate<one>;
    template <typename RecordT>
    using record_type = db::result<RecordT>;
};

template <typename DerivedT, typename SuccessT>
struct read: pg::activity<DerivedT, typename SuccessT::result_type, typename SuccessT::schema_type>{
    typedef typename SuccessT::schema_type schema_type;
    typedef pg::activity<DerivedT, typename SuccessT::result_type, typename SuccessT::schema_type> activity_type;
    typedef pg::select_generator<schema_type, void> generator_type;
    
    template <typename CollectorT>
    read(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): 
        activity_type(collector, pool, io), 
        generate(schema)
        {}
    using activity_type::operator();
    
    template <typename RecordT>
    struct into: pg::activity<DerivedT, typename SuccessT::template record_type<RecordT>, schema_type>{
        typedef pg::activity<DerivedT, typename SuccessT::template record_type<RecordT>, schema_type> into_activity_type;
        
        template <typename CollectorT>
        into(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): into_activity_type(collector, pool, io), generate(schema){}
        using into_activity_type::operator();
        
        template <typename... Fields>
        struct with: into_activity_type, pg::schema<Fields...>{
            typedef pg::schema<Fields...> with_type;
            typedef pg::select_generator<schema_type, with_type> with_generator_type;
            
            template <typename CollectorT>
            with(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): 
                into_activity_type(collector, pool, io), 
                generate(schema, static_cast<with_type&>(*this))
                {}
            template <typename CollectorT, typename... Args>
            with(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): 
                into_activity_type(collector, pool, io), 
                with_type(args...), 
                generate(schema, static_cast<with_type&>(*this))
                {}
            using into_activity_type::operator();
            
            protected:
                schema_type schema;
                with_generator_type generate;
        };
        
        protected:
            schema_type schema;
            generator_type generate;
    };
    
    template <typename... Fields>
    struct with: activity_type, pg::schema<Fields...>{
        typedef pg::schema<Fields...> with_type;
        typedef pg::select_generator<schema_type, with_type> with_generator_type;
        
        template <typename CollectorT>
        with(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): 
            activity_type(collector, pool, io), 
            generate(schema, static_cast<with_type&>(*this))
            {}
        template <typename CollectorT, typename... Args>
        with(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): 
            activity_type(collector, pool, io), 
            with_type(args...),
            generate(schema, static_cast<with_type&>(*this))
            {}
        using activity_type::operator();
        
        protected:
            schema_type schema;
            with_generator_type generate;
    };
    
    protected:
        schema_type schema;
        generator_type generate;
};

template <typename DerivedT, typename... T>
struct write: read<DerivedT, pg::one<T...>>{
    typedef read<DerivedT, pg::one<T...>> base;
    using base::base;
    using base::operator();
};

template <typename DerivedT>
struct write<DerivedT>: pg::activity<DerivedT, db::none>{
    typedef pg::activity<DerivedT, db::none> activity_type;
    using activity_type::activity_type;
    using activity_type::operator();
    
    template <typename... Fields>
    struct with: activity_type, pg::schema<Fields...>{
        typedef pg::schema<Fields...> with_type;
//         typedef pg::generator<void, with_type> with_generator_type;
        
        template <typename CollectorT>
        with(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io): 
            activity_type(collector, pool, io)/*, 
            generate(static_cast<with_type&>(*this))*/
            {}
        template <typename CollectorT, typename... Args>
        with(CollectorT collector, pg::connection::pool& pool, boost::asio::io_service& io, const Args&... args): 
            activity_type(collector, pool, io), 
            with_type(args...)/*,
            generate(static_cast<with_type&>(*this))*/
            {}
        using activity_type::operator();
        
//         protected:
//             with_generator_type generate;
    };
};
    
}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_SCHEMA_H

