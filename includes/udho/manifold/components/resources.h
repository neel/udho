#ifndef UDHO_MANIFOLD_COMPONENTS_RESOURCES_H
#define UDHO_MANIFOLD_COMPONENTS_RESOURCES_H

#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/view/resources/store.h>
#include <udho/manifold/portal.h>

namespace udho{
namespace manifold{

namespace components{

template <typename... Bridges>
struct resources{
    using store_type = udho::view::resources::const_store<Bridges...>;

    UDHO_CONFIG_PARAM(enabled, bool, false);

    using features = udho::manifold::features<udho::manifold::feature::resources_storage>;
    using params   = udho::manifold::params<enabled>;
    static constexpr const udho::utils::string_view name = "resources";

    resources(store_type& store): _store(store) {}

    const store_type& store() const { return _store; }

private:
    store_type& _store;
};

}

template <typename... Bridges>
struct facet<components::resources<Bridges...>, udho::manifold::feature::resources_storage>{
    using component_type  = components::resources<Bridges...>;
    using config_type     = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) {
        next.skip();
    }

private:
    component_type&     _component;
    const config_type&  _config;
};


template <typename... Bridges, typename JournalT>
struct accessor<components::resources<Bridges...>, JournalT>: basic_accessor<components::resources<Bridges...>, JournalT>{
    using basic_accessor_type   = basic_accessor<components::resources<Bridges...>, JournalT>;
    using component_type        = components::resources<Bridges...>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;
    using store_type            = typename component_type::store_type;

    using basic_accessor_type::basic_accessor_type;

    const store_type& resources() const {
        return basic_accessor_type::component().store();
    }

    template <typename Bridge>
    udho::view::resources::tmpl::proxy<Bridge> view(const std::string& prefix, const std::string& name) const {
        return resources().template view<Bridge>(prefix, name);
    }
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_RESOURCES_H
