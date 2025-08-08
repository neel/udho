#ifndef UDHO_MANIFOLD_FACET_H
#define UDHO_MANIFOLD_FACET_H

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

template <typename FacetT, bool HasResult = udho::manifold::has_result<FacetT>::value>
struct facet_interface_internal {
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using facet_type  = FacetT;
    using result_type     = typename udho::manifold::facet_traits<FacetT>::result_type;

    static_assert(std::is_constructible_v<facet_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::facet<FacetT> must be constructible with lvalue reference of ComponentT");

    facet_interface_internal(component_type& component): _facet(component) {}

    template <typename... Facets>
    result_type eval(const udho::manifold::journal<Facets...>& journal, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) {
        return _facet.eval(journal, address, request);
    }

    facet_type& facet() { return _facet; }
    const facet_type& facet() const { return _facet; }

private:
    facet_type _facet;
};

template <typename FacetT>
struct facet_interface_internal<FacetT, false> {
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using facet_type  = FacetT;

    static_assert(std::is_constructible_v<facet_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::facet<FacetT> must be constructible with lvalue reference of ComponentT");

    facet_interface_internal(component_type& component): _facet(component) {}

    facet_type& facet() { return _facet; }
    const facet_type& facet() const { return _facet; }

private:
    facet_type _facet;
};

template <typename FacetT>
struct facet_interface: detail::facet_interface_internal<FacetT> {
    using internal_interface_type = detail::facet_interface_internal<FacetT>;

    using internal_interface_type::internal_interface_type;
};

template <typename FacetT, bool HasResult = udho::manifold::has_result<FacetT>::value>
struct facet_wrapper: facet_interface<FacetT>{
    using facet_type  = FacetT;
    using interface_type = facet_interface<FacetT>;
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using config_type    = udho::manifold::config<component_type>;

    static constexpr const bool has_result = false;

    static_assert(std::is_constructible_v<facet_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::facet<FacetT> must be constructible with lvalue reference of ComponentT");

    facet_wrapper(component_type& component): interface_type(component) {}
};

template <typename FacetT>
struct facet_wrapper<FacetT, true>: facet_interface<FacetT>{
    using facet_type  = FacetT;
    using interface_type = facet_interface<FacetT>;
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using config_type    = udho::manifold::config<component_type>;
    using result_type    = typename udho::manifold::facet_traits<FacetT>::result_type;

    static constexpr const bool has_result = true;

    static_assert(std::is_constructible_v<facet_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::facet<FacetT> must be constructible with lvalue reference of ComponentT");

    facet_wrapper(component_type& component): interface_type(component) {}

    template <typename... Facets, typename... Args>
    bool eval(udho::manifold::journal<Facets...>& journal, Args... args) {
        using journal_facade_type = udho::manifold::journal<Facets...>;
        result_type result = std::move(interface_type::eval(journal, std::forward<Args>(args)...));
        bool accepted = result.accepted();
        journal.template get<facet_type>() = std::move(result);
        return accepted;
    }
};

}

template <typename FacetT, typename... Rest>
struct mediator<FacetT, Rest...>: private detail::facet_wrapper<FacetT>, private mediator<Rest...>{
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using wrapper_type   = detail::facet_wrapper<FacetT>;

    template <typename... Components>
    mediator(composition<Components...>& composition): wrapper_type(composition.template get<component_type>().component()), mediator<Rest...>(composition) {}

    /// @{
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return mediator<Rest...>::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return mediator<Rest...>::template get<FacetQ>(); }
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
    std::size_t apply(Function&& f) { return utils::visit<mediator<FacetT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<mediator<FacetT, Rest...>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

};

template <typename FacetT>
struct mediator<FacetT>: private detail::facet_wrapper<FacetT>{
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using wrapper_type   = detail::facet_wrapper<FacetT>;

    template <typename... Components>
    mediator(composition<Components...>& composition): wrapper_type(composition.template get<component_type>().component()) {}

    /// @{
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
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
    std::size_t apply(Function&& f) { return utils::visit<mediator<FacetT>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<mediator<FacetT>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}
};

}
}

#endif // UDHO_MANIFOLD_FACET_H
