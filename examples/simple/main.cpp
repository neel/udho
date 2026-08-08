#include "urls.h"
#include <udho/www/www.h>
#include <udho/net/listener.h>
#include <udho/view/resources/resources.h>

using framework_type = udho::www::framework<udho::www::stateless::rest>;

int main() {
    boost::asio::io_context io;

    udho::view::resources::store<> store;
    udho::pages::system::setup(store);
    store.assets().base("assets");


    auto cstore    = udho::view::resources::lock(store);
    auto framework = framework_type::apply(udho::url::router(simple::urls(), cstore.assets()));
    auto runtime   = framework.runtime(cstore);
    auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();

    io.run();

    return 0;
}