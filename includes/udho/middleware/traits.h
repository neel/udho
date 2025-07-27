#ifndef UDHO_MIDDLEWARE_TRAITS_H
#define UDHO_MIDDLEWARE_TRAITS_H

#include <type_traits>

namespace udho{
namespace middleware{

/**
 * @brief The component_traits class
 * @details prefer_reference determines whether the component ComponentT will be stored using reference or not
 *          by default componentts that are either not movable or not default constructible will be stored as a reference.
 *          usercode must manage lifetime of these components and provide a reference to them in the facade constructor.
 *
 * @note specialize component_traits<ComponentX> for any ComponentX to override the default settings
 */
template <typename ComponentT>
struct component_traits{
    static constexpr const bool prefer_reference = !std::is_move_constructible_v<ComponentT> || !std::is_default_constructible_v<ComponentT>;
};


}
}

#endif // UDHO_MIDDLEWARE_TRAITS_H
