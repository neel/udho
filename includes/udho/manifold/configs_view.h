#ifndef UDHO_MANIFOLD_CONFIGS_VIEW_H
#define UDHO_MANIFOLD_CONFIGS_VIEW_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

template <typename ComponentT, typename... Components>
struct configs_view<ComponentT, Components...>: configs_view<Components...>{
    using config_type = config<ComponentT>;
    using rest_type   = configs_view<Components...>;

    template <typename... OtherComponents>
    configs_view(configs<OtherComponents...>& other): _config(other.template get<ComponentT>()), configs_view<Components...>(other) { }

    template <typename... OtherComponents>
    configs_view(configs_view<OtherComponents...>& other): _config(other.template get<ComponentT>()), configs_view<Components...>(other) { }

    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const config_type& get() const { return _config; }

    template <typename XComponentT, std::enable_if_t<std::is_same_v<XComponentT, ComponentT>, bool> = true>
    config_type& get() { return _config; }

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    const auto& get() const { return rest_type::template get<XComponentT>(); }

    template <typename XComponentT, std::enable_if_t<!std::is_same_v<XComponentT, ComponentT>, bool> = true>
    auto& get() { return rest_type::template get<XComponentT>(); }

    template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){ return _config[key]; }

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

}
}

#endif // UDHO_MANIFOLD_CONFIGS_VIEW_H
