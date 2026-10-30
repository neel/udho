// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"
#include "urls.h"

#include <boost/asio/io_context.hpp>
#include <udho/cli/server.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/store.h>
#include <udho/view/resources/blobs/setup.h>
#include <udho/www/www.h>

using framework_type = udho::www::framework<udho::www::stateless::lua>;

int main(int argc, char** argv) {
    udho::cli::control control(argc, argv);
    if(control.admin()) return control();
    
    boost::asio::io_context io;

    udho::view::data::bridges::lua lua;
    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};

    udho::pages::system::setup(store);
    store.assets().base("assets");
    udho::view::resources::blobs::setup(store, "hello");

    auto server = control.prepare<framework_type>(store, hello::urls());
    server.run(io, control.ipv4());

    return 0;
}
