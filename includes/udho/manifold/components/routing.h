#ifndef UDHO_MANIFOLD_COMPONENTS_ROUTING_H
#define UDHO_MANIFOLD_COMPONENTS_ROUTING_H

#include <udho/url/router.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

namespace components{

// template <typename RoutingTable>
// struct routing;

template <typename RoutingTableT>
class routing {
public:
    using routing_table_type = RoutingTableT;
private:
    const routing_table_type& _table;

public:
    using features = udho::manifold::features<
        udho::manifold::feature::locator/*,
        udho::manifold::feature::responder*/
    >;

    UDHO_CONFIG_PARAM(use_trie, bool, false);

    using params   = udho::manifold::params<use_trie>;

    static constexpr const udho::utils::string_view name = "router";

    routing(const routing_table_type& table): _table(table) {}

    routing_table_type& table() { return _table; }

    const routing_table_type& table() const { return _table; }

    udho::url::detail::route_index locate(const std::string& subject) {
        udho::url::detail::route_index route = _table.index_of(subject);
        if(!route.valid()) {
            throw std::out_of_range{udho::utils::format("Failed to locate resource corresponding to identifier {}", subject)};
        }
        return route;
    }
};

}



template <typename RoutingTableT>
struct facet<components::routing<RoutingTableT>, udho::manifold::feature::locator> {
    using component_type = components::routing<RoutingTableT>;
    using config_type    = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        udho::manifold::feature::identifier::result res = journal.template at<udho::manifold::feature::identifier>();
        udho::utils::string_view tgt = res.resource();
        std::string target(tgt.begin(), tgt.end());
        udho::url::detail::route_index index = _component.locate(target);
        next.pass(std::move(index));
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

template <typename MountpointsT>
struct facet<components::routing<MountpointsT>, udho::manifold::feature::responder> {
    using component_type = components::routing<MountpointsT>;
    using facet_type     = facet<component_type, udho::manifold::feature::locator>;
    using config_type    = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        udho::url::detail::route_index route_index = journal.template get<facet_type>();
        bool success = _component.invoke_at(route_index);
        if(success) next.pass();
        else        next.fail();
    }



private:
    component_type& _component;
    const config_type& _config;
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_ROUTING_H
