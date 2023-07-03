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

#ifndef UDHO_URL_FORMAT_H
#define UDHO_URL_FORMAT_H

#include <tuple>

#if __cplusplus < 202002L
    #include <fmt/format.h>
#else
    #include <format>
#endif

namespace udho{
namespace url{

#if __cplusplus < 202002L
    template <typename... Args>
    using format_string = fmt::format_string<Args...>;

    template<typename... Args>
    auto format(format_string<Args...> fmt, Args&&... args){
        return fmt::format(fmt, std::move(args)...);
    }
#else
    template <typename... Args>
    using format_string = std::format_string<Args...>;

    template<typename... Args>
    std::string format(format_string<Args...> fmt, Args&&... args){
        return std::format(fmt, std::forward(args)...);
    }
#endif

namespace detail{

template <typename Tuple, std::size_t... Indices>
std::string format_with_tuple_impl(const std::string& formatString, const Tuple& tuple, std::index_sequence<Indices...>){
    return format(formatString, std::get<Indices>(tuple)...);
}

template <typename... Args>
std::string format_with_tuple(const std::string& formatString, const std::tuple<Args...>& tuple){
    return format_with_tuple_impl(formatString, tuple, std::make_index_sequence<sizeof...(Args)>{});
}

}

template<typename... Args>
std::string format(const std::string& fmt, const std::tuple<Args...>& tuple){
    return detail::format_with_tuple(fmt, tuple);
}

}
}

#endif // UDHO_URL_FORMAT_H
