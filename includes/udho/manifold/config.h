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
#include <udho/manifold/patch.h>

namespace udho{
namespace manifold{

template <typename ComponentT = void>
struct basic_config{
    using components_type = ComponentT;
    using params_type = typename udho::manifold::component_traits<ComponentT>::params;

    /**
     * @brief checks if the configuration contains the parameter
     */
    template <typename ParamT>
    using contains = typename params_type::template contains<ParamT>;

    basic_config() = default;

    basic_config(const basic_config<ComponentT>& other): _params(other._params) {}

    /**
     * @brief save
     * @param json
     * @pre expects the provided json is an object
     */
    void save(nlohmann::json& json) const {
        auto params = nlohmann::json::object();
        _params.save(params);
        assert(json.is_object());
        json[udho::manifold::component_name<ComponentT>()]= params;
    }

    /**
     * @brief load
     * @param json
     */
    void load(const nlohmann::json& json) {
        assert(json.is_object());
        assert(json.contains(ComponentT::name));
        _params.load(json[udho::manifold::component_name<ComponentT>()]);
    }

    /**
     * @brief operator [] for accessing the value of a parameter contained in the component configuration
     * @param key
     * @return
     */
    template <typename ParamT>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _params[key]; }

    /**
     * @brief operator [] for accessing the value of a parameter contained in the component configuration
     * @param key
     * @return
     */
    template <typename ParamT>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _params[key]; }

private:
    params_type _params;
};

// template <>
// struct basic_config<void>{
//     using components_type = void;
//     using params_type = params<>;

//     template <typename>
//     using contains = std::false_type;
// };


/**
 * @brief The config specialization provides facilities to con mantain configuration of a component.
 * @note the config template can be specialized for a component by providing a different implementation
 *       of the valid function to enforce runtime validation of the configuration.
 * @tparam ComponentT
 */
template <typename ComponentT = void>
struct config: basic_config<ComponentT>{
    using base = basic_config<ComponentT>;

    using base::base;

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
 * @brief The configs template encapsulates configurations of a set of components
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

}
}

#endif // UDHO_MANIFOLD_CONFIG_H
