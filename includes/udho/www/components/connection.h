#ifndef UDHO_WWW_COMPONENTS_CONNECTION_H
#define UDHO_WWW_COMPONENTS_CONNECTION_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include <udho/logging/macros.h>
#include <udho/manifold/features.h>
#include <udho/manifold/params.h>
#include <udho/manifold/portal.h>
#include <udho/utils/format.h>
#include <udho/utils/string_view.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/www/response_registry.h>

namespace udho {
namespace www {
namespace components {

/**
 * @brief Stream-neutral facilities shared by WWW action contexts.
 *
 * A connection owns the response registry used by the typed handler. It is a
 * runtime component, but it has no pipeline facet: request-local operations
 * receive the flow id from the action context.
 */
class connection {
public:
    using features  = udho::manifold::features<>;
    using params    = udho::manifold::params<>;
    using hook_type = typename udho::www::response_registry::hook_type;

    static constexpr const udho::utils::string_view name = "connection";

    explicit connection(const udho::url::summary::router& summary)
        : _responses(summary)
    {}

    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    /** @brief Return the registry borrowed by the typed response handler. */
    udho::www::response_registry& responses() noexcept { return _responses; }

    /** @brief Return the registry borrowed by the typed response handler. */
    const udho::www::response_registry& responses() const noexcept { return _responses; }

    /** @brief Register a completion hook for an active response. */
    bool add_hook(std::size_t flow_id, hook_type hook) {
        return _responses.add_hook(flow_id, std::move(hook));
    }

    /** @brief Return the immutable router summary associated with this runtime. */
    const udho::url::summary::router& summary() const noexcept {
        return _responses.summary();
    }

private:
    udho::www::response_registry _responses;
};

} // namespace components
} // namespace www

namespace manifold {

/**
 * @brief Action-facing access to stream-neutral WWW connection facilities.
 */
template <typename JournalT>
struct accessor<udho::www::components::connection, JournalT>
    : basic_accessor<udho::www::components::connection, JournalT> {
    using basic_accessor_type = basic_accessor<udho::www::components::connection, JournalT>;
    using component_type      = udho::www::components::connection;
    using hook_type           = typename component_type::hook_type;

    using basic_accessor_type::basic_accessor_type;

    /** @brief Register a completion hook for a flow. */
    void add_hook(std::size_t flow_id, hook_type hook) {
        if(!hook) return;
        if(!basic_accessor_type::component().add_hook(flow_id, std::move(hook))) {
            throw std::logic_error{udho::utils::format("Cannot register a completion hook for detached flow {}", flow_id)};
        }
    }

    /**
     * @brief Construct a layout and retain its shared state until completion.
     */
    template <typename PlaceholderT, typename ContextT, typename PresenterT = udho::view::tmpl::layout::default_presenter<udho::view::tmpl::layout::basic_document<PlaceholderT>>>
    auto layout(ContextT context) {
        using placeholder_type = PlaceholderT;
        using presenter_type   = PresenterT;

        const std::size_t flow_id = context.flow_id();
        auto page = udho::view::tmpl::layout::create<placeholder_type, ContextT, presenter_type>(context);
        add_hook(flow_id, [keepalive = page, flow_id]() {
            static_cast<void>(keepalive);
            UDHO_LOG_TRACE("www::components::connection", "Releasing layout retained for response completion", udho::logging::params::flow_id(flow_id));
        });
        return page;
    }

    /** @brief Return the immutable router summary. */
    const udho::url::summary::router& routes() const noexcept {
        return basic_accessor_type::component().summary();
    }

    /** @brief Return a named mount-point summary. */
    const udho::url::summary::mount_point& route(const std::string& name) const {
        return routes()[name];
    }

    /** @brief Return a mount-point summary named by a compile-time string. */
    template <typename Char, Char... C>
    const udho::url::summary::mount_point& route(udho::hazo::string::str<Char, C...>&& hstr) const {
        return route(hstr.str());
    }
};

} // namespace manifold
} // namespace udho

#endif // UDHO_WWW_COMPONENTS_CONNECTION_H
