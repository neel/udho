#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <string>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <type_traits>

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

void chunk3(udho::net::context context){
    context << "Hello Jupiter";
    context.finish();
}

void chunk2(udho::net::context context){
    context << "Hello Mars";
    std::cout << "chunk 2" << std::endl;
    context.flush(std::bind(&chunk3, context));
}

void f(int){}

TEST_CASE("udho network", "[net]") {
    CHECK(1 == 1);

    boost::asio::io_service service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("0.0.0.0"), 9000);
    auto listner = std::make_shared<http_listener>(service, endpoint);
    listner->listen([](boost::asio::ip::address address, udho::net::context context){
        std::cout << "remote: " << address << std::endl;
        std::cout << context.request() << std::endl;
        std::cout << context.request().target() << std::endl;
        context.encoding(udho::net::types::transfer::encoding::chunked);
        auto request = context.request();
        context.set(boost::beast::http::field::server, "udho");

        std::cout << request[boost::beast::http::field::user_agent] << std::endl;

        context << "Hello World";

        context.flush(std::bind(&chunk2, context));
        // context.finish();
    });

    service.run();
}
