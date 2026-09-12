#ifndef UDHO_PAGES_SYSTEM_H
#define UDHO_PAGES_SYSTEM_H

#include <udho/pages/data.h>
#include <udho/pages/repr.h>
#include <udho/pages/views.h>
#include <udho/pages/assets.h>
#include <udho/pages/layouts.h>

namespace udho{
namespace pages{
namespace system{

/**
 * @brief Checks whether all resources required by the system pages are registered.
 * @tparam Bridges Bridge types supported by the store.
 * @param store Resource store to inspect.
 * @return True when both the required views and assets are ready.
 * @ingroup DoxyG_pages
 */
template <typename... Bridges>
bool ready(udho::view::resources::store<Bridges...>& store){
    return udho::pages::system::views::ready(store)
        && udho::pages::system::assets::ready(store);
}

/**
 * @brief Checks whether all resources required by the system pages are registered.
 * @tparam Bridges Bridge types exposed by the read-only store.
 * @param store Read-only resource store to inspect.
 * @return True when both the required views and assets are ready.
 * @ingroup DoxyG_pages
 */
template <typename... Bridges>
bool ready(const udho::view::resources::const_store<Bridges...>& store){
    return udho::pages::system::views::ready(store)
        && udho::pages::system::assets::ready(store);
}

/**
 * @brief Registers system-page views and assets in a resource store.
 * @tparam Bridges Bridge types supported by the store.
 * @param store Resource store to populate.
 * @ingroup DoxyG_pages
 */
template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store){
    udho::pages::system::views::setup(store);
    udho::pages::system::assets::setup(store);
}

}
}
}

#endif // UDHO_PAGES_SYSTEM_H
