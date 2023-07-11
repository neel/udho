// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_WORD_H
#define UDHO_URL_WORD_H

#include <regex>
#include <string>
#include <vector>
#include <udho/url/detail/format.h>
#include <scn/scn.h>
#include <scn/tuple_return.h>

namespace udho{
namespace url{

template <typename CharT>
struct basic_word{
    using string_type = std::basic_string<CharT>;

    basic_word() = default;
    basic_word(const basic_word& other) = default;
    inline basic_word(const CharT* chars): _str(chars) {}
    inline basic_word(const string_type& str): _str(str) {}
    inline string_type& str() { return _str; }
    inline const string_type& str() const { return _str; }
    inline operator string_type const(){ return str(); }
    private:
        string_type _str;
};

using word = basic_word<char>;

template <typename StreamT, typename CharT>
StreamT& operator<<(StreamT& stream, const basic_word<CharT>& word){
    stream << word.str();
    return stream;
}
template <typename StreamT, typename CharT>
StreamT& operator>>(StreamT& stream, basic_word<CharT>& word){
    stream >> word.str();
    return stream;
}

}
}

namespace scn{

template <typename CharT>
struct scanner<udho::url::basic_word<CharT>> : empty_parser {
    template <typename Context>
    error scan(udho::url::basic_word<CharT>& val, Context& ctx) {
        return scan_usertype(ctx, "{:[^/]}", val.str());
    }
};

}

namespace fmt{

template <typename CharT>
struct formatter<udho::url::basic_word<CharT>>: formatter<std::string_view>{
    auto format(const udho::url::basic_word<CharT>& word, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(ctx.out(), "{}", word.str());
    }
};

}


#endif // UDHO_URL_WORD_H
