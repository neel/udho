#ifndef UDHO_WWW_COMPONENTS_RESOURCES_H
#define UDHO_WWW_COMPONENTS_RESOURCES_H

#include <udho/www/features.h>
#include <udho/manifold/config.h>
#include <udho/view/resources/store.h>
#include <udho/manifold/portal.h>
#include <udho/www/components/params.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{

namespace components{

/**
 * @brief Component exposing a read-only view-resource store.
 * @tparam Bridges Resource bridge types.
 * @ingroup DoxyG_www_components
 */
template <typename... Bridges>
struct resources{
    using store_type = udho::view::resources::const_store<Bridges...>;

    using features = udho::manifold::features<udho::www::feature::resources_storage>;
    using params   = udho::manifold::params<udho::www::params::resources::enabled>;
    static constexpr const udho::utils::string_view name = "resources";

    /**
     * @brief Constructs the component with a referenced resource store.
     * @param store Resource store referenced by the component.
     */
    resources(store_type& store): _store(store) {}

    /** @brief Returns the referenced resource store. */
    const store_type& store() const { return _store; }

private:
    store_type& _store;
};

}
} // www

namespace manifold{

/**
 * @brief Pipeline facet representing availability of resource storage.
 * @tparam Bridges Resource bridge types.
 * @ingroup DoxyG_www_components_facets
 */
template <typename... Bridges>
struct facet<udho::www::components::resources<Bridges...>, udho::www::feature::resources_storage>{
    using component_type  = udho::www::components::resources<Bridges...>;
    using config_type     = udho::manifold::config<component_type>;

    /**
     * @brief Constructs the facet.
     * @param component Resources component.
     * @param config Component configuration.
     * @param id Flow identifier.
     */
    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    /**
     * @brief Skips evaluation and advances the pipeline.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @tparam Stream Stream type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) {
        next.skip();
    }

private:
    component_type&     _component;
    const config_type&  _config;
};


/**
 * @brief Portal accessor for resources and view lookup.
 * @tparam Bridges Resource bridge types.
 * @tparam JournalT Journal view type.
 * @ingroup DoxyG_www_components_accessors
 */
template <typename... Bridges, typename JournalT>
struct accessor<udho::www::components::resources<Bridges...>, JournalT>: basic_accessor<udho::www::components::resources<Bridges...>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::resources<Bridges...>, JournalT>;
    using component_type        = udho::www::components::resources<Bridges...>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;
    using store_type            = typename component_type::store_type;

    using basic_accessor_type::basic_accessor_type;

    /** @brief Returns the component's resource store. */
    const store_type& resources() const {
        return basic_accessor_type::component().store();
    }

    /**
     * @brief Looks up a view for the selected bridge, prefix, and name.
     * @tparam Bridge Resource bridge type.
     * @param prefix View prefix.
     * @param name View name.
     */
    template <typename Bridge>
    udho::view::resources::tmpl::proxy<Bridge> view(const std::string& prefix, const std::string& name) const {
        return resources().template view<Bridge>(prefix, name);
    }
};

} // manifold
} // udho

/** @} */

#endif // UDHO_WWW_COMPONENTS_RESOURCES_H
