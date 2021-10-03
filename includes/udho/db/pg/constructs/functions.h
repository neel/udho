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

#ifndef UDHO_DB_PG_CONSTRUCTS_FUNCTIONS_H
#define UDHO_DB_PG_CONSTRUCTS_FUNCTIONS_H

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
    
template <typename FieldT, typename Fn, typename ResultT>
struct unary: udho::hazo::element<unary<FieldT, Fn, ResultT>, ResultT>{
    typedef udho::hazo::element<unary<FieldT, Fn, ResultT>, ResultT> base;
    typedef unary<typename pg::detail::infer_index_type<FieldT>::type, Fn, ResultT> index_type;
    
    enum {detached = FieldT::detached};
        
    typedef op::eq<unary<FieldT, Fn, ResultT>>       eq;
    typedef op::neq<unary<FieldT, Fn, ResultT>>      neq;
    typedef op::lt<unary<FieldT, Fn, ResultT>>       lt;
    typedef op::gt<unary<FieldT, Fn, ResultT>>       gt;
    typedef op::lte<unary<FieldT, Fn, ResultT>>      lte;
    typedef op::gte<unary<FieldT, Fn, ResultT>>      gte;
    typedef op::like<unary<FieldT, Fn, ResultT>>     like;
    typedef op::not_like<unary<FieldT, Fn, ResultT>> not_like;
    typedef op::is<unary<FieldT, Fn, ResultT>>       is;
    typedef op::is_not<unary<FieldT, Fn, ResultT>>   is_not;
    typedef op::in<unary<FieldT, Fn, ResultT>>       in;
    typedef op::not_in<unary<FieldT, Fn, ResultT>>   not_in;
    
    template <typename AliasT>
    using as = pg::alias<unary<FieldT, Fn, ResultT>, AliasT>;
    
    template <template <typename> class MappingT>
    using attach = unary<typename FieldT::template attach<MappingT>, Fn, ResultT>;
    
    using base::base;
    
    static constexpr auto key() {
        return Fn::key();
    }
    static constexpr decltype(auto) ozo_name() {
        using namespace ozo::literals;
        return std::move(Fn::name()) + "("_SQL + std::move(FieldT::ozo_name()) + ")"_SQL; 
    }
    template <typename RelT>
    static constexpr auto relate(RelT rel) {
        using namespace ozo::literals;
        return std::move(Fn::name()) + "("_SQL + std::move(FieldT::relate(rel)) + ")"_SQL; 
    }
    
    bool null() const {return !udho::hazo::element<unary<FieldT, Fn, ResultT>, ResultT>::initialized();}
    void null(bool flag) { udho::hazo::element<unary<FieldT, Fn, ResultT>, ResultT>::uninitialize(flag);}
};

struct count_f{
    static constexpr auto key() {
        using namespace boost::hana::literals;
        return "count"_s;
    }
    static constexpr auto name() {
        using namespace ozo::literals;
        return "COUNT"_SQL;
    }
};

struct min_f{
    static constexpr auto key() {
        using namespace boost::hana::literals;
        return "min"_s;
    }
    static constexpr auto name() {
        using namespace ozo::literals;
        return "MIN"_SQL;
    }
};

struct max_f{
    static constexpr auto key() {
        using namespace boost::hana::literals;
        return "max"_s;
    }
    static constexpr auto name() {
        using namespace ozo::literals;
        return "MAX"_SQL;
    }
};

struct avg_f{
    static constexpr auto key() {
        using namespace boost::hana::literals;
        return "avg"_s;
    }
    static constexpr auto name() {
        using namespace ozo::literals;
        return "AVG"_SQL;
    }
};

struct sum_f{
    static constexpr auto key() {
        using namespace boost::hana::literals;
        return "sum"_s;
    }
    static constexpr auto name() {
        using namespace ozo::literals;
        return "SUM"_SQL;
    }
};

}

template <typename FieldT>
using count = fn::unary<FieldT, fn::count_f, std::int64_t>;

template <typename FieldT>
using min = fn::unary<FieldT, fn::min_f, std::int64_t>;

template <typename FieldT>
using max = fn::unary<FieldT, fn::max_f, std::int64_t>;

template <typename FieldT>
using avg = fn::unary<FieldT, fn::avg_f, std::int64_t>;

template <typename FieldT>
using sum = fn::unary<FieldT, fn::sum_f, std::int64_t>;

}
}
}

#endif // UDHO_DB_PG_CONSTRUCTS_FUNCTIONS_H
