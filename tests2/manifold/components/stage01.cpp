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
#include <udho/manifold/components/cookies.h>
#include <udho/manifold/components/session.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/fs_mem.h>
#include <udho/session/storage/redis.h>
#include <udho/manifold/portal.h>

#include <udho/manifold/composition_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/manifold/configs_view.h>

using stream_type      = boost::beast::test::stream; // udho::net::types::socket;

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

BOOST_SYMBOL_EXPORT void f0(udho::manifold::portal<udho::manifold::components::cookies> context){
    // context << "f0";
    // context.finish();
    return;
}

BOOST_SYMBOL_EXPORT int f1(udho::manifold::portal<udho::manifold::components::navigators::pretty, udho::manifold::components::cookies> context, std::string a, const std::string& b, const double& c, int d){
    // context << std::to_string(a+b.size()+c+d);
    // context.finish();
    std::cout << "context.resource(): " << context.resource()  << std::endl;
    return 42;
}

BOOST_SYMBOL_EXPORT std::string f2(udho::manifold::portal<udho::manifold::components::cookies> context, int a, const std::string& b){
    // context << std::to_string(a+b.size());
    // context.finish();
    return "hello";
}

BOOST_SYMBOL_EXPORT std::string f_nodef(udho::manifold::portal<> context, nodef, int a){
    // context << std::to_string(a);
    // context.finish();
    return "hello";
}

namespace testing{

auto url() {
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

    return table;
}

using routing_table_type = std::decay_t<decltype(url())>;


template <typename StreamT>
struct www{
    using router_type                = udho::url::basic_router<routing_table_type>;
    using routing_component_type     = udho::manifold::components::routing<router_type>;
    using stream_type                = StreamT;
    using protocol_component_type    = udho::manifold::components::protocols::http2<stream_type>;
    using navigator_component_type   = udho::manifold::components::navigators::pretty;
    using cookies_component_type     = udho::manifold::components::cookies;
    using session_component_type     = udho::manifold::components::session<udho::session::storage::fs, udho::session::modes::lazy>;
};

}

template <typename StreamT>
struct udho::manifold::sketch<testing::www<StreamT>>{
    using www_type = testing::www<StreamT>;

    using composition_type = udho::manifold::composition<
        typename www_type::protocol_component_type,
        typename www_type::navigator_component_type,
        typename www_type::routing_component_type,
        typename www_type::cookies_component_type,
        typename www_type::session_component_type
    >;

    using order_type = udho::manifold::order<
        udho::manifold::feature::header_reader,
        udho::manifold::feature::identifier,
        udho::manifold::feature::locator,
        udho::manifold::feature::cookie_load,
        udho::manifold::feature::session_load,
        udho::manifold::feature::body_reader
    >;
};

namespace testing{

template <typename StreamT = boost::beast::test::stream>
struct framework{
    using label_type             = testing::www<StreamT>;
    using sketch_type            = udho::manifold::sketch<label_type>;
    using runtime_type           = udho::manifold::runtime<label_type>;
    using flow_type              = udho::manifold::flow<label_type>;
    using composition_type       = typename runtime_type::composition_type;
    using routing_component_type = typename label_type::routing_component_type;
    using router_type            = typename label_type::router_type;

    static_assert(runtime_type::Count >= 2);

    template <typename... Components>
    framework(routing_table_type&& table, Components&&... components): _router(std::move(table)), _routing(_router), _runtime(_routing, std::forward<Components>(components)...) {}

    runtime_type& runtime() { return _runtime; }

private:
    router_type             _router;
    routing_component_type  _routing;
    runtime_type            _runtime;
};

}

template <typename StreamT>
struct udho::manifold::terminal<testing::www<StreamT>> {
    using label_type        = testing::www<StreamT>;
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
            } catch(const std::exception& ex) {
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

template <typename StreamT>
struct udho::manifold::patch<testing::www<StreamT>, 1>{
    using label_type             = testing::www<StreamT>;
    using sketch_type            = sketch<label_type>;
    using runtime_type           = runtime<label_type>;
    using flow_type              = flow<label_type>;
    using composition_type       = typename runtime_type::composition_type;
    using journal_type           = typename flow_type::journal_type;
    using pipeline_type          = typename runtime_type::template pipeline_at<1>;
    using configs_type           = typename runtime_type::configs_type;
    using portal_type            = typename udho::manifold::detail::get_portal_type<composition_type>::type;
    using start_pipeline_type    = typename runtime_type::start_pipeline_type;
    using routing_component_type = typename label_type::routing_component_type;
    using routing_table_type     = typename routing_component_type::routing_table_type;

    static void apply(pipeline_type& p, configs_type& config) {
        udho::manifold::patch_config<label_type, 1>::apply(p, config);

        start_pipeline_type& start_pipeline = p.template at<-1>();

        // { essentials
        composition_type& composition = start_pipeline.composition();
        const journal_type& journal   = start_pipeline.journal();
        configs_type& configs         = start_pipeline.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::manifold::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        assert(route_index.valid());
        const routing_component_type& routing_component = composition.template get<routing_component_type>().component();
        const routing_table_type& routing_table = routing_component.table();
        routing_table.reconfigure_for(route_index, configs);
        // }

        // { create portal
        portal_type portal(composition, configs, journal);
        // }

        // { invoke action
        routing_table.invoke_at(route_index, portal.resource(), portal);
        // }
    }
};

static_assert(udho::manifold::feature::body_reader::stage > udho::manifold::feature::identifier::stage);

TEST_CASE("udho manifold pipeline stage 0", "[manifold][pipeline]") {
    using catalogue_type = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;
    catalogue_type catalogue{udho::session::storage::fs{}};

    auto session    = udho::manifold::components::session(catalogue);

    auto framework  = testing::framework(testing::url(), session);
    auto flow       = framework.runtime().spawn();

    using framework_type = std::decay_t<decltype(framework)>;
    using runtime_type   = framework_type::runtime_type;
    using flow_type      = framework_type::flow_type;
    using journal_type   = flow_type::journal_type;

    boost::asio::io_context io_context;
    std::string request_data =
        "POST /f1/hello/world/23/24?name=test&id=42&filter=active HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Cookie: session=s1; theme=dark; uid=42\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, World!"
        "POST /f1/hello/world/25/23?name=test&id=42&filter=active HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 13\r\n"
        "Cookie: session=s2; theme=light; uid=99\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, Earth!"
    ;
    stream_type stream(io_context, request_data);
    flow->start(stream);

    std::size_t counter = 0;

    // the callback gets called in two circumstances
    //  1. pipeline finished processing
    //  2. pipeline encountered error
    // in both circumstances a decision whether to reenter or not
    // has been made already which is passed to reenter argument
    flow->then([&counter](const auto& flow, bool reenter) {
        std::cout << "finished: " << reenter << std::endl;
        const journal_type& journal = flow.journal();

        const auto& w_request    = journal.at<udho::manifold::feature::header_reader>();
        const auto& w_route_desc = journal.at<udho::manifold::feature::identifier>();
        const auto& w_uri        = journal.at<udho::manifold::feature::locator>();
        const auto& w_jar        = journal.at<udho::manifold::feature::cookie_load>();
        const auto& w_body       = journal.at<udho::manifold::feature::body_reader>();

        if(counter == 0) {
            CHECK(reenter);

            REQUIRE(w_request.ready());
            REQUIRE(w_route_desc.ready());
            REQUIRE(w_uri.ready());
            REQUIRE(w_jar.ready());
            REQUIRE(w_body.ready());

            const udho::manifold::feature::header_reader::result& request    = w_request;
            const udho::manifold::feature::identifier::result&    route_desc = w_route_desc;
            const udho::manifold::feature::locator::result&       uri        = w_uri;
            const udho::manifold::feature::cookie_load::result&   jar        = w_jar;
            const udho::manifold::feature::body_reader::result&   body       = w_body;

            CHECK(request.method() == boost::beast::http::verb::post);
            CHECK(request.target() == "/f1/hello/world/23/24?name=test&id=42&filter=active");
            CHECK(request[boost::beast::http::field::host] == "example.com");
            CHECK(request[boost::beast::http::field::content_type] == "text/plain");
            CHECK(request[boost::beast::http::field::content_length] == "13");

            CHECK(route_desc.resource() == "/f1/hello/world/23/24");
            {
                const auto& params = route_desc.params();
                auto get1 = [&](const std::string& k) -> std::string {
                    auto it = params.find(k);
                    REQUIRE(it != params.end());
                    return it->second;
                };
                CHECK(get1("name") == "test");
                CHECK(get1("id") == "42");
                CHECK(get1("filter") == "active");
            }

            CHECK(jar.count("session") == 1);
            CHECK(jar.count("theme")   == 1);
            CHECK(jar.count("uid")     == 1);

            {
                auto v = jar.by_name("session");
                REQUIRE(v.size() == 1);
                CHECK(v[0].name() == "session");
                CHECK(v[0].value() == "s1");
                CHECK(!v[0].domain().has_value());
                CHECK(!v[0].path().has_value());
            } {
                auto v = jar.by_name("theme");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "dark");
            } {
                auto v = jar.by_name("uid");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "42");
            }


            CHECK(body.str() == "Hello, World!");
            CHECK(body.bytes_transferred() == 13);
            CHECK(body.error() == std::error_code{});

        } else if(counter == 1) {
            CHECK(reenter);

            REQUIRE(w_request.ready());
            REQUIRE(w_route_desc.ready());
            REQUIRE(w_uri.ready());
            REQUIRE(w_jar.ready());
            REQUIRE(w_body.ready());

            const udho::manifold::feature::header_reader::result& request    = w_request;
            const udho::manifold::feature::identifier::result&    route_desc = w_route_desc;
            const udho::manifold::feature::locator::result&       uri        = w_uri;
            const udho::manifold::feature::cookie_load::result&   jar        = w_jar;
            const udho::manifold::feature::body_reader::result&   body       = w_body;

            CHECK(request.method() == boost::beast::http::verb::post);
            CHECK(request.target() == "/f1/hello/world/25/23?name=test&id=42&filter=active");
            CHECK(request[boost::beast::http::field::host] == "example.com");
            CHECK(request[boost::beast::http::field::content_type] == "text/plain");
            CHECK(request[boost::beast::http::field::content_length] == "13");

            CHECK(route_desc.resource() == "/f1/hello/world/25/23");
            {
                const auto& params = route_desc.params();
                auto get1 = [&](const std::string& k) -> std::string {
                    auto it = params.find(k);
                    REQUIRE(it != params.end());
                    return it->second;
                };
                CHECK(get1("name") == "test");
                CHECK(get1("id") == "42");
                CHECK(get1("filter") == "active");
            }

            CHECK(jar.count("session") == 1);
            CHECK(jar.count("theme")   == 1);
            CHECK(jar.count("uid")     == 1);

            {
                auto v = jar.by_name("session");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "s2");
            } {
                auto v = jar.by_name("theme");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "light");
            } {
                auto v = jar.by_name("uid");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "99");
            }

            CHECK(body.str() == "Hello, Earth!");
            CHECK(body.bytes_transferred() == 13);
            CHECK(body.error() == std::error_code{});

        } else if(counter == 2) {
            CHECK(!reenter);
            REQUIRE(!w_request.ready());
        }

        ++counter;
    });

    io_context.run();

    CHECK(counter == 3);
    CHECK(framework.runtime().count() == 0);
}
