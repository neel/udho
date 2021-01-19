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

#ifndef UDHO_HAZO_NODE_ENCAPSULATE_H
#define UDHO_HAZO_NODE_ENCAPSULATE_H

#include <type_traits>
#include <udho/hazo/node/fwd.h>


namespace udho{
namespace util{
namespace hazo{

namespace detail{
namespace encapsulate{
    
// { key
template <typename DataT, bool HasKey = false>
struct key_{
    typedef decltype(DataT::key()) key_type;
    static constexpr key_type key() { return DataT::key(); }
};
template <typename DataT>
struct key_<DataT, false>{
    typedef void key_type;
};
// }


// { value
template <typename DataT, bool HasValue = false>
struct value_{
    typedef typename DataT::value_type value_type;
};
template <typename DataT>
struct value_<DataT, false>{
    typedef DataT value_type;
};
// }

// { index
template <typename DataT, bool HasIndex = false>
struct index_{
    typedef typename DataT::index_type index_type;
};
template <typename DataT>
struct index_<DataT, false>{
    typedef DataT index_type;
};
// }
   
}
}
    
template <typename DataT, bool HasKey, bool HasValue, bool HasIndex>
struct encapsulate
    : detail::encapsulate::key_  <DataT, HasKey>,
      detail::encapsulate::value_<DataT, HasValue>,
      detail::encapsulate::index_<DataT, HasIndex>
{};

}
}
}

#endif // UDHO_HAZO_NODE_ENCAPSULATE_H