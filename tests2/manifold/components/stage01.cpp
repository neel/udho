#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/url/url.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/fabric.h>
#include <udho/manifold/pipeline.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/components/navigator.h>
#include <udho/manifold/config.h>
#include <udho/manifold/journal.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/manifold/components/protocol.h>
#include <udho/manifold/components/routing.h>

using stream_type      = boost::beast::test::stream; // udho::net::types::socket;

namespace testing{

template <typename RoutingComponentT, typename StreamT>
struct www{
    using routing_component_type     = RoutingComponentT;
    using stream_type                = StreamT;
    using protocol_component_type    = udho::manifold::components::protocols::http2<stream_type>;
    using navigator_component_type   = udho::manifold::components::navigators::pretty;
};

}

template <typename StreamT, typename RoutingComponentT>
struct udho::manifold::sketch<testing::www<StreamT, RoutingComponentT>>{
    using www_type = testing::www<StreamT, RoutingComponentT>;

    using composition_type = udho::manifold::composition<
        typename www_type::protocol_component_type,
        typename www_type::navigator_component_type,
        typename www_type::routing_component_type
    >;

    using order_type = udho::manifold::order<
        udho::manifold::feature::header_reader,
        udho::manifold::feature::identifier,
        udho::manifold::feature::locator,
        udho::manifold::feature::body_reader
    >;
};

namespace testing{
template <typename RoutingComponentT, typename StreamT = boost::beast::test::stream>
struct framework{
    using label_type        = testing::www<RoutingComponentT, StreamT>;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using runtime_type      = udho::manifold::runtime<label_type>;
    using flow_type         = udho::manifold::flow<label_type>;
    using composition_type  = typename runtime_type::composition_type;

    static_assert(runtime_type::Count == 2);

    framework(RoutingComponentT& routing): _runtime(routing) {}

    runtime_type& runtime() { return _runtime; }

private:
    runtime_type _runtime;
};
}

template <typename RoutingComponentT, typename StreamT>
struct udho::manifold::terminal<testing::www<RoutingComponentT, StreamT>> {
    using label_type        = testing::www<RoutingComponentT, StreamT>;
    using runtime_type      = runtime<label_type>;
    using flow_type         = flow<label_type>;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;

    terminal() = delete;
    terminal(const terminal&) = delete;

    terminal(composition_type& composition, const configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    bool reenter(stream_type& stream) { return true; }
    void prepare(stream_type& stream) { }
    bool error(udho::manifold::exclusive_result success, stream_type& stream){
        if(success.has_exception()) {
            try{
                success.rethrow();
            } catch(std::system_error ex) {
                std::cout << "exception: " << ex.what() << std::endl;
            }
        }
        return false;
    }

private:
    composition_type&   _composition;
    const configs_type& _configs;
    const journal_type& _journal;
};


struct nodef{
    nodef() = delete;
    nodef(int) {}
};

BOOST_SYMBOL_EXPORT void f0(udho::net::stream context){
    context << "f0";
    context.finish();
    return;
}

BOOST_SYMBOL_EXPORT int f1(udho::net::stream context, int a, const std::string& b, const double& c, bool d){
    context << std::to_string(a+b.size()+c+d);
    context.finish();
    return 42;
}

BOOST_SYMBOL_EXPORT std::string f2(udho::net::stream context, int a, const std::string& b){
    context << std::to_string(a+b.size());
    context.finish();
    return "hello";
}

BOOST_SYMBOL_EXPORT std::string f_nodef(udho::net::stream context, nodef, int a){
    context << std::to_string(a);
    context.finish();
    return "hello";
}

static_assert(udho::manifold::feature::body_reader::stage == udho::manifold::feature::identifier::stage +1);

TEST_CASE("udho manifold pipeline stage 0", "[manifold][pipeline]") {
    using namespace udho::hazo::string::literals;
    auto actions1 =
        udho::url::slot("f0"_h,  &f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions2 =
        udho::url::slot("f0"_h,  &f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions3 =
        udho::url::slot("f0"_h,  &f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    udho::url::mount_point mount_point1{"root"_h, "/",    std::move(actions1)};
    udho::url::mount_point mount_point2{"m2"_h,   "/m2",  std::move(actions2)};
    udho::url::mount_point mount_point3{"m3"_h,   "/m3",  std::move(actions2)};

    auto table      = std::move(mount_point1)/* | std::move(mount_point2) | std::move(mount_point3)*/;
    auto router     = udho::url::router(std::move(table));
    auto routing    = udho::manifold::components::routing(router);

    auto framework  = testing::framework(routing);
    auto flow       = framework.runtime().spawn();

    boost::asio::io_context io_context;
    std::string request_data =
        "POST /f1/hello/world/23/24?name=test&id=42&filter=active HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, World!"
        "POST /f1/hello/world/25/23?name=test&id=42&filter=active HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, Earth!"
    ;
    stream_type stream(io_context, request_data);
    flow->start(stream);
    flow->then([](const auto& flow, bool success) {
        std::cout << "finished" << std::endl;
    });

    io_context.run();
}
