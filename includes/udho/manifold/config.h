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

    void load(const nlohmann::json& json) {
        assert(json.is_object());
        assert(json.contains(ComponentT::name));
        _params.load(json[udho::manifold::component_name<ComponentT>()]);
    }

    template <typename ParamT>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _params[key]; }

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
 * @brief The config specialization provides facilities to con mantain configuration of a component
 */
template <typename ComponentT = void>
struct config: basic_config<ComponentT>{
    using base = basic_config<ComponentT>;

    using base::base;

    bool valid() const { return true; }
};

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

    template <typename... Params>
    void patch(const changeset<Params...>&) {

    }
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

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const auto& get() const { return rest_type::template get<XComponentT>(); }

public:
    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _config[key]; }

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return _config[key]; }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return rest_type::template operator[]<ParamT>(key); }

    template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const { return rest_type::template operator[]<ParamT>(key); }

    template <typename... Params>
    void patch(const changeset<Params...>& changeset) {

    }
};

}
}

#endif // UDHO_MANIFOLD_CONFIG_H
