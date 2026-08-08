#ifndef UDHO_WWW_FRAMEWORK_H
#define UDHO_WWW_FRAMEWORK_H

#include <udho/www/components/handler.h>
#include <udho/www/components/routing.h>
#include <udho/www/label.h>
#include <udho/manifold/runtime.h>
#include <udho/www/sketch.h>

namespace udho{
namespace www{

/**
 * @brief Helper object that owns routing and handler components before runtime creation.
 *
 * `runtime_generator` is returned by `framework::apply(router)`. It stores the
 * routing component built from the router and the handler initialized from the
 * router summary. Calling `runtime()` creates the final manifold runtime with
 * these components plus any user-supplied components.
 *
 * @tparam Label Base www label.
 * @tparam StreamT Stream type used by the runtime.
 * @tparam RouterT Router type used by the routing component.
 *
 * @ingroup DoxyG_www
 */
template <typename Label, typename StreamT, typename RouterT>
struct runtime_generator{
    using router_type   = RouterT;
    using handler_type  = udho::www::components::basic_handler<StreamT>;
    using routing_type  = udho::www::components::routing<router_type>;
    using rlabel_type   = typename Label::template append<routing_type>;
    using runtime_type  = udho::manifold::basic_runtime<rlabel_type, StreamT>;

    runtime_generator() = delete;
    runtime_generator(const runtime_generator&) = delete;

    /**
     * @brief Constructs a runtime generator from a router.
     *
     * The router is moved into the routing component. The handler is initialized
     * from `router.summary()`.
     *
     * @param router Router object to move into the generator.
     */
    runtime_generator(RouterT&& router): _handler(router.summary()), _routing(std::move(router)) {}

    /**
     * @brief Creates the manifold runtime.
     *
     * The runtime is created with the routing component, handler component, and any
     * additional components supplied by the caller.
     *
     * @tparam Components Additional component argument types.
     * @param components Additional runtime component arguments.
     * @return Runtime object for the www label extended with the routing component.
     */
    template <typename... Components>
    runtime_type runtime(Components&&... components) {
        return runtime_type(_routing, _handler, std::forward<Components>(components)...);
    }

    /**
     * @brief Creates the manifold runtime.
     *
     * This overload expects the first parameter to be a const_store, from which it creates
     * a resources component which is passed to the other overload.
     * The runtime is created with the routing component, handler component, resources component, and any
     * additional components supplied by the caller.
     *
     * @tparam Bridges... View bridges
     * @tparam Components... Additional component argument types.
     * @param cstore const store
     * @param components... Additional runtime component arguments.
     * @return Runtime object for the www label extended with the routing component.
     */
    template <typename... Bridges, typename... Components>
    runtime_type runtime(udho::view::resources::const_store<Bridges...>& cstore, Components&&... components) {
        auto resources = udho::www::components::resources(cstore);
        return runtime(std::move(resources), std::forward<Components>(components)...);
    }

    /**
     * @brief Gets the router stored inside the routing component.
     *
     * @return Const reference to the router.
     */
    const router_type& router() const { return _routing.router(); }

private:
    handler_type _handler;
    routing_type _routing;
};


template <typename LabelT>
struct framework;

/**
 * @brief Framework front-end for constructing a www runtime from a router.
 *
 * @tparam LabelT www label type.
 *
 * @ingroup DoxyG_www
 */
template <typename StreamT, typename Tag, typename... ExtraComponents>
struct framework<www::basic_label<StreamT, Tag, ExtraComponents...>>  {
    using label_type    = www::basic_label<StreamT, Tag, ExtraComponents...>;
    using endpoint_type = typename StreamT::endpoint_type;

    /**
     * @brief Applies a router and returns a runtime generator.
     *
     * The returned generator can be used to construct the actual runtime after optional
     * user components are supplied.
     *
     * @tparam RoutingTableT Routing table type stored by the router.
     * @param router Router object to move into the framework.
     * @return Runtime generator owning routing and handler components.
     */
    template <typename RoutingTableT>
    static runtime_generator<label_type, StreamT, udho::url::basic_router<RoutingTableT>> apply(udho::url::basic_router<RoutingTableT>&& router) {
        using router_type            = udho::url::basic_router<RoutingTableT>;
        using runtime_generator_type = runtime_generator<label_type, StreamT, router_type>;

        return runtime_generator_type(std::forward<router_type>(router));
    }

};


}
}

#endif // UDHO_WWW_FRAMEWORK_H
