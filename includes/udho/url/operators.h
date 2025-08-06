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
#include <udho/url/tables.h>

namespace udho{
namespace url{

template <typename LFunctionT, typename LStrT, typename LMatchT, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(basic_action<LFunctionT, LStrT, LMatchT>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    return action_table{
                std::forward<basic_action<LFunctionT, LStrT, LMatchT>>(left),
                std::forward<basic_action<RFunctionT, RStrT, RMatchT>>(right)
            };
}

template <typename... Args, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(action_table<Args...>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    return left.append(std::forward<basic_action<RFunctionT, RStrT, RMatchT>>(right));
}

template <typename LStrT, typename LActionsT, typename RStrT, typename RActionsT>
auto operator|(mount_point<LStrT, LActionsT>&& left, mount_point<RStrT, RActionsT>&& right){
    return mountpoints_table{
                std::forward<mount_point<LStrT, LActionsT>>(left),
                std::forward<mount_point<RStrT, RActionsT>>(right)
            };
}

template <typename... Mountpoints, typename RStrT, typename RActionsT>
auto operator|(mountpoints_table<Mountpoints...>&& left,  mount_point<RStrT, RActionsT>&& right){
    return left.append(std::forward<mount_point<RStrT, RActionsT>>(right));
}



template <typename... ArgsL, typename... ArgsR>
auto operator|(const action_table<ArgsL...>& left, const action_table<ArgsR...>& right)
    -> std::enable_if_t<(std::conjunction_v<detail::is_basic_action<ArgsL>...> && std::conjunction_v<detail::is_basic_action<ArgsR>...>), decltype(left.concat(right))>
{ return left.concat(right); }


template <typename... ArgsL, typename... ArgsR>
auto operator|(const mountpoints_table<ArgsL...>& left, const mountpoints_table<ArgsR...>& right)
    -> std::enable_if_t<(std::conjunction_v<detail::is_mount_point<ArgsL>...> && std::conjunction_v<detail::is_mount_point<ArgsR>...>), decltype(left.concat(right))>
{ return left.concat(right); }


}
}

#endif // UDHO_URL_OPERATORS_H
