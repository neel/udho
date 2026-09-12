// SPDX-License-Identifier: BSD-3-Clause
#include "controllers/api.h"
#include "controllers/ui.h"
#include "urls.h"
#include "view/data/project.h"

#include <boost/asio/io_context.hpp>
#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <udho/net/listener.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <udho/www/www.h>

using lua_bridge = udho::view::data::bridges::lua;
using framework_type = udho::www::framework<udho::www::stateless::lua>;

int main() {
    boost::asio::io_context io;
    const char* configured = std::getenv("UDHO_PGX_DATABASE_URL");
    const std::string connection_string = configured ? configured : "host=localhost dbname=udho_pgx user=postgres";

    udho::www::components::db::pg<> database(connection_string);
    pgx::controllers::api api_controller(io, database);
    pgx::controllers::ui ui_controller(io, database);

    lua_bridge lua;
    lua.init();
    lua.bind(udho::view::data::type<pgx::view::data::task>{});
    lua.bind(udho::view::data::type<pgx::view::data::project>{});

    udho::view::resources::store<lua_bridge> store{lua};
    udho::pages::system::setup(store);
    store.assets().base("assets");

    const auto root = boost::filesystem::path(__FILE__).parent_path();
    store["pgx"]
        << udho::view::resources::lua{"project.view", root / "view" / "tmpl" / "project.view"}
        << udho::view::resources::asset::css("app.css", root / "assets" / "app.css")
        << udho::view::resources::asset::js("app.js", root / "assets" / "app.js")
        << udho::view::resources::asset::txt("project.html", root / "assets" / "project.html", "text/html");

    auto cstore = udho::view::resources::lock(store);
    auto framework = framework_type::apply(udho::url::router(pgx::urls(api_controller, ui_controller), cstore.assets()));
    auto runtime = framework.runtime(cstore, database);
    auto listener = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9997});
    listener.start();
    io.run();
    return 0;
}
