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

/**
 * @addtogroup DoxyG_url_op
 * @{
 */

/**
 * @brief Combines two actions into an action table.
 *
 * Creates a new heterogeneous action table containing the two supplied actions
 * in left-to-right order. Both actions are moved into the resulting table.
 *
 * @tparam LFunctionT Callable type used by the left action.
 * @tparam LStrT Compile-time key string type of the left action.
 * @tparam LMatchT URL matching rule type of the left action.
 * @tparam RFunctionT Callable type used by the right action.
 * @tparam RStrT Compile-time key string type of the right action.
 * @tparam RMatchT URL matching rule type of the right action.
 *
 * @param left Left action to place at index zero.
 * @param right Right action to place at index one.
 *
 * @return An action_table containing @p left followed by @p right.
 */
template <typename LFunctionT, typename LStrT, typename LMatchT, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(basic_action<LFunctionT, LStrT, LMatchT>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    return action_table{
                std::forward<basic_action<LFunctionT, LStrT, LMatchT>>(left),
                std::forward<basic_action<RFunctionT, RStrT, RMatchT>>(right)
            };
}

/**
 * @brief Appends an action to an existing action table.
 *
 * The action table and action are supplied as rvalues. The new action is added
 * after all actions already present in the table.
 *
 * @tparam Args Types of the actions already stored in the table.
 * @tparam RFunctionT Callable type used by the appended action.
 * @tparam RStrT Compile-time key string type of the appended action.
 * @tparam RMatchT URL matching rule type of the appended action.
 *
 * @param left Action table to extend.
 * @param right Action to append to the table.
 *
 * @return A new action_table containing the original actions followed by
 *         @p right.
 */
template <typename... Args, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(action_table<Args...>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    return left.append(std::forward<basic_action<RFunctionT, RStrT, RMatchT>>(right));
}

/**
 * @brief Combines two mount points into a mount-points table.
 *
 * Creates a new heterogeneous mount-points table containing the two supplied
 * mount points in left-to-right order. Both mount points are moved into the
 * resulting table.
 *
 * @tparam LStrT Compile-time name string type of the left mount point.
 * @tparam LActionsT Action collection type of the left mount point.
 * @tparam RStrT Compile-time name string type of the right mount point.
 * @tparam RActionsT Action collection type of the right mount point.
 *
 * @param left Left mount point to place at index zero.
 * @param right Right mount point to place at index one.
 *
 * @return A mountpoints_table containing @p left followed by @p right.
 */
template <typename LStrT, typename LActionsT, typename RStrT, typename RActionsT>
auto operator|(mount_point<LStrT, LActionsT>&& left, mount_point<RStrT, RActionsT>&& right){
    return mountpoints_table{
                std::forward<mount_point<LStrT, LActionsT>>(left),
                std::forward<mount_point<RStrT, RActionsT>>(right)
            };
}

/**
 * @brief Appends a mount point to an existing mount-points table.
 *
 * The supplied mount point is placed after all mount points currently stored
 * in the table.
 *
 * @tparam Mountpoints Types of the mount points already stored in the table.
 * @tparam StrT Compile-time name string type of the appended mount point.
 * @tparam ActionsT Action collection type of the appended mount point.
 *
 * @param left Mount-points table to extend.
 * @param right Mount point to append.
 *
 * @return A new mountpoints_table containing the original mount points
 *         followed by @p right.
 */
template <typename... Mountpoints, typename StrT, typename ActionsT>
auto operator|(mountpoints_table<Mountpoints...>&& left,  mount_point<StrT, ActionsT>&& right){
    return left.append(std::forward<mount_point<StrT, ActionsT>>(right));
}

/**
 * @brief Prepends a mount point to an existing mount-points table.
 *
 * Creates a one-element table from the left mount point and concatenates the
 * supplied right-hand table after it.
 *
 * @tparam Mountpoints Types of the mount points stored in the right table.
 * @tparam StrT Compile-time name string type of the prepended mount point.
 * @tparam ActionsT Action collection type of the prepended mount point.
 *
 * @param left Mount point to place at the beginning of the result.
 * @param right Mount-points table to place after @p left.
 *
 * @return A new mountpoints_table containing @p left followed by every mount
 *         point in @p right.
 */
template <typename... Mountpoints, typename StrT, typename ActionsT>
auto operator|(mount_point<StrT, ActionsT>&& left, mountpoints_table<Mountpoints...>&& right){
    return mountpoints_table(std::forward<mount_point<StrT, ActionsT>>(left)).concat(std::forward<mountpoints_table<Mountpoints...>>(right));
}


/**
 * @brief Concatenates two action tables.
 *
 * This overload participates in overload resolution only when every element
 * in both tables is a basic action.
 *
 * The relative ordering of both tables is preserved: all elements from
 * @p left appear before all elements from @p right.
 *
 * @tparam ArgsL Action types stored in the left table.
 * @tparam ArgsR Action types stored in the right table.
 *
 * @param left First action table.
 * @param right Second action table.
 *
 * @return A new action_table containing the actions from @p left followed by
 *         the actions from @p right.
 */
template <typename... ArgsL, typename... ArgsR>
auto operator|(const action_table<ArgsL...>& left, const action_table<ArgsR...>& right)
    -> std::enable_if_t<(std::conjunction_v<detail::is_basic_action<ArgsL>...> && std::conjunction_v<detail::is_basic_action<ArgsR>...>), decltype(left.concat(right))>
{ return left.concat(right); }


/**
 * @brief Concatenates two mount-points tables.
 *
 * This overload participates in overload resolution only when every element
 * in both tables is a mount point.
 *
 * The relative ordering of both tables is preserved: all elements from
 * @p left appear before all elements from @p right.
 *
 * @tparam ArgsL Mount-point types stored in the left table.
 * @tparam ArgsR Mount-point types stored in the right table.
 *
 * @param left First mount-points table.
 * @param right Second mount-points table.
 *
 * @return A new mountpoints_table containing the mount points from @p left
 *         followed by the mount points from @p right.
 */
template <typename... ArgsL, typename... ArgsR>
auto operator|(const mountpoints_table<ArgsL...>& left, const mountpoints_table<ArgsR...>& right)
    -> std::enable_if_t<(std::conjunction_v<detail::is_mount_point<ArgsL>...> && std::conjunction_v<detail::is_mount_point<ArgsR>...>), decltype(left.concat(right))>
{ return left.concat(right); }


/// @}

}
}

#endif // UDHO_URL_OPERATORS_H
