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

#include <udho/url/fwd.h>
#include <udho/url/detail/function.h>
#include <udho/hazo/string/basic.h>
#include <udho/url/pattern.h>
#include <udho/hazo/seq/seq.h>
#include <udho/url/tabulate.h>
#include <boost/hana/concat.hpp>

namespace udho{
namespace url{

template <typename F, typename CharT, CharT... C>
struct basic_slot<F, udho::hazo::string::str<CharT, C...>>{
    using function_type          = F;
    using key_type               = udho::hazo::string::str<CharT, C...>;
    using return_type            = typename function_type::return_type;
    using arguments_type         = typename function_type::arguments_type;
    using decayed_arguments_type = typename function_type::decayed_arguments_type;
    template <typename T>
    using valid_args             = typename function_type::template valid_args<T>;

    // template <typename T>
    // using valid_args             = std::conditional_t<
    //                                     std::is_same_v<arguments_type, T>,
    //                                     arguments_type,
    //                                     std::conditional_t<
    //                                         std::is_same_v<decayed_arguments_type, T>,
    //                                         decayed_arguments_type,
    //                                         void
    //                                     >
    //                                 >;

    enum {
        args = function_type::args
    };
    template <int N>
    using arg = typename function_type::template arg<N>;
    template <int N>
    using decayed_arg = typename function_type::template decayed_arg<N>;

    basic_slot(function_type&& f): _fnc(std::move(f)) {}
    // return_type operator()(decayed_arguments_type&& args){ return _fnc(std::move(args)); }
    // return_type operator()(arguments_type&& args){ return _fnc(std::move(args)); }
    // template <typename... Args, typename TupleT>
    // return_type operator()(Args... args, const TupleT& tuple){
    //     return operator()(std::tuple_cat(std::make_tuple(args...), tuple));
    // }
    template <typename T, typename std::enable_if<std::is_same<valid_args<T>, T>::value>::type* = nullptr>
    return_type operator()(T&& args){ return _fnc(std::move(args)); }
    template <typename IteratorT>
    decayed_arguments_type prepare(IteratorT begin, IteratorT end){ return _fnc.prepare(begin, end); }
    template <typename IteratorT>
    return_type operator()(IteratorT begin, IteratorT end) { return _fnc(begin, end); }
    static constexpr key_type key() { return key_type{}; }

    private:
        function_type _fnc;
};

template <typename F, typename CharT, CharT... C, typename MatchT>
struct basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT>: basic_slot<F, udho::hazo::string::str<CharT, C...>>{
    using slot_type              = basic_slot<F, udho::hazo::string::str<CharT, C...>>;
    using function_type          = F;
    using key_type               = udho::hazo::string::str<CharT, C...>;
    using return_type            = typename function_type::return_type;
    using arguments_type         = typename function_type::arguments_type;
    using decayed_arguments_type = typename function_type::decayed_arguments_type;
    using match_type             = MatchT;
    using pattern_type           = typename match_type::pattern_type;

    basic_action(function_type&& f, const match_type& match): slot_type(std::move(f)), _match(match) {}
    basic_action(slot_type&& slot, match_type&& match): slot_type(std::move(slot)), _match(match) {}
    using slot_type::operator();

    /**
     * checks whether this action matches with the pattern provided
     */
    template <typename Ch>
    bool find(const std::basic_string<Ch>& pattern) const{
        decayed_arguments_type tuple;
        bool found = _match.find(pattern, tuple);
        return found;
    }
    /**
     * invokes the function with the captured arguments if this action matches with the pattern provided
     */
    template <typename Ch>
    bool invoke(const std::basic_string<Ch>& pattern){
        decayed_arguments_type tuple;
        bool found = _match.find(pattern, tuple);
        if(found){
            operator()(std::move(tuple));
        }
        return found;
    }
    template <typename... X>
    std::string operator()(X&&... x) const {
        return fill(decayed_arguments_type(x...));
    }

    std::string fill(const decayed_arguments_type& args) const { return _match.replace(args); }
    const match_type& match() const { return _match; }
    private:
        match_type    _match;
};

template <typename F, typename CharT, CharT... C, typename MatchT>
basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT> operator<<(basic_slot<F, udho::hazo::string::str<CharT, C...>>&& slot, MatchT&& match){
    return basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT>(std::move(slot), std::move(match));
}

template <typename FunctionT, typename CharT, CharT... C>
basic_slot<
    udho::url::detail::encapsulate_function<FunctionT>,
    udho::hazo::string::str<CharT, C...>
> slot(udho::hazo::string::str<CharT, C...>, FunctionT&& function){
    using slot_type = basic_slot<udho::url::detail::encapsulate_function<FunctionT>, udho::hazo::string::str<CharT, C...>>;
    return slot_type(detail::encapsulate_function<FunctionT>(std::move(function)));
}

template <typename FunctionT, typename CharT, CharT... C>
basic_slot<
    udho::url::detail::encapsulate_mem_function<FunctionT>,
    udho::hazo::string::str<CharT, C...>
> slot(udho::hazo::string::str<CharT, C...>, FunctionT&& function, typename detail::function_signature_<FunctionT>::object_type* that){
    using slot_type = basic_slot<udho::url::detail::encapsulate_mem_function<FunctionT>, udho::hazo::string::str<CharT, C...>>;
    return slot_type(detail::encapsulate_mem_function<FunctionT>(std::move(function), that));
}

template <typename LFunctionT, typename LStrT, typename LMatchT, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(basic_action<LFunctionT, LStrT, LMatchT>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    return udho::hazo::make_seq_d(std::move(left), std::move(right));
}

template <typename... Args, typename RFunctionT, typename RStrT, typename RMatchT>
auto operator|(udho::hazo::basic_seq<udho::hazo::by_data, Args...>&& left, basic_action<RFunctionT, RStrT, RMatchT>&& right){
    using lhs_type = udho::hazo::basic_seq<udho::hazo::by_data, Args...>;
    using rhs_type = basic_action<RFunctionT, RStrT, RMatchT>;
    return typename lhs_type::template extend<rhs_type>(left, std::move(right));
}

template <typename... ArgsL, typename... ArgsR>
auto operator|(const udho::hazo::basic_seq<udho::hazo::by_data, ArgsL...>& left, const udho::hazo::basic_seq<udho::hazo::by_data, ArgsR...>& right){
    return left.concat(right);
}


template <typename FunctionT, typename MatchT, typename CharT, CharT... C>
basic_action<udho::url::detail::encapsulate_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>
action(FunctionT&& function, udho::hazo::string::str<CharT, C...>, const MatchT& match){
    using action_type = basic_action<udho::url::detail::encapsulate_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>;
    return action_type(detail::encapsulate_function<FunctionT>(std::move(function)), match);
}

template <typename FunctionT, typename MatchT, typename CharT, CharT... C>
basic_action<udho::url::detail::encapsulate_mem_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>
action(FunctionT&& function, typename detail::function_signature_<FunctionT>::object_type* that, udho::hazo::string::str<CharT, C...>, const MatchT& match){
    using action_type = basic_action<udho::url::detail::encapsulate_mem_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>;
    return action_type(detail::encapsulate_mem_function<FunctionT>(std::move(function), that), match);
}


template <typename StreamT, typename FunctionT, typename MatchT, typename StrT, typename... TailT>
StreamT& operator<<(StreamT& stream, const udho::hazo::basic_seq_d<basic_action<FunctionT, StrT, MatchT>, TailT...>& chain){
    // chain.write(stream);
    tabulate::Table table;
    table.add_row({"method", "label", "args", "pattern", "replacement"});
    for(size_t i = 0; i < 5; ++i) {
        table[0][i].format().font_color(tabulate::Color::yellow).font_style({tabulate::FontStyle::bold});
    }
    tabulize tab(table);
    chain.visit(tab);
    for(size_t i = 0; i < table.size(); ++i) {
        table[i][1].format().font_style({tabulate::FontStyle::bold});
    }
    stream << table;
    return stream;
}

// template <typename StreamT, typename FunctionT, typename MatchT, typename StrT>
// StreamT& operator<<(StreamT& stream, const udho::hazo::capsule<basic_action<FunctionT, StrT, MatchT>>& capsule){
//     stream << std::endl << capsule.data();
//     return stream;
// }
//
// template <typename StreamT, typename FunctionT, typename MatchT, typename StrT>
// StreamT& operator<<(StreamT& stream, const basic_action<FunctionT, StrT, MatchT>& action){
//     std::string args_str;
//     for(int i=0; i != basic_action<FunctionT, StrT, MatchT>::args; ++i){
//         if(i > 0)
//             args_str += ",";
//         args_str += format("${}", i);
//     }
//     std::string label(StrT().c_str());
//     stream << format("{} f({})", label, args_str) << " <= " << action.match().str();
//     return stream;
// }

}
}

#endif // UDHO_URL_ACTION_H
