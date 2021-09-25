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


#ifndef UDHO_ACTIVITIES_DB_PG_CONSTRUCTS_CONCAT_H
#define UDHO_ACTIVITIES_DB_PG_CONSTRUCTS_CONCAT_H

#include <ozo/query_builder.h>
#include <udho/hazo/node/tag.h>
#include <udho/hazo/map/element.h>
#include <udho/db/pg/constructs/fwd.h>
#include <udho/db/pg/constructs/strings.h>
#include <udho/db/pg/constructs/operators.h>
#include <udho/db/pg/schema/detail.h>

namespace udho{
namespace db{
namespace pg{
    
namespace fn{

template <typename FieldT>
struct stringify{
    static constexpr decltype(auto) ozo_name() {
        using namespace ozo::literals;
        return FieldT::ozo_name(); 
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return FieldT::relate(rel); 
    }
};

template <char... C>
struct stringify<constants::string<C...>>{
    static constexpr decltype(auto) ozo_name() {
        using namespace ozo::literals;
        return constants::string<C...>(); 
    }
    template <typename RelT>
    static constexpr auto relate(RelT) {
        using namespace ozo::literals;
        return constants::string<C...>(); 
    }
};

template <typename H, typename... T>
struct implode{
    static auto ozo_name(){
        return stringify<H>::ozo_name() + constants::comma() + implode<T...>::ozo_name();
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        return stringify<H>::relate(rel) + constants::comma() + implode<T...>::relate(rel);
    }
};

template <typename H>
struct implode<H>{
    static auto ozo_name(){
        return stringify<H>::ozo_name();
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        return stringify<H>::relate(rel);
    }
};

template <template <typename> class MappingT, typename X>
struct attach_or_leave{
    using type = typename X::template attach<MappingT>;
};

template <template <typename> class MappingT, char... C>
struct attach_or_leave<MappingT, constants::string<C...>>{
    using type = constants::string<C...>;
};

}

template <typename... X>
struct concat: udho::util::hazo::element<concat<X...>, std::string>{
    template <typename AliasT>
    using as = pg::alias<concat<X...>, AliasT>;
    using index_type = concat<typename pg::detail::infer_index_type<X>::type...>;
    template <template <typename> class MappingT>
    using attach = concat<typename fn::template attach_or_leave<MappingT, X>::type...>;
    using base = udho::util::hazo::element<concat<X...>, std::string>;
    
    using base::base;
    
    typedef op::eq<concat<X...>>       eq;
    typedef op::neq<concat<X...>>      neq;
    typedef op::lt<concat<X...>>       lt;
    typedef op::gt<concat<X...>>       gt;
    typedef op::lte<concat<X...>>      lte;
    typedef op::gte<concat<X...>>      gte;
    typedef op::like<concat<X...>>     like;
    typedef op::not_like<concat<X...>> not_like;
    typedef op::is<concat<X...>>       is;
    typedef op::is_not<concat<X...>>   is_not;
    typedef op::in<concat<X...>>       in;
    typedef op::not_in<concat<X...>>   not_in;
    
    enum {detached = true};
    
    static constexpr auto key() {
        using namespace boost::hana::literals;
        return "concat"_s;
    }
    static constexpr decltype(auto) ozo_name() {
        using namespace ozo::literals;
        return "concat("_SQL + fn::implode<X...>::ozo_name()  + ")"_SQL; 
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return "concat("_SQL + fn::implode<X...>::relate(rel) + ")"_SQL; 
    }
};
    
}
}
}


#endif // UDHO_ACTIVITIES_DB_PG_CONSTRUCTS_CONCAT_H
