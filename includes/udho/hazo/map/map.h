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

#ifndef UDHO_HAZO_MAP_MAP_H
#define UDHO_HAZO_MAP_MAP_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/map/helpers.h>
#include <udho/hazo/map/tag.h>
#include <udho/hazo/map/fwd.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/detail/fwd.h>
#include <udho/hazo/map/basic.h>
#include <udho/hazo/map/operations.h>

namespace udho{
namespace util{
namespace hazo{

template <typename... T>
using map_d = typename operations::flatten<basic_map_d, T...>::type;

template <typename... T>
using map_v = typename operations::flatten<basic_map_v, T...>::type;


template <typename Policy, typename... X>
basic_map<Policy, X...> make_map(const X&... xs){
    return basic_map<Policy, X...>(xs...);
}

template <typename... X>
map_d<X...> make_map_v(const X&... xs){
    return map_d<X...>(xs...);
}

template <typename... X>
map_v<X...> make_map_v(const X&... xs){
    return map_v<X...>(xs...);
}

}
}
}

#endif // UDHO_HAZO_MAP_MAP_H
