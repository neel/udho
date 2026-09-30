// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"
#include "urls.h"

#include <boost/asio/io_context.hpp>
#include <udho/net/listener.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/store.h>
#include <udho/www/www.h>

using framework_type = udho::www::framework<udho::www::stateless::lua>;

int main() {
    boost::asio::io_context io;

    udho::view::data::bridges::lua lua;

    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};

    udho::pages::system::setup(store);

    store["luax"] << udho::view::resources::lua{"home"}
                  << udho::view::resources::lua{"hello"};

    auto cstore     = udho::view::resources::lock(store);
    auto urls       = luax::urls();
    auto router     = udho::url::router(std::move(urls), cstore.assets());
    auto framework  = framework_type::apply(std::move(router));
    auto runtime    = framework.runtime(cstore);
    auto listener   = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();
    io.run();

    return 0;
}
