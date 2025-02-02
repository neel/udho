#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>
#include <udho/net/context.h>
#include <udho/url/router.h>

#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <type_traits>
#include <curl/curl.h>
#include <udho/net/artifacts.h>

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

TEST_CASE("Accessing assets through router via HTTP requests", "[router][asset]") {
    static char buffer_js[]  = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static char buffer_img[] = "console.log('Hello, world!');";

    const std::map<std::string, std::tuple<std::string, udho::view::resources::asset::type, std::string>> assets = {
        {"0profile1.js", {"primary", udho::view::resources::asset::type::js, buffer_js}},
        {"1profile2.js", {"primary", udho::view::resources::asset::type::js, buffer_js1}},
        {"2profile.css", {"primary", udho::view::resources::asset::type::css, buffer_css}},
        {"3profile.png", {"primary", udho::view::resources::asset::type::img, buffer_img}}
    };

    udho::view::resources::asset::store store;

    store["primary"] << udho::view::resources::asset::js("profile0.js", buffer_js, buffer_js+std::strlen(buffer_js));
    {
        auto it = assets.cbegin();
        store[std::get<0>(it->second)] << udho::view::resources::asset::js(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        store[std::get<0>(it->second)] << udho::view::resources::asset::js(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        store[std::get<0>(it->second)] << udho::view::resources::asset::css(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        store[std::get<0>(it->second)] << udho::view::resources::asset::img(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
    }

    CHECK(assets.size()+1 == store.size());

    store.base("assets");

    // TEST Creating a const_store from store should throw exception unless the store is locked.
    REQUIRE_THROWS_AS(udho::view::resources::asset::const_store{store}, std::exception);

    store.lock();
    // TEST Adding resources to a locked store should also throw exception
    REQUIRE_THROWS_AS(store["primary"] << udho::view::resources::asset::js("profile1.js", buffer_js, buffer_js+std::strlen(buffer_js)), std::exception);

    udho::view::resources::asset::const_store cstore{store};

    auto router = udho::url::router(cstore);

    boost::asio::io_service service;

    auto server = udho::net::server<http_listener>(service, 9000);
    // auto artifacts  = udho::net::artifacts<decltype(router), udho::view::resources::store<udho::view::data::bridges::lua> >{router, resources};
    //
    // server.run(artifacts);
    //
    // std::thread thread([&]{
    //     service.run();
    // });


    CHECK(0 == 0);
}
