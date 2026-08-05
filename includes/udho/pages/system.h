#ifndef UDHO_PAGES_SYSTEM_H
#define UDHO_PAGES_SYSTEM_H

#include <udho/pages/data.h>
#include <udho/pages/views.h>
#include <udho/pages/assets.h>
#include <udho/pages/layouts.h>

namespace udho{
namespace pages{
namespace system{

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
