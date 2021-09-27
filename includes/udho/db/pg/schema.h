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
    
}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_SCHEMA_H

