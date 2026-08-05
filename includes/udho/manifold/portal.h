#ifndef UDHO_MANIFOLD_PORTAL_H
#define UDHO_MANIFOLD_PORTAL_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/config.h>
#include <udho/manifold/detail.h>
#include <udho/manifold/composition_view.h>
#include <udho/manifold/configs_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/view/data/data.h>

namespace udho{
namespace manifold{

/**
 * @ingroup manifold
 * @{
 */

template <typename ComponentT, typename JournalViewT>
struct basic_accessor;

/**
 * @brief Read-only access bundle for one component inside a portal.
 *
 * `basic_accessor` binds together a component instance, its configuration,
 * and a const journal view. It is the object passed to portal visitors and is
 * also the base class that users may specialize through `accessor`.
 *
 * @tparam ComponentT Component type exposed by this accessor.
 * @tparam JournalViewT Const journal view type available to the accessor.
 *
 * @see accessor
 * @see portal
 */
template <typename ComponentT, typename... Facets>
struct basic_accessor<ComponentT, udho::manifold::journal_const_view<Facets...>>{
    using component_type    = ComponentT;
    using config_type       = udho::manifold::config<component_type>;
    using journal_view_type = udho::manifold::journal_const_view<Facets...>;

    basic_accessor() = delete;
    basic_accessor(const basic_accessor&) = default;

    /**
     * @brief Constructs an accessor from component, config, and journal view.
     *
     * @param component Component instance exposed by this accessor.
     * @param config Configuration associated with the component.
     * @param journal Const journal view available to user code.
     *
     * @warning `component` and `config` must outlive this accessor.
     */
    basic_accessor(component_type& component, const config_type& config, const journal_view_type& journal): _component(component), _config(config), _journal(journal) {}

    /**
     * @brief Gets the mutable component instance.
     * @return Mutable reference to the component.
     */
    component_type& component() { return _component; }
    /**
     * @brief Gets the const component instance.
     * @return Const reference to the component.
     */
    const component_type& component() const { return _component; }
    /**
     * @brief Gets the component configuration.
     * @return Const reference to the component configuration.
     */
    const config_type& config() const { return _config; }
    /**
     * @brief Gets the const journal view.
     * @return Const reference to the journal view available to this accessor.
     */
    const journal_view_type& journal() const { return _journal; }

private:
    component_type&     _component;
    const config_type&  _config;
    journal_view_type   _journal;
};

/**
 * @brief Extension point for component-specific portal access.
 *
 * The default `accessor` simply inherits `basic_accessor`. Users may specialize
 * this template for a component type to provide higher-level methods while still
 * retaining access to `component()`, `config()`, and `journal()`.
 *
 * @tparam ComponentT Component type exposed by the accessor.
 * @tparam JournalViewT Const journal view type available to the accessor.
 *
 * @see basic_accessor
 * @see portal
 */
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

    template <typename F>
    void visit(F&& visitor){
        accessor<ComponentT, JournalViewT>& accessor = *this;
        visitor(accessor);

        portal<JournalViewT, Components...>::visit(std::forward<F>(visitor));
    }

};

template <typename JournalViewT>
struct portal<JournalViewT> {
    template <typename CompositionT, typename ConfigsT>
    portal(CompositionT&, ConfigsT&, const JournalViewT&) {}

    template <typename F>
    void visit(F&& visitor){}
};

}


/**
 * @brief Runtime access facade over composition, configuration, and journal state.
 *
 * A portal exposes component accessors for all `Components...`. Each accessor
 * provides access to a component instance, the component's configuration, and a
 * const view of the journal.
 *
 * `portal` is non-owning: it references component wrappers, configs, and journal
 * results owned by the runtime/flow.
 *
 * @tparam Components Component types exposed through the portal.
 *
 * @see basic_accessor
 * @see accessor
 * @see composition_view
 * @see configs_view
 * @see journal_const_view
 */
template <typename... Components>
struct portal: detail::portal<typename udho::manifold::detail::get_journal_const_view_for_all_components<Components...>::type, Components...>{
    using composition_type      = udho::manifold::composition<Components...>;
    using configs_type          = udho::manifold::configs<Components...>;
    using journal_type          = typename udho::manifold::detail::get_journal_for_all_components<Components...>::type;
    using composition_view_type = udho::manifold::composition_view<Components...>;
    using configs_view_type     = udho::manifold::configs_view<Components...>;
    using journal_view_type     = typename udho::manifold::detail::get_journal_const_view_for_all_components<Components...>::type;
    using detail_portal_type    = detail::portal<typename udho::manifold::detail::get_journal_const_view_for_all_components<Components...>::type, Components...>;

    /**
     * @brief Deleted conversion to Boost.Asio executor.
     * @internal
     * This prevents accidental treatment of a portal as an executor-like object.
     */
    operator boost::asio::executor() const = delete;

    template <typename... XComponents>
    friend struct portal;

    template <typename StreamT, typename... XComponents>
    friend struct basic_context;

    /**
     * @brief Checks whether the portal exposes a component type.
     * @tparam ComponentQ Component type to query.
     */
    template <typename ComponentQ>
    using has = typename composition_view_type::template has_component<ComponentQ>;

    /**
     * @brief Constructs a portal from runtime-owned state.
     *
     * @param composition Component composition whose wrappers are exposed.
     * @param configs Component configuration collection.
     * @param journal Journal containing facet evaluation results.
     *
     * @warning The referenced composition, configs, and journal must outlive this
     *          portal.
     */
    portal(composition_type& composition, configs_type& configs, const journal_type& journal)
        : detail_portal_type(composition, configs, journal)
        , _composition_view(composition), _configs_view(configs), _journal_view(journal)
    {}

    /**
     * @brief Constructs a portal from another portal.
     *
     * This is used to create a subset portal from a portal that exposes a compatible
     * superset of components.
     *
     * @tparam OtherComponents Component types exposed by the source portal.
     * @param other Source portal.
     *
     * @warning The objects referenced by `other` must outlive this portal.
     */
    template <typename... OtherComponents>
    portal(portal<OtherComponents...>& other)
        : detail_portal_type(other._composition_view, other._configs_view, other._journal_view)
        , _composition_view(other._composition_view), _configs_view(other._configs_view), _journal_view(other._journal_view)
    {}

    /**
     * @brief Visits every component accessor exposed by this portal.
     *
     * The visitor is called once for each component accessor, in portal component
     * order.
     *
     * @tparam F Visitor callable type.
     * @param visitor Callable object receiving `accessor<ComponentT, journal_view_type>&`.
     */
    template <typename F>
    void visit(F&& visitor){
        detail_portal_type::visit(std::forward<F>(visitor));
    }

    // friend auto metatype(udho::view::data::type<portal<Components...>>){
    //     using namespace udho::view::data;

    //     return assoc("portal");
    // }

private:
    composition_view_type _composition_view;
    configs_view_type     _configs_view;
    journal_view_type     _journal_view;
};

/**
 * @brief Empty portal specialization.
 *
 * Represents a portal exposing no components. This specialization is used as
 * the base case for subset portals and recursive portal construction.
 */
template <>
struct portal<>{
    /**
     * @brief Always resolves to `std::false_type` for the empty portal.
     *
     * @tparam ComponentQ Component type to query.
     */
    template <typename ComponentQ>
    using has = std::false_type;

    /**
     * @brief Deleted conversion to Boost.Asio executor.
     */
    operator boost::asio::executor() const = delete;

    /**
     * @brief Constructs an empty portal from arbitrary runtime state.
     *
     * @tparam CompositionT Composition type.
     * @tparam ConfigsT Config collection type.
     * @tparam JournalT Journal type.
     */
    template <typename CompositionT, typename ConfigsT, typename JournalT>
    portal(CompositionT&, ConfigsT&, const JournalT&){}

    /**
     * @brief Constructs an empty portal from another portal.
     *
     * @tparam OtherComponents Component types exposed by the source portal.
     */
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

/**
 * @}
 */

}
}

#endif // UDHO_MANIFOLD_PORTAL_H
