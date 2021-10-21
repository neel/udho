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

#ifndef UDHO_UTIL_HAZO_NODE_IO_H
#define UDHO_UTIL_HAZO_NODE_IO_H

#include <string>
#include <ostream>
#include <udho/hazo/node/capsule.h>
#include <udho/hazo/node/proxy.h>
#include <udho/hazo/detail/has_member.h>
#include <udho/hazo/detail/is_streamable.h>
#include <boost/hana/string.hpp>

namespace udho{
namespace hazo{

#ifndef __DOXYGEN__    

namespace detail{
    template <typename ValueT, typename StreamT = std::ostream, bool IsStreamable = detail::is_streamable<StreamT, ValueT>::value>
    struct print_if_streamable_{
        static StreamT& apply(StreamT& stream, const ValueT& v){ 
            stream << v;
            return stream;
        }
    };
    template <typename ValueT, typename StreamT>
    struct print_if_streamable_<ValueT, StreamT, false>{
        static StreamT& apply(StreamT& stream, const ValueT&){ 
            stream << "UNSTREAMABLE";
            return stream;
        }
    };
    template <typename StreamT, char... C>
    struct print_if_streamable_<boost::hana::string<C...>, StreamT, false>{
        static StreamT& apply(StreamT& stream, const boost::hana::string<C...>& str){ 
            stream << str.c_str();
            return stream;
        }
    };
    template <typename DerivedT, typename StreamT>
    struct print_if_streamable_<udho::hazo::element_t<DerivedT>, StreamT, false>{
        static StreamT& apply(StreamT& stream, const udho::hazo::element_t<DerivedT>& e){
            std::string key(e.c_str());
            key.erase(key.size()-1);
            stream << key;
            return stream;
        }
    };
    template <typename ValueT, typename StreamT = std::ostream>
    StreamT& print_if_streamable(StreamT& stream, const ValueT& v){
        return print_if_streamable_<ValueT, StreamT>::apply(stream, v);
    }
}

template <typename ValueT, std::enable_if_t<!std::is_void_v<typename capsule<ValueT, true>::key_type>, bool> = true>
std::ostream& operator<<(std::ostream& stream, const capsule<ValueT, true>& c){
    detail::print_if_streamable(stream, c.key());
    stream << " -> ";
    detail::print_if_streamable(stream, c.value());
    return stream;
}

template <typename ValueT, std::enable_if_t<std::is_void_v<typename capsule<ValueT, true>::key_type>, bool> = true>
std::ostream& operator<<(std::ostream& stream, const capsule<ValueT, true>& c){
    detail::print_if_streamable(stream, c.data());
    return stream;
}

template <typename ValueT>
std::ostream& operator<<(std::ostream& stream, const capsule<ValueT, false>& c){
    stream << c.data();
    return stream;
}

#else

/**
 * @brief print a hazo capsule to std::ostream
 * prints the data of the capsule if the capsule encapsulats a pod type or a class type which does not provide a key()
 * otherwise prints the key() -> value()
 * @tparam X Data Type inside hazo capsule
 * @param stream std::ostream
 * @param cap udho::hazo::capsule<X>
 * @return std::ostream& 
 * @see @ref udho::hazo::capsule
 * @ingroup hazo
 */
template <typename X>
std::ostream& operator<<(std::ostream& stream, const capsule<X>& cap);

#endif // __DOXYGEN__

    
}    
}

#endif // UDHO_UTIL_HAZO_NODE_IO_H
