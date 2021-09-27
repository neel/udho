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

#ifndef UDHO_DB_PG_AFTER_H
#define UDHO_DB_PG_AFTER_H

#include <udho/activities/after.h>
#include <udho/db/pg/activities/controller.h>
#include <udho/db/pg/activities/subtask.h>
#include <udho/db/pg/ozo/connection.h>

namespace udho{
namespace db{
namespace pg{
namespace activities{
    
template <typename HeadT, typename... TailT>
using after = udho::activities::basic_after<pg::activities::subtask, HeadT, TailT...>;
    
}

template <typename HeadT, typename... TailT>
pg::activities::after<HeadT, TailT...> after(HeadT& head, TailT&... tail){
    return pg::activities::after<HeadT, TailT...>(head, tail...);
}

struct after_none{
    template <typename ActivityT, typename ContextT, typename... T, typename... Args>
    pg::activities::subtask<ActivityT> perform(pg::controller<ContextT, T...>& controller, Args&&... args){
        pg::activities::subtask<ActivityT> sub = pg::activities::subtask<ActivityT>::with(controller, args...);
        return sub;
    }
};

inline after_none after(){
    return after_none();
}

template <typename ActivityT, typename ContextT, typename... Args>
pg::activities::subtask<ActivityT> perform(ContextT ctx, pg::connection::pool& pool, Args&&... args){
    pg::controller<ContextT, ActivityT> controller(ctx, pool);
    pg::activities::subtask<ActivityT> sub = after().perform<ActivityT>(controller, std::forward<Args>(args)...);
    return sub;
}


}
}
}

#endif // UDHO_DB_PG_AFTER_H
