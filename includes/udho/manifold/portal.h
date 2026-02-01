#ifndef UDHO_MANIFOLD_PORTAL_H
#define UDHO_MANIFOLD_PORTAL_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/config.h>
#include <udho/manifold/detail.h>

namespace udho{
namespace manifold{

template <typename ComponentT, typename JournalViewT>
struct basic_accessor;

template <typename ComponentT, typename... Facets>
struct basic_accessor<ComponentT, udho::manifold::journal_const_view<Facets...>>{
    using component_type    = ComponentT;
    using config_type       = udho::manifold::config<component_type>;
    using journal_view_type = udho::manifold::journal_const_view<Facets...>;

    basic_accessor() = delete;
    basic_accessor(const basic_accessor&) = default;

    basic_accessor(component_type& component, const config_type& config, const journal_view_type& journal): _component(component), _config(config), _journal(journal) {}

    component_type& component() { return _component; }
    const component_type& component() const { return _component; }

    const config_type& config() const { return _config; }

    const journal_view_type& journal() const { return _journal; }

private:
    component_type&     _component;
    const config_type&  _config;
    journal_view_type   _journal;
};

template <typename ComponentT, typename JournalViewT>
struct accessor: public basic_accessor<ComponentT, JournalViewT>{
    using basic_accessor_type = basic_accessor<ComponentT, JournalViewT>;

    using basic_accessor_type::basic_accessor_type;

    // specialize accessor for any ComponentT and implement methods
    // access parameters for all components from config(), journal() and the access the component()
    // to provide facilities to interact with the component from usercode through context
};

namespace detail{

template <typename JournalViewT, typename... Components>
struct portal;

template <typename JournalViewT, typename ComponentT, typename... Components>
struct portal<JournalViewT, ComponentT, Components...>: accessor<ComponentT, JournalViewT>, portal<JournalViewT, Components...> {
    template <typename CompositionT, typename ConfigsT>
    portal(CompositionT& composition, ConfigsT& configs, const JournalViewT& journal)
        : accessor<ComponentT, JournalViewT>(composition.template get<ComponentT>().component(), configs.template get<ComponentT>(), journal)
        , portal<JournalViewT, Components...>(composition, configs, journal)
    {}
};

template <typename JournalViewT>
struct portal<JournalViewT> {
    template <typename CompositionT, typename ConfigsT>
    portal(CompositionT&, ConfigsT&, const JournalViewT&) {}
};

}

template <typename... Components>
struct portal: detail::portal<typename udho::manifold::detail::get_journal_const_view_for_all_components<Components...>::type, Components...>{
    using composition_type      = udho::manifold::composition<Components...>;
    using configs_type          = udho::manifold::configs<Components...>;
    using journal_type          = typename udho::manifold::detail::get_journal_for_all_components<Components...>::type;
    using composition_view_type = udho::manifold::composition_view<Components...>;
    using configs_view_type     = udho::manifold::configs_view<Components...>;
    using journal_view_type     = typename udho::manifold::detail::get_journal_const_view_for_all_components<Components...>::type;
    using detail_portal_type    = detail::portal<typename udho::manifold::detail::get_journal_const_view_for_all_components<Components...>::type, Components...>;

    template <typename... XComponents>
    friend struct portal;

    template <typename StreamT, typename... XComponents>
    friend struct basic_context;

    template <typename ComponentQ>
    using has = typename composition_view_type::template has_component<ComponentQ>;

    portal(composition_type& composition, configs_type& configs, const journal_type& journal)
        : detail_portal_type(composition, configs, journal)
        , _composition_view(composition), _configs_view(configs), _journal_view(journal)
    {}

    template <typename... OtherComponents>
    portal(portal<OtherComponents...>& other)
        : detail_portal_type(other._composition_view, other._configs_view, other._journal_view)
        , _composition_view(other._composition_view), _configs_view(other._configs_view), _journal_view(other._journal_view)
    {}

private:
    composition_view_type _composition_view;
    configs_view_type     _configs_view;
    journal_view_type     _journal_view;
};

template <>
struct portal<>{
    template <typename ComponentQ>
    using has = std::false_type;

    template <typename CompositionT, typename ConfigsT, typename JournalT>
    portal(CompositionT&, ConfigsT&, const JournalT&){}

    template <typename... OtherComponents>
    portal(portal<OtherComponents...>&) {}
};

namespace detail{
template <typename CompositionT>
struct get_portal_type;

template <typename... Components>
struct get_portal_type<udho::manifold::composition<Components...>>{
    using type = udho::manifold::portal<Components...>;
};

}

}
}

#endif // UDHO_MANIFOLD_PORTAL_H
