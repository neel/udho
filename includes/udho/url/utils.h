#ifndef UDHO_URL_UTILS_H
#define UDHO_URL_UTILS_H

#include <string>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <magic.h>
#include <iostream>

namespace udho{
namespace url{

namespace utils{


/**
 * @brief Normalizes and secures a filesystem path relative to a root directory
 * @ingroup DoxyG_url_router
 * @tparam Ch Character type (char/wchar_t)
 * @param subject Input path to normalize
 * @param root Base directory to contain the normalized path (default: current_path())
 * @return Normalized absolute path if valid and contained within root, empty path otherwise
 *
 * - Removes leading slash from subject to make it relative to root
 * - Resolves relative paths, symlinks, and dot components using weakly_canonical
 * - Performs security check to prevent directory traversal attacks
 * - Handles filesystem errors by returning empty path
 */
template <typename Ch>
inline std::filesystem::path normalize_path(const std::basic_string<Ch>& subject, const std::filesystem::path& root = std::filesystem::current_path()) {
    std::string relative_subject = subject;
    if (!relative_subject.empty() && relative_subject[0] == '/') {
        relative_subject.erase(0, 1); // Remove the leading slash if present
    }

    std::filesystem::path requested_path = root / relative_subject;
    std::filesystem::path normalized_path;
    try {
        normalized_path = std::filesystem::weakly_canonical(requested_path);
        if (!boost::algorithm::starts_with(normalized_path.string(), root.string())) {
            std::cout << "Security alert: Attempted access outside of the document root. " << normalized_path << " " << root << std::endl;
            return std::filesystem::path{};
        }
    } catch(const std::filesystem::filesystem_error& e) {
        std::cout << "Filesystem error: " << e.what() << std::endl;
        return std::filesystem::path{};
    }
    return normalized_path;
}

/**
 * @brief Determines MIME type of a file using libmagic
 * @ingroup DoxyG_url_router
 * @param path Filesystem path to analyze
 * @return MIME type as string
 * @note Requires libmagic development files during compilation
 */
inline std::string mime_type(const std::filesystem::path& path) {
    static std::unique_ptr<magic_set, decltype(&magic_close)> magic(
        [](){
            auto* ptr = magic_open(MAGIC_MIME_TYPE);
            magic_load(ptr, nullptr);
            return ptr;
        }(),
        &magic_close
    );

    const char* mime = magic_file(magic.get(), path.c_str());
    return mime ? mime : "application/octet-stream";
}

/**
 * @brief Quotes a string with  slash
 * @ingroup DoxyG_url_router
 * @param str std::string
 * @return the same string qouted  with slash (/ caracter)
 * @note If the string already starts with a slash then doest prepend the a slash in the
 *       beginning, similarly if the string already ends with slash then doesn't append a
 *       slash. If the sttring is already quoted with slash on both side then return the
 *       input string as it is.
 */
template <typename Ch>
inline std::basic_string<Ch> slash_quote(const std::basic_string<Ch>& str) {
    if(str.empty()) return "/";

    std::basic_string<Ch> result;
    if(str.front() != '/'){
        result = '/';
    }
    result.append(str);
    if(str.back() != '/'){
        result.push_back('/');
    }
    return result;
}

template <typename Ch>
inline std::basic_string<Ch> slash_quote_left(const std::basic_string<Ch>& str) {
    std::basic_string<Ch> result;
    if(str.front() != '/'){
        result = '/';
    }
    result.append(str);
    return result;
}

template <typename Ch>
inline std::basic_string<Ch> slash_quote_right(const std::basic_string<Ch>& str) {
    std::basic_string<Ch> result;
    result.append(str);
    if(str.back() != '/'){
        result.push_back('/');
    }
    return result;
}

/**
 * @brief Concatenates two path components with exactly one slash between them
 * @ingroup DoxyG_url_router
 * @tparam Ch Character type (char/wchar_t)
 * @param l Left path component
 * @param r Right path component
 * @return Combined path with proper slash separation
 *
 * Handles four cases:
 * 1. Both end/start with slash -> single slash
 * 2. Left ends with slash -> direct concatenation
 * 3. Right starts with slash -> direct concatenation
 * 4. Neither has slash -> add slash between
 */
template <typename Ch>
inline std::basic_string<Ch> slash_concat(const std::basic_string<Ch>& l, const std::basic_string<Ch>& r) {
    if (l.empty()) return r;
    if (r.empty()) return l;

    bool l_ends_with_slash   = (l.back() == '/');
    bool r_starts_with_slash = (r.front() == '/');
    bool double_shash        = ( l_ends_with_slash &&  r_starts_with_slash);
    bool none_slash          = (!l_ends_with_slash && !r_starts_with_slash);
    std::basic_string<Ch> result{l};
    if(double_shash){
        result.pop_back();
    } else if (none_slash) {
        result.push_back('/');
    }
    return result + r;
}


/**
 * @brief Extracts prefix and name from a URI: subject = base/prefix/name
 * @ingroup DoxyG_url_router
 *
 * @param subject  Full URI path
 * @param base     Base URL that must prefix subject (may contain slashes)
 * @param[out] prefix  Path component between base and last slash (may contain slashes)
 * @param[out] name    Final path component (must contain no slashes)
 * @return true if subject starts with normalized base and contains a trailing slash after base, false otherwise
 *
 * @note Normalizes base with a trailing slash. Assumes name contains no '/'.
 */
inline bool extract(const std::string& subject, const std::string& base, std::string& prefix, std::string& name) {
    // Input: /BASE_URL/PREFIX/NAME
    // Assumptions:
    //  BASE_URL may have multiple / characters
    //  PREFIX may have multiple / characters
    //  NAME must not have any / character
    //
    // check if subject starts with base
    // if not return end()
    // split the rest of the string by the last slash
    // take the first part as prefix and the last part as name

    std::string base_url = udho::url::utils::slash_quote(base);
    if (!boost::starts_with(subject, base_url)) {
        return false;
    }

    // Given: subject starts with base_url
    //      -> subject_len >= base_uri_len
    //      -> asset_uri_len >= 0

    const std::size_t subject_len   = subject.size();
    const std::size_t base_uri_len  = base_url.size();
    const std::size_t asset_uri_len = (subject_len-base_uri_len);
    if(0 == asset_uri_len){
        // no asset specified
        return false;
    }
    const auto slash_pos = subject.rfind('/');   // find the seperator between the prefix and the name
    if(slash_pos <= base_uri_len){
        // No '/' found after base_uri
        return false;
    }

    const std::size_t base_uri_slash = base_uri_len;    // since slash_quote adds a trailing slash to the base if there is none
    const std::size_t prefix_len     = slash_pos - base_uri_slash;

    prefix = subject.substr(base_uri_slash, prefix_len);
    name   = subject.substr(slash_pos +1);

    return true;
}

}

}
}

#endif // UDHO_URL_UTILS_H
