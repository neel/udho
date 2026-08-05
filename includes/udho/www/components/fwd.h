#ifndef UDHO_WWW_COMPONENTS_FWD_H
#define UDHO_WWW_COMPONENTS_FWD_H

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{
namespace components{

/**
 * @brief Resource-store component.
 * @tparam Bridges Resource bridge types.
 * @ingroup DoxyG_www_components
 */
template <typename... Bridges>
struct resources;

/**
 * @brief Router component.
 * @tparam RouterT Router type.
 * @ingroup DoxyG_www_components
 */
template <typename RouterT>
class routing;

}
}
}

/** @} */

#endif // UDHO_WWW_COMPONENTS_FWD_H
