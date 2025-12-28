#ifndef UDHO_MANIFOLD_CONFIG_H
#define UDHO_MANIFOLD_CONFIG_H

#include <type_traits>
#include <nlohmann/json_fwd.hpp>
#include <udho/manifold/fwd.h>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <nlohmann/json.hpp>
#include <udho/manifold/traits.h>
#include <udho/manifold/params.h>

namespace udho{
namespace manifold{

/**
 * @addtogroup manifold
 * @{
 */

/**
 * @brief Type-safe configuration container for a component
 *
 * Wraps a component's params with JSON serialization and type-safe access.
 * Each component has its own config instance storing parameter values.
 *
 * @tparam ComponentT The component type this config belongs to
 *
 * # Parameter Access
 *
 * @code
 * basic_config<MyComponent> cfg;
 *
 * // Access by parameter type
 * cfg[MyComponent::timeout::val] = 5000;
 * int timeout = cfg[MyComponent::timeout::val].value();
 * @endcode
 *
 * # JSON Persistence
 *
 * Configs serialize to JSON with the component name as the root key:
 * @code
 * nlohmann::json root;
 * cfg.save(root);
 * // Result: {
 * //   "MyComponent": {
 * //     "timeout": 5000,
 * //     "enabled": true
 * //   }
 * // }
 *
 * cfg.load(root);  // Load from same format
 * @endcode
 *
 * @see params
 * @see config
 * @see configs
 */
template <typename ComponentT>
struct basic_config{
    using components_type = ComponentT;
    using params_type = typename udho::manifold::component_traits<ComponentT>::params;

    /**
     * @brief checks if the configuration contains the parameter
     * @tparam ParamT Parameter type to check
     */
    template <typename ParamT>
    using contains = typename params_type::template contains<ParamT>;

    basic_config() = default;

    basic_config(const basic_config<ComponentT>& other): _params(other._params) {}

    /**
     * @brief Save configuration to JSON
     *
     * Creates a nested JSON structure:
     * @code
     * {
     *   "ComponentName": {
     *     "param1": value1,
     *     "param2": value2
     *   }
     * }
     * @endcode
     *
     * @param json Output JSON object (must be an object)
     * @pre json.is_object() == true
     */
    void save(nlohmann::json& json) const {
        auto params = nlohmann::json::object();
        _params.save(params);
        assert(json.is_object());
        json[udho::manifold::component_name<ComponentT>()]= params;
    }

    /**
     * @brief Load configuration from JSON
     *
     * Expects JSON format matching save():
     * @code
     * {
     *   "ComponentName": {
     *     "param1": value1
     *   }
     * }
     * @endcode
     *
     * @param json Input JSON object
     * @pre json.is_object() == true
     * @pre json.contains(ComponentT::name) == true
     */
    void load(const nlohmann::json& json) {
        assert(json.is_object());
        assert(json.contains(ComponentT::name));
        _params.load(json[udho::manifold::component_name<ComponentT>()]);
    }

    /**
     * @brief Access parameter value (mutable)
     * @tparam ParamT Parameter type
     * @param key Parameter key (use ParamT::val)
     * @return auto& Reference to parameter value
     */
    template <typename ParamT>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _params[key]; }

    /**
     * @brief Access parameter value (const)
     * @tparam ParamT Parameter type
     * @param key Parameter key (use ParamT::val)
     * @return const auto& Const reference to parameter value
     */
    template <typename ParamT>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _params[key]; }

private:
    params_type _params;
};

/**
 * @brief The config specialization provides facilities to con mantain configuration of a component.
 *
 * Extends basic_config with a customizable valid() method for runtime validation.
 * Specialize this template to implement component-specific validation logic.
 *
 * @note the config template can be specialized for a component by providing a different implementation
 *       of the valid function to enforce runtime validation of the configuration.
 * @tparam ComponentT
 * @see basic_config
 * @see configs
 */
template <typename ComponentT>
struct config: basic_config<ComponentT>{
    using base = basic_config<ComponentT>;

    using base::base;

    /**
     * @brief Validate configuration
     *
     * Default implementation always returns true. Specialize to add validation.
     *
     * @return bool True if configuration is valid, false otherwise
     */
    bool valid() const { return true; }
};

#ifndef __DOXYGEN__

template <typename...>
class configs;

template <>
class configs<>{};

template <typename ComponentT>
class configs<ComponentT>{
    using config_type = config<ComponentT>;

    config_type _config;

public:

    configs() = default;

    template <typename... XComponents>
    configs(const configs<XComponents...>& other): _config(other.template get<ComponentT>()) { }

public:

    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const config_type& get() const { return _config; }

public:

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _config[key]; }

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _config[key]; }

public:

    // @{
    void load(const nlohmann::json& json){
        _config.load(json);
    }

    void save(nlohmann::json& json) const {
        _config.save(json);
    }
    // @}
};

template <typename ComponentT, typename... Components>
class configs<ComponentT, Components...>: configs<Components...> {
    using config_type = config<ComponentT>;
    using rest_type   = configs<Components...>;

    config_type _config;

public:

    configs() = default;

    template <typename... XComponents>
    configs(const configs<XComponents...>& other): configs<Components...>(other), _config(other.template get<ComponentT>()) { }

public:

    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const config_type& get() const { return _config; }

    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    config_type& get() { return _config; }

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const auto& get() const { return rest_type::template get<XComponentT>(); }

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    auto& get() { return rest_type::template get<XComponentT>(); }

public:

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _config[key]; }

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _config[key]; }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return rest_type::template operator[]<ParamT>(key); }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return rest_type::template operator[]<ParamT>(key); }

public:

    // @{
    void load(const nlohmann::json& json){
        _config.load(json);
        configs<Components...>::load(json);
    }

    void save(nlohmann::json& json) const {
        _config.save(json);
        configs<Components...>::save(json);
    }
    // @}

};

#else

/**
 * @brief Heterogeneous collection of component configurations
 *
 * Stores configuration for multiple components, providing type-safe access
 * to each component's config. Used by pipelines to pass configuration to
 * all components during evaluation.
 *
 * @tparam Components... Component types to configure
 *
 * # Construction
 *
 * Configs can be default-constructed (all parameters at default values) or
 * copy-constructed from another configs with a **superset of components**:
 * @code
 * // Default construction
 * configs<CompA, CompB, CompC> cfg;
 *
 * // Copy construction (filters to relevant components)
 * configs<CompA, CompB> cfg_subset(cfg);
 * @endcode
 *
 * # Component Access
 *
 * @code
 * configs<CompA, CompB, CompC> cfg;
 *
 * // Get config for specific component
 * auto& cfg_a = cfg.get<CompA>();
 * cfg_a[CompA::param::val] = 100;
 * @endcode
 *
 * # Parameter Access
 *
 * Direct parameter access without specifying component:
 * @code
 * // If param is unique across all components
 * cfg[CompA::timeout::val] = 5000;
 * @endcode
 *
 * # JSON Persistence
 *
 * @code
 * nlohmann::json json = {
 *     {"CompA", {"timeout": 5000}},
 *     {"CompB", {"enabled": true}}
 * };
 *
 * cfg.load(json);   // Load all component configs
 *
 * nlohmann::json out;
 * cfg.save(out);    // Save all component configs
 * @endcode
 *
 * @see config
 * @see params
 */
template <typename... Components>
class configs<Components...> {
public:
    /**
     * @brief get configuration for a component
     * @return
     */
    template <typename XComponentT>
    const config_type& get();

    /**
     * @brief get configuration for a component
     * @return
     */
    template <typename XComponentT>
    const auto& get() const;

    /**
     * @brief operator [] for accessing value of a configuration parameter
     * @param key
     * @return
     */
    template <typename ParamT>
    auto& operator[](const udho::hazo::element_t<ParamT>& key);

    /**
     * @brief operator [] for accessing value of a configuration parameter
     * @param key
     * @return
     */
    template <typename ParamT>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key);
};

#endif // __DOXYGEN__

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_CONFIG_H
