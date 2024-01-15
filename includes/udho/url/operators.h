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

#ifndef UDHO_URL_OPERATORS_H
#define UDHO_URL_OPERATORS_H

#include <udho/url/action.h>
#include <udho/url/mount.h>
#include <udho/hazo/seq/seq.h>

namespace udho{
namespace url{

template <typename LFunctionT, typename LStrT, typename LMatchT, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(basic_action<LFunctionT, LStrT, LMatchT>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    return udho::hazo::make_seq_d(std::move(left), std::move(right));
}

template <typename... Args, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(udho::hazo::basic_seq<udho::hazo::by_data, Args...>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    using lhs_type = udho::hazo::basic_seq<udho::hazo::by_data, Args...>;
    using rhs_type = basic_action<RFunctionT, RStrT, RMatchT>;
    return typename lhs_type::template extend<rhs_type>(left, std::move(right));
}

template <typename LStrT, typename LActionsT, typename RStrT, typename RActionsT>
auto operator|(mount_point<LStrT, LActionsT>&& left, mount_point<RStrT, RActionsT>&& right){
    return udho::hazo::make_seq_d(std::move(left), std::move(right));
}

template <typename... Args, typename RStrT, typename RActionsT>
auto operator|(udho::hazo::basic_seq<udho::hazo::by_data, Args...>&& left, mount_point<RStrT, RActionsT>&& right){
    using lhs_type = udho::hazo::basic_seq<udho::hazo::by_data, Args...>;
    using rhs_type = mount_point<RStrT, RActionsT>;
    return typename lhs_type::template extend<rhs_type>(left, std::move(right));
}

namespace detail{

template <typename T>
struct is_basic_action : std::false_type {};

template <typename FunctionT, typename StrT, typename MatchT>
struct is_basic_action<basic_action<FunctionT, StrT, MatchT>> : std::true_type {};

}

template <typename... ArgsL, typename... ArgsR>
auto operator|(const udho::hazo::basic_seq<udho::hazo::by_data, ArgsL...>& left, const udho::hazo::basic_seq<udho::hazo::by_data, ArgsR...>& right)
    -> std::enable_if_t<(std::conjunction_v<detail::is_basic_action<ArgsL>...> && std::conjunction_v<detail::is_basic_action<ArgsR>...>), decltype(left.concat(right))>
{ return left.concat(right); }


}
}

#endif // UDHO_URL_OPERATORS_H
