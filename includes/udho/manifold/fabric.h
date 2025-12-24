#ifndef UDHO_MANIFOLD_FACET_H
#define UDHO_MANIFOLD_FACET_H

#include <cstdint>
#include <utility>
#include <udho/manifold/fwd.h>
#include <udho/manifold/traits.h>
#include <udho/manifold/config.h>
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
    using facet_type     = FacetT;
    using result_type    = typename udho::manifold::facet_traits<FacetT>::result_type;
    using config_type    = udho::manifold::config<component_type>;

    // static_assert(std::is_constructible_v<facet_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::facet<FacetT> must be constructible with lvalue reference of ComponentT");

    facet_interface_internal(component_type& component, const config_type& config): _facet(component, config) {}

    facet_type& facet() { return _facet; }
    const facet_type& facet() const { return _facet; }

private:
    facet_type _facet;
};

template <typename FacetT>
struct facet_interface_internal<FacetT, false> {
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using facet_type     = FacetT;
    using config_type    = udho::manifold::config<component_type>;

    // static_assert(std::is_constructible_v<facet_type, std::add_lvalue_reference_t<component_type>>, "udho::manifold::facet<FacetT> must be constructible with lvalue reference of ComponentT");

    facet_interface_internal(component_type& component, const config_type& config): _facet(component, config) {}

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

}

template <std::size_t Stage, typename FacetT, bool Enabled=facet_traits<FacetT>::stage == Stage, typename... Rest>
struct basic_fabric;

template <std::size_t Stage, typename FacetT, typename... Rest>
struct basic_fabric<Stage, FacetT, true, Rest...>: private detail::facet_interface<FacetT>, private fabric<Stage, Rest...>{
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using wrapper_type   = detail::facet_interface<FacetT>;
    using self_type      = basic_fabric<Stage, FacetT, true, Rest...>;
    using rest_type      = fabric<Stage, Rest...>;

    template <typename FeatureT, std::uint32_t Idx>
    using facet_type = std::conditional_t<
            std::is_same_v<feature_type, FeatureT> && Idx == 0,
            FacetT,
            typename rest_type::template facet_type<FeatureT, Idx-std::is_same_v<feature_type, FeatureT>>
        >;

    template <typename... Components>
    basic_fabric(composition<Components...>& composition, const configs<Components...>& conf)
        : wrapper_type(composition.template get<component_type>().component(), conf.template get<component_type>()), rest_type(composition, conf) {}

    /// @{
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return rest_type::template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return rest_type::template get<FacetQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return rest_type::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return rest_type::template at<FeatureT, Idx>(); }


    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return rest_type::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    const auto& at() const { return rest_type::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature_type, FeatureT> + rest_type::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}


    /// @{
    // static std::ostream& print(std::ostream& stream) {
    //     stream << "{ Component: " << component_type::name << ", Stage: " << Stage << "}" << " ";
    //     rest_type::print(stream);
    //     return stream;
    // }
    /// @}

};

template <std::size_t Stage, typename FacetT, typename... Rest>
struct basic_fabric<Stage, FacetT, false, Rest...>: public fabric<Stage, Rest...>{
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using wrapper_type   = detail::facet_interface<FacetT>;
    using self_type      = basic_fabric<Stage, FacetT, true, Rest...>;
    using rest_type      = fabric<Stage, Rest...>;

    template <typename FeatureT, std::uint32_t Idx>
    using facet_type = typename rest_type::template facet_type<FeatureT, Idx>;

    template <typename... Components>
    basic_fabric(composition<Components...>& composition, const configs<Components...>& conf): rest_type(composition, conf) {}

    /// @{
    template <typename FacetQ>
    auto& get() { return rest_type::template get<FacetQ>(); }

    template <typename FacetQ>
    const auto& get() const { return rest_type::template get<FacetQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx>
    auto& at() { return rest_type::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx>
    const auto& at() const { return rest_type::template at<FeatureT, Idx>(); }
    /// @}

    /// @{
    template <typename FeatureT>
    static constexpr int count() { return rest_type::template count<FeatureT>(); }
    /// @}

    /// @{
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

    /// @{
    // static std::ostream& print(std::ostream& stream) {
    //     stream << "{ Component: " << component_type::name << ", Stage: " << Stage << "}" << " ";
    //     rest_type::print(stream);
    //     return stream;
    // }
    /// @}
};

template <std::size_t Stage, typename FacetT>
struct basic_fabric<Stage, FacetT, true>: private detail::facet_interface<FacetT>{
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using wrapper_type   = detail::facet_interface<FacetT>;

    template <typename FeatureT, std::uint32_t Idx>
    using facet_type = std::conditional_t<
        std::is_same_v<feature_type, FeatureT> && Idx == 0,
        FacetT,
        void
    >;

    template <typename... Components>
    basic_fabric(composition<Components...>& composition, const configs<Components...>& conf)
        : wrapper_type(composition.template get<component_type>().component(), conf.template get<component_type>()) {}

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
    std::size_t apply(Function&& f) { return utils::visit<fabric<Stage, FacetT>, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<fabric<Stage, FacetT>, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

    /// @{
    // static std::ostream& print(std::ostream& stream) {
    //     stream << "{ Component: " << component_type::name << ", Stage: " << Stage << "}" << " ";
    //     return stream;
    // }
    /// @}
};

template <std::size_t Stage, typename FacetT, typename... Rest>
struct fabric<Stage, FacetT, Rest...>: basic_fabric<Stage, FacetT, facet_traits<FacetT>::stage == Stage, Rest...>{
    using basic_fabric_type = basic_fabric<Stage, FacetT, facet_traits<FacetT>::stage == Stage, Rest...>;

    using basic_fabric_type::basic_fabric_type;
};

template <std::size_t Stage>
struct fabric<Stage>{
    template <typename FeatureT, std::uint32_t Idx>
    using facet_type = void;

    template <typename... Components>
    fabric(composition<Components...>&, const configs<Components...>&) {}

    template <typename FeatureT>
    static constexpr int count() { return 0; }
};

}
}

#endif // UDHO_MANIFOLD_FACET_H
