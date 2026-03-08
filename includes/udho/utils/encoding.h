#ifndef UDHO_UTILS_ENCODING_H
#define UDHO_UTILS_ENCODING_H

#include <iterator>
#include <cctype>
#include <string>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <stdexcept>

namespace udho {
namespace utils {

/**
 * @namespace udho::utils::encoding
 * @brief Namespace containing encoding/decoding implementations
 *
 * Provides RFC-compliant implementations for various encoding schemes
 */
namespace encoding {

    /**
     * @enum udho::utils::encoding::flavours
     * @brief Enumeration of supported encoding/decoding flavors
     *
     * Each flavor corresponds to a specific RFC specification
     */
    enum class flavours {
        cookie,     ///< Cookie encoding per RFC 6265 Section 4.1.1
        escape,     ///< HTML escape
        url,        ///< URL encoding per RFC 3986 Section 2.1
        base64,     ///< Standard Base64 per RFC 4648 Section 4
        base64_url, ///< URL-safe Base64 per RFC 4648 Section 5
        base16      ///< Hexadecimal (Base16) per RFC 4648 Section 8
    };

    namespace detail {
        /**
         * @brief Check if a character is allowed unencoded for specific flavor
         * @tparam F Encoding flavor to check against
         * @tparam CharT Character type
         * @param c Character to check
         * @return true if character can remain unencoded, false otherwise
         *
         * Determines allowed characters based on RFC specifications:
         * - Cookie: - _ . ~
         * - URL: - _ . ! ~ * ' ( )
         */
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

    /**
     * @brief Encode data using specified flavor
     * @tparam F Encoding flavor (cookie/url)
     * @tparam CharT Character type
     * @param src Input data to encode
     * @return Encoded string
     *
     * For cookie/url encoding:
     * - Cookie: Spaces become %20, special characters percent-encoded
     * - URL: Spaces become +, special characters percent-encoded
     *
     * @note Uses Boost.Hex for percent encoding
     */
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
                boost::algorithm::hex(&uc, &uc + 1, std::back_inserter(hex));
                result += '%';
                result += hex;
            }
        }
        return result;
    }

    /**
     * @brief Decode data using specified flavor
     * @tparam F Decoding flavor (cookie/url)
     * @tparam CharT Character type
     * @param src Encoded data to decode
     * @return Decoded original data
     * @throws boost::algorithm::hex_decode_error on invalid hex sequences
     *
     * Handles:
     * - Cookie: %20 → space, preserves +
     * - URL: + → space, percent-encoded sequences
     */
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
                            // best effort forgiving decoder would emit literal '%XY' on invalid hex
                            // result += '%';
                            // result += c1;
                            // result += c2;
                            throw;
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

    /**
     * @brief Base64 encode implementations
     * @tparam F Encoding flavor (base64/base64_url)
     * @param input Data to process
     * @return Processed string
     * @throws std::runtime_error on invalid base64 input
     *
     * Features:
     * - base64: Standard encoding with +/ and padding
     * - base64_url: URL-safe encoding with -_ and optional padding
     *
     * Uses OpenSSL's BIO interface for encoding
     */
    template <flavours F, std::enable_if_t<F==flavours::base64 || F==flavours::base64_url, bool> = true>
    std::string encode(const std::string& input) {
        if (input.empty()) return {};

        using bio_ptr = std::unique_ptr<BIO, decltype(&BIO_free_all)>;

        bio_ptr b64{BIO_new(BIO_f_base64()), BIO_free_all};
        bio_ptr mem{BIO_new(BIO_s_mem()), BIO_free_all};
        BIO*    chain = BIO_push(b64.get(), mem.release());

        BIO_set_flags(chain, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(chain, input.data(), static_cast<int>(input.size()));
        BIO_flush(chain);

        BUF_MEM* buf;
        BIO_get_mem_ptr(chain, &buf);
        std::string out(buf->data, buf->length);

        if constexpr (F == flavours::base64_url) {
            std::replace(out.begin(), out.end(), '+', '-');
            std::replace(out.begin(), out.end(), '/', '_');
            while (!out.empty() && out.back() == '=') out.pop_back();
        }
        return out;
    }

    /**
     * @brief Base64 decode implementations
     * @tparam F Encoding flavor (base64/base64_url)
     * @param input Data to process
     * @return Processed string
     * @throws std::runtime_error on invalid base64 input
     *
     * Features:
     * - base64: Standard encoding with +/ and padding
     * - base64_url: URL-safe encoding with -_ and optional padding
     *
     * Uses OpenSSL's BIO interface for decode
     */
    template <flavours F, std::enable_if_t<F==flavours::base64 || F==flavours::base64_url, bool> = true>
    std::string decode(const std::string& input) {
        if (input.empty()) return {};

        std::string proc = input;
        // URL-safe → standard Base64
        if constexpr (F == flavours::base64_url) {
            std::replace(proc.begin(), proc.end(), '-', '+');
            std::replace(proc.begin(), proc.end(), '_', '/');
        }
        proc.append((4 - proc.size() % 4) % 4, '=');
        if (proc.size() > static_cast<std::size_t>(INT_MAX))
            throw std::length_error("Base64 input too large for OpenSSL BIO");

        // Validate character set
        const std::string legal =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/=";
        if (proc.find_first_not_of(legal) != std::string::npos)
            throw std::invalid_argument("Invalid base64 input (illegal character)");

        // Validate padding positions (only at end, max 2)
        size_t pad_cnt = 0;
        for (size_t i = proc.size(); i-- > 0 && proc[i] == '='; ) {
            ++pad_cnt;
        }
        if (pad_cnt > 2 || proc.find('=') < proc.size() - pad_cnt)
            throw std::invalid_argument("Invalid base64 input (padding error)");

        // Prepare BIO chain

        using bio_ptr = std::unique_ptr<BIO, decltype(&BIO_free_all)>;

        bio_ptr b64{BIO_new(BIO_f_base64()), BIO_free_all};
        BIO*   mem   = BIO_new_mem_buf(proc.data(), static_cast<int>(proc.size()));
        BIO*   chain = BIO_push(b64.get(), mem);
        if (!chain) {
            BIO_free(mem);
            throw std::bad_alloc{};
        }
        BIO_set_flags(chain, BIO_FLAGS_BASE64_NO_NL);

        // Decode into buffer sized to max possible
        std::string out;
        out.resize(proc.size() / 4 * 3);
        int decoded = BIO_read(chain, out.data(), static_cast<int>(out.size()));
        if (decoded <= 0)
            throw std::runtime_error("Invalid base64 input (decode error)");

        // Resize to actual decoded length
        out.resize(decoded);

        // Check decoded size matches expected (after padding)
        size_t expected = proc.size() / 4 * 3 - pad_cnt;
        if (out.size() != expected)
            throw std::runtime_error("Invalid base64 input (length mismatch)");

        return out;
    }

    /**
     * @brief Base16 (hexadecimal) encoding
     * @tparam F Must be base16
     * @param src Data to process
     * @return Processed string
     *
     * - Encoding produces lowercase hexadecimal
     * - Decoding accepts both lowercase and uppercase
     *
     * Uses Boost.Hex for implementation
     */
    template <flavours F, std::enable_if_t<F==flavours::base16, bool> = true>
    std::string encode(const std::string& src) {
        std::string result;
        result.reserve(src.size() * 2);
        boost::algorithm::hex_lower(src.begin(), src.end(), std::back_inserter(result));
        return result;
    }

    /**
     * @brief Base16 (hexadecimal) decoding
     * @tparam F Must be base16
     * @param src Data to process
     * @return Processed string
     *
     * - Encoding produces lowercase hexadecimal
     * - Decoding accepts both lowercase and uppercase
     *
     * Uses Boost.Hex for implementation
     */
    template <flavours F, std::enable_if_t<F==flavours::base16, bool> = true>
    std::string decode(const std::string& src) {
        std::string result;
        result.reserve(src.size() / 2);
        boost::algorithm::unhex(src.begin(), src.end(), std::back_inserter(result));
        return result;
    }

    template <flavours F, std::enable_if_t<F==flavours::escape, bool> = true>
    std::string encode(const std::string& src) {
        std::string result = src;
        boost::replace_all(result, "&",  "&amp;");
        boost::replace_all(result, "<",  "&lt;");
        boost::replace_all(result, ">",  "&gt;");
        boost::replace_all(result, "\"", "&quot;");
        boost::replace_all(result, "\'", "&#39;");
        return result;
    }

    template <flavours F, std::enable_if_t<F==flavours::escape, bool> = true>
    std::string decode(const std::string& src) {
        std::string result = src;
        boost::replace_all(result, "&lt;", "<");
        boost::replace_all(result, "&gt;", ">");
        boost::replace_all(result, "&quot;", "\"");
        boost::replace_all(result, "&#39;", "\'");
        boost::replace_all(result, "&amp;", "&");
        return result;
    }

} // namespace encoding

 /**
 * @namespace udho::utils::encode
 * @brief Convenience namespace for encoding operations
 *
 * Provides simple interface for common encoding tasks
 */
namespace encode {
/// @{
/**
 * @brief Convenience functions for specific encoding flavors
 * @param input Data to encode/decode
 * @return Processed string
 *
 * These functions provide simplified access to the template-based
 * encoding/decoding implementations
 */
    std::string cookie(const std::string& input){
        return encoding::encode<encoding::flavours::cookie>(input);
    }
    std::string url(const std::string& input){
        return encoding::encode<encoding::flavours::url>(input);
    }
    std::string base64(const std::string& input){
        return encoding::encode<encoding::flavours::base64>(input);
    }
    std::string base64_url(const std::string& input){
        return encoding::encode<encoding::flavours::base64_url>(input);
    }
    std::string base16(const std::string& input){
        return encoding::encode<encoding::flavours::base16>(input);
    }
    std::string escape(const std::string& input){
        return encoding::encode<encoding::flavours::escape>(input);
    }
/// @}
}

/**
 * @namespace udho::utils::decode
 * @brief Convenience namespace for decoding operations
 *
 * Provides simple interface for common decoding tasks
 */
namespace decode {
/// @{
/**
 * @brief Convenience functions for specific encoding flavors
 * @param input Data to encode/decode
 * @return Processed string
 *
 * These functions provide simplified access to the template-based
 * encoding/decoding implementations
 */
    std::string cookie(const std::string& input){
        return encoding::decode<encoding::flavours::cookie>(input);
    }
    std::string url(const std::string& input){
        return encoding::decode<encoding::flavours::url>(input);
    }
    std::string base64(const std::string& input){
        return encoding::decode<encoding::flavours::base64>(input);
    }
    std::string base64_url(const std::string& input){
        return encoding::decode<encoding::flavours::base64_url>(input);
    }
    std::string base16(const std::string& input){
        return encoding::decode<encoding::flavours::base16>(input);
    }
    std::string escape(const std::string& input){
        return encoding::decode<encoding::flavours::escape>(input);
    }
/// @}
}

} // namespace utils
} // namespace udho

#endif // UDHO_UTILS_ENCODING_H
