#ifndef UDHO_MANIFOLD_COMPOSITION_VIEW_H
#define UDHO_MANIFOLD_COMPOSITION_VIEW_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/wrapper.h>
#include <udho/manifold/utils.h>

namespace udho{
namespace manifold{

/**
 * @ingroup manifold
 * @{
 */

template <typename ComponentT, typename... Components>
struct composition_view<ComponentT, Components...>: composition_view<Components...>{
    using wrapper_type   = udho::manifold::wrapper<ComponentT>;
    using component_type = ComponentT;
    using features_type  = typename ComponentT::features;

    template <typename ComponentQ>
    using has_component  = std::conditional_t<
        std::is_same_v<ComponentQ, ComponentT>,
        std::true_type,
        typename composition_view<Components...>::template has_component<ComponentQ>
    >;

    template <typename FeatureT, std::size_t Idx>
    using component_at   = std::conditional_t<
        features_type::template has<FeatureT>::value && Idx == 0,
        component_type,
        std::conditional_t<
            features_type::template has<FeatureT>::value && Idx != 0,
            typename composition_view<Components...>::template component_at<FeatureT, Idx - 1>,
            typename composition_view<Components...>::template component_at<FeatureT, Idx>
        >
    >;

    template <typename... OtherComponents>
    composition_view(udho::manifold::composition<OtherComponents...>& other): _wrapper(other.template get<ComponentT>()), composition_view<Components...>(other) {}

    template <typename... OtherComponents>
    composition_view(composition_view<OtherComponents...>& other): _wrapper(other.template get<ComponentT>()), composition_view<Components...>(other) {}

    /// @{
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return _wrapper; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition_view<Components...>::template get<ComponentQ>(); }

    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return _wrapper; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition_view<Components...>::template get<ComponentQ>(); }
    /// @}

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    wrapper_type& at() { return _wrapper; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx != 0, bool> = true>
    auto& at() { return composition_view<Components...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!features_type::template has<FeatureT>::value, bool> = true>
    auto& at() { return composition_view<Components...>::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx == 0, bool> = true>
    const wrapper_type& at() const { return _wrapper; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<features_type::template has<FeatureT>::value && Idx != 0, bool> = true>
    const auto& at() const { return composition_view<Components...>::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!features_type::template has<FeatureT>::value, bool> = true>
    const auto& at() const { return composition_view<Components...>::template at<FeatureT, Idx>(); }
private:
    wrapper_type& _wrapper;
};

template <>
struct composition_view<> {
    template <typename ComponentQ>
    using has_component = std::false_type;

    template <typename FeatureT, std::size_t Idx>
    using component_at = void;

    template <typename... OtherComponents>
    composition_view(udho::manifold::composition<OtherComponents...>& other) {}

    template <typename... OtherComponents>
    composition_view(composition_view<OtherComponents...>& other) {}
};

/**
 * @}
 */

}
}


#endif // UDHO_MANIFOLD_COMPOSITION_VIEW_H
