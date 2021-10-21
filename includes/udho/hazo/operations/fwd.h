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

#ifndef UDHO_HAZO_OPERATIONS_FWD_H
#define UDHO_HAZO_OPERATIONS_FWD_H

namespace udho{
namespace hazo{
    
namespace operations{

/**
 * @brief eliminate the first occurence of U in the container
 * @note specialized for each conatiner
 * @tparam ContainerT 
 * @tparam U 
 * @ingroup hazo
 */
template <typename ContainerT, typename U>
struct eliminate;

/**
 * @brief eliminate all occurences of type T that yields a true value for ConditionT<T>::value
 * @note specialized for each conatiner
 * @tparam ContainerT 
 * @tparam ConditionT 
 * @ingroup hazo
 */
template <typename ContainerT, template <typename> class ConditionT>
struct eliminate_if;

/**
 * @brief eliminate all occurences of U in the container
 * @note specialized for each conatiner
 * @tparam ContainerT 
 * @tparam U 
 * @ingroup hazo
 */
template <typename ContainerT, typename U>
struct eliminate_all;

/**
 * @brief exclude the first occurence of U in the container
 * @note works on all containers
 * @note uses @ref eliminate internally
 * @tparam ContainerT 
 * @tparam T 
 * @tparam Rest 
 */
template <typename ContainerT, typename... X>
struct exclude;

/**
 * @brief exclude all occurence of U in the container
 * @note works on all containers
 * @note uses @ref eliminate_all internally
 * @tparam ContainerT 
 * @tparam T 
 * @tparam Rest 
 */
template <typename ContainerT, typename... X>
struct exclude_all;

/**
 * @brief exclude all occurences of type T that yields a true value for ConditionT<T>::value
 * @note works on all containers
 * @note uses @ref eliminate_if internally
 * @tparam ContainerT 
 * @tparam T 
 * @tparam Rest 
 */
template <typename ContainerT, template <typename> class ConditionT>
struct exclude_if;

/**
 * @brief append a set of types T... into an existing container
 * @note specialized for each conatiner
 * @tparam ContainerT 
 * @tparam T 
 * @ingroup hazo
 */
template <typename ContainerT, typename... T>
struct append;

/**
 * @brief prepend a set of types T... into an existing container
 * @note specialized for each conatiner
 * @tparam ContainerT 
 * @tparam T 
 * @ingroup hazo
 */
template <typename ContainerT, typename... T>
struct prepend;

}
    
}
}

#endif // UDHO_HAZO_OPERATIONS_FWD_H
