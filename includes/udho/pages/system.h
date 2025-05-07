#ifndef UDHO_PAGES_SYSTEM_H
#define UDHO_PAGES_SYSTEM_H

#include <udho/pages/data.h>
#include <udho/pages/views.h>
#include <udho/pages/assets.h>
#include <udho/pages/layouts.h>

namespace udho{
namespace pages{
namespace system{

template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store){
    udho::pages::system::views::setup(store);
    udho::pages::system::assets::setup(store);
}

}
}
}

#endif // UDHO_PAGES_SYSTEM_H
