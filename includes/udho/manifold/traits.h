#ifndef UDHO_MANIFOLD_TRAITS_H
#define UDHO_MANIFOLD_TRAITS_H

#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/utils/string_view.h>
#include <udho/utils/polyfill.h>



namespace udho{
namespace manifold{

/**
 * @addtogroup manifold
 * @{
 */

namespace detail{

template<typename, typename = void>
struct has_result : std::false_type {
    using type = void;
};

template<typename T>
struct has_result<T, std::void_t<typename T::result>> : std::true_type {
    using type = typename T::result;
};

template<typename, typename = void>
struct has_params : std::false_type {
    using type = udho::manifold::params<>;
};

template<typename T>
struct has_params<T, std::void_t<typename T::params>> : std::true_type {
    using type = typename T::params;
};

template<typename, typename = void>
struct has_features : std::false_type {
    using type = udho::manifold::features<>;
};

template<typename T>
struct has_features<T, std::void_t<typename T::features>> : std::true_type {
    using type = typename T::features;
};
}

/**
 * @brief The component_traits class
 * @details shared determines whether the component ComponentT will be stored using reference or not.
 *          By default componentts that are either not movable or not default constructible will be stored as a
 *          reference. Usercode must manage lifetime of these components and provide a reference to them in the
 *          facade constructor.
 *
 * @note specialize component_traits<ComponentX> for any ComponentX to override the default settings
 */
template <typename ComponentT>
struct component_traits{

    /**
     * @brief Whether component uses reference storage semantics
     *
     * True if component should be borrowed (stored as reference).
     * False if component should be owned (stored by value).
     *
     * Default: true if component is non-movable or non-default-constructible
     */
    static constexpr const bool shared = !std::is_move_constructible_v<ComponentT> || !std::is_default_constructible_v<ComponentT>;

    /**
     * @brief Parameters type for this component
     *
     * Defines the configuration parameters. Empty params<> if none.
     */
    using params = typename detail::has_params<ComponentT>::type;
};

namespace detail {

template<typename, typename = void>
struct has_static_name : std::false_type {};

template<typename T>
struct has_static_name<T, udho::utils::void_t<decltype(T::name)>> : std::bool_constant<std::is_convertible_v<decltype(T::name), udho::utils::string_view>> {
    static constexpr udho::utils::string_view get() { return T::name; }
};

template<typename, typename = void>
struct has_traits_name : std::false_type { };

template<typename T>
struct has_traits_name<T, udho::utils::void_t<decltype(component_traits<T>::name)>> : std::bool_constant<std::is_convertible_v<decltype(component_traits<T>::name), udho::utils::string_view>> {
    static constexpr udho::utils::string_view get() { return component_traits<T>::name; }
};

template <typename T>
struct has_name: udho::utils::conditional_t<has_static_name<T>::value, has_static_name<T>, has_traits_name<T>>{};

}

/**
 * @brief Type trait indicating if a component has a name
 *
 * Checks for either ComponentT::name or component_traits<ComponentT>::name.
 *
 * @tparam ComponentT Component type to check
 */
template <typename ComponentT>
struct has_name: detail::has_name<ComponentT> {};

/**
 * @brief Get the name of a component
 *
 * Returns the component's name as a string_view. Name is determined by:
 * 1. ComponentT::name (if present)
 * 2. component_traits<ComponentT>::name (if present)
 * 3. Compile error if neither exists
 *
 * @tparam ComponentT Component type
 * @return constexpr string_view Component name
 *
 * @code
 * struct MyComponent {
 *     static constexpr std::string_view name = "MyComp";
 * };
 *
 * auto name = component_name<MyComponent>();  // "MyComp"
 * @endcode
 */
template <typename ComponentT>
static constexpr udho::utils::string_view component_name() {
    static_assert(has_name<ComponentT>::value, "ComponentT doesn't have a name. Either set ComponentT::name as a static constexpr member or set component_traits<ComponentT>:");
    return has_name<ComponentT>::get();
}

/**
 * @brief Metadata about a facet (component-feature pair)
 *
 * Provides compile-time information about facets, including:
 * - Component and feature types
 * - Result type (if feature yields results)
 * - Pipeline stage
 *
 * @tparam FacetT The facet type (udho::manifold::facet<Component, Feature>)
 *
 * # Trait Members
 *
 * @code
 * using FacetT = facet<MyComponent, MyFeature>;
 *
 * using component = facet_traits<FacetT>::component_type;  // MyComponent
 * using feature = facet_traits<FacetT>::feature_type;      // MyFeature
 * using result = facet_traits<FacetT>::result_type;        // MyFeature::result
 * constexpr size_t stage = facet_traits<FacetT>::stage;    // MyFeature::stage
 * @endcode
 *
 * # Usage
 *
 * Traits are used internally by the pipeline system to:
 * - Build stage-specific fabrics
 * - Construct journals for result storage
 * - Generate evaluation handlers
 *
 * @see facet
 * @see has_result
 */
template <typename FacetT>
struct facet_traits;

template <typename ComponentT, typename FeatureT>
struct facet_traits<udho::manifold::facet<ComponentT, FeatureT>> {
    using component_type = ComponentT;
    using feature_type   = FeatureT;
    using facet_type     = udho::manifold::facet<ComponentT, FeatureT>;
    using result_type    = typename detail::has_result<feature_type>::type;

    static constexpr const std::size_t stage = FeatureT::stage;
};

/**
 * @brief Type trait indicating if a facet yields results
 *
 * Evaluates to true if the feature defines a result type, false otherwise.
 *
 * @tparam FacetT Facet type to check
 *
 * # Usage
 *
 * @code
 * static_assert(has_result<facet<CompA, FeatureX>>::value);
 * static_assert(!has_result<facet<CompB, FeatureY>>::value);
 * @endcode
 *
 * Used internally to determine journal storage requirements.
 */
template <typename FacetT>
struct has_result: std::bool_constant<!std::is_void<typename facet_traits<FacetT>::result_type>::value> {};

/**
 * @brief Type trait indicating if a component has parameters
 *
 * @tparam ComponentT Component type to check
 *
 * Evaluates to true if component defines a params typedef.
 */
template <typename ComponentT>
struct has_params: std::bool_constant<!std::is_void<typename component_traits<ComponentT>::params>::value> {};

/**
 * @brief Type trait indicating if a component provides features
 *
 * @tparam ComponentT Component type to check
 *
 * Evaluates to true if component defines a features typedef.
 */
template <typename ComponentT>
struct has_features: std::bool_constant<!std::is_void<detail::has_features<ComponentT>>::value> {};

/**
 * @struct default_constructed
 * @brief Tag type for default construction in composition
 *
 * Used internally by compositor to indicate that a component should be
 * default-constructed rather than moved or referenced.
 *
 * # Usage (Internal)
 *
 * @code
 * composition<CompA, CompB> comp{
 *     CompA{args},             // Move-constructed
 *     default_constructed{}    // Default-constructed
 * };
 * @endcode
 *
 * User code should prefer `compositor::compose()` which handles this automatically.
 */
struct default_constructed{};

#ifndef __DOXYGEN__

template <typename FacetT>
struct facet_name;

template <typename ComponentT, typename FeatureT>
struct facet_name<facet<ComponentT, FeatureT>>{
    static std::string get(){
        std::string component_name(udho::manifold::component_name<ComponentT>());
        std::string feature_name(FeatureT::name);
        return udho::utils::format("facet<{}, {}>", component_name, feature_name, FeatureT::stage);
    }
};

template <typename... Components>
struct components_name{
    static std::string get(){
        std::string components_str = (std::string(udho::manifold::component_name<Components>()) + "," + ... );
        components_str.pop_back();
        return components_str;
    }
};

template <typename... Facets>
struct facets_name{
    static std::string get(){
        std::string facets_str = ((facet_name<Facets>::get() + ",") + ... );
        facets_str.pop_back();
        return facets_str;
    }
};

template<>
struct facets_name<>{
    static std::string get(){
        std::string facets_str = "";
        return facets_str;
    }
};

template <typename Composition>
struct composition_name;

template <typename Fabric>
struct fabric_name;

#endif // __DOXYGEN__

template <typename... Components>
struct composition_name<composition<Components...>>{
    static std::string get(){
        return "components<"+components_name<Components...>::get()+">";
    }
};

template <std::size_t Stage, typename... Facets>
struct fabric_name<fabric<Stage, Facets...>>{
    static std::string get(){
        return "fabric<" + std::to_string(Stage) + "," + facets_name<Facets...>::get() +">";
    }
};

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_TRAITS_H
