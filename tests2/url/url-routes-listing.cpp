#include <catch2/catch_test_macros.hpp>
#include <udho/url/url.h>
#include <iostream>

#include <udho/net/listener.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/manifold/www.h>
#include <udho/manifold/fabric.h>
#include <curl/curl.h>

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

// using socket_type       = udho::net::types::socket;
// using http_protocol     = udho::net::protocols::http<socket_type>;
// using scgi_protocol     = udho::net::protocols::scgi<socket_type>;
// using http_connection   = udho::net::connection<http_protocol>;
// using scgi_connection   = udho::net::connection<scgi_protocol>;
// using http_listener     = udho::net::listener<http_connection>;
// using scgi_listener     = udho::net::listener<scgi_connection>;
// using http_server       = udho::net::server<http_listener>;
// using scgi_server       = udho::net::server<scgi_listener>;

using stream_type = udho::net::types::socket;

using namespace udho::manifold::components;
using namespace udho::manifold;

using handler = basic_handler<stream_type>;

namespace callbacks{

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

BOOST_SYMBOL_EXPORT void f0(basic_context<stream_type, handler, cookies> context){
    context << "f0";
    context.finish();
    return;
}

BOOST_SYMBOL_EXPORT int f1(basic_context<stream_type, handler, navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
    context << std::to_string(a.size()+b.size()+c+d);
    std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
    context.finish();
    return 42;
}

BOOST_SYMBOL_EXPORT std::string f2(basic_context<stream_type, handler, cookies> context, int a, const std::string& b){
    context << std::to_string(a+b.size());
    context.finish();
    return "hello";
}

BOOST_SYMBOL_EXPORT std::string f_nodef(basic_context<stream_type, handler> context, nodef, int a){
    context << std::to_string(a);
    context.finish();
    return "hello";
}

struct X{
    BOOST_SYMBOL_EXPORT void f0(basic_context<stream_type, handler, cookies> context){
        context << "f0";
        context.finish();
        return;
    }

    BOOST_SYMBOL_EXPORT int f1(basic_context<stream_type, handler, navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
        context << std::to_string(a.size()+b.size()+c+d);
        std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
        context.finish();
        return 42;
    }

    BOOST_SYMBOL_EXPORT std::string f2(basic_context<stream_type, handler, cookies> context, int a, const std::string& b){
        context << std::to_string(a+b.size());
        context.finish();
        return "hello";
    }

    BOOST_SYMBOL_EXPORT int f3(basic_context<stream_type, handler> context, int a, const std::string& b, const double& c, bool d) const{
        context << std::to_string(84);
        context.finish();
        return 0;
    }
};

}

TEST_CASE("URL routes listing", "[url][routing][listing]") {
    using namespace udho::hazo::string::literals;

    callbacks::X x;
    auto chain =
        // udho::url::slot("f0"_h,  &f0)         << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)         << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")                       |
        udho::url::slot("xf0"_h, &callbacks::X::f0, &x)  << udho::url::fixed(udho::url::verb::get, "/x/f0", "/x/f0")                                      |
        udho::url::slot("xf1"_h, &callbacks::X::f1, &x)  << udho::url::regx(udho::url::verb::get,  "/x/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f1/{}/{}/{}");
    auto chain2 =
        udho::url::regx(udho::url::verb::get, "/x/f2-(\\d+)/(\\w+)", "/x/f2-{}/{}")                  >> udho::url::slot("xf2"_h, &callbacks::X::f0, &x)  |
        udho::url::regx(udho::url::verb::get, "/x/f3/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f3/{}/{}/{}") >> udho::url::slot("xf3"_h, &callbacks::X::f1, &x);

    auto chain3 = chain | chain2;

    udho::url::mount_point mount_point{"chain"_h, "/pchain", std::move(chain)};
    auto chain4 = std::move(mount_point) | udho::url::mount_point("root"_h, "/", std::move(chain3));

    std::cout << "chain4" << std::endl << chain4 << std::endl;

    boost::asio::io_context service;

    udho::view::data::bridges::lua lua;
    lua.init();

    udho::view::resources::store<udho::view::data::bridges::lua> resources{lua};
    udho::pages::system::setup(resources);
    resources.assets().base("assets");
    resources.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{resources};

    std::filesystem::path exe_path = std::filesystem::current_path();
    std::filesystem::path docroot  = exe_path / "docroot";

    auto router = udho::url::router(std::move(chain4), cstore.assets(), docroot);


    using framework_type = udho::manifold::framework<udho::manifold::www::stateless::lua>;
    using endpoint_type  = typename framework_type::endpoint_type;

    auto resource_store_component  = udho::manifold::components::resources(cstore);

    auto framework = framework_type::apply(std::move(router));
    auto runtime   = framework.runtime(resource_store_component);

    lua.bind(udho::view::data::type<std::decay_t<decltype(runtime)>::portal_type>{});
    lua.bind(udho::view::data::type<std::decay_t<decltype(runtime)>::context_type>{});

    auto listener  = udho::net::listener(service, runtime, {boost::asio::ip::tcp::v4(), 9000});

    listener.start();

    std::thread thread([&]{
        service.run();
    });

    listener.stop();
    thread.join();

}
