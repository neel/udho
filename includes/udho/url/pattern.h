// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_PATTERN_H
#define UDHO_URL_PATTERN_H

#include <regex>
#include <string>
#include <vector>
#include <udho/url/detail/format.h>
#include <scn/scn.h>
#include <scn/tuple_return.h>

namespace udho{
namespace url{
namespace pattern{

namespace detail{
    // https://stackoverflow.com/a/17856366
    template <int L, typename... T, std::size_t... I>
    auto tuple_trim_(const std::tuple<T...>& t, std::index_sequence<I...>) {
        return std::make_tuple(std::get<L+I>(t)...);
    }

    template <int L, int R, typename... T>
    auto tuple_trim(const std::tuple<T...>& t) {
        return tuple_trim_<L>(t, std::make_index_sequence<sizeof...(T) - (L+R)>());
    }

    template <int R, typename... T>
    auto tuple_trim_right(const std::tuple<T...>& t){
        return tuple_trim<0,R>(t);
    }
    template <int L, typename... T>
    auto tuple_trim_left(const std::tuple<T...>& t){
        return tuple_trim<L,0>(t);
    }

    struct scan_helper{
        template <typename... Args>
        static auto apply(const std::string& subject, const std::string& format, std::tuple<Args...>& tuple){
            auto result = scn::make_result(subject);
            std::tuple<decltype(result.range()), std::string> subject_format(result.range(), format);
            auto args = std::tuple_cat(subject_format, tuple);
            result = std::apply(scn::scan<decltype(result.range()), std::string, Args...>, args);
            tuple = tuple_trim_left<2>(args);
            return result;
        }
    };
}

template <typename SearchT>
struct match{
    using string_type  = std::string;
    using pattern_type = std::string;

    match(const pattern_type& format): _format(format) {}
    const pattern_type format() const { return _format; }
    template <typename TupleT>
    bool find(const string_type& subject, TupleT& tuple) {
        auto result = detail::scan_helper::apply(subject, _format, tuple);
        return (bool) result;
    }

    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_format, args); }

    private:
        pattern_type _format;
};

template <typename CharT>
struct match<std::basic_regex<CharT>>{
    using string_type  = std::basic_string<CharT>;
    using regex_type   = std::basic_regex<CharT>;
    using pattern_type = regex_type;

    match(const regex_type& regex, const std::string& replace): _regex(regex), _replace(replace) {}
    const regex_type regex() const { return _regex; }
    template <typename TupleT>
    bool find(const string_type& subject, TupleT& tuple) {
        std::smatch matches;
        bool found = std::regex_match(subject, matches, _regex);
        if(found){
            auto begin = matches.cbegin(), end = matches.cend();
            ++begin;
            udho::url::detail::arguments_to_tuple(tuple, begin, end);
        }
        return found;
    }

    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return format(_replace, args); }

    private:
        regex_type  _regex;
        std::string _replace;
};

}

template <typename CharT>
struct pattern::match<std::basic_regex<CharT>> match(const std::basic_regex<CharT>& regex, const std::string& replace){
    return pattern::match<std::basic_regex<CharT>>{regex, replace};
}

struct pattern::match<std::string> match(const std::string& format){
    return pattern::match<std::string>{format};
}

}
}

#endif // UDHO_URL_PATTERN_H
