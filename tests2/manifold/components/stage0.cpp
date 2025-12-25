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


TEST_CASE("udho manifold pipeline stage 0", "[manifold][pipeline]") {
    using stream_type      = boost::beast::test::stream; // udho::net::types::socket;

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

    SECTION("Single mount point") {
        auto table  = std::move(mount_point1);
        auto router = udho::url::router(std::move(table));

        auto routing = udho::manifold::components::routing(router);

        using routing_component_type     = std::decay_t<decltype(routing)>;
        using protocol_component_type    = udho::manifold::components::protocols::http2<stream_type>;
        using navigator_component_type   = udho::manifold::components::navigators::pretty;

        using composition_type = udho::manifold::composition<
            protocol_component_type,
            navigator_component_type,
            routing_component_type
        >;

        using configs_type = udho::manifold::configs<
            protocol_component_type,
            navigator_component_type,
            routing_component_type
        >;

        using order_type = udho::manifold::order<
            udho::manifold::feature::header_reader,
            udho::manifold::feature::identifier,
            udho::manifold::feature::locator
        >;

        using pipeline_type = udho::manifold::common_pipepine<0, order_type, composition_type>;
        using journal_type  = pipeline_type::full_journal_type;

        auto composition = composition_type::compose(std::move(routing));

        SECTION("Invalid route identifier") {
            journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal};
            boost::asio::io_context io_context;
            std::string request_data =
                "GET /hello/world/23?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            stream_type stream(io_context, request_data);

            pipeline.then([&stream, &pipeline, &journal](std::variant<bool, std::exception_ptr> success){
                // const auto& journal = pipeline.journal();

                CHECK(success.index() == 1);

                CHECK(journal.count<udho::manifold::feature::header_reader>() == 1);
                CHECK(journal.count<udho::manifold::feature::identifier>()    == 1);
                CHECK(journal.count<udho::manifold::feature::locator>()       == 1);

                CHECK(journal.ready<udho::manifold::feature::header_reader>());
                CHECK(journal.ready<udho::manifold::feature::identifier>());
                CHECK(!journal.ready<udho::manifold::feature::locator>());

                const udho::net::types::headers::request& request           = journal.at<udho::manifold::feature::header_reader>();
                const udho::manifold::feature::identifier::result& resource = journal.at<udho::manifold::feature::identifier>();
                // const udho::url::detail::route_index& route                 = journal.at<udho::manifold::feature::locator>();

                std::exception_ptr exptr = std::get<std::exception_ptr>(success);
                try {
                    std::rethrow_exception(exptr);
                } catch(const std::exception& e) {
                    std::cout << "Caught exception: '" << e.what() << "'\n";
                }
            }).eval(stream);
            io_context.run();
        }

        SECTION("Valid route identifier") {
            journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal};
            boost::asio::io_context io_context;
            std::string request_data =
                "GET /f1/hello/world/23/24?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            stream_type stream(io_context, request_data);

            pipeline.then([&stream, &pipeline, &journal](std::variant<bool, std::exception_ptr> success){
                // const auto& journal = pipeline.journal();

                CHECK(success.index() == 0);

                if(success.index() == 1) {
                    std::exception_ptr exptr = std::get<std::exception_ptr>(success);
                    try {
                        std::rethrow_exception(exptr);
                    } catch(const std::exception& e) {
                        std::cout << "Caught (expected) exception: '" << e.what() << "'\n";
                    }
                    return;
                } else {
                    CHECK(std::get<0>(success) == true);
                }

                CHECK(journal.count<udho::manifold::feature::header_reader>() == 1);
                CHECK(journal.count<udho::manifold::feature::identifier>()    == 1);
                CHECK(journal.count<udho::manifold::feature::locator>()       == 1);

                CHECK(journal.ready<udho::manifold::feature::header_reader>());
                CHECK(journal.ready<udho::manifold::feature::identifier>());
                CHECK(journal.ready<udho::manifold::feature::locator>());

                const udho::net::types::headers::request& request           = journal.at<udho::manifold::feature::header_reader>();
                const udho::manifold::feature::identifier::result& resource = journal.at<udho::manifold::feature::identifier>();
                const udho::url::detail::route_index& route                 = journal.at<udho::manifold::feature::locator>();

                CHECK(route.target() == "/f1/hello/world/23/24");
                CHECK(route.mountpoint() == 0);
                CHECK(route.action() == 1);

                return;
            }).eval(stream);
            io_context.run();
        }
    }

    SECTION("Multiple mount points") {
        auto table  = std::move(mount_point1) | std::move(mount_point2) | std::move(mount_point3);
        auto router = udho::url::router(std::move(table));

        auto routing = udho::manifold::components::routing(router);

        using routing_component_type     = std::decay_t<decltype(routing)>;
        using protocol_component_type    = udho::manifold::components::protocols::http2<stream_type>;
        using navigator_component_type   = udho::manifold::components::navigators::pretty;

        using composition_type = udho::manifold::composition<
            protocol_component_type,
            navigator_component_type,
            routing_component_type
        >;

        using configs_type = udho::manifold::configs<
            protocol_component_type,
            navigator_component_type,
            routing_component_type
        >;

        using order_type = udho::manifold::order<
            udho::manifold::feature::header_reader,
            udho::manifold::feature::identifier,
            udho::manifold::feature::locator
        >;

        using pipeline_type = udho::manifold::common_pipepine<0, order_type, composition_type>;
        using journal_type  = pipeline_type::full_journal_type;

        auto composition = composition_type::compose(std::move(routing));

        SECTION("Invalid route identifier") {
            journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal};
            boost::asio::io_context io_context;
            std::string request_data =
                "GET /hello/world/23?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            stream_type stream(io_context, request_data);

            pipeline.then([&stream, &pipeline, &journal](std::variant<bool, std::exception_ptr> success){
                // const auto& journal = pipeline.journal();

                CHECK(success.index() == 1);

                CHECK(journal.count<udho::manifold::feature::header_reader>() == 1);
                CHECK(journal.count<udho::manifold::feature::identifier>()    == 1);
                CHECK(journal.count<udho::manifold::feature::locator>()       == 1);

                CHECK(journal.ready<udho::manifold::feature::header_reader>());
                CHECK(journal.ready<udho::manifold::feature::identifier>());
                CHECK(!journal.ready<udho::manifold::feature::locator>());

                const udho::net::types::headers::request& request           = journal.at<udho::manifold::feature::header_reader>();
                const udho::manifold::feature::identifier::result& resource = journal.at<udho::manifold::feature::identifier>();
                // const udho::url::detail::route_index& route                 = journal.at<udho::manifold::feature::locator>();

                std::exception_ptr exptr = std::get<std::exception_ptr>(success);
                try {
                    std::rethrow_exception(exptr);
                } catch(const std::exception& e) {
                    std::cout << "Caught exception: '" << e.what() << "'\n";
                }
            }).eval(stream);
            io_context.run();
        }

        SECTION("Valid route identifier") {
            journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal};
            boost::asio::io_context io_context;
            std::string request_data =
                "GET /f1/hello/world/23/24?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            stream_type stream(io_context, request_data);

            pipeline.then([&stream, &pipeline, &journal](std::variant<bool, std::exception_ptr> success){
                // const auto& journal = pipeline.journal();

                CHECK(success.index() == 0);

                if(success.index() == 1) {
                    std::exception_ptr exptr = std::get<std::exception_ptr>(success);
                    try {
                        std::rethrow_exception(exptr);
                    } catch(const std::exception& e) {
                        std::cout << "Caught (expected) exception: '" << e.what() << "'\n";
                    }
                    return;
                } else {
                    CHECK(std::get<0>(success) == true);
                }

                CHECK(journal.count<udho::manifold::feature::header_reader>() == 1);
                CHECK(journal.count<udho::manifold::feature::identifier>()    == 1);
                CHECK(journal.count<udho::manifold::feature::locator>()       == 1);

                CHECK(journal.ready<udho::manifold::feature::header_reader>());
                CHECK(journal.ready<udho::manifold::feature::identifier>());
                CHECK(journal.ready<udho::manifold::feature::locator>());

                const udho::net::types::headers::request& request           = journal.at<udho::manifold::feature::header_reader>();
                const udho::manifold::feature::identifier::result& resource = journal.at<udho::manifold::feature::identifier>();
                const udho::url::detail::route_index& route                 = journal.at<udho::manifold::feature::locator>();

                CHECK(route.target() == "/f1/hello/world/23/24");
                CHECK(route.mountpoint() == 0);
                CHECK(route.action() == 1);

                return;
            }).eval(stream);
            io_context.run();
        }

        SECTION("Valid route identifier") {
            journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal};
            boost::asio::io_context io_context;
            std::string request_data =
                "GET /m2/f1/hello/world/23/24?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            stream_type stream(io_context, request_data);

            pipeline.then([&stream, &pipeline, &journal](std::variant<bool, std::exception_ptr> success){
                // const auto& journal = pipeline.journal();

                CHECK(success.index() == 0);

                if(success.index() == 1) {
                    std::exception_ptr exptr = std::get<std::exception_ptr>(success);
                    try {
                        std::rethrow_exception(exptr);
                    } catch(const std::exception& e) {
                        std::cout << "Caught exception: '" << e.what() << "'\n";
                    }
                    return;
                } else {
                    CHECK(std::get<0>(success) == true);
                }

                CHECK(journal.count<udho::manifold::feature::header_reader>() == 1);
                CHECK(journal.count<udho::manifold::feature::identifier>()    == 1);
                CHECK(journal.count<udho::manifold::feature::locator>()       == 1);

                CHECK(journal.ready<udho::manifold::feature::header_reader>());
                CHECK(journal.ready<udho::manifold::feature::identifier>());
                CHECK(journal.ready<udho::manifold::feature::locator>());

                const udho::net::types::headers::request& request           = journal.at<udho::manifold::feature::header_reader>();
                const udho::manifold::feature::identifier::result& resource = journal.at<udho::manifold::feature::identifier>();
                const udho::url::detail::route_index& route                 = journal.at<udho::manifold::feature::locator>();

                CHECK(route.target() == "/m2/f1/hello/world/23/24");
                CHECK(route.mountpoint() == 1);
                CHECK(route.action() == 1);

                return;
            }).eval(stream);
            io_context.run();
        }
    }

}
