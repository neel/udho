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

#ifndef UDHO_DB_PG_SCHEMA_OPERATUIONS_H
#define UDHO_DB_PG_SCHEMA_OPERATUIONS_H

#include <type_traits>
#include <udho/db/pg/schema/fwd.h>
#include <udho/db/pg/schema/basic.h>
#include <udho/hazo/operations/fwd.h>
#include <udho/hazo/operations/append.h>
#include <udho/hazo/operations/prepend.h>
#include <udho/hazo/operations/exclude.h>
#include <udho/hazo/operations/first_of.h>
#include <udho/hazo/operations/rest_of.h>

namespace udho{
namespace hazo{
    
namespace operations{
    
template <typename... H, typename... T>
struct first_of<udho::db::pg::basic_schema, udho::db::pg::basic_schema<H...>, T...>{
    using type = typename first_of<udho::db::pg::basic_schema, H...>::type;
};

template <typename... H, typename... T>
struct rest_of<udho::db::pg::basic_schema, udho::db::pg::basic_schema<H...>, T...>{
    using type = typename rest_of<udho::db::pg::basic_schema, H..., T...>::type;
};


template <typename InitialT>
struct basic_flatten<udho::db::pg::basic_schema, InitialT, udho::db::pg::basic_schema<>>{
    using initial = InitialT;
    using rest = void;
    using type = initial;
};

template <typename... X, typename... T>
struct append<udho::db::pg::basic_schema<X...>, T...>{
    using type = udho::db::pg::basic_schema<X..., T...>;
};

template <typename... T>
struct append<udho::db::pg::basic_schema<>, T...>{
    using type = udho::db::pg::basic_schema<T...>;
};

template <typename... X, typename... T>
struct prepend<udho::db::pg::basic_schema<X...>, T...>{
    using type = udho::db::pg::basic_schema<T..., X...>;
};

template <typename... T>
struct prepend<udho::db::pg::basic_schema<>, T...>{
    using type = udho::db::pg::basic_schema<T...>;
};

template <typename H, typename... X, typename U>
struct eliminate<udho::db::pg::basic_schema<H, X...>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using tail = udho::db::pg::basic_schema<X...>;
    using type = typename std::conditional<matched, 
        tail,
        typename prepend<typename eliminate<tail, U>::type, H>::type
    >::type;
};

template <typename H, typename U>
struct eliminate<udho::db::pg::basic_schema<H>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using type = typename std::conditional<matched, 
        udho::db::pg::basic_schema<void>,
        udho::db::pg::basic_schema<H>
    >::type;
};
    
}
    
}
}

#endif // UDHO_DB_PG_SCHEMA_OPERATUIONS_H

