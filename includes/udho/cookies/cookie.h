/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_COOKIES_COOKIE_H
#define UDHO_COOKIES_COOKIE_H

#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/beast/http/message.hpp>
#include <udho/utils/encoding.h>
#include <udho/cookies/policy.h>
#include <udho/cookies/detail.h>
#include <udho/hazo/detail/is_streamable.h>

namespace udho{

namespace cookies{

/**
 * @class cookie
 * @brief RFC 6265 compliant HTTP cookie implementation
 *
 * @tparam ValueT Type of cookie value, must be convertible to std::string
 */
template <typename ValueT>
struct cookie{
    /// @brief Type of cookie value
    using value_type = ValueT;
    /// @brief Self type alias
    using self_type  = cookie<value_type>;

    /// @brief Static assertion for value type convertibility
    static_assert( udho::hazo::detail::is_streamable<std::ostream, value_type>::value, "Cookie value must be convertible to std::string" );

    /**
     * @brief convert the cookie value into a different type and return the transformed cookie
     * @tparam T target type
     * @return cookie<T>
     */
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, std::string>, bool> = true>
    cookie<T> as() const {
        cookie<T> res{_name};
        if constexpr (std::is_same_v<value_type, std::string> || std::is_same_v<T, std::string>) {
            res.value(boost::lexical_cast<T>(_value));
        } else {
            res.value(static_cast<T>(_value));
        }
        res.http_only(_http_only).partitioned(_partitioned).secure(_secure);

        if(_domain)     res.domain      (*_domain);
        if(_path)       res.path        (*_path);
        if(_max_age)    res.max_age     (*_max_age);
        if(_same_site)  res.same_site   (*_same_site);
        if(_expires)    res.expires_at  (*_expires);

        return res;
    }

    /**
     * @brief Construct cookie with name only
     * @param name Cookie name
     * @throws std::invalid_argument for invalid names
     *
     * - Validates name characters
     * - Auto-enables Secure for __Host-/__Secure- prefixes
     */
    explicit cookie(const std::string& name): _name(name), _http_only(false), _partitioned(false), _secure(false) {
        if(!validate_name()){
            make_invalid();
            return;
        }
        if(boost::algorithm::starts_with(_name, "__Secure-")){
            _secure = true;
        }

        if (boost::algorithm::starts_with(_name, "__Host-")) {
            _secure = true;
            _path = "/";
            _domain = std::nullopt;
        }
    }

    /**
     * @brief Construct cookie with name and value
     * @param name Cookie name
     * @param value Cookie value
     * @throws std::invalid_argument for invalid names
     */
    cookie(const std::string& name, const value_type& value): _name(name), _value(value), _http_only(false), _partitioned(false), _secure(false) {
        if(!validate_name()){
            make_invalid();
            return;
        }

        if(boost::algorithm::starts_with(_name, "__Secure-")){
            _secure = true;
        }

        if (boost::algorithm::starts_with(_name, "__Host-")) {
            _secure = true;
            _path = "/";
            _domain = std::nullopt;
        }
    }

    /**
     * @brief Construct cookie with name, value and path
     * @param name Cookie name
     * @param value Cookie value
     * @param path Cookie path
     * @throws std::invalid_argument for invalid names/paths
     */
    cookie(const std::string& name, const value_type& value, const std::string& path): _name(name), _value(value), _http_only(false), _partitioned(false), _path(path), _secure(false) {
        if(!validate_name()){
            make_invalid();
            return;
        }

        if(boost::algorithm::starts_with(_name, "__Secure-")){
            _secure = true;
        }

        if (boost::algorithm::starts_with(_name, "__Host-")) {
            _secure = true;
            _path = "/";
            _domain = std::nullopt;
        }
    }

    /**
     * @brief Mark cookie for removal
     * @return Reference to self
     *
     * Sets:
     * - Max-Age=0
     * - Expires to past date
     */
    self_type& remove() {
        _max_age = 0;
        _expires = std::chrono::system_clock::now() - std::chrono::hours(1);
        return *this;
    }

    const std::string& name() const { return _name; }

    /// @brief Get current cookie value
    const value_type& value() const { return _value; }

    /// @brief Set cookie value
    self_type& value(const value_type& v) {
        _value = v;
        return *this;
    }

    /// @brief Assignment operator for cookie value
    self_type& operator=(const value_type& v) {
        return value(v);
    }

    /**
     * @brief Set cookie path
     * @param p Path value
     * @return Reference to self
     * @throws std::invalid_argument if:
     * - Path doesn't start with /
     * - __Host- prefix with non-/ path
     */
    self_type& path(const std::string& p) {
        if(!p.empty() && p[0] != '/') {
            throw std::invalid_argument("Cookie path must start with / ; constraint not satisfied for path: " + p);
        }

        if (boost::algorithm::starts_with(_name, "__Host-") && p != "/"){
            throw std::invalid_argument("Cookie path be set to / when the cookie name is prefixed by __Host-; constraint not satisfied for path: " + p);
        }

        _path = p;
        return *this;
    }
    /// @brief Get current path
    const std::optional<std::string>& path() const { return _path; }

    /**
     * @brief Set cookie domain
     * @param d Domain value
     * @return Reference to self
     * @throws std::invalid_argument if:
     * - Contains port
     * - Invalid characters
     * - __Host- prefix used
     */
    self_type& domain(const std::string& d) {
        if (d.empty()) {
            _domain = std::nullopt;  // Clear domain
            return *this;
        } else if (boost::algorithm::starts_with(_name, "__Host-")){
            throw std::invalid_argument("Cookie domain cannot be set when the cookie name is prefixed by __Host-");
        }

        if (d.find(':') != std::string::npos) {
            throw std::invalid_argument("Cookie domain cannot contain port; constraint not satisfied for domain: " + d);
        }
        bool all_valid = std::all_of(d.begin(), d.end(), [](char c) {
            return std::isalnum(c) || c == '.' || c == '-';
        });
        if (!all_valid) {
            throw std::invalid_argument("Invalid domain characters in domain "+ d);
        }
        _domain = d;
        boost::algorithm::to_lower(*_domain);
        return *this;
    }
    /// @brief Get current domain
    const std::optional<std::string>& domain() const { return _domain; }

    /// @brief Set Max-Age attribute
    self_type& max_age(unsigned long age) { _max_age = age; return *this; }
    /// @brief Get current Max-Age
    const std::optional<unsigned long>& max_age() const { return _max_age; }

    /**
     * @brief Set SameSite policy
     * @param p Policy value
     * @return Reference to self
     * @note Automatically enables Secure for SameSite=None
     */
    self_type& same_site(policy p) {
        _same_site = p;
        if(_same_site == policy::none){
            secure(true);
        }
        return *this;
    }
    /// @brief Get current SameSite policy
    const std::optional<policy>& same_site() const { return _same_site; }

    /// @brief Set HttpOnly flag
    self_type& http_only(bool flag) { _http_only = flag; return *this; }
    /// @brief Get HttpOnly status
    bool http_only() const { return _http_only; }

    /**
     * @brief Set Secure flag
     * @param flag Security status
     * @return Reference to self
     * @throws std::invalid_argument if:
     * - Disabling Secure for SameSite=None/Partitioned
     * - Disabling for __Host-/__Secure- prefixes
     */
    self_type& secure(bool flag) {
        if ((_same_site == policy::none || _partitioned) && !flag) {
            throw std::invalid_argument("Secure must be true when SameSite=None or Partitioned");
        }

        if(boost::algorithm::starts_with(_name, "__Secure-") || boost::algorithm::starts_with(_name, "__Host-")){
            throw std::invalid_argument("Secure must be true if the cookie name starts with __Secure- or __Host- prefix");
        }

        _secure = flag;
        return *this;
    }
    /// @brief Get Secure status
    bool secure() const { return _secure; }

    /**
     * @brief Set Partitioned flag
     * @param flag Partitioned status
     * @return Reference to self
     * @note Enables Secure automatically
     */
    self_type& partitioned(bool flag) {
        _partitioned = flag;
        if(_partitioned) secure(true);
        return *this;
    }
    /// @brief Get Partitioned status
    bool partitioned() const { return _partitioned; }

    /// @brief Set absolute expiration time
    self_type& expires_at(std::chrono::system_clock::time_point t) {
        _expires = t;
        return *this;
    }
    /// @brief Get expiration time point
    const std::optional<std::chrono::system_clock::time_point>& expires_at() const { return _expires; }


    /**
     * @brief Set relative expiration time
     * @tparam Rep Duration arithmetic type
     * @tparam Period Duration period type
     * @param duration Time until expiration
     * @return Reference to self
     */
    template <typename Rep, typename Period>
    self_type& expires(std::chrono::duration<Rep, Period> duration) {
        auto now = std::chrono::system_clock::now();
        _expires = now + duration;
        return *this;
    }
    /// @brief Get remaining time until expiration
    std::optional<std::chrono::seconds> expires() const {
        if (!_expires) return std::nullopt;

        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(*_expires - now);
    }

    /**
     * @brief Create a canonical id of a cookie of teh format “`name[:domain][:path]`”
     * @return string
     */
    std::string id() const {
        if(_name.empty()) return "";

        std::string cookie_id = _name;
        if(_domain){
            std::string domain_lower = *_domain;
            boost::algorithm::to_lower(domain_lower);
            cookie_id += ":" + domain_lower;
        }
        if(_path)   cookie_id += ":"+*_path;
        return cookie_id;
    }

    /**
     * @brief a cookies is valid if it has a non-empty name
     * @return
     */
    bool valid() const { return !_name.empty(); }
private:
    bool validate_name() const {
        static const char* allowed = "!#$%&'*+-.^_`|~";
        bool all_valid = std::all_of(_name.begin(), _name.end(), [](char c) {
            return std::isalnum(c) || std::strchr(allowed, c);
        });
        return all_valid;
        // if(!all_valid) {
        //     throw std::invalid_argument("Invalid cookie name: " + _name);
        // }
    }

    explicit cookie(): _http_only(false), _partitioned(false), _secure(false) {}
    self_type& name(const std::string& name) {
        _name = name;
        validate_name();
    }

    void make_invalid() {
        _name = "";
    }

    template <typename StreamT, typename V>
    friend StreamT& write(StreamT& stream, const cookie<V>& c);
    friend cookie<std::string> read(std::string_view header);

private:
    std::string                   _name;
    value_type                    _value;
    bool                          _http_only;
    bool                          _partitioned;
    std::optional<std::string>    _domain;
    std::optional<std::string>    _path;
    std::optional<unsigned long>  _max_age;
    bool                          _secure;
    std::optional<policy>         _same_site;
    std::optional<std::chrono::system_clock::time_point> _expires;
};

/**
 * @brief Render cookie as Set-Cookie header
 * @tparam StreamT Output stream type
 * @param stream Output stream
 * @return Reference to stream
 *
 * Produces RFC-compliant Set-Cookie header with URL-encoded value and Automatic Expires for Max-Age=0
 */
template <typename StreamT, typename ValueT>
inline StreamT& write(StreamT& stream, const cookie<ValueT>& c) {
    stream << c._name << '=' << udho::utils::encode::cookie(boost::lexical_cast<std::string>(c._value));
    if(c._domain && !(*c._domain).empty()) stream << "; Domain=" << *c._domain;
    if(c._path && !(*c._path).empty())     stream << "; Path="   << *c._path;

    if(c._max_age) {
        stream << "; Max-Age=" << *c._max_age;

        if(*c._max_age == 0) {
            stream << "; Expires=Fri, 01 Jan 1971 01:00:00 GMT";
        } else if(c._expires) {
            stream << "; Expires=" << udho::cookies::detail::format_rfc7231(*c._expires);
        }
    } else if(c._expires) {
        stream << "; Expires=" << udho::cookies::detail::format_rfc7231(*c._expires);
    }

    if(c._same_site) {
        std::string same_site_str_val = udho::cookies::detail::same_site_str(*c._same_site);
        if(!same_site_str_val.empty()){
            if(c._same_site)   stream << "; SameSite=" << same_site_str_val;
        }
    }
    if(c._secure)      stream << "; Secure";
    if(c._http_only)   stream << "; HttpOnly";
    if(c._partitioned) stream << "; Partitioned";

    return stream;
}

/// @brief Convert cookie to header string
template <typename ValueT>
inline std::string to_string(const cookie<ValueT>& c) {
    std::stringstream ss;
    write(ss, c);
    return ss.str();
}

/**
 * @brief Parse Set-Cookie header into a cookie object
 * @param header Set-Cookie header value
 * @return Parsed cookie<std::string>
 *
 * Parses RFC 6265-compliant cookies with case-insensitive attribute handling
 */
inline cookie<std::string> read(std::string_view header) {
    if (header.empty()) {
        return cookie<std::string>{};
        // throw std::invalid_argument("Empty cookie header");
    }

    size_t eq_pos = header.find('=');
    if (eq_pos == std::string_view::npos) {
        return cookie<std::string>{};
        // throw std::invalid_argument("Missing cookie name/value separator");
    }

    std::string name = std::string{header.substr(0, eq_pos)};
    boost::algorithm::trim(name);
    if (name.empty()) {
        return cookie<std::string>{};
        // throw std::invalid_argument("Empty cookie name");
    }

    size_t sc_pos = header.find(';', eq_pos + 1);
    // Trailing semicolon is optional
    // Set-Cookie: <cookie-name>=<cookie-value>
    // Set-Cookie: <cookie-name>=<cookie-value>; Secure
    // Both are valid
    std::string value{
        header.substr(
            eq_pos + 1,
            (sc_pos != std::string_view::npos)
                ? (sc_pos - eq_pos - 1)
                : std::string_view::npos
        )
    };
    boost::algorithm::trim(value);

    cookie<std::string> c(name);
    c.value(udho::utils::decode::cookie(value));

    if(sc_pos == std::string_view::npos) return c;

    // value is parsed
    // now start from the the first semicolon after the value
    // Example: ; Domain= <domain-value>; Secure; HttpOnly
    size_t pos = sc_pos;
    while (pos < header.size()) {
        // Find next attribute
        pos = header.find_first_not_of(" ;", pos);  // pos is attr_key start position
        if (pos == std::string_view::npos) break;   // no more attr_key found

        // Extract key
        size_t attr_end = header.find_first_of("=;", pos);      // either end of attr_key or end of attr reached
        std::string_view key = detail::trim_view(
            header.substr(
                pos,
                (attr_end != std::string_view::npos)
                    ? attr_end - pos
                    : std::string_view::npos
            )
        );
        if(attr_end == std::string_view::npos){
            // attr_end reached
            break;
        }
        pos = attr_end;

        // Extract value if exists
        std::string_view val;
        if (header[pos] == '=') {  // end of attr_key reached
            pos++;
            size_t val_end = header.find(';', pos);                 // looking for attr_end
            val = detail::trim_view(
                header.substr(
                    pos,
                    (val_end != std::string_view::npos)
                        ? val_end - pos
                        : std::string_view::npos                    // Last attribute does not end with a semicolon
                )
            );
            pos = val_end;
        }

        try{
            // Process attributes
            if (boost::algorithm::iequals(key, "Domain")) {
                c.domain(std::string(val));
            } else if (boost::algorithm::iequals(key, "Path")) {
                c.path(std::string(val));
            } else if (boost::algorithm::iequals(key, "Max-Age")) {
                unsigned long max_age = 0;
                if(detail::parse_unsigned(val, max_age)){
                    c.max_age(max_age);
                }
            } else if (boost::algorithm::iequals(key, "Expires")) {
                std::chrono::system_clock::time_point time;
                if(detail::parse_rfc7231(val, time)){
                    c.expires_at(time);
                }
            } else if (boost::algorithm::iequals(key, "SameSite")) {
                policy p;
                if(detail::parse_policy(val, p)){
                    c.same_site(p);
                }
            } else if (boost::algorithm::iequals(key, "Secure")) {
                c.secure(true);
            } else if (boost::algorithm::iequals(key, "HttpOnly")) {
                c.http_only(true);
            } else if (boost::algorithm::iequals(key, "Partitioned")) {
                c.partitioned(true);
            }
        } catch(const std::exception& ex){
            // report and ignore
            // throw std::runtime_error{"Invalid cookie attribute"}
        }
    }

    return c;
}



}

}

#endif // UDHO_COOKIES_COOKIE_H
