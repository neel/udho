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

#ifndef UDHO_URL_FWD_H
#define UDHO_URL_FWD_H


namespace udho{
namespace url{


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
}

template <typename F, typename CharT, CharT... C>
struct basic_slot;

template <typename FunctionT, typename StrT, typename MatchT>
struct basic_action;

template <typename StrT, typename ActionsT>
struct mount_point;

template <typename MountPointsT>
struct basic_router;

namespace summary{
    struct match;
    struct slot;
    struct action;
    struct mount_point;
    struct router;
}

}
}


#endif // UDHO_URL_FWD_H
