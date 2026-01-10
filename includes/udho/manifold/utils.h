#ifndef UDHO_MANIFOLD_UTILS_H
#define UDHO_MANIFOLD_UTILS_H

#include <utility>
#include <cstdint>
#include <type_traits>

namespace udho{
namespace manifold {

/**
 * @addtogroup manifold
 * @{
 */

namespace utils {

namespace detail {
template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() == Idx , bool> = true>
std::size_t visit(FacadeT& facade, Function&& function) { return 0; }

template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() != Idx , bool> = true>
std::size_t visit(FacadeT& facade, Function&& function) {
    auto& wrapper = facade.template at<FeatureT, Idx>();
    bool result = function(wrapper);
    std::size_t count = result;
    if(!wrapper.has_result || result) {
        count += visit<FacadeT, FeatureT, Function, Idx+1>(facade, std::forward<Function>(function));
    }
    return count;
}



template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() == Idx , bool> = true>
std::size_t visit(const FacadeT& facade, Function&& function) { return 0; }

template <typename FacadeT, typename FeatureT, typename Function, std::uint32_t Idx, std::enable_if_t< FacadeT::template count<FeatureT>() != Idx , bool> = true>
std::size_t visit(const FacadeT& facade, Function&& function) {
    auto& wrapper = facade.template at<FeatureT, Idx>();
    bool result = function(wrapper);
    std::size_t count = result;
    if(!wrapper.has_result || result) {
        count += visit<FacadeT, FeatureT, Function, Idx+1>(facade, std::forward<Function>(function));
    }
    return count;
}
}

/**
 * @brief visits a facade and applies the given function on all elements providing the requested feature
 * @tparam FacadeT type of the facade, could be a composition, journal, or facets
 * @tparam FeatureT target feature
 * @param facade
 * @param function
 * @note expects the FacadeT to provide at<FeatureT, Idx> method
 * @return
 */
template <typename FacadeT, typename FeatureT, typename Function>
std::size_t visit(FacadeT& facade, Function&& function) {
    return detail::visit<FacadeT, FeatureT, Function, 0>(facade, std::forward<Function>(function));
}

/**
 * @brief visits a facade and applies the given function on all elements providing the requested feature
 * @tparam FacadeT type of the facade, could be a composition, journal, or facets
 * @tparam FeatureT target feature
 * @param facade
 * @param function
 * @note expects the FacadeT to provide at<FeatureT, Idx> method
 * @return
 */
template <typename FacadeT, typename FeatureT, typename Function>
std::size_t visit(const FacadeT& facade, Function&& function) {
    return detail::visit<FacadeT, FeatureT, Function, 0>(facade, std::forward<Function>(function));
}

}

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_UTILS_H
