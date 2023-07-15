#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <string>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/http.h>
#include <udho/net/common.h>
#include <type_traits>

using http_connection = udho::net::connection<udho::net::protocols::http>;
using http_listener   = udho::net::listener<http_connection>;

TEST_CASE("udho network", "[net]") {
    CHECK(1 == 1);

    boost::asio::io_service service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("0.0.0.0"), 8080);
    auto listner = std::make_shared<http_listener>(service, endpoint);
    listner->listen([](boost::asio::ip::address address, udho::net::types::headers::request request){
        std::cout << address << std::endl;
        std::cout << request << std::endl;
        std::cout << request.target() << std::endl;
    });

    service.run();
}
