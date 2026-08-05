#ifndef UDHO_MANIFOLD_CONFIGS_VIEW_H
#define UDHO_MANIFOLD_CONFIGS_VIEW_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

/**
 * @ingroup DoxyG_manifold
 * @{
 */

/**
 * @brief Non-owning typed view over a subset of component configurations.
 *
 * `configs_view` references configuration objects stored in a `configs`
 * instance or in another `configs_view`. It allows APIs to expose only the
 * configurations relevant to a selected component subset.
 *
 * @tparam ComponentT First component type exposed by this view.
 * @tparam Components Remaining component types exposed by this view.
 *
 * @see configs
 * @see config
 */
template <typename ComponentT, typename... Components>
struct configs_view<ComponentT, Components...>: configs_view<Components...>{
    /**
     * @brief Configuration type for the current component.
     */
    using config_type = config<ComponentT>;
    /**
     * @brief Recursive tail view for the remaining components.
     */
    using rest_type   = configs_view<Components...>;

    /**
     * @brief Constructs a configuration view from a configuration collection.
     *
     * The source collection must contain every component type requested by this
     * view.
     *
     * @tparam OtherComponents Component types present in the source collection.
     * @param other Source configuration collection.
     *
     * @warning The source collection must outlive this view.
     */
    template <typename... OtherComponents>
    configs_view(configs<OtherComponents...>& other): _config(other.template get<ComponentT>()), configs_view<Components...>(other) { }

    /**
     * @brief Constructs a configuration view from another configuration view.
     *
     * @tparam OtherComponents Component types present in the source view.
     * @param other Source configuration view.
     *
     * @warning The objects referenced by `other` must outlive this view.
     */
    template <typename... OtherComponents>
    configs_view(configs_view<OtherComponents...>& other): _config(other.template get<ComponentT>()), configs_view<Components...>(other) { }

    /**
     * @brief Gets the const configuration for a component.
     *
     * @tparam XComponentT Component type to retrieve. Must be `ComponentT`.
     * @return Const reference to a component configuration.
     */
    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const config_type& get() const { return _config; }

    /**
     * @brief Gets the mutable configuration for a current component.
     *
     * @tparam XComponentT Component type to retrieve. Must be `ComponentT`.
     * @return Mutable reference to a component configuration.
     */
    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    config_type& get() { return _config; }

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const auto& get() const { return rest_type::template get<XComponentT>(); }

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    auto& get() { return rest_type::template get<XComponentT>(); }

    /**
     * @brief Accesses a mutable parameter value from a component configuration.
     *
     * @tparam ParamT Parameter type.
     * @param key Parameter key, normally `ParamT::val`.
     * @return Mutable reference to the stored parameter value wrapper.
     */
    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _config[key]; }

    /**
     * @brief Accesses a const parameter value from a component configuration.
     *
     * @tparam ParamT Parameter type.
     * @param key Parameter key, normally `ParamT::val`.
     * @return Const reference to the stored parameter value wrapper.
     */
    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _config[key]; }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return rest_type::template operator[]<ParamT>(key); }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return rest_type::template operator[]<ParamT>(key); }
private:
    config_type& _config;
};

template <>
struct configs_view<>{
    template <typename... OtherComponents>
    configs_view(udho::manifold::configs<OtherComponents...>& other) {}

    template <typename... OtherComponents>
    configs_view(configs_view<OtherComponents...>& other) {}
};

/**
 * @}
 */


}
}

#endif // UDHO_MANIFOLD_CONFIGS_VIEW_H
