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

#ifndef UDHO_FOLDING_NODE_PROXY_H
#define UDHO_FOLDING_NODE_PROXY_H

#include <utility>
#include <type_traits>
#include <udho/folding/node/capsule.h>
#include <udho/folding/node/tag.h>
#include <udho/folding/node/node.h>
#include <ctti/type_id.hpp>

namespace udho{
namespace util{
namespace folding{
    
template <typename BeforeT, typename ExpectedT, typename NextT>
struct counter{
    enum {value = BeforeT::template count<NextT>::value + std::is_same<ExpectedT, NextT>::value};
};
  
template <typename PreviousT = void, typename NextT = void>
struct before{
    typedef NextT next_type;
    
    template <typename T>
    using count = counter<PreviousT, NextT, T>;
};

template <typename ExpectedT, typename NextT>
struct counter<before<>, ExpectedT, NextT>{
    enum {value = std::is_same<ExpectedT, NextT>::value};
};

template <>
struct before<void, void>{
    typedef void next_type;
    
    template <typename T>
    using count = counter<before<>, void, T>;
};

template <typename NextT>
struct before<before<>, NextT>{
    typedef NextT next_type;
    
    template <typename T>
    using count = counter<before<>, NextT, T>;
};

template <typename T, int N>
struct group{
    typedef T type;
    enum {index = N};
    
    template <typename StreamT>
    void print(StreamT& stream){
        stream << ctti::nameof<T>() << ", " << N << std::endl;
    }
};

/**
 * sanitize_<
 *      before<>, 
 *      A, B, C, D, E
 *  >
 *  : sanitize_<
 *          before<before<>, A>, 
 *          B, C, D, E
 *      >
 *      : sanitize_<
 *              before<before<before<>, A>, B>, 
 *              C, D, E
 *          >
 *          : sanitize_<
 *                  before<before<before<before<>, A>, B>, C>, 
 *                  D, E
 *              >
 *              : sanitize_<
 *                      before<before<before<before<before<>, A>, B>, C>, D>, 
 *                      E
 *                  >
 *                  : sanitize_<
 *                          before<before<before<before<before<before<>, A>, B>, C>, D>, E>
 *                      >
 */
template <typename BeforeT, typename H = void, typename... Rest>
struct sanitize_: sanitize_<before<BeforeT, H>, Rest...> {
    typedef H head_type;
    typedef sanitize_<before<BeforeT, H>, Rest...> base_type;
    typedef group<H, before<BeforeT, H>::template count<H>::value> group_type;
    
    group_type _group;
    
    template <typename T>
    using count = typename sanitize_<before<BeforeT, H>, Rest...>::template count<T>;
    
    template <typename... ArgT>
    sanitize_(ArgT&... args): base_type(args...), _group(args...){}
    
    template <typename StreamT>
    void print(StreamT& stream){
        _group.print(stream);
        sanitize_<before<BeforeT, H>, Rest...>::print(stream);
    }
};

template <typename BeforeT>
struct sanitize_<BeforeT, void>{
    typedef BeforeT before_type;
    
    template <typename T>
    using count = typename BeforeT::template count<T>;
    
    template <typename... ArgT>
    sanitize_(ArgT&... args){}
    
    template <typename StreamT>
    void print(StreamT&){}
};

template <typename... T>
using sanitize = sanitize_<before<>, T...>;
    
}
}
}

#endif // UDHO_FOLDING_NODE_PROXY_H
