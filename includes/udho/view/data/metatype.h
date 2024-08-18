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

#ifndef UDHO_VIEW_METATYPE_H
#define UDHO_VIEW_METATYPE_H

#include <string>
#include <utility>
#include <type_traits>
#include <udho/hazo/seq/seq.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/detail.h>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

namespace detail{

template <typename Head, typename Tail = void>
struct associative;

template <typename PolicyT, typename KeyT, typename ValueT, typename Tail>
struct associative<data::nvp<PolicyT, KeyT, ValueT>, Tail>{
    using head_type = data::nvp<PolicyT, KeyT, ValueT>;
    using tail_type = Tail;

    associative(head_type&& head, tail_type&& tail): _head(std::move(head)), _tail(std::move(tail)) {}

    /**
     * @brief apply the function f untill it returns true
     */
    template <typename Function>
    std::size_t apply_(Function& f, std::size_t count = 0){
        bool res = f(_head);
        if(!res){
            return _tail.apply_(f, count +1);
        } else {
            return count;
        }
    }

    template <typename Function, typename MatchT>
    std::size_t apply_if(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
        }
        return _tail.apply_if(std::forward<Function>(f), std::forward<MatchT>(match), count +1);
    }

    template <typename Function>
    std::size_t apply_all(Function&& f){
        return apply_if(std::forward<Function>(f), detail::match_all{});
    }


#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    nlohmann::json json(const DataT& data) {
        nlohmann::json root = nlohmann::json::object();
        detail::to_json_f jsonifier{root, data};
        apply_if(jsonifier, detail::match_all{});
        return root;
    }

    template <typename DataT>
    void json(DataT& data, const nlohmann::json& json) {
        detail::from_json_f jsonifier{json, data};
        apply_if(jsonifier, detail::match_all{});
    }

#endif

    private:
        head_type _head;
        tail_type _tail;
};

template <typename PolicyT, typename KeyT, typename ValueT>
struct associative<data::nvp<PolicyT, KeyT, ValueT>, void>{
    using head_type = data::nvp<PolicyT, KeyT, ValueT>;
    using tail_type = void;

    associative(head_type&& head): _head(std::move(head)) {}

    template <typename Function>
    std::size_t apply_(Function& f, std::size_t count = 0){
        bool res = f(_head);
        if(!res){
            return count;
        } else {
            return count +1;
        }
    }

    template <typename Function, typename MatchT>
    std::size_t apply_if(Function&& f, MatchT&& match, std::size_t count = 0){
        if(match(_head)){
            f(_head);
            return count +1;
        }
        return count;
    }

    template <typename Function>
    std::size_t apply_all(Function&& f){
        return apply_if(std::forward<Function>(f), detail::match_all{});
    }


#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    nlohmann::json json(const DataT& data) {
        nlohmann::json root = nlohmann::json::object();
        detail::to_json_f jsonifier{root, data};
        apply_if(jsonifier, detail::match_all{});
        return root;
    }

    template <typename DataT>
    void json(DataT& data, const nlohmann::json& json) {
        detail::from_json_f jsonifier{json, data};
        apply_if(jsonifier, detail::match_all{});
    }

#endif

    private:
        head_type _head;
};

// template <>
// struct associative<void, void>{
//     using head_type = void;
//     using tail_type = void;
//
//     template <typename Function>
//     std::size_t apply_(Function& f, std::size_t count = 0){
//         return count;
//     }
//
//     template <typename Function, typename MatchT>
//     std::size_t apply_if(Function&&, MatchT&&, std::size_t count = 0){
//         return count;
//     }
// };

template <typename Lhs, typename Rhs>
struct concat_;

template <typename PolicyT1, typename KeyT1, typename ValueT1, typename PolicyT2, typename KeyT2, typename ValueT2>
struct concat_<data::nvp<PolicyT1, KeyT1, ValueT1>, data::nvp<PolicyT2, KeyT2, ValueT2>>{
    using lhs_type    = data::nvp<PolicyT1, KeyT1, ValueT1>;
    using rhs_type    = data::nvp<PolicyT2, KeyT2, ValueT2>;
    using result_type = associative<rhs_type, associative<lhs_type>>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{associative<rhs_type>{std::forward<rhs_type>(rhs), std::forward<lhs_type>(lhs)}};
    }
};

template <typename HeadT, typename TailT, typename PolicyT, typename KeyT, typename ValueT>
struct concat_<associative<HeadT, TailT>, data::nvp<PolicyT, KeyT, ValueT>>{
    using lhs_type    = associative<HeadT, TailT>;
    using rhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using result_type = associative<rhs_type, lhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::forward<rhs_type>(rhs), std::forward<lhs_type>(lhs)};
    }
};

template <typename PolicyT, typename KeyT, typename ValueT, typename HeadT, typename TailT>
struct concat_<data::nvp<PolicyT, KeyT, ValueT>, associative<HeadT, TailT>>{
    using lhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using rhs_type    = associative<HeadT, TailT>;
    using result_type = associative<HeadT, typename concat_<TailT, lhs_type>::result_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(rhs._head), concat_<TailT, lhs_type>::apply(std::move(rhs._tail), std::forward<lhs_type>(lhs))};
    }
};

template <typename PolicyT, typename KeyT, typename ValueT, typename HeadT>
struct concat_<data::nvp<PolicyT, KeyT, ValueT>, associative<HeadT, void>>{
    using lhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using rhs_type    = associative<HeadT, void>;
    using result_type = associative<HeadT, lhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(rhs._head), std::forward<lhs_type>(lhs)};
    }
};

template <typename PolicyT, typename KeyT, typename ValueT, typename HeadT>
struct concat_<associative<HeadT, void>, data::nvp<PolicyT, KeyT, ValueT>>{
    using lhs_type    = associative<HeadT, void>;
    using rhs_type    = data::nvp<PolicyT, KeyT, ValueT>;
    using result_type = associative<rhs_type, lhs_type>;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::forward<rhs_type>(rhs), std::forward<lhs_type>(lhs)};
    }
};

template <typename HeadT1, typename TailT1, typename HeadT2, typename TailT2>
struct concat_<associative<HeadT1, TailT1>, associative<HeadT2, TailT2>>{
    using lhs_type      = associative<HeadT1, TailT1>;
    using rhs_type      = associative<HeadT2, TailT2>;
    using result_type   = associative<HeadT2, typename concat_<TailT2, lhs_type>::result_type >;

    static result_type apply(lhs_type&& lhs, rhs_type&& rhs){
        return result_type{std::move(rhs._head), concat_<TailT2, lhs_type>::apply(std::move(rhs._tail), std::forward<lhs_type>(lhs)) };
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
// template <typename HeadT1, typename HeadT2, typename TailT2>
// struct concat_<associative<HeadT1, void>, associative<HeadT2, TailT2>>{
//     using lhs_type      = associative<HeadT1, void>;
//     using rhs_type      = associative<HeadT2, TailT2>;
//     using result_type   = associative<HeadT2, typename concat_<TailT2, lhs_type>::result_type >;
//
//     result_type apply(lhs_type&& lhs, rhs_type&& rhs){
//         return result_type{std::move(rhs._head), concat_<TailT2, lhs_type>::apply(std::move(rhs._tail), std::forward<lhs_type>(lhs)) };
//     }
// };

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

template <typename MembersT = void>
struct assoc_;

template <>
struct assoc_<void>{
    assoc_(const std::string& name): _name(name) {}
    const std::string& name() const { return _name; }
    private:
        std::string  _name;
};

template <typename HeadT, typename TailT>
struct assoc_<associative<HeadT, TailT>>{
    using members_type = associative<HeadT, TailT>;

    assoc_(const std::string& name, members_type&& members): _name(name), _members(std::move(members)) {}
    const std::string& name() const { return _name; }

    members_type& members() { return _members; }
    const members_type& members() const { return _members; }

    private:
        std::string  _name;
        members_type _members;
};

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

detail::assoc_<> assoc(const std::string& name){
    return detail::assoc_<>{name};
}

}
}
}

#endif // UDHO_VIEW_METATYPE_H
