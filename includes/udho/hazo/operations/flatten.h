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

#ifndef UDHO_HAZO_OPERATIONS_FLATTEN_H
#define UDHO_HAZO_OPERATIONS_FLATTEN_H

#include <udho/hazo/operations/fwd.h>
#include <udho/hazo/operations/append.h>
#include <udho/hazo/operations/first_of.h>
#include <udho/hazo/operations/rest_of.h>

namespace udho{
namespace util{
namespace hazo{
    
namespace operations{

template <template <typename...> class ContainerT, typename InitialT, typename... X>
struct basic_flatten{
    using head = typename first_of<ContainerT, X...>::type;
    using initial = typename append<InitialT, head>::type;
    using rest = typename rest_of<ContainerT, X...>::type;
    using type = typename basic_flatten<ContainerT, initial, rest>::type;
};

template <template <typename...> class ContainerT, typename... X>
struct flatten{
    using type = typename basic_flatten<ContainerT, ContainerT<>, X...>::type;
};

}
    
}
}
}

#endif // UDHO_HAZO_OPERATIONS_FLATTEN_H

