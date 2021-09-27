/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_DB_PG_DECORATORS_KEYS_H
#define UDHO_DB_PG_DECORATORS_KEYS_H

#include <utility>
#include <type_traits>
#include <ozo/query_builder.h>
#include <udho/db/pg/decorators/logical.h>
#include <udho/db/pg/decorators/comma.h>
#include <udho/db/pg/decorators/traits.h>
#include <udho/db/pg/schema/column.h>

namespace udho{
namespace db{
namespace pg{

namespace decorators{
    
template <typename FieldTraitT>
struct basic_keys: private FieldTraitT{
    template <typename... Args>
    basic_keys(Args&&... args): FieldTraitT(std::forward<Args>(args)...){}
    
    template <typename FieldT>
    decltype(auto) operator()(const FieldT& f){
        return FieldTraitT::apply(f);
    }
    template <typename FieldT, typename ResultT>
    decltype(auto) operator()(const FieldT& f, ResultT res){
        using namespace ozo::literals;
        return pg::decorators::comma(""_SQL, std::move(operator()(f)), std::move(res));
    }
    template <typename ResultT>
    auto finish(ResultT res){
        return res;
    }
};

template <typename FieldTraitT, template <bool...> class Enabler, typename... Fields>
struct selected_keys: private FieldTraitT{
    template <typename SubjectT>
    using enable  = typename std::enable_if<Enabler<pg::is_equivalent<SubjectT, Fields>::value...>::value>;
    template <typename SubjectT>
    using disable = typename std::enable_if<!Enabler<pg::is_equivalent<SubjectT, Fields>::value...>::value>;
    
    template <typename... Args>
    selected_keys(Args&&... args): FieldTraitT(std::forward<Args>(args)...){}
    
    template <typename FieldT, typename = typename enable<FieldT>::type>
    auto operator()(const FieldT& f){
        return FieldTraitT::apply(f);
    }
    template <typename FieldT, typename ResultT, typename = typename enable<FieldT>::type>
    auto operator()(const FieldT& f, ResultT res){
        using namespace ozo::literals;
        return pg::decorators::comma(""_SQL, std::move(operator()(f)), std::move(res));
    }
    template <typename FieldT, int = 0, typename = typename disable<FieldT>::type>
    decltype(auto) operator()(const FieldT& /*f*/){
        using namespace ozo::literals;
        return ""_SQL;
    }
    template <typename FieldT, typename ResultT, int = 0, typename = typename disable<FieldT>::type>
    decltype(auto) operator()(const FieldT& /*f*/, ResultT res){
        return res;
    }
    template <typename ResultT>
    auto finish(ResultT res){
        return res;
    }
};


template <typename... Fields>
using keys_only_unqualified = selected_keys<traits::fields::unqualified, logical_or, Fields...>;
template <typename PrefixT, typename... Fields>
using keys_only_prefixed = selected_keys<traits::fields::prefixed<PrefixT>, logical_or, Fields...>;
template <typename... Fields>
struct keys_only: selected_keys<traits::fields::transparent, logical_or, Fields...>{
    template <typename PrefixT>
    static constexpr keys_only_prefixed<PrefixT, Fields...> prefixed(PrefixT&& k) {
        return keys_only_prefixed<PrefixT, Fields...>(std::forward<PrefixT>(k));
    }
    using unqualified = keys_only_unqualified<Fields...>;
};

template <typename... Fields>
using keys_except_unqualified = selected_keys<traits::fields::unqualified, logical_nor, Fields...>;
template <typename PrefixT, typename... Fields>
using keys_except_prefixed = selected_keys<traits::fields::prefixed<PrefixT>, logical_nor, Fields...>;
template <typename... Fields>
struct keys_except: selected_keys<traits::fields::transparent, logical_nor, Fields...>{
    template <typename PrefixT>
    static constexpr keys_except_prefixed<PrefixT, Fields...> prefixed(PrefixT&& k) {
        return keys_except_prefixed<PrefixT, Fields...>(std::forward<PrefixT>(k));
    }
    using unqualified = keys_except_unqualified<Fields...>;
};

struct keys_unqualified: basic_keys<traits::fields::unqualified>{
    template <typename... Fields>
    using only = keys_only_unqualified<Fields...>;
    template <typename... Fields>
    using except = keys_except_unqualified<Fields...>;
};
template <typename PrefixT>
using keys_prefixed = basic_keys<traits::fields::prefixed<PrefixT>>;

struct keys: basic_keys<traits::fields::transparent>{
    template <typename... Fields>
    using only = keys_only<Fields...>;
    template <typename... Fields>
    using except = keys_except<Fields...>;
    
    template <typename PrefixT>
    static constexpr keys_prefixed<PrefixT> prefixed(PrefixT&& k) {
        return keys_prefixed<PrefixT>(std::forward<PrefixT>(k));
    }
    using unqualified = keys_unqualified;
};

}

}
}
}

#endif // UDHO_DB_PG_DECORATORS_KEYS_H
