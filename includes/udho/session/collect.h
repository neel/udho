#ifndef UDHO_SESSION_COLLECT_H
#define UDHO_SESSION_COLLECT_H

#include <regex>
#include <udho/cookies/jar.h>
#include <udho/session/defs.h>

namespace udho {
namespace session {
namespace collection{

enum class strategies {
    cookies,
    header,
    url
};

template <strategies Strategy>
struct strategy;

/**
 * @brief strategy to collect and send session id from cookies
 */
template <>
struct strategy<strategies::cookies> {

    /**
     * @brief Construct cookie strategy
     * @param cookies Reference to cookie jar
     * @param key Cookie name (default: "UDHOSESSID")
     * @param domain Cookie domain (optional)
     * @param path Cookie path (optional)
     * @param secure Enable Secure flag (default: true)
     * @param http_only Enable HttpOnly flag (default: true)
     * @param same_site SameSite policy (default: lax)
     * @param partitioned Enable Partitioned flag (default: false)
     * @param max_age Max-Age in seconds (optional)
     * @param expires Absolute expiration time (optional)
     */
    strategy(udho::cookies::jar& cookies,  const std::string& key = "UDHOSESSID", std::optional<std::string> domain = std::nullopt, std::optional<std::string> path = std::nullopt, bool secure = true, bool http_only = true, udho::cookies::policy same_site = udho::cookies::policy::lax, bool partitioned = false, std::optional<unsigned long> max_age = std::nullopt, std::optional<std::chrono::system_clock::time_point> expires = std::nullopt)
        : _cookies(cookies), _key(key), _domain(domain), _path(path), _secure(secure), _http_only(http_only), _same_site(same_site), _partitioned(partitioned), _max_age(max_age), _expires(expires)
    {}

    strategy(strategy&& other): _cookies(other._cookies), _key(std::move(other._key)), _domain(std::move(other._domain)), _path(std::move(other._path)), _secure(std::move(other._secure)), _http_only(std::move(other._http_only)), _same_site(std::move(other._same_site)), _partitioned(std::move(other._partitioned)), _max_age(std::move(other._max_age)), _expires(std::move(other._expires)) {}

    /**
     * @brief Collect session ID from request cookies
     * @tparam Fields HTTP header fields type
     * @param request HTTP request header
     * @return Optional session ID if found and valid
     */
    template <typename Fields>
    std::optional<udho::session::id> collect(const boost::beast::http::header<true, Fields>& request) const {
        std::string domain_str  = _domain.value_or("");
        std::string path_str    = _path.value_or("");
        auto cookie = _cookies.get(_key, domain_str, path_str);

        if (cookie.valid()) {
            if (cookie.expires_at()) {
                auto now = std::chrono::system_clock::now();
                if (*cookie.expires_at() < now) {
                    return std::nullopt;
                }
            }

            udho::session::id id = from_string(cookie.value());
            return id.is_nil() ? std::nullopt : std::make_optional(id);
        }
        return std::nullopt;
    }

    /**
     * @brief commit session ID to response
     * @tparam Fields HTTP header fields type
     * @param response HTTP response header
     * @param sessid session id
     */
    template <typename Fields>
    void commit(boost::beast::http::header<false, Fields>& response, const udho::session::id& sessid) {
        udho::cookies::cookie<std::string> c(_key, udho::session::to_string(sessid));
        if (_domain) c.domain(*_domain);
        if (_path)   c.path(*_path);
        c.secure(_secure);
        c.http_only(_http_only);
        c.same_site(_same_site);
        c.partitioned(_partitioned);
        if (_max_age) c.max_age(*_max_age);
        if (_expires) c.expires_at(*_expires);
        _cookies.add(c);
    }

    /**
     * @brief remove session ID from the response
     * @tparam Fields HTTP header fields type
     * @param response HTTP response header
     */
    template <typename Fields>
    void remove(boost::beast::http::header<false, Fields>& response) {
        udho::cookies::cookie<std::string> c(_key);
        if (_domain) c.domain(*_domain);
        if (_path)   c.path(*_path);
        c.remove();
        c.secure(_secure);
        c.http_only(_http_only);
        c.same_site(_same_site);
        c.partitioned(_partitioned);
        _cookies.add(c);
    }

    private:
        udho::cookies::jar&          _cookies;
        std::string                  _key;
        std::optional<std::string>   _domain;
        std::optional<std::string>   _path;
        bool                         _secure;
        bool                         _http_only;
        udho::cookies::policy        _same_site;
        bool                         _partitioned;
        std::optional<unsigned long> _max_age;
        std::optional<std::chrono::system_clock::time_point> _expires;
};

/**
 * @brief strategy to collect and send session id using custom http headers
 */
template <>
struct strategy<strategies::header> {

    /**
     * @brief Construct header strategy
     * @param key Header name (default: "X-Session-ID")
     * @param regex to extract the session id from value
     * @details the regex could be useful to extract the session id when the value contents are not
     *          only the session id, e.g. `Authorization: Bearer <token>`. However, when there is
     *          only session id to extract e.g. `X-Session-ID: <session_key>` then keep the pattern
     *          empty.
     */
    explicit strategy(const std::string& key = "X-Session-ID", const std::string& pattern = "") : _key(key), _pattern(pattern) {}

    strategy(strategy&& other): _key(std::move(other._key)), _pattern(std::move(other._pattern)) {}

    /**
     * @brief Collect session ID from request headers
     * @tparam Fields HTTP header fields type
     * @param request HTTP request header
     * @return Optional session ID if found
     */
    template <typename Fields>
    std::optional<udho::session::id> collect(const boost::beast::http::header<true, Fields>& request) const {
        auto it = request.find(_key);
        if (it != request.end()) {
            if(_pattern.empty()) {
                udho::session::id id = from_string(it->value());
                return id.is_nil() ? std::nullopt : std::make_optional(id);
            } else {
                auto pattern = std::regex(_pattern);
                std::smatch match;
                if (std::regex_match(it->value(), match, pattern)){
                    if (match.size() > 1) {
                        udho::session::id id = from_string(match[1].str());
                        return id.is_nil() ? std::nullopt : std::make_optional(id);
                    }
                }
            }
        }
        return std::nullopt;
    }

    /**
     * @brief commit session ID to response
     * @tparam Fields HTTP header fields type
     * @param response HTTP response header
     * @param sessid session id
     */
    template <typename Fields>
    void commit(boost::beast::http::header<false, Fields>& response, const udho::session::id& sessid) {
        response.set(_key, udho::session::to_string(sessid));
    }

    /**
     * @brief remove session ID from the response
     * @tparam Fields HTTP header fields type
     * @param response HTTP response header
     */
    template <typename Fields>
    void remove(boost::beast::http::header<false, Fields>& response) {
        response.set(_key, "");
    }


private:
    std::string _key;
    std::string _pattern;
};


/**
 * @class collector
 * @brief Manages session lifecycle and data access for a single request
 *
 * @tparam CatalogueT Session catalogue type (e.g., udho::session::catalogue)
 * @tparam StrategyT Session ID collection strategy type (cookie, header, etc.)
 * @tparam Fields HTTP header fields type
 *
 * This class coordinates:
 * - Session ID extraction from requests
 * - Session data access via notes
 * - Session ID communication to clients
 * - Session lifecycle management
 *
 * @note Instantiated by context, not directly by user code
 */
template <typename CatalogueT, typename StrategyT, typename Fields>
struct collector{
    using catalogue_type = CatalogueT;
    using strategy_type  = StrategyT;
    using optional_id    = std::optional<udho::session::id>;
    using request_type   = boost::beast::http::header<true, Fields>;
    using response_type  = boost::beast::http::header<false, Fields>;

    collector(strategy_type&& strategy, catalogue_type& catalogue, const request_type& request, response_type& response): _strategy(std::move(strategy)), _catalogue(catalogue), _request(request), _response(response) {}

    collector(catalogue_type&& other): _catalogue(other._catalogue), _strategy(std::move(other._strategy)), _request(other._request), _response(other._response) {}

    /**
     * @brief Get the session ID from the request
     * @return Optional session ID if found by strategy
     *
     * Uses the configured strategy to extract session ID from the request
     */
    optional_id id() {
        return _strategy.collect(_request);
    }

    /**
     * @brief Generate a new session ID
     * @return Newly generated session ID
     *
     * Creates a new session ID but doesn't automatically create session storage or communicate it to the client.
     * If commited then the new ID will be sent to client via strategy when response is finalized.
     */
    static udho::session::id generate() {
        return udho::session::random();
    }

    /**
     * @brief renew session id
     * @return newly generated session id
     * If the request contains a session id then removes that from storage as well as through the strategy.
     * Generates a new sessid and commits that to the HTTP response through the strategy.
     * @note the newly generated sessid has no data associated with it and is not stored in the storage yet.
     */
    udho::session::id renew() {
        optional_id old_id = id();
        if(old_id) {
            remove(*old_id);
            _strategy.remove(_response);
        }
        auto sessid = generate();
        _strategy.commit(_response, sessid);
        return sessid;
    }

    /**
     * @brief Sends the sessid to the client through the underlying strategy e.g. by setting cookie or appropriate response header
     * @param sessid Session ID to commit
     * @note Does not have any impact on session storage
     */
    void commit(const udho::session::id& id) {
        _strategy.commit(_response, id);
    }

    /**
     * @brief Borrow a session note
     * @param sessid Session ID to access
     * @return Session note providing data access
     *
     * Creates or loads session data from storage. The note uses RAII for automatic
     * reference counting and session persistence.
     */
    typename catalogue_type::note_type borrow(const udho::session::id& id) {
        return _catalogue.borrow(id);
    }

    /**
     * @brief checks whether a session exists for the given id or not (either loaded in memory or in the storage)
     * @param id
     * @return boolean
     */
    bool exists(const udho::session::id& id) {
        return _catalogue.exist(id);
    }

    /**
     * @brief clear session
     * @note does not remove any session from server storage
     * Facilitates removal of the sessid either through set cookie header or any other http response header
     */
    void clear() {
        _strategy.remove(_response);
    }

    /**
     * @brief Remove session from server storage
     * @param sessid Session ID to remove
     * Marks session for deletion in catalogue. Actual removal occurs when:
     * 1. Last note is destroyed (RAII)
     * 2. Catalogue synchronizes with storage
     */
    void remove(const udho::session::id& id) {
        _catalogue.remove(id);
    }


    private:
        catalogue_type&     _catalogue;
        strategy_type       _strategy;
        const request_type& _request;
        response_type&      _response;
};

}



}
}


#endif // UDHO_SESSION_COLLECT_H
