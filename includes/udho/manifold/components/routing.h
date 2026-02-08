#ifndef UDHO_MANIFOLD_COMPONENTS_ROUTING_H
#define UDHO_MANIFOLD_COMPONENTS_ROUTING_H

#include <udho/url/router.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/exceptions/exceptions.h>

namespace udho{
namespace manifold{

namespace components{

template <typename RouterT>
class routing{
    static_assert(udho::url::is_router<RouterT>::value);
    using router_type = RouterT;

private:
    router_type _router;
public:
    using features = udho::manifold::features<
        udho::manifold::feature::locator,
        udho::manifold::feature::responder
    >;

    UDHO_CONFIG_PARAM(use_trie, bool, false);

    using params   = udho::manifold::params<use_trie>;

    static constexpr const udho::utils::string_view name = "router";

    routing(router_type&& router): _router(std::move(router)) {}

    router_type& router() { return _router; }
    const router_type& router() const { return _router; }

    udho::url::detail::route_index locate(const std::string& subject) {
        udho::url::detail::route_index route = _router.index_of(subject);
        return route;
    }
};

}



template <typename RouterT>
struct facet<components::routing<RouterT>, udho::manifold::feature::locator> {
    using component_type = components::routing<RouterT>;
    using config_type    = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        udho::manifold::feature::identifier::result res = journal.template at<udho::manifold::feature::identifier>();
        // udho::utils::string_view tgt = res.resource();
        // std::string target(tgt.begin(), tgt.end());
        udho::url::detail::route_index index = _component.locate(res.resource());
        if(!index.valid()) {
            index = _component.locate(res.path());
        }
        if(index.type() != udho::url::detail::route_index::type::none) {
            next.pass(std::move(index));
        } else {
            next.fail(udho::http::error(boost::beast::http::status::not_found, udho::utils::format("route not found {}", res.resource())));
        }
    }

    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        std::cout << "-> facet<components::routing<RoutingTableT>, udho::manifold::feature::locator>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }

private:
    component_type& _component;
    const config_type& _config;
};

template <typename RouterT>
struct facet<components::routing<RouterT>, udho::manifold::feature::responder> {
    using component_type = components::routing<RouterT>;
    using facet_type     = facet<component_type, udho::manifold::feature::locator>;
    using config_type    = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        udho::url::detail::route_index route_index = journal.template get<facet_type>();
        bool success = _component.router().invoke_at(route_index, stream);
        if(success) next.pass();
        else        next.fail();
    }

    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        std::cout << "-> facet<components::routing<RoutingTableT>, udho::manifold::feature::responder>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }

private:
    component_type& _component;
    const config_type& _config;
};

template <typename RouterT, typename JournalT>
struct accessor<components::routing<RouterT>, JournalT>: basic_accessor<components::routing<RouterT>, JournalT>{
    using basic_accessor_type   = basic_accessor<components::routing<RouterT>, JournalT>;
    using component_type        = components::routing<components::routing<RouterT>>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;

};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_ROUTING_H
