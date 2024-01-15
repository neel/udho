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


#ifndef UDHO_HAZO_MAP_HELPERS_H
#define UDHO_HAZO_MAP_HELPERS_H

#include <udho/hazo/node/tag.h>
#include <udho/hazo/detail/extraction_helper.h>
#include <udho/hazo/detail/call_helper.h>

namespace udho{
namespace hazo{

template <typename Policy>
struct map_by_key_helper;

template <>
struct map_by_key_helper<by_data>{
    template <typename DataT>
    static DataT& apply(DataT& d){
        return d;
    }
    template <typename DataT>
    static const DataT& apply(const DataT& d){
        return d;
    }
};
template <>
struct map_by_key_helper<by_value>{
    template <typename DataT, std::enable_if_t< std::is_trivial_v<DataT> || std::is_same_v<std::string, DataT>, bool > = true >
    static DataT& apply(DataT& d){
        return d;
    }
    template <typename DataT, std::enable_if_t< std::is_trivial_v<DataT> || std::is_same_v<std::string, DataT>, bool > = true >
    static const DataT& apply(const DataT& d){
        return d;
    }

    template <typename DataT, std::enable_if_t< !std::is_same_v<std::string, DataT>, bool > = true >
    static typename DataT::value_type& apply(DataT& d){
        return d.value();
    }
    template <typename DataT, std::enable_if_t< !std::is_same_v<std::string, DataT>, bool > = true >
    static const typename DataT::value_type& apply(const DataT& d){
        return d.value();
    }
};

template <typename Policy, typename LevelT, std::size_t N>
struct at_helper{
    template <typename NodeT>
    decltype(auto) operator()(NodeT& node){
        return extraction_helper<Policy, NodeT, N>::apply(node);
    }
    template <typename NodeT>
    decltype(auto) operator()(const NodeT& node) const{
        return const_extraction_helper<Policy, NodeT, N>::apply(node);
    }
};

}
}

#endif // UDHO_HAZO_MAP_HELPERS_H

