#ifndef UDHO_VIEW_RESOURCES_BLOBS_SETUP_H
#define UDHO_VIEW_RESOURCES_BLOBS_SETUP_H

#include <udho/view/resources/blobs/blob.h>
#include <udho/view/resources/blobs/store.h>
#include <udho/view/resources/store.h>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/lua.h>

namespace udho {
namespace view{
namespace resources {
namespace blobs {

void setup(udho::view::resources::asset::store& store, const char* prefix) {
    const udho::view::resources::blobs::store& bstore = udho::view::resources::blobs::registry();
    auto pbstore = udho::view::resources::asset::prefixed_store{store, prefix};

    for(const auto& p: bstore.js())   pbstore << udho::view::resources::asset::js(p.second);
    for(const auto& p: bstore.css())  pbstore << udho::view::resources::asset::css(p.second);
    for(const auto& p: bstore.imgs()) pbstore << udho::view::resources::asset::img(p.second);
}

template <typename... Bridges>
void setup(udho::view::resources::store<Bridges...>& store, const char* prefix) {
    setup(store.assets(), prefix);

    const udho::view::resources::blobs::store& bstore = udho::view::resources::blobs::registry();
    auto pbstore = udho::view::resources::prefixed_store{store, prefix};

    for(const auto& p: bstore.views()) pbstore << udho::view::resources::lua(p.second);
}

}
}
}
}

#endif // UDHO_VIEW_RESOURCES_BLOBS_SETUP_H
