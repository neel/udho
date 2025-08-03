#ifndef UDHO_MANIFOLD_DELEGATE_H
#define UDHO_MANIFOLD_DELEGATE_H

#include <cstdint>
#include <utility>
#include <udho/manifold/fwd.h>
#include <udho/manifold/traits.h>
#include <udho/manifold/utils.h>
#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>

namespace udho{
namespace manifold{

namespace detail {

template <typename ComponentT, typename FeatureT, bool HasState = udho::manifold::has_state<ComponentT>::value>
struct delegate_interface_internal {
    using component_type    = ComponentT;
    using feature           = FeatureT;
    using delegate_type     = udho::manifold::delegate<ComponentT>;
    using state_type        = typename component_traits<ComponentT>::state;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<ComponentT>>, "udho::manifold::delegate<ComponentT> must be constructible with lvalue reference of ComponentT");

    delegate_interface_internal(component_type& component): _delegate(component) {}

    template <typename... Components>
    state_type eval(const udho::manifold::states<Components...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return _delegate.eval(states, address, request);
    }

    delegate_type& delegate() { return _delegate; }
    const delegate_type& delegate() const { return _delegate; }

private:
    delegate_type _delegate;
};

template <typename ComponentT, typename FeatureT>
struct delegate_interface_internal<ComponentT, FeatureT, false> {
    using component_type    = ComponentT;
    using feature           = FeatureT;
    using delegate_type     = udho::manifold::delegate<ComponentT>;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<ComponentT>>, "udho::manifold::delegate<ComponentT> must be constructible with lvalue reference of ComponentT");

    delegate_interface_internal(component_type& component): _delegate(component) {}

    delegate_type& delegate() { return _delegate; }
    const delegate_type& delegate() const { return _delegate; }

private:
    delegate_type _delegate;
};

}

template <typename ComponentT, typename FeatureT>
struct delegate_interface: detail::delegate_interface_internal<ComponentT, FeatureT> {
    using internal_interface_type = detail::delegate_interface_internal<ComponentT, FeatureT>;

    using internal_interface_type::internal_interface_type;
};

template <typename ComponentT, bool HasState = udho::manifold::has_state<ComponentT>::value>
struct delegate_wrapper: delegate_interface<ComponentT, typename ComponentT::feature>{
    using component_type = ComponentT;
    using feature        = typename ComponentT::feature;
    using interface_type = delegate_interface<ComponentT, feature>;
    using delegate_type  = udho::manifold::delegate<ComponentT>;
    using config_type    = udho::manifold::config<ComponentT>;

    static constexpr const bool has_state = false;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<ComponentT>>, "udho::manifold::delegate<ComponentT> must be constructible with lvalue reference of ComponentT");

    delegate_wrapper(component_type& component): interface_type(component) {}
};

template <typename ComponentT>
struct delegate_wrapper<ComponentT, true>: delegate_interface<ComponentT, typename ComponentT::feature>{
    using component_type = ComponentT;
    using feature        = typename ComponentT::feature;
    using interface_type = delegate_interface<ComponentT, feature>;
    using state_type     = typename component_traits<ComponentT>::state;
    using config_type    = udho::manifold::config<ComponentT>;
    using delegate_type  = udho::manifold::delegate<ComponentT>;

    static constexpr const bool has_state = true;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<ComponentT>>, "udho::manifold::delegate<ComponentT> must be constructible with lvalue reference of ComponentT");

    delegate_wrapper(component_type& component): interface_type(component) {}

    template <typename... Components, typename... Args>
    bool eval(udho::manifold::states<Components...>& states, Args... args) {
        using states_facade_type = udho::manifold::states<Components...>;
        state_type state = std::move(interface_type::eval(states, std::forward<Args>(args)...));
        bool result = state.accepted();
        states.template get<ComponentT>() = std::move(state);
        return result;
    }
};

template <typename ComponentT, typename... Rest>
struct delegates<ComponentT, Rest...>: private delegate_wrapper<ComponentT>, private delegates<Rest...>{
    using component_type = ComponentT;
    using wrapper_type   = delegate_wrapper<ComponentT>;

    delegates(composition<ComponentT, Rest...>& composition): wrapper_type(composition.template get<ComponentT>().component()), delegates<Rest...>(composition.tail()) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return delegates<Rest...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return delegates<Rest...>::template get<ComponentQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return delegates<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    auto& at() { return delegates<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return delegates<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<typename component_type::feature, FeatureT>, bool> = true>
    const auto& at() const { return delegates<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT> + delegates<Rest...>::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<delegates<ComponentT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<delegates<ComponentT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

};

template <typename ComponentT>
struct delegates<ComponentT>: private delegate_wrapper<ComponentT>{
    using component_type = ComponentT;
    using wrapper_type   = delegate_wrapper<ComponentT>;

    delegates(composition<ComponentT>& composition): wrapper_type(composition.template get<ComponentT>().component()) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<typename component_type::feature, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<typename component_type::feature, FeatureT>; }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<delegates<ComponentT>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<delegates<ComponentT>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}
};

}
}

#endif // UDHO_MANIFOLD_DELEGATE_H
