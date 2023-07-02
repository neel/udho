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

#ifndef UDHO_URL_ACTION_H
#define UDHO_URL_ACTION_H

#include <udho/url/detail/function.h>
#include <udho/hazo/string/basic.h>
#include <udho/url/pattern.h>

namespace udho{
namespace url{

template <typename FunctionT, typename SearchT, typename StrT>
struct basic_action;

template <typename F, typename SearchT, typename CharT, CharT... C>
struct basic_action<udho::url::detail::encapsulate_function<F>, SearchT, udho::hazo::string::str<CharT, C...>>{
    using function_type          = udho::url::detail::encapsulate_function<F>;
    using key_type               = udho::hazo::string::str<CharT, C...>;
    using return_type            = typename function_type::return_type;
    using arguments_type         = typename function_type::arguments_type;
    using decayed_arguments_type = typename function_type::decayed_arguments_type;
    using pattern_type           = SearchT;
    using match_type             = udho::url::pattern::match<pattern_type>;
    using results_type           = typename match_type::results_type;

    enum {
        args = function_type::args
    };
    template <int N>
    using arg = typename function_type::template arg<N>;
    template <int N>
    using decayed_arg = typename function_type::template decayed_arg<N>;

    basic_action(function_type&& f, const pattern_type& pattern): _fnc(std::move(f)), _pattern(pattern), _match(pattern) {}
    return_type operator()(decayed_arguments_type&& args){ return _fnc(std::move(args)); }
    return_type operator()(arguments_type&& args){ return _fnc(std::move(args)); }
    template <typename IteratorT>
    decayed_arguments_type prepare(IteratorT begin, IteratorT end){ return _fnc.prepare(begin, end); }
    template <typename IteratorT>
    return_type operator()(IteratorT begin, IteratorT end) { return _fnc(begin, end); }
    static constexpr key_type key() { return key_type{}; }

    const pattern_type& pattern() const { return _pattern; }
    results_type match(const std::string& subject) const{ return _match(subject); }

    private:
        function_type _fnc;
        pattern_type  _pattern;
        match_type    _match;
};

template <typename F, typename SearchT, typename CharT, CharT... C>
struct basic_action<udho::url::detail::encapsulate_mem_function<F>, SearchT, udho::hazo::string::str<CharT, C...>>{
    using function_type          = udho::url::detail::encapsulate_mem_function<F>;
    using key_type               = udho::hazo::string::str<CharT, C...>;
    using return_type            = typename function_type::return_type;
    using arguments_type         = typename function_type::arguments_type;
    using object_type            = typename function_type::object_type;
    using decayed_arguments_type = typename function_type::decayed_arguments_type;
    using pattern_type           = SearchT;
    using match_type             = udho::url::pattern::match<pattern_type>;
    using results_type           = typename match_type::results_type;

    enum {
        args = function_type::args
    };
    template <int N>
    using arg = typename function_type::template arg<N>;
    template <int N>
    using decayed_arg = typename function_type::template decayed_arg<N>;

    basic_action(function_type&& f, const pattern_type& pattern): _fnc(std::move(f)), _pattern(pattern), _match(pattern)  {}
    return_type operator()(decayed_arguments_type&& args){ return _fnc(std::move(args)); }
    return_type operator()(arguments_type&& args){ return _fnc(std::move(args)); }
    template <typename IteratorT>
    decayed_arguments_type prepare(IteratorT begin, IteratorT end){ return _fnc.prepare(begin, end); }
    template <typename IteratorT>
    return_type operator()(IteratorT begin, IteratorT end) { return _fnc(begin, end); }
    static constexpr key_type key() { return key_type{}; }

    const pattern_type& pattern() const { return _pattern; }
    results_type match(const std::string& subject) const{ return _match(subject); }

    private:
        function_type _fnc;
        pattern_type  _pattern;
        match_type    _match;
};

template <typename F, typename SearchT, typename CharT, CharT... C>
basic_action<udho::url::detail::encapsulate_mem_function<F>, SearchT, udho::hazo::string::str<CharT, C...>> action(udho::url::detail::encapsulate_mem_function<F>&& function, const SearchT& pattern, udho::hazo::string::str<CharT, C...>){
    return basic_action<udho::url::detail::encapsulate_mem_function<F>, SearchT, udho::hazo::string::str<CharT, C...>>(std::move(function), pattern);
}

template <typename F, typename SearchT, typename CharT, CharT... C>
basic_action<udho::url::detail::encapsulate_function<F>, SearchT, udho::hazo::string::str<CharT, C...>> action(udho::url::detail::encapsulate_function<F>&& function, const SearchT& pattern, udho::hazo::string::str<CharT, C...>){
    return basic_action<udho::url::detail::encapsulate_function<F>, SearchT, udho::hazo::string::str<CharT, C...>>(std::move(function), pattern);
}


}
}

#endif // UDHO_URL_ACTION_H
