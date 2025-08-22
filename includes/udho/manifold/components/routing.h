#ifndef UDHO_MANIFOLD_COMPONENTS_ROUTING_H
#define UDHO_MANIFOLD_COMPONENTS_ROUTING_H

#include <udho/url/router.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

namespace components{

template <typename RoutingTable>
struct routing;

template <typename MountpointsT>
class routing<udho::url::detail::routing_table<MountpointsT>> {
    using routing_table_type = udho::url::detail::routing_table<MountpointsT>;

    routing_table_type _table;

public:
    using features = udho::manifold::features<udho::manifold::feature::locator, udho::manifold::feature::responder>;
    using params   = udho::manifold::params<>;

    static constexpr const udho::utils::string_view name = "router";

    routing(routing_table_type&& table): _table(std::move(table)) {}

    udho::url::detail::route_index locate(const std::string& subject) {
        return _table.index_of(subject);
    }
};

}



template <typename MountpointsT>
struct facet<components::routing<MountpointsT>, udho::manifold::feature::locator> {
    using component_type = components::routing<MountpointsT>;
    using result = udho::url::detail::route_index;

    facet(component_type& component, const typename component_type::params& params): _component(component), _params(params) {}

    template <typename... Components>
    result eval(const udho::manifold::journal<Components...>& journal, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        udho::utils::string_view tgt = request.target();
        std::string target(tgt.begin(), tgt.end());
        return _component.locate(target);
    }

private:
    component_type& _component;
    typename component_type::params& _params;
};

class routing_invocation_result{
    bool _success;
public:
    inline explicit routing_invocation_result(bool success): _success(success) {}
    routing_invocation_result(const routing_invocation_result&) = default;

    inline bool success() const { return _success; }
};

template <typename MountpointsT>
struct facet<components::routing<MountpointsT>, udho::manifold::feature::responder> {
    using component_type = components::routing<MountpointsT>;
    using result = routing_invocation_result;
    using facet_type = facet<component_type, udho::manifold::feature::locator>;

    facet(component_type& component, const typename component_type::params& params): _component(component), _params(params) {}

    template <typename... Components>
    result eval(const udho::manifold::journal<Components...>& journal, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        udho::url::detail::route_index route_index = journal.template get<facet_type>();
        bool success = _component.invoke_at(route_index);
        return routing_invocation_result{success};
    }

private:
    component_type& _component;
    typename component_type::params& _params;
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_ROUTING_H
