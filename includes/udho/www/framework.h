#ifndef UDHO_WWW_FRAMEWORK_H
#define UDHO_WWW_FRAMEWORK_H

#include <udho/www/components/handler.h>
#include <udho/www/components/routing.h>
#include <udho/www/label.h>
#include <udho/manifold/runtime.h>
#include <udho/www/sketch.h>

namespace udho{
namespace www{

namespace detail{

template <typename Label, typename StreamT, typename RouterT>
struct runtime_proxy{
    using router_type   = RouterT;
    using handler_type  = udho::www::components::basic_handler<StreamT>;
    using routing_type  = udho::www::components::routing<router_type>;
    using rlabel_type   = typename Label::template append<routing_type>;
    using runtime_type  = udho::manifold::basic_runtime<rlabel_type, StreamT>;

    runtime_proxy() = delete;
    runtime_proxy(const runtime_proxy&) = delete;

    runtime_proxy(RouterT&& router): _handler(router.summary()), _routing(std::move(router)) {}

    template <typename... Components>
    runtime_type runtime(Components&&... components) {
        return runtime_type(_routing, _handler, std::forward<Components>(components)...);
    }

    const router_type& router() const { return _routing.router(); }

private:
    handler_type _handler;
    routing_type _routing;
};

}

template <typename LabelT>
struct framework;

template <typename StreamT, typename Tag, typename... ExtraComponents>
struct framework<www::basic_label<StreamT, Tag, ExtraComponents...>>  {
    using endpoint_type = typename StreamT::endpoint_type;

    template <typename RoutingTableT>
    static detail::runtime_proxy<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, udho::url::basic_router<RoutingTableT>> apply(udho::url::basic_router<RoutingTableT>&& router) {
        return detail::runtime_proxy<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT, udho::url::basic_router<RoutingTableT>>(std::forward<udho::url::basic_router<RoutingTableT>>(router));
    }

};


}
}

#endif // UDHO_WWW_FRAMEWORK_H
