#ifndef UDHO_UTILS_URLENCODE_H
#define UDHO_UTILS_URLENCODE_H

#include <iterator>
#include <cctype>
#include <string>
#include <algorithm>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/algorithm/hex.hpp>
#include <stdexcept>

namespace udho {
namespace utils {

namespace encoding {

    enum class flavours {
        cookie, // RFC 6265 Section 4.1.1
        url,    // RFC 3986 Section 2.1
        base64, // RFC 4648 Section 4
        base16  // RFC 4648 Section 8
    };

    namespace detail {
        template <flavours F, typename CharT>
        constexpr bool is_allowed_mark(CharT c) {
            const char ch = static_cast<char>(c);
            if constexpr (F == flavours::cookie) {
                return ch == '-' || ch == '_' || ch == '.' || ch == '~';
            } else if constexpr (F == flavours::url) {
                return ch == '-' || ch == '_' || ch == '.' || ch == '!' ||
                       ch == '~' || ch == '*' || ch == '\'' || ch == '(' || ch == ')';
            }
            return false;
        }
    }

    template <flavours F, typename CharT, std::enable_if_t<F==flavours::cookie || F==flavours::url, bool> = true>
    std::basic_string<CharT> encode(const std::basic_string<CharT>& src) {
        std::basic_string<CharT> result;
        result.reserve(src.size() * 3);

        for (auto c : src) {
            if (c == ' ') {
                if constexpr (F == flavours::cookie) {
                    result += '%'; result += '2'; result += '0';
                } else {
                    result += '+';
                }
            }
            else if (std::isalnum(static_cast<unsigned char>(c))) {
                result += c;
            }
            else if (detail::is_allowed_mark<F>(c)) {
                result += c;
            }
            else {
                // percent-encode via Boost.Hex (lowercase)
                std::string hex;
                hex.reserve(2);
                unsigned char uc = static_cast<unsigned char>(c);
                if constexpr (F == flavours::url) {
                    boost::algorithm::hex(&uc, &uc + 1, std::back_inserter(hex));
                } else { // cookie
                    boost::algorithm::hex_lower(&uc, &uc + 1, std::back_inserter(hex));
                }
                result += '%';
                result += hex;
            }
        }
        return result;
    }

    template <flavours F, typename CharT, std::enable_if_t<F==flavours::cookie || F==flavours::url, bool> = true>
    std::basic_string<CharT> decode(const std::basic_string<CharT>& src) {
        std::basic_string<CharT> result;
        result.reserve(src.size());

        for (auto it = src.begin(); it != src.end(); ++it) {
            switch (*it) {
                case '+':
                    if constexpr (F == flavours::url) result += ' ';
                    else                        result += '+';
                    break;

                case '%': {
                    // need at least '%' + 2 hex digits
                    auto remain = std::distance(it, src.end());
                    if (remain >= 3) {
                        char c1 = *(++it);
                        char c2 = *(++it);
                        // decode two hex chars via Boost.Unhex (case-insensitive)
                        std::string hex;
                        hex.reserve(2);
                        hex.push_back(c1);
                        hex.push_back(c2);
                        std::string out;
                        out.reserve(1);
                        try {
                            boost::algorithm::unhex(hex.begin(), hex.end(), std::back_inserter(out));
                            result += static_cast<CharT>(out[0]);
                        } catch (const boost::algorithm::hex_decode_error&) {
                            // on invalid hex, emit literal '%XY'
                            result += '%'; result += c1; result += c2;
                        }
                    } else {
                        result += '%';
                    }
                    break;
                }

                default:
                    result += *it;
            }
        }
        return result;
    }

    template <flavours F, std::enable_if_t<F==flavours::base64, bool> = true>
    std::string encode(const std::string& input) {
        std::string output;
        output.resize(boost::beast::detail::base64::encoded_size(input.size()));
        auto sz = boost::beast::detail::base64::encode(
            output.data(), input.data(), input.size());
        output.resize(sz);
        return output;
    }

    template <flavours F, std::enable_if_t<F==flavours::base64, bool> = true>
    std::string decode(const std::string& input) {
        std::string output;
        output.resize(boost::beast::detail::base64::decoded_size(input.size()));

        auto [decoded_bytes, processed_chars] =  boost::beast::detail::base64::decode(output.data(), input.data(), input.size());

        const bool is_valid =
            // 1. All characters must be base64 or padding
            (processed_chars == input.size()) &&
            // 2. Length must be multiple of 4
            (input.size() % 4 == 0) &&
            // 3. Padding only at end (max 2 '=')
            (input.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=") == std::string::npos);

        if (!is_valid) {
            throw std::runtime_error("Invalid base64 input");
        }

        output.resize(decoded_bytes);
        return output;
    }

    template <flavours F, std::enable_if_t<F==flavours::base16, bool> = true>
    std::string encode(const std::string& src) {
        std::string result;
        result.reserve(src.size() * 2);
        boost::algorithm::hex_lower(src.begin(), src.end(), std::back_inserter(result));
        return result;
    }

    template <flavours F, std::enable_if_t<F==flavours::base16, bool> = true>
    std::string decode(const std::string& src) {
        std::string result;
        result.reserve(src.size() / 2);
        boost::algorithm::unhex(src.begin(), src.end(), std::back_inserter(result));
        return result;
    }

} // namespace encoding

namespace encode {
    std::string cookie(const std::string& input){
        return encoding::encode<encoding::flavours::cookie>(input);
    }
    std::string url(const std::string& input){
        return encoding::encode<encoding::flavours::url>(input);
    }
    std::string base64(const std::string& input){
        return encoding::encode<encoding::flavours::base64>(input);
    }
    std::string base16(const std::string& input){
        return encoding::encode<encoding::flavours::base16>(input);
    }
}

namespace decode {
    std::string cookie(const std::string& input){
        return encoding::decode<encoding::flavours::cookie>(input);
    }
    std::string url(const std::string& input){
        return encoding::decode<encoding::flavours::url>(input);
    }
    std::string base64(const std::string& input){
        return encoding::decode<encoding::flavours::base64>(input);
    }
    std::string base16(const std::string& input){
        return encoding::decode<encoding::flavours::base16>(input);
    }
}

} // namespace utils
} // namespace udho

#endif // UDHO_UTILS_URLENCODE_H
