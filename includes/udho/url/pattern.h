// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_PATTERN_H
#define UDHO_URL_PATTERN_H

#include <regex>
#include <string>
#include <vector>

namespace udho{
namespace url{
namespace pattern{

template <typename CharT, typename ResultsT>
struct results{
    using string_type  = std::basic_string<CharT>;
    using regex_type   = std::basic_regex<CharT>;
    using results_type = ResultsT;
    using size_type    = typename results_type::size_type;

    bool matched() const { return _matched; }
    const string_type& operator[](std::size_t index) const { return _results.at(index); }
    size_type matches() const { return _results.size(); }
    typename results_type::const_iterator begin() const { return _results.begin(); }
    typename results_type::const_iterator end() const { return _results.end(); }
    size_type count() const { return _results.size(); }

    void match(const string_type& subject, const regex_type& regex){
        _matched = std::regex_match(subject, _results, regex);
    }

    protected:
        bool _matched;
        results_type _results;
};

template <typename SearchT>
struct match;

template <typename CharT>
struct match<std::basic_regex<CharT>>{
    using string_type  = std::basic_string<CharT>;
    using regex_type   = std::basic_regex<CharT>;
    using results_type = results<CharT, std::match_results<typename std::basic_string<CharT>::const_iterator>>;

    match(const regex_type& regex): _regex(regex) {}
    const regex_type regex() const { return _regex; }
    results_type operator()(const string_type& subject) const {
        results_type matches;
        matches.match(subject, _regex);
        return matches;
    }

    private:
        regex_type  _regex;
};


}
}
}

#endif // UDHO_URL_PATTERN_H
