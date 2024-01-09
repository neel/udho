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
#include <udho/url/word.h>
#include <udho/url/verb.h>
#include <boost/algorithm/string.hpp>
#include <exception>

namespace udho{
namespace url{

namespace pattern{

enum class formats{
    p1729, // Specs: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1729r2.html using library https://github.com/eliaskosunen/scnlib
    regex,
    fixed
};

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

    template <typename T>
    struct sanitize{
        using type = T;
    };

    template <typename CharT>
    struct sanitize<std::basic_string<CharT>>{
        using type = udho::url::basic_word<CharT>;
    };

    struct scan_helper{
        template <typename... Args>
        static auto apply(const std::string& subject, const std::string& format, std::tuple<Args...>& tuple){
            using sanitized_tuple_type = std::tuple<typename sanitize<Args>::type...>;
            sanitized_tuple_type sanitized(tuple);
            auto result = scn::make_result(subject);
            std::tuple<decltype(result.range()), std::string> subject_format(result.range(), format);
            auto args = std::tuple_cat(subject_format, sanitized);
            result = std::apply(scn::scan<decltype(result.range()), std::string, typename sanitize<Args>::type...>, args);
            tuple = tuple_trim_left<2>(args);
            return result;
        }
    };
}

template <pattern::formats format, typename CharT = char>
struct match;

template <typename CharT>
struct match<pattern::formats::p1729, CharT>{
    using string_type  = std::basic_string<CharT>;
    using pattern_type = std::basic_string<CharT>;

    match(udho::url::verb method, const pattern_type& format, const pattern_type& replace = ""): _method(method), _format(format), _replace(!replace.empty() ? replace : format) { check(); }
    const pattern_type format() const { return _format; }
    std::string pattern() const { return format(); }
    std::string replacement() const { return _replace; }
    std::string str() const { return pattern() == replacement() ? pattern() : pattern()+" -> "+replacement(); }
    template <typename TupleT>
    bool find(const string_type& subject, TupleT& tuple) const {
        auto result = detail::scan_helper::apply(subject, _format, tuple);
        return (bool) result;
    }

    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }
    udho::url::verb method() const { return _method; }

    private:
        void check(){
            if(_format.empty()){
                throw std::invalid_argument(udho::url::format("empty format not allowed"));
            }

            if(_format.front() != '/' || _replace.front() != '/'){
                throw std::invalid_argument(udho::url::format("the format ({}) and the replacement ({}) must begin with / character", _format, _replace));
            }
        }
    private:
        udho::url::verb _method;
        pattern_type _format;
        pattern_type _replace;
};


template <typename CharT>
struct match<pattern::formats::fixed, CharT>{
    using string_type  = std::basic_string<CharT>;
    using pattern_type = std::basic_string<CharT>;

    match(udho::url::verb method, const pattern_type& format, const pattern_type& replace = ""): _method(method), _format(format), _replace(!replace.empty() ? replace : format) { check(); }
    const pattern_type format() const { return _format; }
    std::string pattern() const { return format(); }
    std::string replacement() const { return _replace; }
    std::string str() const { return pattern() == replacement() ? pattern() : pattern()+" -> "+replacement(); }
    template <typename TupleT>
    bool find(const string_type& subject, TupleT&) const {
        auto result = boost::starts_with(subject, _format);
        return (bool) result;
    }

    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }
    udho::url::verb method() const { return _method; }

    private:
        void check(){
            if(_format.empty()){
                throw std::invalid_argument(udho::url::format("empty format not allowed"));
            }

            if(_format.front() != '/' || _replace.front() != '/'){
                throw std::invalid_argument(udho::url::format("the format ({}) and the replacement ({}) must begin with / character", _format, _replace));
            }
        }
    private:
        udho::url::verb _method;
        pattern_type _format;
        pattern_type _replace;
};

template <typename CharT>
struct match<pattern::formats::regex, CharT>{
    using string_type  = std::basic_string<CharT>;
    using regex_type   = std::basic_regex<CharT>;
    using pattern_type = regex_type;

    match(udho::url::verb method, const string_type& pattern, const std::string& replace): _method(method), _regex(pattern), _pattern(pattern), _replace(replace) { check(); }
    const regex_type regex() const { return _regex; }
    std::string pattern() const { return _pattern; }
    std::string replacement() const { return _replace; }
    std::string str() const { return pattern() + " -> " + replacement(); }
    template <typename TupleT>
    bool find(const string_type& subject, TupleT& tuple) const {
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
    udho::url::verb method() const { return _method; }

    private:
        void check(){
            if(_pattern.empty()){
                throw std::invalid_argument(udho::url::format("empty format not allowed"));
            }

            if(_pattern.front() != '/' || _replace.front() != '/'){
                throw std::invalid_argument(udho::url::format("the format ({}) and the replacement ({}) must begin with / character", _pattern, _replace));
            }
        }
    private:
        udho::url::verb _method;
        regex_type  _regex;
        std::string _pattern;
        std::string _replace;
};

}

template <typename CharT>
struct pattern::match<pattern::formats::regex, CharT> regx(boost::beast::http::verb method, const std::basic_string<CharT>& pattern, const std::basic_string<CharT>& replace){
    return pattern::match<pattern::formats::regex, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M, std::size_t N>
struct pattern::match<pattern::formats::regex, CharT> regx(boost::beast::http::verb method, const CharT(&pattern)[M], const CharT(&replace)[N]){
    return pattern::match<pattern::formats::regex, CharT>{method, pattern, replace};
}

template <typename CharT>
struct pattern::match<pattern::formats::p1729, CharT> scan(boost::beast::http::verb method, const std::basic_string<CharT>& pattern, const std::basic_string<CharT>& replace){
    return pattern::match<pattern::formats::p1729, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M, std::size_t N>
struct pattern::match<pattern::formats::p1729, CharT> scan(boost::beast::http::verb method, const CharT(&pattern)[M], const CharT(&replace)[N]){
    return pattern::match<pattern::formats::p1729, CharT>{method, pattern, replace};
}

template <typename CharT>
struct pattern::match<pattern::formats::fixed, CharT> fixed(boost::beast::http::verb method, const std::basic_string<CharT>& pattern, const std::basic_string<CharT>& replace){
    return pattern::match<pattern::formats::fixed, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M, std::size_t N>
struct pattern::match<pattern::formats::fixed, CharT> fixed(boost::beast::http::verb method, const CharT(&pattern)[M], const CharT(&replace)[N]){
    return pattern::match<pattern::formats::fixed, CharT>{method, pattern, replace};
}


}
}




#endif // UDHO_URL_PATTERN_H
