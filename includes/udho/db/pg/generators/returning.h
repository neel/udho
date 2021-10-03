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

#ifndef UDHO_DB_PG_GENERATORS_PARTS_RETURNING_H
#define UDHO_DB_PG_GENERATORS_PARTS_RETURNING_H

#include <udho/db/pg/crud/fwd.h>
#include <udho/db/pg/generators/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
namespace generators{
    
/**
 * returning [field]* part of the insert and update query
 */
template <typename... Fields>
struct returning<pg::basic_schema<Fields...>>{
    returning(){}
    
    auto operator()(){
        return all();
    }
    
    auto all(){
        using namespace ozo::literals;
        
        return "returning "_SQL + _schema.unqualified_fields();
    }
    
    template <typename... OnlyFields>
    auto only(){
        using namespace ozo::literals;
        
        return "returning "_SQL + _schema.template unqualified_fields_only<OnlyFields...>();
    }
    
    template <typename... ExceptFields>
    auto except(){
        using namespace ozo::literals;
        
        return "returning "_SQL + _schema.template unqualified_fields_except<ExceptFields...>();
    }
    
    private:
        pg::schema<Fields...> _schema;
};
    
}

}
}
}

#endif // UDHO_DB_PG_GENERATORS_PARTS_RETURNING_H