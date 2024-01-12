#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <string>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <type_traits>
#include <curl/curl.h>
#include <udho/url/url.h>

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

void f0(udho::net::context context){
    context << "Hello f0";
    context.finish();
}

int f1(udho::net::context context, int a, const std::string& b, const double& c){
        context << "Hello f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
}

struct X{
    void f0(udho::net::context context){
        context << "Hello X::f0";
        context.finish();
    }

    int f1(udho::net::context context, int a, const std::string& b, const double& c){
        context << "Hello X::f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
    }
};


TEST_CASE("udho network", "[net]") {
    CHECK(1 == 1);

    using namespace udho::hazo::string::literals;

    X x;
    auto a =
        udho::url::slot("f0"_h,  &f0)         << udho::url::home  (udho::url::verb::get)                                                         |
        udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed (udho::url::verb::get, "/x/f0", "/x/f0");
    auto b =
        udho::url::slot("f1"_h,  &f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx  (udho::url::verb::get, "/x/f1/(\\w+)/(\\w+)/(\\d+)", "/x/f1/{}/{}/{}");

    auto mountpoints = udho::url::root(std::move(a)) | udho::url::mount("b"_h, "/b", std::move(b));
    auto router = udho::url::router(std::move(mountpoints));

    boost::asio::io_service service;

    auto server = udho::net::server<http_listener>(service, std::move(router), 9000);

    server.run();

    // auto listner = std::make_shared<http_listener>(service, endpoint);
    // listner->listen([](boost::asio::ip::address address, udho::net::context context){
    //     std::cout << "remote: " << address << std::endl;
    //     std::cout << context.request() << std::endl;
    //     std::cout << context.request().target() << std::endl;
    //     context.encoding(udho::net::types::transfer::encoding::chunked);
    //     auto request = context.request();
    //     context.set(boost::beast::http::field::server, "udho");
    //
    //     std::cout << request[boost::beast::http::field::user_agent] << std::endl;
    //
    //     context << "Hello World";
    //
    //     context.flush(std::bind(&chunk2, context));
    //     // context.finish();
    // });

    service.run();
}
