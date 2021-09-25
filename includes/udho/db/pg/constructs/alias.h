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

#ifndef UDHO_ACTIVITIES_DB_PG_CONSTRUCTS_ALIAS_H
#define UDHO_ACTIVITIES_DB_PG_CONSTRUCTS_ALIAS_H

#include <ozo/query_builder.h>
#include <udho/hazo/node/tag.h>
#include <udho/hazo/map/element.h>
#include <udho/db/pg/constructs/fwd.h>

namespace udho{
namespace db{
namespace pg{
    
template <typename SourceT, typename AliasT>
struct alias: AliasT{
    typedef AliasT index_type;
    
    enum {detached = true};
    
    using AliasT::AliasT;
    
    template <template <typename> class MappingT>
    using attach = alias<typename SourceT::template attach<MappingT>, AliasT>;
    
    static constexpr auto key() {
        return AliasT::key();
    }
    static constexpr decltype(auto) ozo_name() {
        using namespace ozo::literals;
        return std::move(SourceT::ozo_name()) + " as "_SQL + std::move(AliasT::ozo_name()); 
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return std::move(SourceT::relate(rel)) + " as "_SQL + std::move(AliasT::ozo_name()); 
    }
};
    
}
}
}

#endif // UDHO_ACTIVITIES_DB_PG_CONSTRUCTS_ALIAS_H

