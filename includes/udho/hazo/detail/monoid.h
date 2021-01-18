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


#ifndef UDHO_HAZO_DETAIL_MONOID_H
#define UDHO_HAZO_DETAIL_MONOID_H

namespace udho{
namespace util{
namespace hazo{
namespace detail{

/**
 * seq<seq<A, B, C>, D, E>
 *  : node<monoid<seq<A, B, C>>::head, seq<monoid<seq<A, B, C>>::rest, D, E>>
 * -> node<A, seq<seq<B, C>, D, E>>
 * 
 * seq<seq<B, C>, D, E>
 *  : node<monoid<seq<B, C>>::head, seq<monoid<seq<B, C>>::rest, D, E>>
 * -> node<B, seq<C, D, E>>
 */
template <template <typename, typename...> class ContainerT, typename H>
struct monoid{
    using head = H;
    template <typename Policy, typename... V>
    using extend = ContainerT<Policy, V...>;
    using rest = void;
};

template <template <typename, typename...> class ContainerT, typename Policy, typename H, typename... T>
struct monoid<ContainerT, ContainerT<Policy, H, T...>>{
    using head = typename monoid<ContainerT, H>::head;
    template <typename OtherPolicy, typename... V>
    using extend = typename monoid<ContainerT, H>::template extend<OtherPolicy, T..., V...>;
    using rest = extend<Policy>;
};

}    
}
}
}

#endif // UDHO_HAZO_DETAIL_MONOID_H
