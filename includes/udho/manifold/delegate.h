#ifndef UDHO_MANIFOLD_DELEGATE_H
#define UDHO_MANIFOLD_DELEGATE_H

#include <cstdint>
#include <utility>
#include <udho/manifold/fwd.h>
#include <udho/manifold/traits.h>
#include <udho/manifold/utils.h>
#include <udho/manifold/features.h>
#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>

namespace udho{
namespace manifold{

namespace detail {

template <typename DelegateT, bool HasState = udho::manifold::has_state<DelegateT>::value>
struct delegate_interface_internal {
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature        = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using delegate_type  = DelegateT;
    using state_type     = typename delegate_traits<DelegateT>::state;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_interface_internal(component_type& component): _delegate(component) {}

    template <typename... Delegates>
    state_type eval(const udho::manifold::states<Delegates...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return _delegate.eval(states, address, request);
    }

    delegate_type& delegate() { return _delegate; }
    const delegate_type& delegate() const { return _delegate; }

private:
    delegate_type _delegate;
};

template <typename DelegateT>
struct delegate_interface_internal<DelegateT, false> {
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature        = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using delegate_type  = DelegateT;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_interface_internal(component_type& component): _delegate(component) {}

    delegate_type& delegate() { return _delegate; }
    const delegate_type& delegate() const { return _delegate; }

private:
    delegate_type _delegate;
};

template <typename DelegateT>
struct delegate_interface: detail::delegate_interface_internal<DelegateT> {
    using internal_interface_type = detail::delegate_interface_internal<DelegateT>;

    using internal_interface_type::internal_interface_type;
};

template <typename DelegateT, bool HasState = udho::manifold::has_state<DelegateT>::value>
struct delegate_wrapper: delegate_interface<DelegateT>{
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature        = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using interface_type = delegate_interface<DelegateT>;
    using config_type    = udho::manifold::config<component_type>;
    using delegate_type  = DelegateT;

    static constexpr const bool has_state = false;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_wrapper(component_type& component): interface_type(component) {}
};

template <typename DelegateT>
struct delegate_wrapper<DelegateT, true>: delegate_interface<DelegateT>{
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature        = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using interface_type = delegate_interface<DelegateT>;
    using state_type     = typename udho::manifold::delegate_traits<DelegateT>::state;
    using config_type    = udho::manifold::config<component_type>;
    using delegate_type  = DelegateT;

    static constexpr const bool has_state = true;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_wrapper(component_type& component): interface_type(component) {}

    template <typename... Delegates, typename... Args>
    bool eval(udho::manifold::states<Delegates...>& states, Args... args) {
        using states_facade_type = udho::manifold::states<Delegates...>;
        state_type state = std::move(interface_type::eval(states, std::forward<Args>(args)...));
        bool result = state.accepted();
        states.template get<delegate_type>() = std::move(state);
        return result;
    }
};

}

template <typename DelegateT, typename... Rest>
struct delegates<DelegateT, Rest...>: private detail::delegate_wrapper<DelegateT>, private delegates<Rest...>{
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature        = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using wrapper_type   = detail::delegate_wrapper<DelegateT>;

    template <typename... Components>
    delegates(composition<Components...>& composition): wrapper_type(composition.template get<component_type>().component()), delegates<Rest...>(composition) {}

    /// @{
    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename DelegateQ, std::enable_if_t<!std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    auto& get() { return delegates<Rest...>::template get<DelegateQ>(); }

    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename DelegateQ, std::enable_if_t<!std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const auto& get() const { return delegates<Rest...>::template get<DelegateQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return delegates<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature, FeatureT>, bool> = true>
    auto& at() { return delegates<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return delegates<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature, FeatureT>, bool> = true>
    const auto& at() const { return delegates<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature, FeatureT> + delegates<Rest...>::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<delegates<DelegateT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<delegates<DelegateT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

};

template <typename DelegateT>
struct delegates<DelegateT>: private detail::delegate_wrapper<DelegateT>{
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature        = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using wrapper_type   = detail::delegate_wrapper<DelegateT>;

    template <typename... Components>
    delegates(composition<Components...>& composition): wrapper_type(composition.template get<component_type>().component()) {}

    /// @{
    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const wrapper_type& get() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature, FeatureT>; }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<delegates<DelegateT>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<delegates<DelegateT>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}
};

}
}

#endif // UDHO_MANIFOLD_DELEGATE_H
