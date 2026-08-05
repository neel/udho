#ifndef UDHO_MANIFOLD_JOURNAL_VIEW_H
#define UDHO_MANIFOLD_JOURNAL_VIEW_H

#include <stdlib.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/journal.h>

namespace udho{
namespace manifold{

/**
 * @ingroup DoxyG_manifold
 * @{
 */

/**
 * @brief Const, non-owning typed view over selected journal results.
 *
 * `journal_const_view` exposes read-only access to result containers stored in
 * a `journal` or in another journal view. It is typically passed to accessors
 * and portals so user code can inspect already evaluated facet results without
 * mutating the owning journal.
 *
 * @tparam FacetT First facet type exposed by this view.
 * @tparam Rest Remaining facet types exposed by this view.
 *
 * @see journal
 * @see result_wrapper
 * @see detail::result_container
 */
template <typename FacetT, typename... Rest>
struct journal_const_view<FacetT, Rest...>{
    /**
     * @brief Feature type evaluated by the current facet.
     */
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    /**
     * @brief Result container type for the current facet.
     */
    using container_type = detail::result_container<FacetT>;

    /**
     * @brief Constructs a const journal view from a journal.
     *
     * The source journal must contain result containers for every facet type
     * requested by this view.
     *
     * @tparam OtherFacets Facet types present in the source journal.
     * @param other Source journal.
     *
     * @warning The source journal must outlive this view.
     */
    template <typename... OtherFacets>
    journal_const_view(const udho::manifold::journal<OtherFacets...>& other): _result(other.template get_wrapper<FacetT>()), _rest(other) {}

    /**
     * @brief Constructs a const journal view from another const journal view.
     *
     * @tparam OtherComponents Facet types present in the source view.
     * @param other Source journal view.
     *
     * @warning The objects referenced by `other` must outlive this view.
     */
    template <typename... OtherComponents>
    journal_const_view(const journal_const_view<OtherComponents...>& other): _result(other.template get_wrapper<FacetT>()), _rest(other) {}

    /// @{
    /**
     * @brief Gets the result wrapper for a facet.
     *
     * @tparam FacetQ Facet type to retrieve.
     * @return Const reference to the result wrapper for `FacetQ`.
     */
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return _result.template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return _rest.template get<FacetQ>(); }
    /// @}

    /// @{
    /**
     * @brief Gets the result container for a facet.
     *
     * @tparam FacetQ Facet type to retrieve.
     * @return Const reference to the result container for `FacetQ`.
     */
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return _result; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return _rest.template get_wrapper<FacetQ>(); }
    /// @}

    /// @{
    /**
     * @brief Gets the Idx'th result wrapper for a feature.
     * @tparam FeatureT Feature type to retrieve.
     * @tparam Idx Zero-based index among result-producing facets for `FeatureT`.
     * @return Reference to the selected result wrapper.
     */
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    auto& at() { return _result.template at<FeatureT, Idx>(); }

    /**
     * @brief Gets the Idx'th const result wrapper for a feature.
     * @tparam FeatureT Feature type to retrieve.
     * @tparam Idx Zero-based index among result-producing facets for `FeatureT`.
     * @return Reference to the selected result wrapper.
     */
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const auto& at() const { return _result.template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return _rest.template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return _rest.template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return _rest.template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    const auto& at() const { return _rest.template at<FeatureT, Idx>(); }
    /// @}

private:
    const typename detail::result_container<FacetT>& _result;
    journal_const_view<Rest...> _rest;
};

template <>
struct journal_const_view<> {
    template <typename... OtherComponents>
    journal_const_view(const udho::manifold::journal<OtherComponents...>& other) {}

    template <typename... OtherComponents>
    journal_const_view(const journal_const_view<OtherComponents...>& other) {}
};


/**
 * @}
 */

}
}

#endif // UDHO_MANIFOLD_JOURNAL_VIEW_H
