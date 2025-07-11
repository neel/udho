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

#ifndef UDHO_COOKIES_JAR_H
#define UDHO_COOKIES_JAR_H

#include <mutex>
#include <boost/tokenizer.hpp>
#include <udho/cookies/cookie.h>
#include <boost/beast/http/message.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>
#include <udho/utils/format.h>

namespace udho {
namespace cookies{

/**
 * @brief Thread-safe cookie container for HTTP request/response handling
 *
 * The jar class provides a thread-safe container for managing HTTP cookies.
 * It can parse cookies from incoming HTTP requests and apply cookies to
 * outgoing HTTP responses. The class uses internal locking to ensure
 * thread safety across all operations.
 *
 * @note This class is non-copyable to prevent accidental copying of the
 *       mutex and internal state.
 */
struct jar{
    /// @brief Type alias for string-based cookie storage
    using cookie_str_type = udho::cookies::cookie<std::string>;

    struct tags{
        struct by_full_key  {};
        struct by_id        {};
        struct by_name      {};
        struct by_domain    {};
        struct by_path      {};
        struct by_scope     {};
        struct by_name_domain {};
        struct by_name_path   {};

    };

    using container_type = boost::multi_index::multi_index_container<
        cookie_str_type,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<>,

            boost::multi_index::ordered_unique<
                boost::multi_index::tag<tags::by_full_key>,
                boost::multi_index::composite_key<
                    cookie_str_type,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::string&, &cookie_str_type::name>,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::domain>,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::path>
                >
            >,

            boost::multi_index::ordered_unique<
                boost::multi_index::tag<tags::by_id>,
                boost::multi_index::const_mem_fun<cookie_str_type, std::string, &cookie_str_type::id>
            >,

            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::by_name>,
                boost::multi_index::const_mem_fun<cookie_str_type, const std::string&, &cookie_str_type::name>
            >,

            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::by_domain>,
                boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::domain>
            >,

            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::by_path>,
                boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::path>
            >,

            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::by_scope>,
                boost::multi_index::composite_key<
                    cookie_str_type,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::domain>,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::path>
                >
            >,

            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::by_name_domain>,
                boost::multi_index::composite_key<
                    cookie_str_type,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::string&, &cookie_str_type::name>,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::domain>
                >
            >,

            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::by_name_path>,
                boost::multi_index::composite_key<
                    cookie_str_type,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::string&, &cookie_str_type::name>,
                    boost::multi_index::const_mem_fun<cookie_str_type, const std::optional<std::string>&, &cookie_str_type::path>
                >
            >
        >
    >;

    /**
     * @brief Default constructor
     *
     * Creates an empty cookie jar ready for use.
     */
    jar() = default;

    /**
     * @brief Parse and store cookies from an HTTP request
     *
     * Extracts cookies from the Cookie header of an HTTP request and stores
     * them in the jar. This method clears any existing cookies before parsing.
     *
     * @tparam Fields The fields type of the HTTP request
     *
     * @param request The HTTP request containing cookies to parse
     * @return The number of valid cookies successfully parsed and stored
     *
     * @note This method is thread-safe and will clear existing cookies
     * @note Invalid cookies are silently ignored
     */
    template <typename Fields>
    jar(const boost::beast::http::header<true, Fields>& request) {
        apply(request);
    }

    /**
     * @brief Copy constructor (deleted)
     *
     * Cookie jars cannot be copied due to internal mutex and to prevent
     * accidental duplication of cookie state.
     */
    jar(const jar&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     *
     * Cookie jars cannot be copied due to internal mutex and to prevent
     * accidental duplication of cookie state.
     */
    jar& operator=(const jar&) = delete;


    /**
     * @brief Parse and store cookies from an HTTP request
     *
     * Extracts cookies from the Cookie header of an HTTP request and stores
     * them in the jar. This method clears any existing cookies before parsing.
     *
     * @tparam Fields The fields type of the HTTP request
     *
     * @param request The HTTP request containing cookies to parse
     * @return The number of valid cookies successfully parsed and stored
     *
     * @note This method is thread-safe and will clear existing cookies
     * @note Invalid cookies are silently ignored
     */
    template <typename Fields>
    std::size_t apply(const boost::beast::http::header<true, Fields>& request){
        clear();
        const std::lock_guard<std::mutex> lock(_mutex);
        std::size_t count = 0;
        if(request.count(boost::beast::http::field::cookie)){
            std::string_view cookie_header = request[boost::beast::http::field::cookie];
            boost::char_separator<char> sep(";");
            boost::tokenizer<boost::char_separator<char>> tokens(cookie_header, sep);
            for (const auto& token : tokens) {
                cookie_str_type cookie = udho::cookies::read(token);
                if (cookie.valid()) {
                    _add(std::move(cookie));
                    ++count;
                }
            }
        }
        return count;
    }

    /**
     * @brief Apply stored cookies to an HTTP response
     *
     * Adds Set-Cookie headers to an HTTP response for all valid cookies
     * currently stored in the jar.
     *
     * @tparam Fields The fields type of the HTTP response
     *
     * @param response The HTTP response to add Set-Cookie headers to
     * @return The number of valid cookies added to the response
     *
     * @note This method is thread-safe
     * @note Only valid cookies are added to the response
     */
    template <typename Fields>
    std::size_t apply(boost::beast::http::header<false, Fields>& response) const{
        const std::lock_guard<std::mutex> lock(_mutex);
        std::size_t count = 0;
        for (const cookie_str_type& cookie : _cookies) {
            if(cookie.valid()){
                response.insert(boost::beast::http::field::set_cookie, udho::cookies::to_string(cookie));
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Add a cookie to the jar
     *
     * Adds a cookie to the jar by converting it to string storage.
     *
     * @tparam V The value type of the input cookie
     * @param c The cookie to add to the jar
     * @param replace if set to true then replace existing cookie with same name (if exists)
     * @return returns true if and only if the insertion took place.
     * @note This method is thread-safe
     * @note The cookie is converted to string storage internally
     */
    template <typename V>
    bool add(const udho::cookies::cookie<V>& c, bool replace = true){
        const std::lock_guard<std::mutex> lock(_mutex);
        cookie_str_type converted = c.template as<std::string>();
        return _add(std::move(converted), replace);
    }

    /**
     * @brief Move a cookie to the jar
     *
     * Moves a cookie to the jar by converting it to string storage.
     *
     * @tparam V The value type of the input cookie
     * @param c The cookie to move into the jar
     * @param replace if set to true then replace existing cookie with same name (if exists)
     * @return returns true if and only if the insertion took place.
     * @note This method is thread-safe
     * @note The cookie is converted to string storage internally
     */
    template <typename V>
    bool add(udho::cookies::cookie<V>&& c, bool replace = true){
        const std::lock_guard<std::mutex> lock(_mutex);
        cookie_str_type converted = c.template as<std::string>();
        return _add(std::move(converted), replace);
    }

    /**
     * @brief Clear all cookies from the jar
     *
     * Removes all stored cookies from the jar.
     *
     * @note This method is thread-safe
     */
    inline void clear() {
        const std::lock_guard<std::mutex> lock(_mutex);
        _cookies.clear();
    }

    /**
     * @brief Fetch the cookie with this **id**.
     * @throws std::out_of_range if the id is not found.
     * @note The *id* is the canonical “`name[:domain][:path]`” string returned by `cookie.id()`
     */
    const cookie_str_type& by_id(const std::string& id) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        const auto& idx = boost::multi_index::get<tags::by_id>(_cookies);
        auto it = idx.find(id);
        if (it == idx.end()) throw std::out_of_range("No cookie with id: " + id);
        return *it;
    }

    /**
     * @brief Locked range query on any index.
     *
     * A thin, thread-safe wrapper that acquires the jar’s mutex and then
     * delegates to the private `_by<TagT>()` helper.  The function returns the
     * *half-open* iterator range `<lo, hi)` that matches **arg** in the
     * requested index.
     *
     * @tparam TagT  **Index tag** that decides *how* the lookup is performed.
     *               Choose one of the tags in the table below.
     * @tparam Arg   Key type expected by that tag (usually
     *               `std::string`, `std::optional<std::string>` or a
     *               `std::tuple` of those).  The compiler checks this for you.
     * @param  arg   Key value to match.
     * @return       `std::pair<iterator,iterator>` — begin/end of the matching
     *               range.  If no element matches, both iterators compare equal.
     *
     * | Tag (use as `by<tags::…>()`) | Matches on…                  | Typical helper that calls it            |
     * |------------------------------|------------------------------|-----------------------------------------|
     * | `by_id`           | cookie id string (`name[:domain][:path]`) | *internal* (single-element helpers)     |
     * | `by_name`         | cookie name                             | `by_name()` / `count(name)`             |
     * | `by_domain`       | cookie domain (`std::optional<string>`) | `by_domain()`                           |
     * | `by_path`         | cookie path (`std::optional<string>`)   | `by_path()`                             |
     * | `by_scope`        | *(domain,path)* tuple                   | `by_scope()`                            |
     * | `by_name_domain`  | *(name,domain)* tuple                   | `by_name_domain()`                      |
     * | `by_name_path`    | *(name,path)* tuple                     | `by_name_path()`                        |
     * | `by_full_key`     | *(name,domain,path)* tuple (unique)     | *internal* insert/replace logic         |
     *
     * @note Most callers should prefer the ready-made helpers (`by_name`,
     *       `by_domain`, …) which hide the exact *Arg* type.  This template is
     *       useful when you want to iterate over the raw Boost index yourself.
     */
    template <typename TagT, typename Arg>
    std::pair<typename container_type::index<TagT>::type::const_iterator, typename container_type::index<TagT>::type::const_iterator> by(const Arg& arg) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        return _by<TagT>(arg);
    }

    /**
     * @brief Count the number of cookies that matches <tt>name / domain / path</tt>.
     * @param name The name of the cookie
     * @param domain The domain of the cookie
     * @param path The path of the cookie
     * @return size_type
     * @throws std::out_of_range if name is empty.
     */
    container_type::size_type count(const std::string& name, const std::string& domain, const std::string& path) const {
        const std::lock_guard<std::mutex> lock(_mutex);

        if(name.empty()) throw std::out_of_range{"empty name"};
        auto [lo,hi] = _by<tags::by_full_key>(std::make_tuple(name, std::make_optional(domain), std::make_optional(path)));
        return std::distance(lo, hi);
    }

    /**
     * @brief Count the number of cookies with matching name and domain/path
     * @param name The name of the cookie
     * @param domain The domain or path of the cookie
     * @note If the domain_or_path starts with `/` it is treated as a **path**. Otherwise it is treated as a **domain**.
     * @return size_type
     * @throws std::out_of_range if name is empty.
     */
    container_type::size_type count(const std::string& name, const std::string& domain_or_path) const {
        const std::lock_guard<std::mutex> lock(_mutex);

        if(name.empty()) throw std::out_of_range{"empty name"};
        if(domain_or_path.front() == '/') {
            auto [lo,hi] = _by<tags::by_name_path>(std::make_tuple(name, std::make_optional(domain_or_path)));
            return std::distance(lo, hi);
        } else {
            auto [lo,hi] = _by<tags::by_name_domain>(std::make_tuple(name, std::make_optional(domain_or_path)));
            return std::distance(lo, hi);
        }
    }

    /**
     * @brief Count the number of cookies with matching name
     * @param name The name of the cookie
     * @return size_type
     * @throws std::out_of_range if name is empty.
     */
    container_type::size_type count(const std::string& name) const {
        const std::lock_guard<std::mutex> lock(_mutex);

        if(name.empty()) throw std::out_of_range{"empty name"};
        auto [lo,hi] = _by<tags::by_name>(name);
        return std::distance(lo, hi);
    }


    /**
     * @brief Check whether an **id** exists.
     *
     * The *id* is the canonical “`name[:domain][:path]`” string returned by
     * `cookie.id()`.
     */
    bool exists(const std::string& id) const {
        auto [lo, hi] = by<tags::by_id>(id);
        return std::distance(lo, hi) > 0;
    }

    /**
     * @brief Get the very first cookie that matches <tt>name / domain / path</tt>.
     * @param name The name of the cookie
     * @param domain The domain of the cookie
     * @param path The path of the cookie
     * @return A const reference to the requested cookie
     * @throws std::out_of_range if no match exists.
     */
    const cookie_str_type& get(const std::string& name, const std::string& domain, const std::string& path) const {
        const std::lock_guard<std::mutex> lock(_mutex);

        if(name.empty()) throw std::out_of_range{"empty name"};

        auto [lo,hi] = _by<tags::by_full_key>(std::make_tuple(name, std::make_optional(domain), std::make_optional(path)));
        if(std::distance(lo, hi) > 0) return *lo;
        throw std::out_of_range{udho::utils::format("no cookie found with name: {}, domain: {}, path: {}", name, domain, path)};
    }

    /**
     * @brief Get the first cookie whose *name* equals `name` **and** either its *domain* **or** *path* equals `domain_or_path`.
     * @param name The name of the cookie
     * @param domain The domain or path of the cookie
     * @return A const reference to the requested cookie
     * @note If the domain_or_path starts with `/` it is treated as a **path**. Otherwise it is treated as a **domain**.
     * @throws std::out_of_range if no match exists.
     */
    const cookie_str_type& get(const std::string& name, const std::string& domain_or_path) const {
        const std::lock_guard<std::mutex> lock(_mutex);

        if(name.empty()) throw std::out_of_range{"empty name"};

        if(domain_or_path.front() == '/') {
            auto [lo,hi] = _by<tags::by_name_path>(std::make_tuple(name, std::make_optional(domain_or_path)));
            if(std::distance(lo, hi) > 0) return *lo;
        } else {
            auto [lo,hi] = _by<tags::by_name_domain>(std::make_tuple(name, std::make_optional(domain_or_path)));
            if(std::distance(lo, hi) > 0) return *lo;
        }

        throw std::out_of_range{udho::utils::format("no cookie found with name: {}, domain or path: {}", name, domain_or_path)};
    }

    /**
     * @brief Get the first cookie whose *name* equals `name`, or whose **id** equals `name`.
     * @param key The name or id of the cookie
     * @return A const reference to the requested cookie
     * @throws std::out_of_range if no match exists.
     */
    const cookie_str_type& get(const std::string& key) const {
        const std::lock_guard<std::mutex> lock(_mutex);

        if(key.empty()) throw std::out_of_range{"empty name"};

        auto [lo,hi] = _by<tags::by_id>(key);
        if(std::distance(lo, hi) > 0) {
            return *lo;
        } else {
            auto [lo,hi] = _by<tags::by_name>(key);
            if(std::distance(lo, hi) > 0) return *lo;
        }

        throw std::out_of_range{udho::utils::format("no cookie found with name or id: {}", key)};
    }

    /**
     * @brief returns a list of cookies with matching name
     * @param domain
     * @return vector of cookies
     */
    std::vector<cookie_str_type> by_name(const std::string& name) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto [lo, hi] = _by<tags::by_name>(name);
        return {lo, hi};
    }


    /**
     * @brief returns a list of cookies with matching domain
     * @param domain
     * @return vector of cookies
     */
    std::vector<cookie_str_type> by_domain(const std::string& domain) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto [lo, hi] = _by<tags::by_domain>(std::make_optional(domain));
        return {lo, hi};
    }

    /**
     * @brief returns a list of cookies with matching domain and name
     * @param domain
     * @param name
     * @return vector of cookies
     */
    std::vector<cookie_str_type> by_domain(const std::string& domain, const std::string& name) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto [lo, hi] = _by<tags::by_name_domain>(std::make_tuple(name, std::make_optional(domain)));
        return {lo, hi};
    }

    /**
     * @brief returns a list of cookies with matching path
     * @param path
     * @return vector of cookies
     */
    std::vector<cookie_str_type> by_path(const std::string& path) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto [lo, hi] = _by<tags::by_path>(std::make_optional(path));
        return {lo, hi};
    }

    /**
     * @brief returns a list of cookies with matching path and name
     * @param path
     * @param name
     * @return vector of cookies
     */
    std::vector<cookie_str_type> by_path(const std::string& path, const std::string& name) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto [lo, hi] = _by<tags::by_name_path>(std::make_tuple(name, std::make_optional(path)));
        return {lo, hi};
    }

    /**
     * @brief returns a list of cookies with matching domain and path
     * @param domain
     * @param path
     * @return vector of cookies
     */
    std::vector<cookie_str_type> by_scope(const std::string& domain, const std::string& path) const {
        const std::lock_guard<std::mutex> lock(_mutex);
        auto [lo,hi] = _by<tags::by_scope>(std::make_tuple(std::make_optional(domain), std::make_optional(path)));
        return {lo, hi};
    }

    /**
     * @brief Subscript operator for cookie access
     *
     * Provides array-like access to cookies by name or id.
     *
     * @param key The name/ID of the cookie to retrieve
     * @return A const reference to the requested cookie
     * @throws std::out_of_range if no cookie with the given name exists
     *
     * @note This method is thread-safe
     */
    inline const cookie_str_type& operator[](const std::string& key) const{ return get(key); }

    /**
     * @brief Function call operator for HTTP requests
     *
     * Convenience operator that calls apply() for HTTP requests.
     *
     * @tparam Body The body type of the HTTP request
     * @tparam Fields The fields type of the HTTP request
     * @param request The HTTP request to parse cookies from
     * @return The number of valid cookies successfully parsed and stored
     *
     * @see apply(const boost::beast::http::request<Body, Fields>&)
     */
    template <typename Body, typename Fields>
    std::size_t operator()(const boost::beast::http::request<Body, Fields>& request){ return apply(request); }

    /**
     * @brief Function call operator for HTTP responses
     *
     * Convenience operator that calls apply() for HTTP responses.
     *
     * @tparam Body The body type of the HTTP response
     * @tparam Fields The fields type of the HTTP response
     * @param response The HTTP response to add cookies to
     * @return The number of valid cookies added to the response
     *
     * @see apply(boost::beast::http::response<Body, Fields>&) const
     */
    template <typename Body, typename Fields>
    std::size_t operator()(boost::beast::http::response<Body, Fields>& response){ return apply(response); }

    private:
        template <typename TagT, typename Arg>
        std::pair<typename container_type::index<TagT>::type::const_iterator, typename container_type::index<TagT>::type::const_iterator> _by(const Arg& arg) const {
            const auto& idx = boost::multi_index::get<TagT>(_cookies);
            return idx.equal_range(arg);
        }
        bool _add(cookie_str_type&& cookie_stringified, bool replace = true){
            auto& idx = _cookies.get<tags::by_full_key>();
            auto it = idx.find(std::make_tuple(cookie_stringified.name(), cookie_stringified.domain(), cookie_stringified.path()));
            if (it != idx.end()) {
                if (!replace) return false;
                idx.replace(it, std::move(cookie_stringified));
                return true;
            }
            return idx.insert(std::move(cookie_stringified)).second;
        }

    private:
        container_type      _cookies;
        mutable std::mutex  _mutex;

    /**
     * @brief Friend declaration for stream insertion operator
     *
     * Allows the stream insertion operator to access private add() method.
     */
    template <typename V>
    friend udho::cookies::jar& operator<<(udho::cookies::jar& cookies, const udho::cookies::cookie<V>& cookie);
};

/**
 * @brief Stream insertion operator for adding cookies to jar
 *
 * Provides a convenient stream-like interface for adding cookies to a jar.
 *
 * @tparam V The value type of the cookie
 * @param cookies The cookie jar to add the cookie to
 * @param cookie The cookie to add to the jar
 * @return A reference to the cookie jar for method chaining
 *
 * @note This operator is thread-safe
 *
 * @par Example:
 * @code
 * udho::cookies::jar jar;
 * udho::cookies::cookie<std::string> cookie("key", "value");
 * jar << cookie;  // Add cookie to jar
 * @endcode
 */
template <typename V>
udho::cookies::jar& operator<<(udho::cookies::jar& cookies, const udho::cookies::cookie<V>& cookie){
    cookies.add(cookie);
    return cookies;
}

}
}

#endif // UDHO_COOKIES_JAR_H
