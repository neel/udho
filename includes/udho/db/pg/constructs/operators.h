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

#ifndef UDHO_DB_PG_CONSTRUCTS_OPERATORS_H
#define UDHO_DB_PG_CONSTRUCTS_OPERATORS_H

#include <udho/hazo/node/tag.h>
#include <ozo/query_builder.h>
#include <udho/hazo/map/element.h>
#include <boost/hana/string.hpp>
#include <udho/db/pg/constructs/types.h>
#include <udho/db/pg/schema/detail.h>
#include <udho/hazo/seq/seq.h>
#include <boost/algorithm/string/replace.hpp>

namespace udho{
namespace db{
namespace pg{
 
namespace op{
    
template <typename FieldT>
struct neq: FieldT{
    using FieldT::FieldT;
    using index_type = neq<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = neq<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<neq<FieldT>> val = udho::util::hazo::element_t<neq<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<neq<FieldT>> neq<FieldT>::val;

template <typename FieldT>
struct eq: FieldT{
    using FieldT::FieldT;
    using index_type = eq<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = eq<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<eq<FieldT>> val = udho::util::hazo::element_t<eq<FieldT>>();

    typedef neq<FieldT> no; 
};

template <typename FieldT>
const udho::util::hazo::element_t<eq<FieldT>> eq<FieldT>::val;

template <typename FieldT>
struct lt: FieldT{
    using FieldT::FieldT;
    using index_type = lt<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = lt<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<lt<FieldT>> val = udho::util::hazo::element_t<lt<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<lt<FieldT>> lt<FieldT>::val;

template <typename FieldT>
struct gt: FieldT{
    using FieldT::FieldT;
    using index_type = gt<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = gt<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<gt<FieldT>> val = udho::util::hazo::element_t<gt<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<gt<FieldT>> gt<FieldT>::val;

template <typename FieldT>
struct lte: FieldT{
    using FieldT::FieldT;
    using index_type = lte<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = lte<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<lte<FieldT>> val = udho::util::hazo::element_t<lte<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<lte<FieldT>> lte<FieldT>::val;

template <typename FieldT>
struct gte: FieldT{
    using FieldT::FieldT;
    using index_type = gte<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = gte<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<gte<FieldT>> val = udho::util::hazo::element_t<gte<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<gte<FieldT>> gte<FieldT>::val;

template <typename FieldT>
struct not_like: FieldT{
    using FieldT::FieldT;
    using index_type = not_like<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = not_like<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<not_like<FieldT>> val = udho::util::hazo::element_t<not_like<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<not_like<FieldT>> not_like<FieldT>::val;

template <typename FieldT>
struct like: FieldT{
    using FieldT::FieldT;
    using index_type = like<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = like<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<like<FieldT>> val = udho::util::hazo::element_t<like<FieldT>>();
    
    typedef not_like<FieldT> no;
};

template <typename FieldT>
const udho::util::hazo::element_t<like<FieldT>> like<FieldT>::val;

template <typename FieldT>
struct is_not: FieldT{
    using FieldT::FieldT;
    using index_type = is_not<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = is_not<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<is_not<FieldT>> val = udho::util::hazo::element_t<is_not<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<is_not<FieldT>> is_not<FieldT>::val;

template <typename FieldT>
struct is: FieldT{
    using FieldT::FieldT;
    using index_type = is<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = is<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<is<FieldT>> val = udho::util::hazo::element_t<is<FieldT>>();
    
    typedef is_not<FieldT> no;
};

template <typename FieldT>
const udho::util::hazo::element_t<is<FieldT>> is<FieldT>::val;

template <typename FieldT>
struct not_in: FieldT{
    using FieldT::FieldT;
    using index_type = not_in<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = not_in<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<not_in<FieldT>> val = udho::util::hazo::element_t<not_in<FieldT>>();
};

template <typename FieldT>
const udho::util::hazo::element_t<not_in<FieldT>> not_in<FieldT>::val;

template <typename FieldT>
struct in: FieldT{
    using FieldT::FieldT;
    using index_type = in<typename detail::infer_index_type<FieldT>::type>;
    template <template <typename> class MappingT>
    using attach = in<typename FieldT::template attach<MappingT>>;
    
    const static constexpr udho::util::hazo::element_t<in<FieldT>> val = udho::util::hazo::element_t<in<FieldT>>();

    typedef not_in<FieldT> no;
};

template <typename FieldT>
const udho::util::hazo::element_t<in<FieldT>> in<FieldT>::val;
    
}

}
}
}

#endif // UDHO_DB_PG_CONSTRUCTS_OPERATORS_H
