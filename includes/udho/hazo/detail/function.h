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

#ifndef UDHO_HAZO_DETAIL_HELPER_FUNCTION_H
#define UDHO_HAZO_DETAIL_HELPER_FUNCTION_H

#include <utility>
#include <functional>
#include <type_traits>
#include <udho/hazo/detail/indices.h>
#include <udho/hazo/detail/extraction_helper.h>
#include <udho/hazo/node/fwd.h>

namespace udho{
namespace hazo{

namespace detail{

    /**
     * extract the function signature
     */
    template <typename T>
    struct function_signature: function_signature<typename T::function_type>{};

    template <typename R, typename... Args>
    struct function_signature<R (*)(Args...)>{
        typedef R                                   return_type;
        typedef std::tuple<std::decay_t<Args>...>   arguments_type;
    };

}

}
}

#endif // UDHO_HAZO_DETAIL_HELPER_FUNCTION_H
