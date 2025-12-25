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

/**
 * @addtogroup manifold
 * @{
 */

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

/**
 * @brief Core fabric implementation for organizing and accessing stage-specific facets
 *
 * The basic_fabric template organizes facets for a specific pipeline stage, providing
 * type-safe access, counting, and visitation capabilities. It filters facets at compile-time
 * based on their stage number, including only those where `facet_traits<FacetT>::stage == Stage`.
 *
 * @tparam Stage The pipeline stage number this fabric represents (0, 1, 2, etc.)
 * @tparam FacetT The facet type being considered for inclusion
 * @tparam Enabled Whether this facet belongs to the current stage (compile-time boolean)
 * @tparam Rest Remaining facet types to process
 *
 * @note This is an internal implementation class; users should use the `fabric` alias
 *
 * @see fabric
 * @see facet_interface
 */
template <std::size_t Stage, typename FacetT, bool Enabled=facet_traits<FacetT>::stage == Stage, typename... Rest>
struct basic_fabric;

/**
 * @brief Specialization for enabled facets with remaining facets to process
 *
 * Includes the current facet (since Enabled=true) and recursively processes remaining facets.
 * Provides complete access methods for facet lookup by type, feature, and index.
 *
 * @tparam Stage Current pipeline stage
 * @tparam FacetT Current facet type (belongs to this stage)
 * @tparam Rest Remaining facet types
 */
template <std::size_t Stage, typename FacetT, typename... Rest>
struct basic_fabric<Stage, FacetT, true, Rest...>: private detail::facet_interface<FacetT>, private fabric<Stage, Rest...>{
    using component_type = typename udho::manifold::facet_traits<FacetT>::component_type;
    using feature_type   = typename udho::manifold::facet_traits<FacetT>::feature_type;
    using wrapper_type   = detail::facet_interface<FacetT>;
    using self_type      = basic_fabric<Stage, FacetT, true, Rest...>;
    using rest_type      = fabric<Stage, Rest...>;

    /**
     * @brief Type alias for the Idx-th facet providing FeatureT
     *
     * Compile-time lookup that recursively searches through facets to find the
     * Idx-th facet offering the requested feature.
     *
     * @tparam FeatureT The feature type to search for
     * @tparam Idx Zero-based index among facets with this feature
     */
    template <typename FeatureT, std::uint32_t Idx>
    using facet_type = std::conditional_t<
            std::is_same_v<feature_type, FeatureT> && Idx == 0,
            FacetT,
            typename rest_type::template facet_type<FeatureT, Idx-std::is_same_v<feature_type, FeatureT>>
        >;

    /**
     * @brief Constructs a fabric from composition and configuration
     *
     * Initializes all facet wrappers by extracting components from the composition
     * and their corresponding configurations from the configs.
     *
     * @tparam Components... Types of components in the composition
     * @param composition The composition containing component instances
     * @param conf Configuration for all components
     */
    template <typename... Components>
    basic_fabric(composition<Components...>& composition, const configs<Components...>& conf)
        : wrapper_type(composition.template get<component_type>().component(), conf.template get<component_type>()), rest_type(composition, conf) {}

    /// @name Facet Access by Type
    /// @{

    /**
     * @brief Get wrapper for a specific facet type
     *
     * Direct access to the wrapper containing the requested facet.
     * Compile-time resolution through recursive template matching.
     *
     * @tparam FacetQ The facet type to retrieve
     * @return wrapper_type& Reference to wrapper for FacetQ
     *
     * @pre FacetQ must be in the fabric (compile-time check)
     */
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    wrapper_type& get() { return *this; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    auto& get() { return rest_type::template get<FacetQ>(); }

    /**
     * @brief Get wrapper for a facet type (const version)
     */
    template <typename FacetQ, std::enable_if_t<std::is_same_v<FacetQ, FacetT>, bool> = true>
    const wrapper_type& get() const { return *this; }

    template <typename FacetQ, std::enable_if_t<!std::is_same_v<FacetQ, FacetT>, bool> = true>
    const auto& get() const { return rest_type::template get<FacetQ>(); }
    /// @}

    /// @name Feature-Based Access
    /// @{

    /**
     * @brief Access the Idx-th facet providing a feature
     *
     * Returns the wrapper for the Idx-th facet that offers FeatureT.
     * Facets are indexed in the order they appear in the template parameter list.
     *
     * @tparam FeatureT The feature type to search for
     * @tparam Idx Zero-based index among facets with this feature
     * @return wrapper_type& Reference to wrapper for the Idx-th facet with FeatureT
     *
     * @pre Idx must be less than count<FeatureT>() (compile-time check)
     */
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    wrapper_type& at() { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    auto& at() { return rest_type::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    auto& at() { return rest_type::template at<FeatureT, Idx>(); }

    /**
     * @brief Access the Idx-th facet providing a feature (const version)
     */
    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx == 0, bool> = true>
    const wrapper_type& at() const { return *this; }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<std::is_same_v<feature_type, FeatureT> && Idx != 0, bool> = true>
    const auto& at() const { return rest_type::template at<FeatureT, Idx-1>(); }

    template <typename FeatureT, std::uint32_t Idx, std::enable_if_t<!std::is_same_v<feature_type, FeatureT>, bool> = true>
    const auto& at() const { return rest_type::template at<FeatureT, Idx>(); }
    /// @}

    /// @name Feature Queries
    /// @{

    /**
     * @brief Count how many facets provide a specific feature
     *
     * Compile-time calculation of the number of facets in this fabric
     * that offer the given feature.
     *
     * @tparam FeatureT The feature type to count
     * @return int Number of facets providing FeatureT
     */
    template <typename FeatureT>
    static constexpr int count() { return std::is_same_v<feature_type, FeatureT> + rest_type::template count<FeatureT>(); }
    /// @}

    /// @name Facet Visitation
    /// @{

    /**
     * @brief Apply a function to all facets providing a specific feature
     *
     * Visits each facet in the fabric that offers FeatureT and applies
     * the given function. The function receives a reference to the facet wrapper.
     *
     * @tparam FeatureT The feature type to visit
     * @tparam Function Type of the function to apply (must be callable with wrapper_type&)
     * @param f Function to apply to each matching facet
     * @return std::size_t Number of facets visited
     *
     * @note The function should return a boolean indicating whether to continue
     *       visiting remaining facets (true=continue, false=stop)
     */
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }

    /**
     * @brief Apply a function to all facets providing a specific feature (const version)
     */
    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}

};

/**
 * @brief Specialization for disabled facets with remaining facets to process
 *
 * Excludes the current facet (since Enabled=false) and recursively processes
 * remaining facets. All access methods delegate to the rest of the fabric.
 *
 * @tparam Stage Current pipeline stage
 * @tparam FacetT Current facet type (does NOT belong to this stage)
 * @tparam Rest Remaining facet types
 */
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

    /// @name Delegated Access Methods
    /// All methods delegate to the rest of the fabric since this facet is excluded
    /// @{
    template <typename FacetQ>
    auto& get() { return rest_type::template get<FacetQ>(); }

    template <typename FacetQ>
    const auto& get() const { return rest_type::template get<FacetQ>(); }

    template <typename FeatureT, std::uint32_t Idx>
    auto& at() { return rest_type::template at<FeatureT, Idx>(); }

    template <typename FeatureT, std::uint32_t Idx>
    const auto& at() const { return rest_type::template at<FeatureT, Idx>(); }

    template <typename FeatureT>
    static constexpr int count() { return rest_type::template count<FeatureT>(); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }

    template <typename FeatureT, typename Function>
    std::size_t apply(Function&& f) const { return utils::visit<self_type, FeatureT, Function>(*this, std::forward<Function>(f)); }
    /// @}
};

/**
 * @brief Specialization for single enabled facet (terminal case)
 *
 * Base case for recursion when only one facet remains and it belongs to the current stage.
 *
 * @tparam Stage Current pipeline stage
 * @tparam FacetT The facet type (belongs to this stage)
 */
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
};

/**
 * @brief Public interface for stage-specific fabric of facets
 *
 * The fabric class organizes and provides access to all facets belonging to a specific
 * stage. It inherits from basic_fabric.
 *
 * A fabric is essentially a compile-time filtered view of facets for a given stage,
 * providing:
 * - Type-safe facet access by type or feature
 * - Compile-time counting of facets by feature
 * - Facet visitation for bulk operations
 *
 * @tparam Stage The pipeline stage number (0, 1, 2, ...)
 * @tparam FacetT First facet type to consider
 * @tparam Rest Remaining facet types
 *
 * @code
 * // Given facets with stages:
 * // facet<C0, F0> where F0::stage = 0
 * // facet<C1, F1> where F1::stage = 1
 * // facet<C2, F2> where F2::stage = 0
 *
 * using fabric_0 = fabric<0, facet<C0, F0>, facet<C1, F1>, facet<C2, F2>>;
 * // Contains: facet<C0, F0>, facet<C2, F2> (stage 0 facets)
 *
 * using fabric_1 = fabric<1, facet<C0, F0>, facet<C1, F1>, facet<C2, F2>>;
 * // Contains: facet<C1, F1> (stage 1 facet)
 * @endcode
 */
template <std::size_t Stage, typename FacetT, typename... Rest>
struct fabric<Stage, FacetT, Rest...>: basic_fabric<Stage, FacetT, facet_traits<FacetT>::stage == Stage, Rest...>{
    using basic_fabric_type = basic_fabric<Stage, FacetT, facet_traits<FacetT>::stage == Stage, Rest...>;

    using basic_fabric_type::basic_fabric_type;
};

/**
 * @brief Specialization for empty fabric
 *
 * Represents a fabric with no facets for the given stage.
 * Provides null implementations for all access methods.
 *
 * @tparam Stage The pipeline stage number
 */
template <std::size_t Stage>
struct fabric<Stage>{

    /**
     * @brief Type alias indicating no facet exists for the given feature/index
     *
     * Always returns void since an empty fabric contains no facets.
     */
    template <typename FeatureT, std::uint32_t Idx>
    using facet_type = void;

    /**
     * @brief Constructs an empty fabric
     *
     * Takes composition and configs but does nothing with them
     * since there are no facets to initialize.
     */
    template <typename... Components>
    fabric(composition<Components...>&, const configs<Components...>&) {}

    /**
     * @brief Count facets providing a feature (always 0 for empty fabric)
     *
     * @tparam FeatureT The feature type to count
     * @return int Always 0
     */
    template <typename FeatureT>
    static constexpr int count() { return 0; }
};


/**
 * @}
 */

}
}


#endif // UDHO_MANIFOLD_FACET_H
