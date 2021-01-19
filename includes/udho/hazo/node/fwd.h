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

#ifndef UDHO_HAZO_NODE_FWD_H
#define UDHO_HAZO_NODE_FWD_H

#include <type_traits>
#include <udho/hazo/detail/has_member.h>
#include <udho/hazo/detail/has_member_type.h>

namespace udho{
namespace util{
namespace hazo{

namespace detail{
GENERATE_HAS_MEMBER(key);
GENERATE_HAS_MEMBER(value);
GENERATE_HAS_MEMBER_TYPE(index_type);
GENERATE_HAS_MEMBER_TYPE(value_type);
}
    
template <typename HeadT, typename TailT = void>
struct node;

/**
 * capsule holds data of type data_type inside 
 * some special capsules have value_type which may refer to something  
 * other than data_type depending on what the capsule is encapsulating
 * otherwise data_type and value_type are same
 */
template <typename ValueT, bool IsClass = std::is_class<ValueT>::value>
struct capsule;

template <
    typename DataT, 
    bool HasKey = detail::has_member_key<DataT>::value, 
    bool HasValue = detail::has_member_value<DataT>::value && detail::has_member_type_value_type<DataT>::value,
    bool HasIndex = detail::has_member_type_index_type<DataT>::value
>
struct encapsulate;

}
}
}

#endif // UDHO_HAZO_NODE_FWD_H