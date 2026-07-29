#include <catch2/catch_test_macros.hpp>
#include <udho/url/url.h>
#include <iostream>

#include <udho/net/listener.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/manifold/fabric.h>
#include <curl/curl.h>
#include <udho/www/www.h>
#include <udho/logging/setup.h>

#include "curl.h"

using stream_type = udho::net::types::socket;

using namespace udho::www::components;
using namespace udho::www;

namespace callbacks{

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

using namespace udho::www;
using namespace udho::www::components;

BOOST_SYMBOL_EXPORT void f0(context<cookies> context){
    context << "f0";
    context.finish();
    return;
}

BOOST_SYMBOL_EXPORT int f1(context<navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
    context << std::to_string(a.size()+b.size()+c+d);
    std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
    context.finish();
    return 42;
}

BOOST_SYMBOL_EXPORT std::string f2(context<cookies> context, int a, const std::string& b){
    context << std::to_string(a+b.size());
    context.finish();
    return "hello";
}

BOOST_SYMBOL_EXPORT std::string f_nodef(context<> context, nodef, int a){
    context << std::to_string(a);
    context.finish();
    return "hello";
}

struct X{
    BOOST_SYMBOL_EXPORT void f0(context<cookies> context){
        context << "f0";
        context.finish();
        return;
    }

    BOOST_SYMBOL_EXPORT int f1(context<navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
        context << std::to_string(a.size()+b.size()+c+d);
        std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
        context.finish();
        return 42;
    }

    BOOST_SYMBOL_EXPORT std::string f2(context<cookies> context, int a, const std::string& b){
        context << std::to_string(a+b.size());
        context.finish();
        return "hello";
    }

    BOOST_SYMBOL_EXPORT int f3(context<> context, int a, const std::string& b, const double& c, bool d) const{
        context << std::to_string(84);
        context.finish();
        return 0;
    }
};

}

constexpr std::string_view MOUNT_BEGIN = "<!-- begin udho::mark::mount -->";
constexpr std::string_view MOUNT_END   = "<!-- end udho::mark::mount -->";
constexpr std::string_view POINT_BEGIN = "<!-- begin udho::mark::point -->";
constexpr std::string_view POINT_END   = "<!-- end udho::mark::point -->";
constexpr std::string_view ROUTE_BEGIN = "<!-- begin udho::mark::route -->";
constexpr std::string_view ROUTE_END   = "<!-- end udho::mark::route -->";

// Extract the substring between two markers (markers excluded)
std::string extract_between(const std::string& src, udho::utils::string_view start, udho::utils::string_view end) {
    auto pos = src.find(start);
    if (pos == std::string::npos) return {};
    pos += start.size();
    auto end_pos = src.find(end, pos);
    if (end_pos == std::string::npos) return {};
    return src.substr(pos, end_pos - pos);
}

struct ExpectedRoute {
    std::string method;
    std::string pattern;
    std::string replacement;
    std::string label;
    int args;
    std::string format;
};

// Expected routes for the two mount points
const std::vector<ExpectedRoute> expected_chain = {
    {"GET", R"(/)", R"(/)", "f0", 0, "home"},
    {"GET", R"(/f1/(\w+)/(\w+)/(\d+)/(\d+))", R"(/f1/{}/{}/{})", "f1", 4, "regex"},
    {"GET", R"(/f2-(\d+)/(\w+))", R"(/f2-{}/{})", "f2", 2, "regex"},
    {"GET", R"(/x/f0)", R"(/x/f0)", "xf0", 0, "fixed"},
    {"GET", R"(/x/f1/(\w+)/(\w+)/(\d+)/(\d+))", R"(/x/f1/{}/{}/{})", "xf1", 4, "regex"}
};

const std::vector<ExpectedRoute> expected_root = {
    {"GET", R"(/)", R"(/)", "f0", 0, "home"},
    {"GET", R"(/f1/(\w+)/(\w+)/(\d+)/(\d+))", R"(/f1/{}/{}/{})", "f1", 4, "regex"},
    {"GET", R"(/f2-(\d+)/(\w+))", R"(/f2-{}/{})", "f2", 2, "regex"},
    {"GET", R"(/x/f0)", R"(/x/f0)", "xf0", 0, "fixed"},
    {"GET", R"(/x/f1/(\w+)/(\w+)/(\d+)/(\d+))", R"(/x/f1/{}/{}/{})", "xf1", 4, "regex"},
    {"GET", R"(/x/f2-(\d+)/(\w+))", R"(/x/f2-{}/{})", "xf2", 0, "regex"},
    {"GET", R"(/x/f3/(\w+)/(\w+)/(\d+)/(\d+))", R"(/x/f3/{}/{}/{})", "xf3", 4, "regex"}
};

const std::pair<std::string, std::string> mount_expect[] = {
    {"chain", "/pchain"},
    {"root", "/"}
};

using logger_type = udho::logging::setup<udho::logging::fixed_file>;

TEST_CASE("URL routes listing", "[url][routing][listing]") {
    // if(!logger_type::apply()) return;

    // assert(logger_type::running());

    using namespace udho::hazo::string::literals;

    callbacks::X x;
    auto chain =
        udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)         << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")                       |
        udho::url::slot("xf0"_h, &callbacks::X::f0, &x)  << udho::url::fixed(udho::url::verb::get, "/x/f0", "/x/f0")                                      |
        udho::url::slot("xf1"_h, &callbacks::X::f1, &x)  << udho::url::regx(udho::url::verb::get,  "/x/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f1/{}/{}/{}");
    auto chain2 =
        udho::url::regx(udho::url::verb::get, "/x/f2-(\\d+)/(\\w+)", "/x/f2-{}/{}")                  >> udho::url::slot("xf2"_h, &callbacks::X::f0, &x)  |
        udho::url::regx(udho::url::verb::get, "/x/f3/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f3/{}/{}/{}") >> udho::url::slot("xf3"_h, &callbacks::X::f1, &x);

    auto chain3 = chain | chain2;

    auto mount_point = udho::url::mount("chain"_h, "/pchain", std::move(chain));
    auto chain4 = std::move(mount_point) | udho::url::mount("root"_h, "/", std::move(chain3));

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

    SECTION("Routes summary") {
        udho::url::summary::router summary = router.summary();

        REQUIRE(summary.size() == 2);

        const std::vector<ExpectedRoute>* expected_vec[] = {&expected_chain, &expected_root};

        std::size_t mount_idx = 0;
        for (const auto& [name, mp] : summary) {
            REQUIRE(mount_idx < 2);

            // Check mount identity
            CHECK(name         == mount_expect[mount_idx].first);
            CHECK(mp.path()    == mount_expect[mount_idx].second);

            const auto& expected_routes = *expected_vec[mount_idx];
            REQUIRE(mp.size() == expected_routes.size());

            std::size_t route_idx = 0;
            for (const auto& [key, action] : mp) {
                const auto& exp = expected_routes[route_idx];

                // Slot key must match the expected label
                CHECK(key == exp.label);
                CHECK(action.slot().key() == exp.label);

                // Slot argument count: displayed args = nargs‑1
                CHECK(action.slot().nargs() == static_cast<std::uint8_t>(exp.args + 1));

                // Match details
                const auto& match = action.match();
                CHECK(match.method()      == exp.method);
                CHECK(match.pattern()     == exp.pattern);
                CHECK(match.replacement() == exp.replacement);
                CHECK(match.format()      == exp.format);

                ++route_idx;
            }
            ++mount_idx;
        }
    }

    using framework_type = udho::www::framework<udho::www::stateless::lua>;
    using endpoint_type  = typename framework_type::endpoint_type;

    auto resource_store_component  = udho::www::components::resources(cstore);

    auto framework = framework_type::apply(std::move(router));
    auto runtime   = framework.runtime(resource_store_component);

    lua.bind(udho::view::data::type<std::decay_t<decltype(runtime)>::portal_type>{});
    lua.bind(udho::view::data::type<std::decay_t<decltype(runtime)>::context_type>{});

    auto listener  = udho::net::listener(service, runtime, {boost::asio::ip::tcp::v4(), 9000});

    listener.start();

    std::thread thread([&]{
        service.run();
    });

    CURL* curl;
    curl = curl_easy_init();
    CHECK(curl != 0x0);


    SECTION("Route Listing on 404") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/NON_EXISTENT_RESOURCE");
        CHECK(results.code == 404);
        std::string body = results.body;

        std::size_t mount_count = 0;
        std::string remaining = body;
        while (true) {
            std::string mount_block = extract_between(remaining, MOUNT_BEGIN, MOUNT_END);
            if (mount_block.empty()) break;

            // Advance remaining past the current mount block
            auto end_pos = remaining.find(MOUNT_END);
            if (end_pos == std::string::npos) break;
            remaining = remaining.substr(end_pos + MOUNT_END.size());

            REQUIRE(mount_count < 2);

            // ---- Verify mount header using the POINT markers ----
            std::string point_block = extract_between(mount_block, POINT_BEGIN, POINT_END);
            CHECK(!point_block.empty());
            // The expected label and path must appear inside the point block
            CHECK(point_block.find(mount_expect[mount_count].first) != std::string::npos);
            CHECK(point_block.find(mount_expect[mount_count].second) != std::string::npos);

            // ---- Verify route entries ----
            const auto& expected_routes = (mount_count == 0) ? expected_chain : expected_root;
            std::size_t route_index = 0;
            std::string routes_part = mount_block;   // routes are somewhere inside the mount block

            while (true) {
                std::string route_block = extract_between(routes_part, ROUTE_BEGIN, ROUTE_END);
                if (route_block.empty()) break;

                auto route_end = routes_part.find(ROUTE_END);
                if (route_end == std::string::npos) break;
                routes_part = routes_part.substr(route_end + ROUTE_END.size());

                REQUIRE(route_index < expected_routes.size());
                const auto& exp = expected_routes[route_index];

                // Every expected value must appear as plain text inside the route block
                CHECK(route_block.find(exp.method) != std::string::npos);
                CHECK(route_block.find(exp.pattern) != std::string::npos);
                CHECK(route_block.find(exp.replacement) != std::string::npos);
                CHECK(route_block.find(exp.label) != std::string::npos);
                CHECK(route_block.find(std::to_string(exp.args)) != std::string::npos);

                ++route_index;
            }
            // Exact count of routes for this mount
            CHECK(route_index == expected_routes.size());

            ++mount_count;
        }
        // Total mount count
        CHECK(mount_count == 2);
    }

    curl_easy_cleanup(curl);

    listener.stop();
    thread.join();

    logger_type::stop();
}
