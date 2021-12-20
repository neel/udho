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

#ifndef UDHO_DB_PG_GENERATORS_PARTS_LIMIT_H
#define UDHO_DB_PG_GENERATORS_PARTS_LIMIT_H

#include <ozo/query_builder.h>
#include <udho/db/pg/crud/limit.h>
#include <udho/db/pg/generators/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
namespace generators{
    
/**
 * limit N Offset M part of the select query
 */
template <int Limit, int Offset>
struct limit<pg::limited<Limit, Offset>>{
    const pg::limited<Limit, Offset>& _limited;
    
    limit(const pg::limited<Limit, Offset>& limited): _limited(limited){}
    
    auto operator()(){
        return clause();
    }
    
    auto clause() const {
        using namespace ozo::literals;
        
        return "limit "_SQL + _limited.limit() + " offset "_SQL + _limited.offset();
    }
};

/**
 * limit all offset 0 part of the select query
 */
template <>
struct limit<pg::limited<-1, 0>>{
    const pg::limited<-1, 0>& _limited;
    
    limit(const pg::limited<-1, 0>& limited): _limited(limited){}
    
    auto operator()(){
        using namespace ozo::literals;
        
        return " limit ALL offset 0"_SQL;
    }
    
    auto clause() const {
        using namespace ozo::literals;
        
        return " limit ALL offset 0"_SQL;
    }
};
    
}

}
}
}

#endif // UDHO_DB_PG_GENERATORS_PARTS_LIMIT_H
