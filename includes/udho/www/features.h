#ifndef UDHO_WWW_FEATURES_H
#define UDHO_WWW_FEATURES_H

#include <map>
#include <type_traits>
#include <udho/manifold/fwd.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <udho/cookies/jar.h>
#include <udho/session/note.h>
#include <udho/url/route_index.h>
#include <udho/net/protocols/form_data.h>
#include <udho/net/protocols/body_reader_result.h>
#include <boost/asio/buffers_iterator.hpp>
#include <nlohmann/json.hpp>

namespace udho{
namespace www{

namespace feature{

/**
 * @brief reads the HTTP request header.
 *
 * This feature represents the first HTTP parsing stage. Its result is the
 * Beast request-header object containing method, target, version, and fields.
 *
 * @ingroup DoxyG_www
 */
struct header_reader{
    static constexpr const std::size_t stage = 0;
    static constexpr const std::string_view name = "header_reader";

    using request_type    = boost::beast::http::header<true,  boost::beast::http::fields>;
    using result          = request_type;
};


/**
 * @brief extracts route-identification data from a request target.
 *
 * The identifier feature normalizes the request target into the pieces needed
 * by the routing subsystem: original resource, path, extension, and query
 * parameters.
 *
 * @ingroup DoxyG_www
 */
struct identifier{
    static constexpr const std::size_t stage = 0;
    static constexpr const std::string_view name = "identifier";

    /**
     * @brief Routing-identification result extracted from a request target.
     *
     * Stores the request resource, normalized path, file extension, and decoded
     * query parameters. The value is produced by the `identifier` feature and is
     * later consumed by route-location and action-dispatch logic.
     */
    class result{
    public:
        using query_params_type = std::multimap<std::string, std::string>;
    private:
        std::string              _resource;
        std::string              _path;
        std::string              _extension;
        query_params_type        _params;
    public:
        result() = default;
        result(const result&) = default;
        inline result(const std::string_view& target): _resource(target) {}

        template <typename StrT>
        inline void resource(StrT&& name) { _resource = std::move(name); }

        /**
         * @brief Gets the original request resource.
         *
         * @return Const reference to the stored resource string.
         */
        inline const std::string& resource() const { return _resource; }

        template <typename StrT>
        inline void path(StrT&& name) { _path = std::move(name); }

        /**
         * @brief Gets the normalized route path.
         *
         * @return Const reference to the stored path string.
         */
        inline const std::string& path() const { return _path; }

        template <typename StrT>
        inline void extension(StrT&& ext) { _extension = std::move(ext); }

        /**
         * @brief Gets the resource extension.
         *
         * @return Const reference to the stored extension string.
         */
        inline const std::string& extension() const { return _extension; }

        /**
         * @brief Gets decoded query parameters.
         *
         * @return Const reference to the query-parameter multimap.
         */
        inline const query_params_type& params() const { return _params; }

        template <typename StrT>
        inline void add(StrT&& key, StrT&& value) {
            _params.emplace(std::make_pair(std::move(key), std::move(value)));
        }
    };
};

/**
 * @brief loads request cookies.
 *
 * Produces a cookie jar from the incoming request. The result can be consumed
 * by later features, accessors, handlers, or view code through the journal.
 *
 * @ingroup DoxyG_www
 */
struct cookie_load{
    static constexpr const std::size_t stage = 1;
    static constexpr const std::string_view name = "cookie_load";

    /**
     * @brief Cookie jar produced by `cookie_load`.
     */
    using result = udho::cookies::jar;
};

/**
 * @brief loads or initializes the session note.
 *
 * This feature runs after cookie loading and produces the session note visible
 * to later stages.
 *
 * @ingroup DoxyG_www
 */
struct session_load{
    static constexpr const std::size_t stage = 1;
    static constexpr const std::string_view name = "session_load";

    /**
     * @brief Session note produced by `session_load`.
     */
    using result = udho::session::note;
};

/**
 * @brief reads the HTTP request body.
 *
 * The body reader stores the detected MIME type, body buffer, form-data result,
 * transfer status, and body length. It supports both contiguous and segmented
 * Beast buffers.
 *
 * @ingroup DoxyG_www
 */
struct body_reader{
    static constexpr const std::size_t stage = 2;
    static constexpr const std::string_view name = "body_reader";

    /**
     * @brief Result object produced by the request body reader.
     *
     * Owns the released body buffer and parsed form-data state returned by the
     * protocol body reader.
     */
    struct result{
        /**
         * @brief Parsed form-data representation.
         */
        using form_type             = udho::net::protocols::detail::form_data;
        /**
         * @brief Variant over supported Beast body-buffer storage types.
         */
        using buffer_variant_type   = std::variant<
            boost::beast::flat_buffer,
            boost::beast::multi_buffer
        >;

        /**
         * @brief Constructs a body-reader result from a protocol reader result.
         *
         * The constructor takes ownership of the reader's released buffer and parsed
         * form-data state. It also records the MIME type, error code, transferred byte
         * count, and whether the underlying buffer is contiguous.
         *
         * @tparam Buffer Concrete Beast buffer type released by the protocol reader.
         * @param mime MIME type associated with the request body.
         * @param result Protocol reader result whose buffer and form state are moved.
         * @param ec Error code produced while reading the body.
         * @param bytes Number of bytes transferred while reading the body.
         */
        template <typename Buffer>
        result(const std::string& mime, udho::net::protocols::body_reader_result<Buffer>&& result, boost::system::error_code ec, std::size_t bytes): _mime(mime), _error(ec), _bytes_transferred(bytes) {
            _buffer     = result.release_buffer();
            _contiguous = std::is_same_v<Buffer, boost::beast::flat_buffer>;
            _form       = result.release_form();
        }

        /**
         * @brief Gets the MIME type associated with the body.
         *
         * @return Const reference to the MIME type string.
         */
        const std::string& mime() const { return _mime; }
        /**
         * @brief Checks whether the stored body buffer is contiguous.
         *
         * @return `true` when the stored buffer is `boost::beast::flat_buffer`,
         *         otherwise `false`.
         */
        bool contiguous() const { return _contiguous; }
        /**
         * @brief Gets parsed form-data state.
         *
         * @return Const reference to parsed form-data.
         */
        const udho::net::protocols::detail::form_data& form() const { return _form; }

        /**
         * @brief Gets the number of transferred body bytes.
         *
         * @return Number of bytes transferred by the body reader.
         */
        std::size_t bytes_transferred() const { return _bytes_transferred; }
        /**
         * @brief Gets the body-reader error code.
         *
         * @return Error code captured during body reading.
         */
        std::error_code error() const { return  _error; }
        /**
         * @brief Gets the stored body buffer.
         *
         * @return Const reference to the buffer variant.
         */
        const buffer_variant_type& buffer() const { return _buffer; }

        /**
         * @brief Parses the stored body as JSON.
         *
         * The function reads from either the flat buffer or multi buffer depending on
         * the storage mode and forwards the byte range to `nlohmann::json::parse`.
         *
         * @return Parsed JSON value.
         *
         * @throws nlohmann::json::parse_error if the body is not valid JSON.
         */
        nlohmann::json json() const {
            if(contiguous()) {
                const boost::beast::flat_buffer& flat_buffer = std::get<boost::beast::flat_buffer>(_buffer);
                auto begin =  boost::asio::buffers_begin(flat_buffer.data());
                auto end   =  boost::asio::buffers_end(flat_buffer.data());
                return nlohmann::json::parse(begin, end);
            } else {
                const boost::beast::multi_buffer& multi_buffer = std::get<boost::beast::multi_buffer>(_buffer);
                auto begin =  boost::asio::buffers_begin(multi_buffer.data());
                auto end   =  boost::asio::buffers_end(multi_buffer.data());
                return nlohmann::json::parse(begin, end);
            }
        }

        /**
         * @brief Converts the stored body buffer to a string.
         *
         * @return Body content as a string.
         */
        std::string str() const {
            if(contiguous()) {
                const boost::beast::flat_buffer& flat_buffer = std::get<boost::beast::flat_buffer>(_buffer);
                return boost::beast::buffers_to_string(flat_buffer.data());
            } else {
                const boost::beast::multi_buffer& multi_buffer = std::get<boost::beast::multi_buffer>(_buffer);
                return boost::beast::buffers_to_string(multi_buffer.data());
            }
        }
    private:
        std::string                 _mime;
        std::error_code             _error;
        std::size_t                 _bytes_transferred;
        buffer_variant_type         _buffer;
        bool                        _contiguous;
        form_type                   _form;
    };
};

/**
 * @brief Manifold feature that resolves the selected route.
 *
 * Produces a route index identifying the route registry entry or the absence of
 * a matching route.
 *
 * @ingroup DoxyG_www
 */
struct locator{
    static constexpr const std::size_t stage = 0;
    static constexpr const std::string_view name = "locator";

    /**
     * @brief Route-index result produced by `locator`.
     */
    using result = udho::url::detail::route_index;
};

struct resources_storage{
    static constexpr const std::size_t stage = 0;
    static constexpr const std::string_view name = "resources_storage";
};

struct responder{
    static constexpr const std::size_t stage = 2;
    static constexpr const std::string_view name = "responder";
};

}

}
}

#endif // UDHO_WWW_FEATURES_H
