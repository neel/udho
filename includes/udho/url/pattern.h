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
#include <iostream>

namespace udho{
namespace url{

/**
 * @brief patterns to match against string url
 */
namespace pattern{

/**
 * @brief Enumerates the different types of URL pattern formats available for matching and generating URLs in web applications.
 *
 * This enum class provides identifiers for each pattern matching strategy used by the `match` template class,
 * allowing developers to specify the type of pattern matching behavior appropriate for different routing scenarios.
 *
 * @details
 * - `p1729`: Uses a scanf-like pattern matching based on the C++ proposal P1729R2. This format allows for sophisticated
 *   extraction and formatting of URL segments using types and conversions, akin to scanf's functionality in C.
 *   Specification available at: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1729r2.html
 *   Implemented using the `scnlib` library, which can be found at: https://github.com/eliaskosunen/scnlib
 *
 * - `regex`: Supports regular expression based pattern matching. This format is versatile for complex URL pattern
 *   recognition and manipulation, using the standard C++ `<regex>` functionality.
 *
 * - `fixed`: Matches URLs based on fixed string comparisons. This format is used when exact, unparameterized matching
 *   is needed, such as matching specific static paths.
 *
 * - `home`: Specifically designed to match the homepage URL, recognizing both an empty string and the root path ("/").
 *   This is particularly useful for ensuring that the base URL of a site is correctly interpreted as the home page.
 */
enum class formats{
    p1729, ///< Pattern matching using the p1729 scanf-like format specifications.
    regex, ///< Regular expression-based pattern matching.
    fixed, ///< Fixed string comparison for exact URL matching.
    home   ///< Special format for matching the homepage or root URL.
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

/**
 * @brief A template struct for matching and generating URLs based on specified pattern formats.
 *
 * This template is designed to handle different types of URL pattern matching strategies defined by the `formats` enum.
 * It allows the creation of specialized matchers for URLs, depending on the pattern matching technique (like regex, fixed strings, etc.)
 *
 * @tparam format A `pattern::formats` value that specifies the type of pattern matching to be used.
 * @tparam CharT The character type for the URL strings. Default is `char`.
 *
 * @details
 * The `match` struct is specialized based on the provided `format` parameter, which determines how URLs are matched
 * and generated. Each specialization uses a different technique for parsing and constructing URLs, allowing for
 * flexibility in how routing and URL management is handled within applications. The default character type is `char`
 *
 * ### Specializations:
 * - `pattern::formats::p1729`: Uses a scanf-like pattern for sophisticated URL segment extraction.
 * - `pattern::formats::regex`: Utilizes regular expressions for flexible and powerful URL pattern matching.
 * - `pattern::formats::fixed`: Compares URLs against fixed string patterns for exact matching.
 * - `pattern::formats::home`: Matches the root or home URL (`"/"`), treating empty paths as the homepage.
 */
template <pattern::formats format, typename CharT = char>
struct match;

/**
 * @brief A template struct for matching and transforming URL patterns based on scanf-like patterns specified in p1729.
 *
 * This class supports matching URLs using a simplified scanf format, allowing for extraction of components from the URL
 *
 * @tparam CharT Character type for strings.
 * @see pattern::p1729() for a convenient way to create instances of this class.
 * @note Dependency: This class requires the scnlib library to parse the p1729 format strings.
 * @example
 * match<pattern::formats::p1729, char> matcher(udho::url::verb::get, "/user/{}/{:d}", "/user/{}/{}");
 * std::string url = "/user/john/42";
 * std::tuple<std::string, int> args;
 * if(matcher.find(url, args)) {
 *     std::cout << "Captured: " << std::get<0>(args) << ", " << std::get<1>(args) << std::endl; // Outputs: Captured: john, 42
 *     std::string href = matcher.replace(args);
 *     std::cout << href << std::endl;
 *     // Outputs: /user/john/42
 * }
 */
template <typename CharT>
struct match<pattern::formats::p1729, CharT>{
    using string_type  = std::basic_string<CharT>;
    using pattern_type = std::basic_string<CharT>;

    /**
     * @brief Constructs a new match object with method, pattern, and optionally a replacement string (The pattern is copied as replacement if replacement is not provided).
     * @note both the pattern and the replacement string must begin with a /
     * @param method HTTP method associated with the pattern.
     * @param format p1729 scanf-like pattern string for matching.
     * @param replace Replacement string (in p2216 format std::format or fmt) for the matched pattern, using placeholders {} that correspond to captured groups in the format.
     */
    match(udho::url::verb method, const pattern_type& format, const pattern_type& replace = ""): _method(method), _format(format), _replace(!replace.empty() ? replace : format) { check(); }

    /**
     * @brief Returns the scanf-like format string.
     *
     * @return The format pattern.
     */
    const pattern_type format() const { return _format; }
    /**
     * @brief Returns the pattern as a string.
     *
     * @return The pattern as a standard string.
     */
    std::string pattern() const { return format(); }
    /**
     * @brief Returns the replacement string.
     *
     * @return The string used for replacements.
     */
    std::string replacement() const { return _replace; }
    /**
     * @brief Returns a string combining the pattern and replacement.
     *
     * @return Concatenation of pattern and replacement with " -> " in between if they differ.
     */
    std::string str() const { return pattern() == replacement() ? pattern() : pattern()+" -> "+replacement(); }

    /**
     * @brief Attempts to match the pattern within the given subject string and fills a tuple with captured groups.
     *
     * @param subject The string to be checked against the pattern.
     * @param tuple Tuple to store the matched elements.
     * @return True if the pattern matches the subject, otherwise false.
     */
    template <typename TupleT>
    bool find(const string_type& subject, TupleT& tuple) const {
        auto result = detail::scan_helper::apply(subject, _format, tuple);
        return (bool) result;
    }

    /**
     * @brief Replaces the matched elements in the pattern with the provided arguments to create a new string.
     *
     * @tparam Args Types of arguments to format into the replacement string.
     * @param args Arguments to be used for replacing the matched parts.
     * @return A string with the replaced content.
     */
    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }

    /**
     * @brief Returns the HTTP method associated with this pattern.
     *
     * @return The method as an `udho::url::verb`.
     */
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


/**
 * @brief A template struct for strict URL pattern matching
 *
 * This class provides the functionality to match URLs strictly against fixed string patterns.
 *
 * @tparam CharT Character type for strings.
 * @see pattern::fixed() for a convenient way to create instances of this class.
 * @example
 * // Example of matching and generating URLs from a fixed pattern
 * match<char> matcher(udho::url::verb::get, "/example/path", "/example/path");
 * std::string url = "/example/path";
 * if(matcher.find(url)) {
 *     std::cout << "URL matches." << std::endl; // This will output since the URL is an exact match
 *     std::string href = matcher.replace(std::make_tuple()); // Generates URL
 *     std::cout << href << std::endl;
 *     // Outputs: /example/path
 * }
 */
template <typename CharT>
struct match<pattern::formats::fixed, CharT>{
    using string_type  = std::basic_string<CharT>;
    using pattern_type = std::basic_string<CharT>;

    /**
     * @brief Constructs a new match object with an HTTP method, a pattern for matching, and optionally a replacement string.
     *
     * @param method HTTP method associated with the pattern.
     * @param format Fixed string pattern for matching.
     * @param replace String used for URL generation, defaults to the format if not specified.
     */
    match(udho::url::verb method, const pattern_type& format, const pattern_type& replace = ""): _method(method), _format(format), _replace(!replace.empty() ? replace : format) { check(); }
    /**
     * @brief Returns the fixed format string used for matching.
     *
     * @return The fixed format pattern.
     */
    const pattern_type format() const { return _format; }
    /**
     * @brief Returns the pattern as a string.
     *
     * @return The pattern as a standard string.
     */
    std::string pattern() const { return format(); }
    /**
     * @brief Returns the string.
     *
     * @return The string used for URL generation.
     */
    std::string replacement() const { return _replace; }
    /**
     * @brief Returns a string combining the pattern and replacement if they differ, indicating the template for generating URLs.
     *
     * @return Concatenation of pattern and replacement with " -> " in between if they differ, or just the pattern otherwise.
     */
    std::string str() const { return pattern() == replacement() ? pattern() : pattern()+" -> "+replacement(); }

    /**
     * @brief Checks if the subject exactly matches the fixed pattern. As there is nothing to capture the tuple is unused.
     *
     * @param subject The string to be checked against the pattern.
     * @return True if the subject matches the fixed pattern exactly, otherwise false.
     */
    template <typename TupleT>
    bool find(const string_type& subject, TupleT&) const {
        auto result = subject == _format;
        return (bool) result;
    }
    /**
     * @brief Checks if the subject exactly matches the fixed pattern.
     *
     * @param subject The string to be checked against the pattern.
     * @return True if the subject matches the fixed pattern exactly, otherwise false.
     */
    bool find(const string_type& subject) const {
        auto result = subject == _format;
        return (bool) result;
    }

    /**
     * @brief Generates a URL using the provided replacement string.
     * @note: In this fixed pattern specialization, additional arguments are not utilized for the replacement.
     *
     * @tparam Args Types of arguments to format into the generated URL.
     * @param args Arguments to be formatted into the replacement string, although ignored in this fixed pattern context.
     * @return A string with the generated URL.
     */
    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }

    /**
     * @brief Returns the HTTP method associated with this pattern.
     *
     * @return The method as an `udho::url::verb`.
     */
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

/**
 * @brief A specialized match struct for handling the home or root URL pattern in web applications.
 *
 * This specialization of the match struct is designed specifically for matching the root ("/") or an empty string.
 * The ability to match an empty string as home is crucial for the root paths where the absence of a path segment
 * (e.g., accessing the domain without any additional slash) should logically route to the home page.
 *
 * @tparam CharT Character type, specialized to char in this instance.
 * @see pattern::home() for a convenient way to create instances of this class.
 * @example
 * match<udho::url::pattern::formats::home, char> matcher(udho::url::verb::GET);
 * std::string url = "/";
 * if(matcher.find(url)) {
 *     std::cout << "Home URL matched." << std::endl; // Outputs: Home URL matched.
 * }
 *
 * std::string empty_url = "";
 * if(matcher.find(empty_url)) {
 *     std::cout << "Empty URL treated as home." << std::endl; // Outputs: Empty URL treated as home.
 * }
 */
template <>
struct match<pattern::formats::home, char>{
    using string_type  = std::basic_string<char>;
    using pattern_type = std::basic_string<char>;

    /**
     * @brief Constructs a new match object with an HTTP method, implicitly set to handle the root or home pattern.
     *
     * @param method HTTP method associated with the root URL pattern, typically GET for home page requests.
     */
    match(udho::url::verb method): _method(method) {  }

    /**
     * @brief Returns the root path as the pattern for matching.
     *
     * @return Always returns "/", the root path.
     */
    std::string pattern() const { return "/"; }
    /**
     * @brief Returns the root path as the replacement output.
     *
     * @return Always returns "/", the root path.
     */
    std::string replacement() const { return "/"; }


    /**
     * @brief Checks if the subject string exactly matches the root path '/' or is empty. The tuple is unused as there is nothing to capture.
     *
     * @param subject The URL string to be checked against the root path.
     * @return True if the subject matches the root path or is empty, otherwise false.
     */
    template <typename TupleT>
    bool find(const string_type& subject, TupleT&) const {
        auto result = subject.empty() || subject == "/";
        return (bool) result;
    }

    /**
     * @brief Checks if the subject string exactly matches the root path '/' or is empty.
     *
     * @param subject The URL string to be checked against the root path.
     * @return True if the subject matches the root path or is empty, otherwise false.
     */
    bool find(const string_type& subject) const {
        auto result = subject.empty() || subject == "/";
        return (bool) result;
    }

    /**
     * @brief Generates the root path URL, which is /, ignoring any additional arguments.
     *
     * Since the home pattern is fixed, this method ignores any provided arguments and consistently returns the root path.
     *
     * @tparam Args Types of arguments, ignored in this context.
     * @param args Arguments provided, but not utilized.
     * @return Always returns "/", the root path.
     */
    template <typename... Args>
    std::string replace(const std::tuple<Args...>&) const { return "/"; }

    /**
     * @brief Returns the HTTP method associated with this pattern.
     *
     * @return The HTTP method, typically GET for the home page.
     */
    udho::url::verb method() const { return _method; }

    private:
        udho::url::verb _method;
};

/**
 * @brief A template struct for matching url patterns based on regular expressions.
 *
 * This class allows matching of URLs against specified regular expressions and provides functionality
 * to capture parts of the match and use them for constructing new strings.
 *
 * @tparam CharT Character type for strings and regular expressions.
 * @see pattern::regx() for a convenient way to create instances of this class.
 * @example
 * match<pattern::formats::regex, char> matcher(udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}");
 * std::string url = "/user/john/42";
 * if(matcher.find(url)) {
 *     std::cout << "URL matches!" << std::endl;
 * }
 *
 * std::tuple<std::string, std::uint32_t> args;
 * if(matcher.find(url, args)) {
 *     std::string href = matcher.replace(args);
 *     std::cout << href << std::endl;
 *     // Outputs: /user/john/42
 * }
 */
template <typename CharT>
struct match<pattern::formats::regex, CharT>{
    using string_type  = std::basic_string<CharT>;
    using regex_type   = std::basic_regex<CharT>;
    using pattern_type = regex_type;

    /**
     * @brief Constructs a new match object.
     * @note both the pattern and the replacement string must begin with a /
     * @param method HTTP method associated with the pattern.
     * @param pattern Regex pattern for matching URLs must begin with a /
     * @param replace Replacement string (in p2216 format std::format or fmt) for the matched pattern must begin with a /
     */
    match(udho::url::verb method, const string_type& pattern, const std::string& replace): _method(method), _regex(pattern), _pattern(pattern), _replace(replace) { check(); }
    /**
     * @brief getter for the regex pattern.
     *
     * @return The regex pattern.
     */
    const regex_type regex() const { return _regex; }

    /**
     * @brief Returns the string representation of the pattern.
     *
     * @return The pattern as a string.
     */
    std::string pattern() const { return _pattern; }
    /**
     * @brief Returns the replacement string.
     *
     * @return The string used for replacements.
     */
    std::string replacement() const { return _replace; }
    /**
     * @brief Returns a string combining the pattern and replacement.
     *
     * @return Concatenation of pattern and replacement with " -> " in between.
     */
    std::string str() const { return pattern() + " -> " + replacement(); }
    /**
     * @brief Attempts to find and match the regex pattern within the given subject string, storing the results in a tuple.
     * @note The tuple will be filled only if the pattern is completely matched.
     * @param subject The string to be checked against the regex pattern.
     * @param tuple Tuple to store the matched elements.
     * @return True if the pattern matches the subject, otherwise false.
     *
     * @example
     * match<pattern::formats::regex, char> matcher(udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}");
     * std::tuple<std::string, std::uint32_t> args;
     * if(matcher.find("/user/alexa/12345", args)) {
     *     // args now holds the captured "alexa" and 12345
     * }
     */
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

    /**
     * @brief Checks if the subject matches the regex pattern.
     *
     * @param subject The string to be checked.
     * @return True if the pattern matches the subject, otherwise false.
     *
     * @example
     * match<pattern::formats::regex, char> matcher(udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}");
     * auto href = matcher.replace(std::make_tuple("alexa", 12345));
     * // href would be "/user/alexa/12345"
     */
    bool find(const string_type& subject) const {
        std::smatch matches;
        bool found = std::regex_match(subject, matches, _regex);
        return found;
    }

    /**
     * @brief Replaces the matched elements in the pattern with the provided arguments.
     *
     * @tparam Args Types of arguments to format into the replacement string.
     * @param args Arguments to be used for replacing the matched parts.
     * @return A string with the replaced content.
     */
    template <typename... Args>
    std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }

    /**
     * @brief Returns the HTTP method associated with this pattern.
     *
     * @return The method as an `udho::url::verb`.
     */
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


/**
 * @brief Creates a regex pattern match object.
 *
 * This function constructs a `match` object specialized for regex pattern matching.
 * It uses the provided HTTP method, pattern, and replacement string.
 *
 * @tparam CharT The character type of the strings.
 * @param method The HTTP method associated with this pattern (using `boost::beast::http::verb`).
 * @param pattern The regex pattern as a string.
 * @param replace The replacement string formatted according to P2216 (`std::format` style).
 * @return A `match<pattern::formats::regex, CharT>` object.
 * @see pattern::match<pattern::formats::regex, CharT>
 */
template <typename CharT>
struct pattern::match<pattern::formats::regex, CharT> regx(boost::beast::http::verb method, const std::basic_string<CharT>& pattern, const std::basic_string<CharT>& replace){
    return pattern::match<pattern::formats::regex, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M, std::size_t N>
struct pattern::match<pattern::formats::regex, CharT> regx(boost::beast::http::verb method, const CharT(&pattern)[M], const CharT(&replace)[N]){
    return pattern::match<pattern::formats::regex, CharT>{method, pattern, replace};
}

/**
 * @brief Creates a p1729 pattern match object with optional same-pattern replacement.
 *
 * Overloads allow passing string literals directly. If only one pattern is provided,
 * it is used for both matching and replacement, simplifying cases where no transformation is needed.
 *
 * @tparam CharT The character type of the strings.
 * @tparam M, N Compile-time sizes for array inputs.
 * @param method The HTTP method associated with this pattern.
 * @param pattern The p1729 pattern as a string or string literal.
 * @param replace (Optional) The replacement string formatted according to P2216 (`std::format` style), defaults to the pattern if not provided.
 * @return A `match<pattern::formats::p1729, CharT>` object.
 * @see pattern::match<pattern::formats::p1729, CharT>
 */
template <typename CharT>
struct pattern::match<pattern::formats::p1729, CharT> scan(boost::beast::http::verb method, const std::basic_string<CharT>& pattern, const std::basic_string<CharT>& replace){
    return pattern::match<pattern::formats::p1729, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M, std::size_t N>
struct pattern::match<pattern::formats::p1729, CharT> scan(boost::beast::http::verb method, const CharT(&pattern)[M], const CharT(&replace)[N]){
    return pattern::match<pattern::formats::p1729, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M>
struct pattern::match<pattern::formats::p1729, CharT> scan(boost::beast::http::verb method, const CharT(&pattern)[M]){
    return pattern::match<pattern::formats::p1729, CharT>{method, pattern, pattern};
}

/**
 * @brief Creates a fixed pattern match object, optionally using the same string for matching and replacement.
 *
 * This function simplifies the creation of match objects for fixed patterns,
 *
 * @tparam CharT The character type of the strings.
 * @param method The HTTP method associated with this pattern.
 * @param pattern The fixed pattern as a string.
 * @param replace (Optional) The replacement string, defaults to the pattern if not provided.
 * @return A `match<pattern::formats::fixed, CharT>` object.
 * @see pattern::match<pattern::formats::fixed, CharT>
 */
template <typename CharT>
struct pattern::match<pattern::formats::fixed, CharT> fixed(boost::beast::http::verb method, const std::basic_string<CharT>& pattern, const std::basic_string<CharT>& replace){
    return pattern::match<pattern::formats::fixed, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M, std::size_t N>
struct pattern::match<pattern::formats::fixed, CharT> fixed(boost::beast::http::verb method, const CharT(&pattern)[M], const CharT(&replace)[N]){
    return pattern::match<pattern::formats::fixed, CharT>{method, pattern, replace};
}
template <typename CharT, std::size_t M>
struct pattern::match<pattern::formats::fixed, CharT> fixed(boost::beast::http::verb method, const CharT(&pattern)[M]){
    return pattern::match<pattern::formats::fixed, CharT>{method, pattern, pattern};
}

/**
 * @brief Creates a home pattern match object.
 *
 * This function returns a match object specialized for the home URL pattern ("/" or empty string).
 *
 * @param method The HTTP method, typically GET for home page accesses.
 * @return A `match<pattern::formats::home, char>` object.
 * @see pattern::match<pattern::formats::home, char>
 */
inline struct pattern::match<pattern::formats::home, char> home(boost::beast::http::verb method){
    return pattern::match<pattern::formats::home, char>{method};
}


}
}




#endif // UDHO_URL_PATTERN_H
