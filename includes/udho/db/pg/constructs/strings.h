/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_DB_PG_CONSTRUCTS_STRINGS_H
#define UDHO_DB_PG_CONSTRUCTS_STRINGS_H

#include <ozo/query_builder.h>

namespace udho{
namespace db{
namespace pg{
    
/**
 * @brief handful of frequently used ozo string constants 
 * @ingroup pg
 */
namespace constants{
    
/**
 * @brief Declare an ozo string
 * @code 
 * typedef udho::db::pg::constants::string<'h', 'e', 'l', 'l', 'o'> hello;
 * @endcode 
 * @tparam C... 
 */
template <char... C>
using string = ozo::query_builder<
    boost::hana::tuple<
        ozo::query_element<
            boost::hana::string<C...>, 
        ozo::query_text_tag> 
    > 
>;


using empty  = string<>;      ///< empty ozo string
using space  = string<' '>;   ///< space ( )
using hyphen = string<'-'>;   ///< hyphen (-)
using comma  = string<','>;   ///< comma (,)
using dot    = string<'.'>;   ///< dot (.)

/**
 * @brief quoted versions of frequently used characters
 */
namespace quoted{
    
    using empty  = string<>;                    ///< empty
    using space  = string<'\'', ' ', '\''>;     ///< quoted space (' ')
    using hyphen = string<'\'', '-', '\''>;     ///< quoted hyphen ('-')
    using comma  = string<'\'', ',', '\''>;     ///< quoted comma (',')
    using dot    = string<'\'', '.', '\''>;     ///< quoted dot ('.')
    
}

/**
 * @brief parenthesis characters
 */
namespace parenthesis{
    using open  = string<'('>;                  ///< parenthesis open (()
    using close = string<')'>;                  ///< parenthesis close ())
}

/**
 * @brief curly brace characters
 */
namespace curly{
    using open  = string<'{'>;                  ///< curly brace open ({)
    using close = string<'}'>;                  ///< curly brace clone (})
}

/**
 * @brief square brace characters
 */
namespace square{
    using open  = string<'['>;                  ///< square brace open ([)
    using close = string<']'>;                  ///< square brace close (])
}

}
    
/**
 * @}
 */

}
}
}

#endif // UDHO_DB_PG_CONSTRUCTS_STRINGS_H
