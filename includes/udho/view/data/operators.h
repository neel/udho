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

#ifndef UDHO_VIEW_DATA_OPERATORS_H
#define UDHO_VIEW_DATA_OPERATORS_H

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/seq/seq.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/associative.h>
#include <udho/view/data/operators.h>

namespace udho{
namespace view{
namespace data{

namespace detail{

template <typename Lhs, typename Rhs>
struct concat_;

template <typename PolicyT1, typename KeyT1, typename ValueT1, typename PolicyT2, typename KeyT2, typename ValueT2>
struct concat_<data::nvp<PolicyT1, KeyT1, ValueT1>, data::nvp<PolicyT2, KeyT2, ValueT2>>{
    using lhs_type    = data::nvp<PolicyT1, KeyT1, ValueT1>;
    using rhs_type    = data::nvp<PolicyT2, KeyT2, ValueT2>;
    using result_type = associative<lhs_type, associative<rhs_type>>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::forward<lhs_type>(lhs), associative<rhs_type>{std::forward<rhs_type>(rhs)}};
    }
};

template <typename HeadT, typename TailT, typename PolicyT, typename KeyT, typename ValueT>
struct concat_<associative<HeadT, TailT>, data::nvp<PolicyT, KeyT, ValueT>>{
    using lhs_type    = associative<HeadT, TailT>;
    using rhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using result_type = associative<HeadT, typename concat_<TailT, rhs_type>::result_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(lhs._head), concat_<TailT, rhs_type>::apply(std::move(lhs._tail), std::forward<rhs_type>(rhs))};
    }
};

template <typename PolicyT, typename KeyT, typename ValueT, typename HeadT, typename TailT>
struct concat_<data::nvp<PolicyT, KeyT, ValueT>, associative<HeadT, TailT>>{
    using lhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using rhs_type    = associative<HeadT, TailT>;
    using result_type = associative<lhs_type, rhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::forward<lhs_type>(lhs), std::forward<rhs_type>(rhs)};
    }
};

template <typename PolicyT, typename KeyT, typename ValueT, typename HeadT>
struct concat_<data::nvp<PolicyT, KeyT, ValueT>, associative<HeadT, void>>{
    using lhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using rhs_type    = associative<HeadT, void>;
    using result_type = associative<lhs_type, rhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::forward<lhs_type>(lhs), std::forward<rhs_type>(rhs)};
    }
};

template <typename PolicyT, typename KeyT, typename ValueT, typename HeadT>
struct concat_<associative<HeadT, void>, data::nvp<PolicyT, KeyT, ValueT>>{
    using lhs_type    = associative<HeadT, void>;
    using rhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using result_type = associative<HeadT, associative<rhs_type>>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(lhs._head), associative<rhs_type>{std::forward<rhs_type>(rhs)}};
    }
};

template <typename HeadT1, typename TailT1, typename HeadT2, typename TailT2>
struct concat_<associative<HeadT1, TailT1>, associative<HeadT2, TailT2>>{
    using lhs_type      = associative<HeadT1, TailT1>;
    using rhs_type      = associative<HeadT2, TailT2>;
    using result_type   = associative<HeadT1, typename concat_<TailT1, rhs_type>::result_type >;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(lhs._head), concat_<TailT1, rhs_type>::apply(std::move(lhs._tail), std::forward<rhs_type>(rhs)) };
    }
};

template <typename HeadT1, typename TailT1, typename HeadT2>
struct concat_<associative<HeadT1, TailT1>, associative<HeadT2, void>>{
    using lhs_type      = associative<HeadT1, TailT1>;
    using rhs_type      = associative<HeadT2, void>;
    using result_type   = associative<HeadT2, lhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(rhs._head), std::forward<lhs_type>(lhs)};
    }
};

// already covered
template <typename HeadT1, typename HeadT2, typename TailT2>
struct concat_<associative<HeadT1, void>, associative<HeadT2, TailT2>>{
    using lhs_type      = associative<HeadT1, void>;
    using rhs_type      = associative<HeadT2, TailT2>;
    using result_type   = associative<HeadT1, rhs_type>;

    result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(lhs._head), std::forward<rhs_type>(rhs) };
    }
};

template <typename P1, typename K1, typename V1, typename P2, typename K2, typename V2>
typename concat_<data::nvp<P1, K1, V1>, data::nvp<P2, K2, V2>>::result_type concat(data::nvp<P1, K1, V1>&& lhs, data::nvp<P2, K2, V2>&& rhs){
    return concat_<data::nvp<P1, K1, V1>, data::nvp<P2, K2, V2>>::apply(std::forward<data::nvp<P1, K1, V1>>(lhs), std::forward<data::nvp<P2, K2, V2>>(rhs));
}

template <typename P, typename K, typename V, typename H, typename T = void>
typename concat_<data::nvp<P, K, V>, associative<H, T>>::result_type concat(data::nvp<P, K, V>&& lhs, associative<H, T>&& rhs){
    return concat_<data::nvp<P, K, V>, associative<H, T>>::apply(std::forward<data::nvp<P, K, V>>(lhs), std::forward<associative<H, T>>(rhs));
}

template <typename P, typename K, typename V, typename H, typename T = void>
typename concat_<associative<H, T>, data::nvp<P, K, V>>::result_type concat(associative<H, T>&& lhs, data::nvp<P, K, V>&& rhs){
    return concat_<associative<H, T>, data::nvp<P, K, V>>::apply(std::forward<associative<H, T>>(lhs), std::forward<data::nvp<P, K, V>>(rhs));
}

template <typename H1, typename T1, typename H2, typename T2>
typename concat_<associative<H1, T1>, associative<H2, T2>>::result_type concat(associative<H1, T1>&& lhs, associative<H2, T2>&& rhs){
    return concat_<associative<H1, T1>, associative<H2, T2>>::apply(std::forward<associative<H1, T1>>(lhs), std::forward<associative<H2, T2>>(rhs));
}


template <typename HeadT, typename TailT, typename RhsT>
struct concat_<assoc_<associative<HeadT, TailT>>, RhsT>{
    using lhs_type      = assoc_<associative<HeadT, TailT>>;
    using rhs_type      = RhsT;
    using result_type   = assoc_<typename concat_<associative<HeadT, TailT>, rhs_type>::result_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{lhs.name(), concat(std::move(lhs.members()), std::forward<rhs_type>(rhs))};
    }
};

template <typename P, typename K, typename V>
struct concat_<assoc_<void>, data::nvp<P, K, V>>{
    using lhs_type      = assoc_<void>;
    using rhs_type      = data::nvp<P, K, V>;
    using result_type   = assoc_<associative<rhs_type>>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{lhs.name(), associative<rhs_type>(std::forward<rhs_type>(rhs))};
    }
};

template <typename H, typename T>
struct concat_<assoc_<void>, associative<H, T>>{
    using lhs_type      = assoc_<void>;
    using rhs_type      = associative<H, T>;
    using result_type   = assoc_<rhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{lhs.name(), std::forward<rhs_type>(rhs)};
    }
};

template <typename MembersT, typename RhsT>
typename concat_<assoc_<MembersT>, RhsT>::result_type concat(assoc_<MembersT>&& lhs, RhsT&& rhs){
    return concat_<assoc_<MembersT>, RhsT>::apply(std::forward<assoc_<MembersT>>(lhs), std::forward<RhsT>(rhs));
}

// operator, {

template <typename P1, typename K1, typename V1, typename P2, typename K2, typename V2>
typename concat_<data::nvp<P1, K1, V1>, data::nvp<P2, K2, V2>>::result_type operator,(data::nvp<P1, K1, V1>&& lhs, data::nvp<P2, K2, V2>&& rhs){
    return concat(std::forward<data::nvp<P1, K1, V1>>(lhs), std::forward<data::nvp<P2, K2, V2>>(rhs));
}

template <typename P, typename K, typename V, typename H, typename T = void>
typename concat_<data::nvp<P, K, V>, associative<H, T>>::result_type operator,(data::nvp<P, K, V>&& lhs, associative<H, T>&& rhs){
    return concat(std::forward<data::nvp<P, K, V>>(lhs), std::forward<associative<H, T>>(rhs));
}

template <typename P, typename K, typename V, typename H, typename T = void>
typename concat_<associative<H, T>, data::nvp<P, K, V>>::result_type operator,(associative<H, T>&& lhs, data::nvp<P, K, V>&& rhs){
    return concat(std::forward<associative<H, T>>(lhs), std::forward<data::nvp<P, K, V>>(rhs));
}

template <typename H1, typename T1, typename H2, typename T2>
typename concat_<associative<H1, T1>, associative<H2, T2>>::result_type operator,(associative<H1, T1>&& lhs, associative<H2, T2>&& rhs){
    return concat(std::forward<associative<H1, T1>>(lhs), std::forward<associative<H2, T2>>(rhs));
}

template <typename MembersT, typename RhsT>
typename concat_<assoc_<MembersT>, RhsT>::result_type operator,(assoc_<MembersT>&& lhs, RhsT&& rhs){
    return concat(std::forward<assoc_<MembersT>>(lhs), std::forward<RhsT>(rhs));
}

// }



}

}
}
}

#endif // UDHO_VIEW_DATA_OPERATORS_H
