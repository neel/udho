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

#ifndef UDHO_DB_PG_GENERATORS_PARTS_KEYS_H
#define UDHO_DB_PG_GENERATORS_PARTS_KEYS_H

#include <ozo/query_builder.h>
#include <udho/db/pg/crud/fwd.h>
#include <udho/db/pg/schema/fwd.h>
#include <udho/db/pg/generators/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
namespace generators{
    
/**
 * @brief Generated comma separeted keys as OZO string.
 * e.g. id, first_name, last_name, which can be used inside an insert query.
 * @tparam Fields... One or more fields 
 * @ingroup generators
 */
template <typename... Fields>
struct keys<pg::basic_schema<Fields...>>{
    keys(const pg::basic_schema<Fields...>& schema): _schema(schema){}
    
    /**
     * @brief The function call operator overload generates comma seperated keys for all keys
     */
    auto operator()(){
        return all();
    }
    
    /**
     * @brief Generate comma seperated keys for all keys
     */
    auto all(){
        using namespace ozo::literals;
        return "("_SQL + _schema.unqualified_fields() + ")"_SQL;
    }
    
    /**
     * @brief Generate a comma seperated keys for the selected fields only.
     * @tparam OnlyFields... A subset of fields
     */
    template <typename... OnlyFields>
    auto only(){
        using namespace ozo::literals;
        return "("_SQL + _schema.template unqualified_fields_only<OnlyFields...>() + ")"_SQL;
    }
    
    /**
     * @brief Generate a comma seperated keys for all fields excluding the selected fields.
     * @tparam ExceptFields... A subset of fields
     */
    template <typename... ExceptFields>
    auto except(){
        using namespace ozo::literals;
        return "("_SQL + _schema.template unqualified_fields_except<ExceptFields...>() + ")"_SQL;
    }
    
    private:
        const pg::basic_schema<Fields...>& _schema;
};
    
}

}
}
}

#endif // UDHO_DB_PG_GENERATORS_PARTS_KEYS_H
