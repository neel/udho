#ifndef UDHO_MANIFOLD_JOURNAL_VIEW_H
#define UDHO_MANIFOLD_JOURNAL_VIEW_H

#include <stdlib.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/journal.h>

namespace udho{
namespace manifold{

/**
 * @ingroup manifold
 * @{
 */

template <typename FacetT, typename... Rest>
struct journal_const_view<FacetT, Rest...>{
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using container_type = detail::result_container<FacetT>;

    template <typename... OtherFacets>
    journal_const_view(const udho::manifold::journal<OtherFacets...>& other): _result(other.template get_wrapper<FacetT>()), _rest(other) {}

    template <typename... OtherComponents>
    journal_const_view(const journal_const_view<OtherComponents...>& other): _result(other.template get_wrapper<FacetT>()), _rest(other) {}

    /// @{
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return _result.template get<FacetQ>(); }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return _rest.template get<FacetQ>(); }
    /// @}

    /// @{
    template <typename FacetQ, std::enable_if_t<!container_type::skipped && std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return _result; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get_wrapper() const { return _rest.template get_wrapper<FacetQ>(); }
    /// @}

    /// @{
    template <typename FeatureT, std::uint32_t Idx = 0, std::enable_if_t<!container_type::skipped && std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    auto& at() { return _result.template at<FeatureT, Idx>(); }

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
