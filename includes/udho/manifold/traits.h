#ifndef UDHO_MANIFOLD_TRAITS_H
#define UDHO_MANIFOLD_TRAITS_H

#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/utils/string_view.h>
#include <udho/utils/polyfill.h>

namespace udho{
namespace manifold{

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
    static constexpr const bool shared = !std::is_move_constructible_v<ComponentT> || !std::is_default_constructible_v<ComponentT>;
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

template <typename ComponentT>
struct has_name: detail::has_name<ComponentT> {};

template <typename ComponentT>
static constexpr udho::utils::string_view component_name() {
    static_assert(has_name<ComponentT>::value, "ComponentT doesn't have a name. Either set ComponentT::name as a static constexpr member or set component_traits<ComponentT>:");
    return has_name<ComponentT>::get();
}

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

template <typename FacetT>
struct has_result: std::bool_constant<!std::is_void<typename facet_traits<FacetT>::result_type>::value> {};

template <typename ComponentT>
struct has_params: std::bool_constant<!std::is_void<typename component_traits<ComponentT>::params>::value> {};

template <typename ComponentT>
struct has_features: std::bool_constant<!std::is_void<detail::has_features<ComponentT>>::value> {};

struct default_constructed{};


}
}

#endif // UDHO_MANIFOLD_TRAITS_H
