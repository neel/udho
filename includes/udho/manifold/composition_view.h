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
/**
 * @brief Non-owning typed view over a subset of a composition.
 *
 * `composition_view` exposes the same type-based and feature-based access pattern
 * as `composition`, but stores references to wrappers already owned by another
 * composition or referenced by another composition view.
 *
 * This is useful when an API needs access only to a selected subset of components
 * while keeping the original component storage and ownership unchanged.
 *
 * @tparam ComponentT First component type exposed by this view.
 * @tparam Components Remaining component types exposed by this view.
 *
 * @see composition
 * @see wrapper
 */
template <typename ComponentT, typename... Components>
struct composition_view<ComponentT, Components...>: composition_view<Components...>{
    /**
     * @brief Wrapper type used to expose `ComponentT`.
     */
    using wrapper_type   = udho::manifold::wrapper<ComponentT>;
    /**
     * @brief Component type represented by the current node of the recursive view.
     */
    using component_type = ComponentT;
    /**
     * @brief Feature list declared by `ComponentT`.
     */
    using features_type  = typename ComponentT::features;

    /**
     * @brief Checks whether this view contains a component type.
     *
     * Resolves to `std::true_type` when `ComponentQ` is one of the component types
     * exposed by the view, otherwise resolves to `std::false_type`.
     *
     * @tparam ComponentQ Component type to query.
     */
    template <typename ComponentQ>
    using has_component  = std::conditional_t<
        std::is_same_v<ComponentQ, ComponentT>,
        std::true_type,
        typename composition_view<Components...>::template has_component<ComponentQ>
    >;

    /**
     * @brief Resolves the component type at a feature occurrence index.
     *
     * The index is counted only among components that provide `FeatureT`, in the
     * order in which components appear in the view.
     *
     * @tparam FeatureT Feature type to search for.
     * @tparam Idx Zero-based index among components providing `FeatureT`.
     *
     * @note Resolves to `void` in the empty-view terminator when no matching
     *       component exists.
     */
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

    /**
     * @brief Constructs a view from a composition.
     *
     * Each wrapper referenced by this view is obtained from `other` by component
     * type. The source composition must contain every component type requested by
     * this view.
     *
     * @tparam OtherComponents Component types present in the source composition.
     * @param other Source composition whose wrappers are referenced.
     *
     * @warning The source composition must outlive this view.
     */
    template <typename... OtherComponents>
    composition_view(udho::manifold::composition<OtherComponents...>& other): _wrapper(other.template get<ComponentT>()), composition_view<Components...>(other) {}

    /**
     * @brief Constructs a view from another composition view.
     *
     * This creates a narrower or compatible view by referencing wrappers already
     * exposed by `other`.
     *
     * @tparam OtherComponents Component types present in the source view.
     * @param other Source composition view.
     *
     * @warning The objects referenced by `other` must outlive this view.
     */
    template <typename... OtherComponents>
    composition_view(composition_view<OtherComponents...>& other): _wrapper(other.template get<ComponentT>()), composition_view<Components...>(other) {}

    /// @{
    /**
     * @brief Gets the wrapper for a component type.
     * @tparam ComponentQ Component type to retrieve.
     * @return Reference to the wrapper for `ComponentQ`.
     */
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    wrapper_type& get() { return _wrapper; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    auto& get() { return composition_view<Components...>::template get<ComponentQ>(); }

    /**
     * @brief Gets the const wrapper for a component type.
     * @tparam ComponentQ Component type to retrieve.
     * @return Const reference to the wrapper for `ComponentQ`.
     */
    template <typename ComponentQ, std::enable_if_t<std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const wrapper_type& get() const { return _wrapper; }

    template <typename ComponentQ, std::enable_if_t<!std::is_same_v<ComponentQ, ComponentT>, bool> = true>
    const auto& get() const { return composition_view<Components...>::template get<ComponentQ>(); }
    /// @}

    /**
     * @brief Gets the wrapper for the Idx'th occurrence of a feature.
     *
     * @tparam FeatureT Feature type to search for.
     * @tparam Idx Zero-based occurrence index among components providing `FeatureT`.
     * @return Reference to the wrapper for the selected component.
     */
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
