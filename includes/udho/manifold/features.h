#ifndef UDHO_MANIFOLD_FEATURES_H
#define UDHO_MANIFOLD_FEATURES_H

#include <map>
#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/net/common.h>
#include <udho/url/router.h>
#include <boost/beast/core/flat_buffer.hpp>


namespace udho {
namespace manifold {

/**
 * @addtogroup manifold
 * @{
 */

/**
 * @brief manifold features
 *
 * Defines the types of functionality a manifold component can provide
 */
namespace feature{

    /**
     * @brief generates an unique signature for the request
     */
    struct hash{
        static constexpr const std::size_t stage = 0;
    };

    /**
     * @brief tracks events associated with the request (e.g. mini logging)
     */
    struct track{
        static constexpr const std::size_t stage = 0;
    };

    /**
     * @brief decides whether to accept or reject this request
     */
    struct filter{
        static constexpr const std::size_t stage = 0;
    };

    /**
     * @brief rate control, reject requests when exceeds server capacity
     */
    struct throttle{
        static constexpr const std::size_t stage = 0;
    };

    struct header_reader{
        static constexpr const std::size_t stage = 0;
        static constexpr const std::string_view name = "header_reader";

        using request_type    = udho::net::types::headers::request;
        using result          = request_type;
    };

    /**
     * @brief extract sufficient information from the request for routing module
     */
    struct identifier{
        static constexpr const std::size_t stage = 0;
        static constexpr const std::string_view name = "identifier";

        class result{
        public:
            using query_params_type = std::multimap<std::string, std::string>;
        private:
            std::string              _resource;
            std::string              _extension;
            query_params_type        _params;
        public:
            result() = default;
            result(const result&) = default;
            inline result(const std::string_view& target): _resource(target) {}
            inline const std::string& resource() const { return _resource; }
            template <typename StrT>
            inline void resource(StrT&& name) { _resource = std::move(name); }
            inline const std::string& extension() const { return _extension; }
            template <typename StrT>
            inline void extension(StrT&& ext) { _extension = std::move(ext); }
            inline const query_params_type& params() const { return _params; }

            template <typename StrT>
            inline void add(StrT&& key, StrT&& value) {
                _params.emplace(std::make_pair(std::move(key), std::move(value)));
            }
        };
    };

    struct body_reader{
        static constexpr const std::size_t stage = 1;
        static constexpr const std::string_view name = "body_reader";

        enum errors{
            none = 0,
            timeout,
            size_limit_exceeded,
            invalid_content_length,
            malformed_request,
            unknown
        };

        struct result{
            result(const std::string& mime, bool contiguous)
                : _mime(mime), _contiguous(contiguous) {}
            result(const std::string& mime, bool contiguous, boost::beast::flat_buffer&& flat_buffer, boost::beast::multi_buffer&& multi_buffer)
                : _mime(mime), _contiguous(contiguous), _flat_buffer(std::move(flat_buffer)), _multi_buffer(std::move(multi_buffer)) {}

            const std::string& mime() const { return _mime; }
            bool contiguous() const { return _contiguous; }

            const boost::beast::flat_buffer& flat_buffer() const { return _flat_buffer; }
            const boost::beast::multi_buffer& multi_buffer() const { return _multi_buffer; }

            boost::beast::flat_buffer& flat_buffer() { return _flat_buffer; }
            boost::beast::multi_buffer& multi_buffer() { return _multi_buffer; }

            std::size_t bytes_transferred() const { return _bytes_transferred; }
            std::error_code error() const { return  _error; }

            result& bytes_transferred(std::size_t bytes) { _bytes_transferred = bytes; return *this; }
            result& error(std::error_code ec) { _error = ec; return *this; }
        private:
            std::string                 _mime;
            bool                        _contiguous;
            std::error_code             _error;
            std::size_t                 _bytes_transferred;
            boost::beast::flat_buffer   _flat_buffer;
            boost::beast::multi_buffer  _multi_buffer;

        };
    };

    struct header_writer{
        static constexpr const std::size_t stage = 1;
    };
    struct body_writer{
        static constexpr const std::size_t stage = 1;
    };

    struct locator{
        static constexpr const std::size_t stage = 0;
        static constexpr const std::string_view name = "locator";

        using result = udho::url::detail::route_index;
    };

    struct responder{
        static constexpr const std::size_t stage = 2;
    };

    /**
     * @brief provides global and local cache facility
     */
    struct cache{
        static constexpr const std::size_t stage = 1;
    };

    /**
     * @brief provides facilities for generating, and verification of scalar tokens with TTL
     */
    struct token{
        static constexpr const std::size_t stage = 1;
    };

    /**
     * @brief provide policy based session extraction policy connected with session storage and management system
     */
    struct session{
        static constexpr const std::size_t stage = 1;
    };

    /**
     * @brief checks uploaded file satisfies constraints and if it does then copies
     */
    struct upload{
        static constexpr const std::size_t stage = 1;
    };

}

namespace detail{

template <typename... Features>
struct feature_max_stage;

template <typename F, typename... Features>
struct feature_max_stage<F, Features...>{
private:
    static constexpr std::size_t value_rest = feature_max_stage<Features...>::value;
public:
    static constexpr std::size_t value = F::stage >= value_rest ? F::stage : value_rest;
};

template <typename F>
struct feature_max_stage<F>{
private:
public:
    static constexpr std::size_t value = F::stage;
};

}

template <typename... Features>
struct features{
    template <typename FeatureT>
    using has = std::disjunction<std::is_same<FeatureT, Features>...>;

    static constexpr std::size_t max_stage = detail::feature_max_stage<Features...>::value;
};

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_FEATURES_H
