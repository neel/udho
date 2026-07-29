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

#ifndef UDHO_DB_PG_CONTROLLER_H
#define UDHO_DB_PG_CONTROLLER_H

#include <udho/db/pg/ozo/connection.h>
#include <udho/db/pg/activities/subtask.h>
#include <udho/activities/start.h>
#include <udho/activities/fwd.h>

namespace udho{
namespace db{
namespace pg{
namespace activities{
    
/**
 * @brief Controls one or more pg activities
 * Contains a reference to connection pool, and boost io service .
 * Generally a controller is passed to the constructor of an activity insted of passing
 * the collector, io service, connector seperately. The controller serves the collector
 * through the underlying init activity.
 * @see udho::activities::init
 * @tparam T... Activity types that are to be performed. 
 * @ingroup DoxyG_db_pg
 */
template <typename... Activities>
struct controller: udho::db::pg::activities::subtask<udho::activities::init<Activities...>>{
    typedef udho::activities::init<Activities...> activity_type;
    typedef udho::db::pg::activities::subtask<activity_type> base;
    typedef typename activity_type::collector_type collector_type;
    typedef typename activity_type::accessor_type accessor_type;
    
    /**
     * @brief Construct a new controller object
     * 
     * @param io boost::asio::io_context
     * @param pool udho::db::pg::connection::pool
     */
    controller(boost::asio::io_context& io, pg::connection::pool& pool): _pool(pool), _io(io) {}
    
    /**
     * @brief get the collector object
     * 
     * @return auto 
     */
    auto collector() { return base::_activity->collector(); }
    /**
     * @brief get the collector object
     * 
     * @return auto 
     */
    auto data() const { return base::_activity->collector(); }
    /**
     * @brief get the collector object
     * 
     * @return auto 
     */
    auto data() { return base::_activity->collector(); }
    
    /**
     * @brief Get a reference to the postgresql connection pool
     * 
     * @return pg::connection::pool& 
     */
    pg::connection::pool& pool() { return _pool; }
    /**
     * @brief Get a reference to the underlying boost asio IO Service
     * 
     * @return boost::asio::io_context& 
     */
    boost::asio::io_context& io() { return _io; }
    
    /**
     * @brief Gets success value associated with an activity of type ActivityT
     * 
     * @tparam ActivityT 
     * @return auto 
     */
    template <typename ActivityT>
    auto success() const {
        return udho::activities::accessor<ActivityT>(data()).template success<ActivityT>();
    }
    /**
     * @brief Gets failure value associated with an activity of type ActivityT
     * 
     * @tparam ActivityT 
     * @return auto 
     */
    template <typename ActivityT>
    auto failure() const {
        return udho::activities::accessor<ActivityT>(data()).template failure<ActivityT>();
    }
    
    private:
        pg::connection::pool& _pool;
        boost::asio::io_context& _io;
};

}

template <typename... T>
using controller = pg::activities::controller<T...>;

}
}
}

namespace udho{
namespace activities{

    template <typename... Activities>
    struct collector_of<udho::db::pg::activities::controller<Activities...>>{
        using type = udho::activities::collector<Activities...>;
        static std::shared_ptr<type> apply(udho::db::pg::activities::controller<Activities...>& controller){ return controller->collector(); }
    };

    namespace detail{
        
        template <typename... T>
        struct after<udho::db::pg::activities::controller<T...>>{
            after(udho::db::pg::activities::controller<T...>& before): _before(before){}
            
            template <typename... X>
            void attach(udho::db::pg::activities::subtask<X...>& sub){
                sub.after(_before);
            }

            private:
            udho::db::pg::activities::controller<T...>& _before;
        };
        
    }
}
}

#endif // UDHO_DB_PG_CONTROLLER_H
