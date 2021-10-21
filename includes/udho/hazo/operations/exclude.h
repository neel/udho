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

#ifndef UDHO_HAZO_OPERATIONS_EXCLUDE_H
#define UDHO_HAZO_OPERATIONS_EXCLUDE_H

#include <udho/hazo/operations/fwd.h>

namespace udho{
namespace hazo{
    
namespace operations{
    
namespace detail{

template <typename ContainerT, typename T, typename... Rest>
struct exclude{
    using type = typename exclude<
        typename eliminate<ContainerT, T>::type, 
        Rest...
    >::type;
};

template <typename ContainerT, typename T>
struct exclude<ContainerT, T>{
    using type = typename eliminate<ContainerT, T>::type;
};

template <typename ContainerT, typename T, typename... Rest>
struct exclude_all{
    using type = typename exclude_all<
        typename eliminate_all<ContainerT, T>::type, 
        Rest...
    >::type;
};

template <typename ContainerT, typename T>
struct exclude_all<ContainerT, T>{
    using type = typename eliminate_all<ContainerT, T>::type;
};

}

template <typename ContainerT, typename... X>
struct exclude{
    using type = typename detail::exclude<ContainerT, X...>::type;
};

template <typename ContainerT, typename... X>
struct exclude_all{
    using type = typename detail::exclude_all<ContainerT, X...>::type;
};

template <typename ContainerT, template <typename> class ConditionT>
struct exclude_if{
    using type = typename eliminate_if<ContainerT, ConditionT>::type;
};


}
    
}
}

#endif // UDHO_HAZO_OPERATIONS_EXCLUDE_H
