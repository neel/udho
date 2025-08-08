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

template <typename DelegateT, bool HasResult = udho::manifold::has_result<DelegateT>::value>
struct delegate_interface_internal {
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using delegate_type  = DelegateT;
    using result_type     = typename udho::manifold::delegate_traits<DelegateT>::result_type;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_interface_internal(component_type& component): _delegate(component) {}

    template <typename... Delegates>
    result_type eval(const udho::manifold::states<Delegates...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
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
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
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

template <typename DelegateT, bool HasResult = udho::manifold::has_result<DelegateT>::value>
struct delegate_wrapper: delegate_interface<DelegateT>{
    using delegate_type  = DelegateT;
    using interface_type = delegate_interface<DelegateT>;
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using config_type    = udho::manifold::config<component_type>;

    static constexpr const bool has_result = false;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_wrapper(component_type& component): interface_type(component) {}
};

template <typename DelegateT>
struct delegate_wrapper<DelegateT, true>: delegate_interface<DelegateT>{
    using delegate_type  = DelegateT;
    using interface_type = delegate_interface<DelegateT>;
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using config_type    = udho::manifold::config<component_type>;
    using result_type    = typename udho::manifold::delegate_traits<DelegateT>::result_type;

    static constexpr const bool has_result = true;

    static_assert(std::is_constructible_v<delegate_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::delegate<DelegateT> must be constructible with lvalue reference of ComponentT");

    delegate_wrapper(component_type& component): interface_type(component) {}

    template <typename... Delegates, typename... Args>
    bool eval(udho::manifold::states<Delegates...>& states, Args... args) {
        using states_facade_type = udho::manifold::states<Delegates...>;
        result_type result = std::move(interface_type::eval(states, std::forward<Args>(args)...));
        bool accepted = result.accepted();
        states.template get<delegate_type>() = std::move(result);
        return accepted;
    }
};

}

template <typename DelegateT, typename... Rest>
struct mediator<DelegateT, Rest...>: private detail::delegate_wrapper<DelegateT>, private mediator<Rest...>{
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using wrapper_type   = detail::delegate_wrapper<DelegateT>;

    template <typename... Components>
    mediator(composition<Components...>& composition): wrapper_type(composition.template get<component_type>().component()), mediator<Rest...>(composition) {}

    /// @{
    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename DelegateQ, std::enable_if_t<!std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    auto& get() { return mediator<Rest...>::template get<DelegateQ>(); }

    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename DelegateQ, std::enable_if_t<!std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const auto& get() const { return mediator<Rest...>::template get<DelegateQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return mediator<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return mediator<Rest...>::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return mediator<Rest...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    const auto& at() const { return mediator<Rest...>::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature_type, FeatureT> + mediator<Rest...>::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<mediator<DelegateT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<mediator<DelegateT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

};

template <typename DelegateT>
struct mediator<DelegateT>: private detail::delegate_wrapper<DelegateT>{
    using component_type = typename udho::manifold::delegate_traits<DelegateT>::component_type;
    using feature_type   = typename udho::manifold::delegate_traits<DelegateT>::feature_type;
    using wrapper_type   = detail::delegate_wrapper<DelegateT>;

    template <typename... Components>
    mediator(composition<Components...>& composition): wrapper_type(composition.template get<component_type>().component()) {}

    /// @{
    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename DelegateQ, std::enable_if_t<std::is_same_v<DelegateQ, DelegateT>, bool> = true>
    const wrapper_type& get() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature_type, FeatureT>; }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<mediator<DelegateT>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<mediator<DelegateT>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}
};

}
}

#endif // UDHO_MANIFOLD_DELEGATE_H
