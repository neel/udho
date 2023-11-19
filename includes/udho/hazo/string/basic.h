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


#ifndef UDHO_HAZO_STRING_BASIC_H
#define UDHO_HAZO_STRING_BASIC_H

#include <utility>
#include <array>
#include <type_traits>

namespace udho{
namespace hazo{
namespace string{

namespace detail{
    template <int X>
    struct is_zero: std::false_type{};

    template <>
    struct is_zero<0>: std::true_type{};

    template <typename CharT, CharT X, CharT Y>
    struct is_same: is_zero<X-Y>{};
};

namespace tags{
    struct literal{};
}

template <typename Tag, typename CharT, CharT... C>
struct basic{
    using tag = Tag;

    enum {
        length = sizeof... (C)
    };

    using container_type    = std::array<CharT, length+1>;
    using difference_type   = typename container_type::difference_type;
    using value_type        = typename container_type::value_type;
    using reference         = typename container_type::const_reference;
    using pointer           = typename container_type::const_pointer;
    using iterator          = typename container_type::const_iterator;

    template <CharT... X>
    struct compare{
        enum {
            value = (sizeof... (X) == length) && std::conjunction_v<detail::is_zero<C - X>...>
        };
    };

    constexpr static bool empty() { return length == 0; }
    template <typename OtherTag, typename OtherCharT, OtherCharT... X>
    constexpr bool operator==(const basic<OtherTag, CharT, X...>&) const {
        return std::is_same_v<Tag, OtherTag> && std::is_same_v<CharT, OtherCharT> && compare<X...>::value;
    }
    template <typename OtherTag, typename OtherCharT, OtherCharT... X>
    constexpr bool operator!=(const basic<OtherTag, CharT, X...>& other) const {
        return !operator==(other);
    }

    static CharT at(int i) { return _str[i]; }
    static const CharT* c_str(){ return _str.data(); }
    static std::string str() { return std::string{_str.data()}; }

    iterator begin() const { return _str.begin(); }
    iterator end() const { return _str.end(); }

    private:
        static constexpr std::array<CharT, length+1> _str = {C..., 0};
};

template <typename CharT, CharT... C>
using str = basic<tags::literal, CharT, C...>;

namespace literals{

template <typename CharT, CharT... C>
constexpr str<CharT, C...> operator""_h(){
    return str<CharT, C...>{};
}

}

template <typename StreamT, typename CharT, CharT... C>
StreamT& operator<<(StreamT& stream, const str<CharT, C...>& s){
    stream << s.c_str();
    return stream;
}


}
}
}

#endif // UDHO_HAZO_STRING_BASIC_H
