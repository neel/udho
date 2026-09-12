#include "urls.h"
#include <udho/www/www.h>
#include <udho/net/listener.h>
#include <udho/view/resources/resources.h>
#include <boost/filesystem/path.hpp>
#include "counter.h"

using framework_type = udho::www::framework<udho::www::stateless::rest>;

int main() {
    boost::asio::io_context io;

    udho::view::resources::store<> store;
    udho::pages::system::setup(store);
    store.assets().base("assets");

    const boost::filesystem::path assets = boost::filesystem::path(__FILE__).parent_path() / "assets";
    store["simple"] << udho::view::resources::asset::css("simple.css", assets / "css" / "simple.css")
                    << udho::view::resources::asset::js("hello.js", assets / "js" / "hello.js");
    store["cdn"]    << udho::view::resources::asset::remote::js("umbrella.js", "https://cdn.jsdelivr.net/npm/umbrellajs@3.3.3/umbrella.min.js");

    auto cstore    = udho::view::resources::lock(store);

    simple::actions::counter counter;

    auto framework = framework_type::apply(udho::url::router(simple::urls(counter), cstore.assets()));
    auto runtime   = framework.runtime(cstore);
    auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();

    io.run();

    return 0;
}
