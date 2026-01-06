#ifndef UDHO_MANIFOLD_PORTAL_H
#define UDHO_MANIFOLD_PORTAL_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

template <typename CompositionT, typename JournalT>
struct portal;

template <typename ComponentT, typename JournalT>
struct basic_accessor{
    using component_type  = ComponentT;
    using config_type     = udho::manifold::config<component_type>;
    using journal_type    = JournalT;

    basic_accessor() = delete;
    basic_accessor(const basic_accessor&) = delete;

    basic_accessor(component_type& component, const config_type& config, const journal_type& journal): _component(component), _config(config), _journal(journal) {}

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

    const config_type& config() const { return _config; }

    const journal_type& journal() const { return _journal; }

private:
    component_type&     _component;
    const config_type&  _config;
    const journal_type& _journal;
};

template <typename ComponentT, typename JournalT>
struct accessor: public basic_accessor<ComponentT, JournalT>{
    using basic_accessor_type = basic_accessor<ComponentT, JournalT>;

    using basic_accessor_type::basic_accessor_type;

    // specialize accessor for any ComponentT and implement methods
    // access parameters for all components from config(), journal() and the access the component()
    // to provide facilities to interact with the component from usercode through context
};

template <typename Component, typename... Components, typename JournalT>
struct portal<udho::manifold::composition<Component, Components...>, JournalT>: public accessor<Component, JournalT>, public portal<udho::manifold::composition<Components...>, JournalT> {
    portal() = delete;
    portal(const portal&) = delete;

    template <typename... XComponents>
    portal(udho::manifold::composition<XComponents...>& composition, const udho::manifold::configs<XComponents...>& configs, const JournalT& journal)
        : accessor<Component, JournalT>(composition.template get<Component>().component(), configs.template get<Component>(), journal)
        , portal<udho::manifold::composition<Components...>, JournalT>(composition, configs, journal)
    {}

};

template <typename Component, typename JournalT>
struct portal<udho::manifold::composition<Component>, JournalT>: public accessor<Component, JournalT> {
    portal() = delete;
    portal(const portal&) = delete;

    template <typename... XComponents>
    portal(udho::manifold::composition<XComponents...>& composition, const udho::manifold::configs<XComponents...>& configs, const JournalT& journal)
        : accessor<Component, JournalT>(composition.template get<Component>().component(), configs.template get<Component>(), journal)
    {}
};

}
}

#endif // UDHO_MANIFOLD_PORTAL_H
