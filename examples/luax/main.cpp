// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"
#include "urls.h"

#include <boost/asio/io_context.hpp>
#include <boost/filesystem/path.hpp>
#include <udho/net/listener.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/store.h>
#include <udho/www/www.h>

using lua_bridge = udho::view::data::bridges::lua;
using framework_type = udho::www::framework<udho::www::stateless::lua>;

int main() {
    boost::asio::io_context io;

    lua_bridge lua;
    lua.init();
    lua.bind(udho::view::data::type<luax::data::page_data>{});

    udho::view::resources::store<lua_bridge> store{lua};

    udho::pages::system::setup(store);

    const boost::filesystem::path views = boost::filesystem::path(__FILE__).parent_path() / "views";

    store["luax"] << udho::view::resources::lua{"home", views / "home.view"}
                  << udho::view::resources::lua{"hello", views / "hello.view"};

    auto cstore     = udho::view::resources::lock(store);
    auto framework  = framework_type::apply(udho::url::router(luax::urls(), cstore.assets()));
    auto runtime    = framework.runtime(cstore);
    auto listener   = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();
    io.run();

    return 0;
}

