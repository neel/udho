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

#ifndef UDHO_HAZO_SEQ_OPERATIONS_H
#define UDHO_HAZO_SEQ_OPERATIONS_H

#include <type_traits>
#include <udho/hazo/seq/fwd.h>
#include <udho/hazo/seq/basic.h>
#include <udho/hazo/operations/fwd.h>
#include <udho/hazo/operations/append.h>
#include <udho/hazo/operations/prepend.h>
#include <udho/hazo/operations/eliminate.h>

namespace udho{
namespace hazo{

namespace operations{
    
template <typename... H, typename... T>
struct first_of<basic_seq_d, basic_seq_d<H...>, T...>{
    using type = typename first_of<basic_seq_d, H...>::type;
};

template <typename... H, typename... T>
struct first_of<basic_seq_v, basic_seq_v<H...>, T...>{
    using type = typename first_of<basic_seq_v, H...>::type;
};

template <typename... H, typename... T>
struct rest_of<basic_seq_d, basic_seq_d<H...>, T...>{
    using type = typename rest_of<basic_seq_d, H..., T...>::type;
};

template <typename... H, typename... T>
struct rest_of<basic_seq_v, basic_seq_v<H...>, T...>{
    using type = typename rest_of<basic_seq_v, H..., T...>::type;
};

template <typename InitialT>
struct basic_flatten<basic_seq_d, InitialT, basic_seq_d<>>{
    using initial = InitialT;
    using rest = void;
    using type = initial;
};

template <typename InitialT>
struct basic_flatten<basic_seq_v, InitialT, basic_seq_v<>>{
    using initial = InitialT;
    using rest = void;
    using type = initial;
};
    
template <typename Policy, typename... X, typename... T>
struct append<basic_seq<Policy, X...>, T...>{
    using type = basic_seq<Policy, X..., T...>;
};

template <typename Policy, typename... T>
struct append<basic_seq<Policy>, T...>{
    using type = basic_seq<Policy, T...>;
};

template <typename Policy, typename... X, typename... T>
struct prepend<basic_seq<Policy, X...>, T...>{
    using type = basic_seq<Policy, T..., X...>;
};

template <typename Policy, typename... T>
struct prepend<basic_seq<Policy>, T...>{
    using type = basic_seq<Policy, T...>;
};

template <typename Policy, typename H, typename... X, typename U>
struct eliminate<basic_seq<Policy, H, X...>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using tail = basic_seq<Policy, X...>;
    using type = typename std::conditional<matched, 
        tail,
        typename prepend<typename eliminate<tail, U>::type, H>::type
    >::type;
};

template <typename Policy, typename H, typename U>
struct eliminate<basic_seq<Policy, H>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using type = typename std::conditional<matched, 
        basic_seq<Policy, void>,
        basic_seq<Policy, H>
    >::type;
};
    
}

}
}

#endif // UDHO_HAZO_MAP_OPERATIONS_H
