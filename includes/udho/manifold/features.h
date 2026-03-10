#ifndef UDHO_MANIFOLD_FEATURES_H
#define UDHO_MANIFOLD_FEATURES_H

#include <map>
#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/net/common.h>
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
            std::string              _path;
            std::string              _extension;
            query_params_type        _params;
        public:
            result() = default;
            result(const result&) = default;
            inline result(const std::string_view& target): _resource(target) {}

            template <typename StrT>
            inline void resource(StrT&& name) { _resource = std::move(name); }
            inline const std::string& resource() const { return _resource; }

            template <typename StrT>
            inline void path(StrT&& name) { _path = std::move(name); }
            inline const std::string& path() const { return _path; }

            template <typename StrT>
            inline void extension(StrT&& ext) { _extension = std::move(ext); }
            inline const std::string& extension() const { return _extension; }

            inline const query_params_type& params() const { return _params; }

            template <typename StrT>
            inline void add(StrT&& key, StrT&& value) {
                _params.emplace(std::make_pair(std::move(key), std::move(value)));
            }
        };
    };

    struct cookie_load{
        static constexpr const std::size_t stage = 1;
        static constexpr const std::string_view name = "cookie_load";

        using result = udho::cookies::jar;
    };

    struct session_load{
        static constexpr const std::size_t stage = 1;
        static constexpr const std::string_view name = "session_load";

        using result = udho::session::note;
    };

    struct body_reader{
        static constexpr const std::size_t stage = 2;
        static constexpr const std::string_view name = "body_reader";

        struct result{
            using form_type             = udho::net::protocols::detail::form_data;
            using buffer_variant_type   = std::variant<
                    boost::beast::flat_buffer,
                    boost::beast::multi_buffer
                >;

            template <typename Buffer>
            result(const std::string& mime, udho::net::protocols::body_reader_result<Buffer>&& result, boost::system::error_code ec, std::size_t bytes): _mime(mime), _error(ec), _bytes_transferred(bytes) {
                _buffer     = result.release_buffer();
                _contiguous = std::is_same_v<Buffer, boost::beast::flat_buffer>;
                _form       = result.release_form();
            }

            const std::string& mime() const { return _mime; }
            bool contiguous() const { return _contiguous; }
            const udho::net::protocols::detail::form_data& form() { return _form; }

            std::size_t bytes_transferred() const { return _bytes_transferred; }
            std::error_code error() const { return  _error; }
            const buffer_variant_type& buffer() const { return _buffer; }

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

    struct locator{
        static constexpr const std::size_t stage = 0;
        static constexpr const std::string_view name = "locator";

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

template <>
struct feature_max_stage<>{
private:
public:
    static constexpr std::size_t value = 0;
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
