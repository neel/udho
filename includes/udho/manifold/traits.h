#ifndef UDHO_MANIFOLD_TRAITS_H
#define UDHO_MANIFOLD_TRAITS_H

#include <type_traits>
#include <udho/manifold/fwd.h>

namespace udho{
namespace manifold{

namespace detail{

template<typename, typename = void>
struct has_state : std::false_type {
    using type = void;
};

template<typename T>
struct has_state<T, std::void_t<typename T::state>> : std::true_type {
    using type = typename T::state;
};

template<typename, typename = void>
struct has_params : std::false_type {
    using type = udho::manifold::params<>;
};

template<typename T>
struct has_params<T, std::void_t<typename T::config>> : std::true_type {
    using type = typename T::params;
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

template <typename DelegateT>
struct delegate_traits;

template <typename ComponentT, typename FeatureT>
struct delegate_traits<udho::manifold::delegate<ComponentT, FeatureT>> {
    using component_type = ComponentT;
    using feature_type   = FeatureT;
    using delegate_type  = udho::manifold::delegate<ComponentT, FeatureT>;
    using state          = typename detail::has_state<delegate_type>::type;
};

template <typename DelegateT>
struct has_state: std::bool_constant<!std::is_void<typename delegate_traits<DelegateT>::state>::value> {};

template <typename ComponentT>
struct has_params: std::bool_constant<!std::is_void<typename component_traits<ComponentT>::config>::value> {};

struct default_constructed{};


}
}

#endif // UDHO_MANIFOLD_TRAITS_H
