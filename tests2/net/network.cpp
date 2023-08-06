#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <string>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/http.h>
#include <udho/net/protocols/scgi.h>
#include <udho/net/common.h>
#include <type_traits>

using http_connection = udho::net::connection<udho::net::protocols::http>;
using scgi_connection = udho::net::connection<udho::net::protocols::scgi>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

TEST_CASE("udho network", "[net]") {
    CHECK(1 == 1);

    boost::asio::io_service service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("0.0.0.0"), 9000);
    auto listner = std::make_shared<http_listener>(service, endpoint);
    listner->listen([](boost::asio::ip::address address, udho::net::context&& context){
        std::cout << "remote: " << address << std::endl;
        std::cout << context.request() << std::endl;
        std::cout << context.request().target() << std::endl;

        auto request = context.request();
        context.set(boost::beast::http::field::server, "udho");

        std::cout << request[boost::beast::http::field::user_agent] << std::endl;

        context << "Hello World";

        context.flush();
    });

    service.run();
}
