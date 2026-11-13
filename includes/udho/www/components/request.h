#ifndef UDHO_WWW_COMPONENTS_REQUEST_H
#define UDHO_WWW_COMPONENTS_REQUEST_H

#include <cstddef>
#include <utility>
#include <udho/manifold/config.h>
#include <udho/manifold/portal.h>
#include <udho/utils/string_view.h>
#include <udho/www/features.h>

namespace udho {
namespace www {
namespace components {

/**
 * @brief Transport-neutral access to the parsed HTTP request.
 * @ingroup DoxyG_www_components
 */
struct request {
    using features = udho::manifold::features<udho::www::feature::request_bind>;
    using params = udho::manifold::params<>;

    static constexpr const udho::utils::string_view name = "request";
};

}
}

namespace manifold {

/**
 * @brief Binds request results after protocol parsing completes.
 */
template <>
struct facet<udho::www::components::request, udho::www::feature::request_bind> {
    using component_type = udho::www::components::request;
    using config_type    = udho::manifold::config<component_type>;

    facet(component_type&, const config_type&, std::size_t) {}

    template <typename... Facets, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Facets...>& journal, NextT&& next, Stream&) const {
        using namespace udho::www::feature;
        next.pass(request_bind::result{
            journal.template at<header_reader>().value(),
            journal.template at<body_reader>().value()
        });
    }

    template <typename... Facets, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Facets...>& journal, NextT&& next, Stream& stream) const {
        eval(journal, std::forward<NextT>(next), stream);
    }
};

/**
 * @brief Read-only request access through the current request's journal.
 * @note both protocol and request should not be present in the context. A context containing protocol component's
 *       accessor requires knowledge about the underlying stream type (which could vary depending on transport type).
 *       However, using request component's accessor doesn't require that. It references the protocol component's
 *       results without requiring knowledge about the transport used.
 */
template <typename JournalT>
struct accessor<udho::www::components::request, JournalT>: basic_accessor<udho::www::components::request, JournalT> {
    using basic_accessor_type = basic_accessor<udho::www::components::request, JournalT>;
    using component_type = udho::www::components::request;
    using request_type = udho::www::feature::header_reader::result;
    using body_type = udho::www::feature::body_reader::result;

    using basic_accessor_type::basic_accessor_type;

    /**
     * @brief Returns the parsed header; requires an evaluated request_bind result.
     */
    const request_type& request() const {
        return basic_accessor_type::journal().template at<udho::www::feature::request_bind>().value().header;
    }

    /**
     * @brief Returns the parsed body; requires an evaluated request_bind result.
     */
    const body_type& body() const {
        return basic_accessor_type::journal().template at<udho::www::feature::request_bind>().value().body;
    }
};

}
}

#endif // UDHO_WWW_COMPONENTS_REQUEST_H
