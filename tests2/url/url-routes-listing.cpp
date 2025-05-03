#include <catch2/catch_test_macros.hpp>
#include <udho/url/url.h>
#include <iostream>

#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <curl/curl.h>
#include <udho/net/artifacts.h>

// { experiment
// template <typename Policy, template<typename...> class T, typename X>
// struct is_basic_seq_of : std::false_type {};

// template <typename Policy, template<typename...> class T, typename... Args>
// struct is_basic_seq_of<Policy, T, udho::hazo::basic_seq<Policy, T<Args...>> > : std::true_type {};

// template <template<typename...> class T, typename X>
// using is_basic_seq_d_of = is_basic_seq_of<udho::hazo::by_data, T, X>;

// template <template<typename...> class T, typename X>
// using is_basic_seq_v_of = is_basic_seq_of<udho::hazo::by_value, T, X>;

// template <typename X>
// using is_routable = is_basic_seq_d_of<udho::url::mount_point, X>;
// }

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

void f0(udho::net::stream context){
    context << "f0";
    context.finish();
    return;
}

int f1(udho::net::stream context, int a, const std::string& b, const double& c, bool d){
    context << std::to_string(a+b.size()+c+d);
    context.finish();
    return 42;
}

std::string f2(udho::net::stream context, int a, const std::string& b){
    context << std::to_string(a+b.size());
    context.finish();
    return "hello";
}

std::string f_nodef(udho::net::stream context, nodef, int a){
    context << std::to_string(a);
    context.finish();
    return "hello";
}

struct X{
    void f0(udho::net::stream context){
        context << "f0";
        context.finish();
        return;
    }

    int f1(udho::net::stream context, int a, const std::string& b, const double& c, bool d){
        context << std::to_string(a+b.size()+c+d);
        context.finish();
        return a+b.size()+c+d;
    }

    std::string f2(udho::net::stream context, int a, const std::string& b){
        context << std::to_string(a+b.size());
        context.finish();
        return "world";
    }

    int f3(udho::net::stream context, int a, const std::string& b, const double& c, bool d) const{
        context << std::to_string(84);
        context.finish();
        return 0;
    }
};

TEST_CASE("URL routes listing", "[url][routing][listing]") {
    using namespace udho::hazo::string::literals;

    X x;
    auto chain =
        // udho::url::slot("f0"_h,  &f0)         << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &f1)         << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &f2)         << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")                       |
        udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed(udho::url::verb::get, "/x/f0", "/x/f0")                                      |
        udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx(udho::url::verb::get,  "/x/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f1/{}/{}/{}");
    auto chain2 =
        udho::url::regx(udho::url::verb::get, "/x/f2-(\\d+)/(\\w+)", "/x/f2-{}/{}")                  >> udho::url::slot("xf2"_h, &X::f0, &x)  |
        udho::url::regx(udho::url::verb::get, "/x/f3/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f3/{}/{}/{}") >> udho::url::slot("xf3"_h, &X::f1, &x);

    auto chain3 = chain | chain2;

    udho::url::mount_point mount_point{"chain"_h, "/pchain", std::move(chain)};
    auto chain4 = std::move(mount_point) | udho::url::mount_point("root"_h, "/", std::move(chain3));

    std::cout << "chain4" << std::endl << chain4 << std::endl;

    boost::asio::io_context service;

    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<udho::net::context<udho::view::data::bridges::lua>>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resources{lua};
    udho::pages::system::setup(resources);
    resources.assets().base("assets");
    resources.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{resources};

    std::filesystem::path exe_path = std::filesystem::current_path();
    std::filesystem::path docroot  = exe_path / "docroot";

    auto router = udho::url::router(std::move(chain4), cstore.assets(), docroot);

    auto server = udho::net::server<http_listener>(service, 9000);
    auto artifacts  = udho::net::artifacts{router, resources};

    server.run(artifacts);

    std::thread thread([&]{
        service.run();
    });

    server.stop();
    thread.join();

}
