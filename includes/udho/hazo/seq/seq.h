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


#ifndef UDHO_HAZO_SEQ_SEQ_H
#define UDHO_HAZO_SEQ_SEQ_H

#include <utility>
#include <type_traits>
#include <udho/hazo/node/node.h>
#include <udho/hazo/seq/fwd.h>
#include <udho/hazo/seq/tag.h>
#include <udho/hazo/seq/helpers.h>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/seq/basic.h>
#include <udho/hazo/seq/operations.h>

namespace udho{
namespace util{
namespace hazo{

template <typename... T>
using seq_d = typename operations::flatten<basic_seq_d, T...>::type;

template <typename... T>
using seq_v = typename operations::flatten<basic_seq_v, T...>::type;
    
template <typename Policy, typename... X>
basic_seq<Policy, X...> make_seq(const X&... xs){
    return basic_seq<Policy, X...>(xs...);
}

template <typename... X>
seq_d<X...> make_seq_d(const X&... xs){
    return seq_d<X...>(xs...);
}

template <typename... X>
seq_v<X...> make_seq_v(const X&... xs){
    return seq_v<X...>(xs...);
}


}
}
}

#endif // UDHO_HAZO_SEQ_SEQ_H
