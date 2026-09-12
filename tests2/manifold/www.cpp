#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/url/url.h>
#include <udho/www/www.h>
#include <udho/net/listener.h>
#include <udho/manifold/visualize.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace udho{
namespace net{

template <>
struct basic_ostream_timeout<boost::beast::test::stream> {
    inline static constexpr auto value = std::chrono::milliseconds{20};
};

}
}

namespace callbacks{

    using namespace udho::www;
    // using namespace udho::manifold::www;
    using namespace udho::www::components;

    struct nodef{
        nodef() = delete;
        nodef(int) {}
    };

    static void f0(context<cookies> context){
        context << "f0";
        context.finish();
        return;
    }

    static int f1(context<navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
        context << std::to_string(a.size()+b.size()+c+d);
        std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
        context.finish();
        return 42;
    }

    static std::string f2(context<cookies> context, int a, const std::string& b){
        context << std::to_string(a+b.size());
        context.finish();
        return "hello";
    }

    static std::string f_nodef(context<> context, nodef, int a){
        context << std::to_string(a);
        context.finish();
        return "hello";
    }
}

namespace test_callbacks{
    using stream_type           = boost::beast::test::stream;
    using context_type          = udho::www::basic_context<stream_type, udho::www::components::cookies>;
    using query_context_type    = udho::www::basic_context<stream_type, udho::www::components::navigators::pretty, udho::www::components::cookies>;

    struct invocation {
        int id;
        std::string path;
        std::string extension;
        udho::www::feature::identifier::result::query_params_type params;
    };

    static std::vector<invocation> invocations;

    static void show(context_type context, int id) {
        invocations.push_back({id, {}, {}, {}});
        context << std::to_string(id);
        context.finish();
    }

    static void inspect(query_context_type context, int id) {
        const auto& query = context.portal().query();
        invocations.push_back({id, query.path(), query.extension(), query.params()});
        context << std::to_string(id) << ":" << query.path() << ":" << query.extension();
        context.finish();
    }
}

namespace www_test_callbacks{
    using stream_type       = boost::beast::test::stream;
    using protocol_type     = udho::www::components::protocols::http<stream_type>;
    using post_context_type = udho::www::basic_context<stream_type, protocol_type>;

    struct body_invocation {
        std::string body;
        std::size_t bytes;
        bool contiguous;
    };

    static std::vector<body_invocation> body_invocations;

    static void consume_body(post_context_type context) {
        const auto& body = context.portal().body();
        body_invocations.push_back({body.str(), body.bytes_transferred(), body.contiguous()});
        context << body.str();
        context.finish();
    }

    struct form_invocation {
        std::string value;
        std::size_t fields;
    };

    static std::vector<form_invocation> form_invocations;

    static void consume_form(post_context_type context) {
        const auto& fields = context.portal().body().form().fields();
        const auto found = fields.find("name");
        if(found == fields.end()) {
            throw std::runtime_error{"expected form field name"};
        }

        form_invocations.push_back({found->second.string(), fields.size()});
        context << found->second.string();
        context.finish();
    }

    using action_context_type = udho::www::basic_context<stream_type>;

    static std::vector<std::string> exception_invocations;
    static std::vector<std::string> exception_bodies;

    static void clear_exception_state() {
        exception_invocations.clear();
        exception_bodies.clear();
    }

    static void record_exception_body(const post_context_type& context) {
        exception_bodies.push_back(context.portal().body().str());
    }

    static void ok(action_context_type context, int id) {
        exception_invocations.push_back("ok:" + std::to_string(id));
        context << "ok:" << std::to_string(id);
        context.finish();
    }

    static void stall(action_context_type) {
        exception_invocations.push_back("stall");
    }

    static void write_then_stall(action_context_type context) {
        exception_invocations.push_back("write-then-stall");
        context << "partial-response";
    }

    static auto timeout_router() {
        using namespace udho::hazo::string::literals;

        auto actions =
            udho::url::slot("ok"_h,               &ok)               << udho::url::regx(udho::url::verb::get, "/ok/(\\d+)", "/ok/{}") |
            udho::url::slot("stall"_h,            &stall)            << udho::url::fixed(udho::url::verb::get, "/stall", "/stall") |
            udho::url::slot("write-then-stall"_h, &write_then_stall) << udho::url::fixed(udho::url::verb::get, "/write-then-stall", "/write-then-stall")
        ;
        return udho::url::router(udho::url::mount("root"_h, "/", std::move(actions)));
    }

    static void throw_http_keep_alive(post_context_type context) {
        record_exception_body(context);
        exception_invocations.push_back("http:keep-alive");
        throw udho::http::error{
            boost::beast::http::status::unprocessable_entity,
            "unprocessable test request",
            udho::http::error::options::keep_alive
        };
    }

    static void throw_http_close(post_context_type context) {
        record_exception_body(context);
        exception_invocations.push_back("http:close");
        throw udho::http::error{
            boost::beast::http::status::unprocessable_entity,
            "unprocessable test request",
            udho::http::error::options::close
        };
    }

    static void throw_not_found_keep_alive(post_context_type context) {
        record_exception_body(context);
        exception_invocations.push_back("not-found:keep-alive");
        throw udho::http::error{
            boost::beast::http::status::not_found,
            "user generated not found",
            udho::http::error::options::keep_alive
        };
    }

    static void throw_not_found_close(post_context_type context) {
        record_exception_body(context);
        exception_invocations.push_back("not-found:close");
        throw udho::http::error{
            boost::beast::http::status::not_found,
            "user generated not found",
            udho::http::error::options::close
        };
    }

    static void throw_cpp(post_context_type context) {
        record_exception_body(context);
        exception_invocations.push_back("cpp");
        throw std::runtime_error{"ordinary test exception"};
    }

    static auto error_router() {
        using namespace udho::hazo::string::literals;

        auto actions =
            udho::url::slot("ok"_h,                   &ok)                         << udho::url::regx(udho::url::verb::get, "/ok/(\\d+)", "/ok/{}") |
            udho::url::slot("http-keep-alive"_h,      &throw_http_keep_alive)      << udho::url::fixed(udho::url::verb::post, "/throw/http/keep-alive", "/throw/http/keep-alive") |
            udho::url::slot("http-close"_h,           &throw_http_close)           << udho::url::fixed(udho::url::verb::post, "/throw/http/close", "/throw/http/close") |
            udho::url::slot("not-found-keep-alive"_h, &throw_not_found_keep_alive) << udho::url::fixed(udho::url::verb::post, "/throw/not-found/keep-alive", "/throw/not-found/keep-alive") |
            udho::url::slot("not-found-close"_h,      &throw_not_found_close)      << udho::url::fixed(udho::url::verb::post, "/throw/not-found/close", "/throw/not-found/close") |
            udho::url::slot("cpp"_h,                  &throw_cpp)                  << udho::url::fixed(udho::url::verb::post, "/throw/cpp", "/throw/cpp")
        ;
        return udho::url::router(udho::url::mount("root"_h, "/", std::move(actions)));
    }
}

namespace www_test{
    struct response {
        std::string body;
        std::size_t active_flows;
    };

    template <typename RouterT>
    response execute_minimal(RouterT&& router, const std::string& request) {
        boost::asio::io_context io;
        udho::view::resources::store<> store;
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type     = udho::www::test<udho::www::tags::minimal<>>;
        using framework_type = udho::www::framework<label_type>;

        auto framework = framework_type::apply(std::forward<RouterT>(router));
        auto runtime   = framework.runtime(resources);

        boost::beast::test::stream request_stream(io, request);
        boost::beast::test::stream response_stream(io);
        request_stream.connect(response_stream);
        response_stream.close();

        auto& flow = runtime.spawn(std::move(request_stream));
        flow.start();
        io.run();

        return {response_stream.str(), runtime.count()};
    }

    template <typename LabelT, typename RouterT, typename ResourcesT, typename SetupT>
    response execute(RouterT&& router, ResourcesT& resources, const std::string& request, SetupT&& setup) {
        boost::asio::io_context io;
        using framework_type = udho::www::framework<LabelT>;

        auto framework = framework_type::apply(std::forward<RouterT>(router));
        auto runtime   = framework.runtime(resources);
        setup(runtime);

        boost::beast::test::stream request_stream(io, request);
        boost::beast::test::stream response_stream(io);
        request_stream.connect(response_stream);
        response_stream.close();

        auto& flow = runtime.spawn(std::move(request_stream));
        flow.start();
        io.run();

        return {response_stream.str(), runtime.count()};
    }

    inline std::string get_request(const std::string& target) {
        return "GET " + target + " HTTP/1.1\r\n"
               "Host: example.com\r\n"
               "\r\n";
    }

    inline std::string post_request(const std::string& target, const std::string& body) {
        return "POST " + target + " HTTP/1.1\r\n"
               "Host: example.com\r\n"
               "Content-Type: text/plain\r\n"
               "Content-Length: " + std::to_string(body.size()) + "\r\n"
               "\r\n" + body;
    }

    inline std::string status_marker(unsigned int status) {
        return "HTTP/1.1 " + std::to_string(status);
    }

    inline std::string concatenate_requests(std::initializer_list<std::string> messages) {
        std::string request_stream;
        for(const auto& request : messages) {
            request_stream += request;
        }
        return request_stream;
    }

    inline void check_statuses(const std::string& response, std::initializer_list<unsigned int> statuses) {
        constexpr const char* response_marker = "HTTP/1.1 ";
        std::size_t response_count = 0;
        std::size_t response_offset = 0;
        while((response_offset = response.find(response_marker, response_offset)) != std::string::npos) {
            ++response_count;
            response_offset += std::char_traits<char>::length(response_marker);
        }

        CHECK(response_count == statuses.size());

        std::size_t offset = 0;
        for(const auto status : statuses) {
            const auto found = response.find(status_marker(status), offset);
            INFO("expected ordered status " << status << " after offset " << offset);
            CHECK(found != std::string::npos);
            if(found != std::string::npos) {
                offset = found + status_marker(status).size();
            }
        }
    }

    template <typename LabelT, typename ResourcesT, typename SetupT>
    void check_exception_cases(ResourcesT& resources, SetupT&& setup) {
        auto execute_case = [&](const std::string& request) {
            return execute<LabelT>(
                www_test_callbacks::error_router(),
                resources,
                request,
                setup
            );
        };

        SECTION("framework exception before an action") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_case(get_request("/ok/1?bad=%ZZ"));

            CHECK(www_test_callbacks::exception_invocations.empty());
            CHECK(www_test_callbacks::exception_bodies.empty());
            CHECK(result.body.find(status_marker(500)) != std::string::npos);
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("action throws a keep-alive HTTP status exception") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_case(post_request("/throw/http/keep-alive", "keep-alive-body"));

            REQUIRE(www_test_callbacks::exception_invocations == std::vector<std::string>{"http:keep-alive"});
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"keep-alive-body"});
            CHECK(result.body.find(status_marker(422)) != std::string::npos);
            CHECK(result.body.find("unprocessable test request") != std::string::npos);
            CHECK(result.body.find("Connection: keep-alive") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("action throws a closing HTTP status exception") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_case(post_request("/throw/http/close", "close-body"));

            REQUIRE(www_test_callbacks::exception_invocations == std::vector<std::string>{"http:close"});
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"close-body"});
            CHECK(result.body.find(status_marker(422)) != std::string::npos);
            CHECK(result.body.find("unprocessable test request") != std::string::npos);
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("action throws a standard C++ exception") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_case(post_request("/throw/cpp", "cpp-body"));

            REQUIRE(www_test_callbacks::exception_invocations == std::vector<std::string>{"cpp"});
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"cpp-body"});
            CHECK(result.body.find(status_marker(500)) != std::string::npos);
            CHECK(result.body.find("ordinary test exception") != std::string::npos);
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

    }

    template <typename LabelT, typename ResourcesT, typename SetupT>
    void check_error_sequences(ResourcesT& resources, SetupT&& setup) {
        auto execute_sequence = [&](std::initializer_list<std::string> requests) {
            return execute<LabelT>(www_test_callbacks::error_router(), resources, concatenate_requests(requests), setup);
        };

        SECTION("framework 404 as the first response closes before both following requests") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                post_request("/missing", "unread-body"),
                get_request("/ok/1"),
                get_request("/ok/2")
            });

            CHECK(www_test_callbacks::exception_invocations.empty());
            CHECK(www_test_callbacks::exception_bodies.empty());
            check_statuses(result.body, {404});
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("framework 404 as the second response closes before the final request") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                get_request("/ok/1"),
                post_request("/missing", "unread-body"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"ok:1"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies.empty());
            check_statuses(result.body, {200, 404});
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("standard exception as the first response closes before both following requests") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                post_request("/throw/cpp", "cpp-first"),
                get_request("/ok/1"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"cpp"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"cpp-first"});
            check_statuses(result.body, {500});
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("standard exception as the second response closes before the final request") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                get_request("/ok/1"),
                post_request("/throw/cpp", "cpp-second"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"ok:1", "cpp"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"cpp-second"});
            check_statuses(result.body, {200, 500});
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("user 404 with keep-alive as the first response re-enters for both following requests") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                post_request("/throw/not-found/keep-alive", "keep-alive-first"),
                get_request("/ok/1"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"not-found:keep-alive", "ok:1", "ok:2"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"keep-alive-first"});
            check_statuses(result.body, {404, 200, 200});
            CHECK(result.body.find("Connection: keep-alive") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("user 404 with keep-alive as the second response re-enters for the final request") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                get_request("/ok/1"),
                post_request("/throw/not-found/keep-alive", "keep-alive-second"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"ok:1", "not-found:keep-alive", "ok:2"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"keep-alive-second"});
            check_statuses(result.body, {200, 404, 200});
            CHECK(result.body.find("Connection: keep-alive") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("user 404 with close as the first response closes before both following requests") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                post_request("/throw/not-found/close", "close-first"),
                get_request("/ok/1"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"not-found:close"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"close-first"});
            check_statuses(result.body, {404});
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }

        SECTION("user 404 with close as the second response closes before the final request") {
            www_test_callbacks::clear_exception_state();
            const auto result = execute_sequence({
                get_request("/ok/1"),
                post_request("/throw/not-found/close", "close-second"),
                get_request("/ok/2")
            });
            const std::vector<std::string> expected{"ok:1", "not-found:close"};

            REQUIRE(www_test_callbacks::exception_invocations == expected);
            CHECK(www_test_callbacks::exception_bodies == std::vector<std::string>{"close-second"});
            check_statuses(result.body, {200, 404});
            CHECK(result.body.find("Connection: close") != std::string::npos);
            CHECK(result.active_flows == 0);
        }
    }
}

TEST_CASE("udho www framework dispatches through a Beast test stream", "[www][test-stream]") {
    using namespace udho::hazo::string::literals;

    test_callbacks::invocations.clear();

    auto actions =
        udho::url::slot("show"_h, &test_callbacks::show) <<
        udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}")
    ;
    auto table  = udho::url::mount("root"_h, "/", std::move(actions));
    auto router = udho::url::router(std::move(table));

    boost::asio::io_context io;
    udho::view::resources::store<> store;
    store.lock();
    udho::view::resources::const_store<> resources{store};

    using label_type     = udho::www::test<udho::www::tags::minimal<>>;
    using framework_type = udho::www::framework<label_type>;

    auto framework = framework_type::apply(std::move(router));
    auto runtime   = framework.runtime(resources);

    boost::beast::test::stream request_stream(io,
        "GET /projects/42 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n");
    boost::beast::test::stream response_stream(io);
    request_stream.connect(response_stream);
    response_stream.close();

    auto& flow = runtime.spawn(std::move(request_stream));
    flow.start();
    io.run();

    REQUIRE(test_callbacks::invocations.size() == 1);
    CHECK(test_callbacks::invocations[0].id == 42);
    CHECK(response_stream.str().find("HTTP/1.1 200 OK") != std::string::npos);
    CHECK(response_stream.str().find("42") != std::string::npos);
    CHECK(runtime.count() == 0);
}

TEST_CASE("udho www framework dispatches mounted extension routes with query parameters", "[www][test-stream][routing]") {
    using namespace udho::hazo::string::literals;

    test_callbacks::invocations.clear();

    auto actions =
        udho::url::slot("inspect"_h, &test_callbacks::inspect) <<
        udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}")
    ;
    auto table  = udho::url::mount("api"_h, "/api", std::move(actions));
    auto router = udho::url::router(std::move(table));

    boost::asio::io_context io;
    udho::view::resources::store<> store;
    store.lock();
    udho::view::resources::const_store<> resources{store};

    using label_type     = udho::www::test<udho::www::tags::minimal<>>;
    using framework_type = udho::www::framework<label_type>;

    auto framework = framework_type::apply(std::move(router));
    auto runtime   = framework.runtime(resources);

    boost::beast::test::stream request_stream(io,
        "GET /api/projects/73.html?mode=full&tag=one&tag=two HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n");
    boost::beast::test::stream response_stream(io);
    request_stream.connect(response_stream);
    response_stream.close();

    auto& flow = runtime.spawn(std::move(request_stream));
    flow.start();
    io.run();

    REQUIRE(test_callbacks::invocations.size() == 1);
    const auto& invocation = test_callbacks::invocations[0];
    CHECK(invocation.id == 73);
    CHECK(invocation.path == "/api/projects/73.html");
    CHECK(invocation.extension == "html");
    CHECK(invocation.params.count("mode") == 1);
    CHECK(invocation.params.find("mode")->second == "full");
    CHECK(invocation.params.count("tag") == 2);
    CHECK(response_stream.str().find("HTTP/1.1 200 OK") != std::string::npos);
    CHECK(runtime.count() == 0);
}

TEST_CASE("udho www framework rematches captures for sequential requests", "[www][test-stream][routing][reentry]") {
    using namespace udho::hazo::string::literals;

    test_callbacks::invocations.clear();

    auto actions =
        udho::url::slot("inspect"_h, &test_callbacks::inspect) <<
        udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}")
    ;
    auto table  = udho::url::mount("root"_h, "/", std::move(actions));
    auto router = udho::url::router(std::move(table));

    boost::asio::io_context io;
    udho::view::resources::store<> store;
    store.lock();
    udho::view::resources::const_store<> resources{store};

    using label_type     = udho::www::test<udho::www::tags::minimal<>>;
    using framework_type = udho::www::framework<label_type>;

    auto framework = framework_type::apply(std::move(router));
    auto runtime   = framework.runtime(resources);

    boost::beast::test::stream request_stream(io,
        "GET /projects/1.html?request=first HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n"
        "GET /projects/2.json?request=second HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n");
    boost::beast::test::stream response_stream(io);
    request_stream.connect(response_stream);
    response_stream.close();

    auto& flow = runtime.spawn(std::move(request_stream));
    flow.start();
    io.run();

    REQUIRE(test_callbacks::invocations.size() == 2);
    CHECK(test_callbacks::invocations[0].id == 1);
    CHECK(test_callbacks::invocations[0].extension == "html");
    CHECK(test_callbacks::invocations[0].params.find("request")->second == "first");
    CHECK(test_callbacks::invocations[1].id == 2);
    CHECK(test_callbacks::invocations[1].extension == "json");
    CHECK(test_callbacks::invocations[1].params.find("request")->second == "second");

    const std::string response = response_stream.str();
    const std::size_t first_response  = response.find("HTTP/1.1 200 OK");
    const std::size_t second_response = response.find("HTTP/1.1 200 OK", first_response + 1);
    CHECK(first_response != std::string::npos);
    CHECK(second_response != std::string::npos);
    CHECK(runtime.count() == 0);
}

TEST_CASE("udho www framework reconfigures route options and resets them on reentry", "[www][test-stream][routing][config][reentry]") {
    using namespace udho::hazo::string::literals;
    namespace protocol = udho::www::params::protocol;

    www_test_callbacks::body_invocations.clear();

    auto actions =
        udho::url::slot("segmented"_h, &www_test_callbacks::consume_body) <<
        udho::url::fixed(udho::url::verb::post, "/segmented", "/segmented").options(protocol::contiguous_buffer(false)) |
        udho::url::slot("baseline"_h, &www_test_callbacks::consume_body) <<
        udho::url::fixed(udho::url::verb::post, "/baseline", "/baseline")
    ;
    auto table  = udho::url::mount("root"_h, "/", std::move(actions));
    auto router = udho::url::router(std::move(table));

    boost::asio::io_context io;
    udho::view::resources::store<> store;
    store.lock();
    udho::view::resources::const_store<> resources{store};

    using label_type     = udho::www::test<udho::www::tags::minimal<>>;
    using framework_type = udho::www::framework<label_type>;

    auto framework = framework_type::apply(std::move(router));
    auto runtime   = framework.runtime(resources);

    boost::beast::test::stream request_stream(io,
        "POST /segmented HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "first"
        "POST /baseline HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 6\r\n"
        "\r\n"
        "second");
    boost::beast::test::stream response_stream(io);
    request_stream.connect(response_stream);
    response_stream.close();

    auto& flow = runtime.spawn(std::move(request_stream));
    flow.start();
    io.run();

    REQUIRE(www_test_callbacks::body_invocations.size() == 2);
    CHECK(www_test_callbacks::body_invocations[0].body == "first");
    CHECK(www_test_callbacks::body_invocations[0].bytes == 5);
    CHECK_FALSE(www_test_callbacks::body_invocations[0].contiguous);
    CHECK(www_test_callbacks::body_invocations[1].body == "second");
    CHECK(www_test_callbacks::body_invocations[1].bytes == 6);
    CHECK(www_test_callbacks::body_invocations[1].contiguous);

    const std::string response = response_stream.str();
    const std::size_t first_response = response.find("HTTP/1.1 200 OK");
    CHECK(first_response != std::string::npos);
    CHECK(response.find("HTTP/1.1 200 OK", first_response + 1) != std::string::npos);
    CHECK(runtime.count() == 0);
}

TEST_CASE("udho www framework consumes post bodies within configured limits", "[www][test-stream][body][limits]") {
    using namespace udho::hazo::string::literals;
    namespace protocol = udho::www::params::protocol;

    SECTION("accepts a plain body at its route memory limit") {
        www_test_callbacks::body_invocations.clear();

        auto actions =
            udho::url::slot("plain"_h, &www_test_callbacks::consume_body) <<
            udho::url::fixed(udho::url::verb::post, "/plain", "/plain").options(protocol::body_memory_limit(5))
        ;
        auto table = udho::url::mount("root"_h, "/", std::move(actions));

        const auto result = www_test::execute_minimal(
            udho::url::router(std::move(table)),
            "POST /plain HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "hello"
        );

        REQUIRE(www_test_callbacks::body_invocations.size() == 1);
        CHECK(www_test_callbacks::body_invocations[0].body == "hello");
        CHECK(www_test_callbacks::body_invocations[0].bytes == 5);
        CHECK(result.body.find("HTTP/1.1 200 OK") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("rejects a plain body above its route memory limit") {
        www_test_callbacks::body_invocations.clear();

        auto actions =
            udho::url::slot("plain"_h, &www_test_callbacks::consume_body) <<
            udho::url::fixed(udho::url::verb::post, "/plain", "/plain").options(protocol::body_memory_limit(4))
        ;
        auto table = udho::url::mount("root"_h, "/", std::move(actions));

        const auto result = www_test::execute_minimal(
            udho::url::router(std::move(table)),
            "POST /plain HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "hello"
        );

        CHECK(www_test_callbacks::body_invocations.empty());
        CHECK(result.body.find("HTTP/1.1 413 Payload Too Large") != std::string::npos);
        CHECK(result.body.find("Connection: close") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("accepts a chunked body at its route memory limit") {
        www_test_callbacks::body_invocations.clear();

        auto actions =
            udho::url::slot("chunked"_h, &www_test_callbacks::consume_body) <<
            udho::url::fixed(udho::url::verb::post, "/chunked", "/chunked").options(protocol::body_memory_limit(5))
        ;
        auto table = udho::url::mount("root"_h, "/", std::move(actions));

        const auto result = www_test::execute_minimal(
            udho::url::router(std::move(table)),
            "POST /chunked HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: text/plain\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "hello\r\n"
            "0\r\n"
            "\r\n"
        );

        REQUIRE(www_test_callbacks::body_invocations.size() == 1);
        CHECK(www_test_callbacks::body_invocations[0].body == "hello");
        CHECK(www_test_callbacks::body_invocations[0].bytes == 5);
        CHECK(result.body.find("HTTP/1.1 200 OK") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("rejects a chunked body above its route memory limit") {
        www_test_callbacks::body_invocations.clear();

        auto actions =
            udho::url::slot("chunked"_h, &www_test_callbacks::consume_body) <<
            udho::url::fixed(udho::url::verb::post, "/chunked", "/chunked").options(protocol::body_memory_limit(4))
        ;
        auto table = udho::url::mount("root"_h, "/", std::move(actions));

        const auto result = www_test::execute_minimal(
            udho::url::router(std::move(table)),
            "POST /chunked HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: text/plain\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "hello\r\n"
            "0\r\n"
            "\r\n"
        );

        CHECK(www_test_callbacks::body_invocations.empty());
        CHECK(result.body.find("HTTP/1.1 413 Payload Too Large") != std::string::npos);
        CHECK(result.body.find("Connection: close") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("accepts a urlencoded field at its route field limit") {
        www_test_callbacks::form_invocations.clear();

        auto actions =
            udho::url::slot("form"_h, &www_test_callbacks::consume_form) <<
            udho::url::fixed(udho::url::verb::post, "/form", "/form").options(
                protocol::body_memory_limit(10),
                protocol::field_memory_limit(5)
            )
        ;
        auto table = udho::url::mount("root"_h, "/", std::move(actions));

        const auto result = www_test::execute_minimal(
            udho::url::router(std::move(table)),
            "POST /form HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: 10\r\n"
            "\r\n"
            "name=abcde"
        );

        REQUIRE(www_test_callbacks::form_invocations.size() == 1);
        CHECK(www_test_callbacks::form_invocations[0].value == "abcde");
        CHECK(www_test_callbacks::form_invocations[0].fields == 1);
        CHECK(result.body.find("HTTP/1.1 200 OK") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("rejects a urlencoded field above its route field limit") {
        www_test_callbacks::form_invocations.clear();

        auto actions =
            udho::url::slot("form"_h, &www_test_callbacks::consume_form) <<
            udho::url::fixed(udho::url::verb::post, "/form", "/form").options(
                protocol::body_memory_limit(10),
                protocol::field_memory_limit(4)
            )
        ;
        auto table = udho::url::mount("root"_h, "/", std::move(actions));

        const auto result = www_test::execute_minimal(
            udho::url::router(std::move(table)),
            "POST /form HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: 10\r\n"
            "\r\n"
            "name=abcde"
        );

        CHECK(www_test_callbacks::form_invocations.empty());
        CHECK(result.body.find("HTTP/1.1 413 Payload Too Large") != std::string::npos);
        CHECK(result.body.find("Connection: close") != std::string::npos);
        CHECK(result.active_flows == 0);
    }
}

TEST_CASE("udho www framework renders exceptions across resource stores", "[www][test-stream][error][exceptions]") {
    SECTION("Populated store without a view bridge") {
        udho::view::resources::store<> store;
        udho::pages::system::setup(store);
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type = udho::www::test<udho::www::tags::minimal<>>;
        auto setup = [](auto&) {};
        www_test::check_exception_cases<label_type>(resources, setup);
    }

    SECTION("Populated store with a Lua view bridge") {
        udho::view::data::bridges::lua lua;
        lua.init();
        udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
        udho::pages::system::setup(store);
        store.lock();
        udho::view::resources::const_store<udho::view::data::bridges::lua> resources{store};

        using label_type = udho::www::test<
            udho::www::tags::minimal<udho::view::data::bridges::lua>
        >;
        auto setup = [&lua](auto& runtime) {
            using runtime_type = std::decay_t<decltype(runtime)>;
            lua.bind(udho::view::data::type<typename runtime_type::portal_type>{});
            lua.bind(udho::view::data::type<typename runtime_type::context_type>{});
        };
        www_test::check_exception_cases<label_type>(resources, setup);
    }

    SECTION("Empty store without a view bridge") {
        udho::view::resources::store<> store;
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type = udho::www::test<udho::www::tags::minimal<>>;
        auto setup = [](auto&) {};
        www_test::check_exception_cases<label_type>(resources, setup);
    }
}

TEST_CASE("udho www framework applies connection policy after error responses", "[www][test-stream][error][sequence]") {
    SECTION("Populated store without a view bridge") {
        udho::view::resources::store<> store;
        udho::pages::system::setup(store);
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type = udho::www::test<udho::www::tags::minimal<>>;
        auto setup = [](auto&) {};
        www_test::check_error_sequences<label_type>(resources, setup);
    }

    SECTION("Populated store with a Lua view bridge") {
        udho::view::data::bridges::lua lua;
        lua.init();
        udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
        udho::pages::system::setup(store);
        store.lock();
        udho::view::resources::const_store<udho::view::data::bridges::lua> resources{store};

        using label_type = udho::www::test<
            udho::www::tags::minimal<udho::view::data::bridges::lua>
        >;
        auto setup = [&lua](auto& runtime) {
            using runtime_type = std::decay_t<decltype(runtime)>;
            lua.bind(udho::view::data::type<typename runtime_type::portal_type>{});
            lua.bind(udho::view::data::type<typename runtime_type::context_type>{});
        };
        www_test::check_error_sequences<label_type>(resources, setup);
    }

    SECTION("Empty store without a view bridge") {
        udho::view::resources::store<> store;
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type = udho::www::test<udho::www::tags::minimal<>>;
        auto setup = [](auto&) {};
        www_test::check_error_sequences<label_type>(resources, setup);
    }
}

TEST_CASE("udho www framework times out unfinished responses", "[www][test-stream][response][timeout]") {
    SECTION("action neither writes nor finishes") {
        www_test_callbacks::clear_exception_state();

        const auto result = www_test::execute_minimal(
            www_test_callbacks::timeout_router(),
            www_test::get_request("/stall")
        );

        REQUIRE(www_test_callbacks::exception_invocations == std::vector<std::string>{"stall"});
        CHECK(result.body.find(www_test::status_marker(503)) != std::string::npos);
        CHECK(result.body.find("udho::net::ostream timed out waiting for write") != std::string::npos);
        CHECK(result.body.find("Connection: close") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("action writes but does not finish") {
        www_test_callbacks::clear_exception_state();

        const auto result = www_test::execute_minimal(
            www_test_callbacks::timeout_router(),
            www_test::get_request("/write-then-stall")
        );

        REQUIRE(www_test_callbacks::exception_invocations == std::vector<std::string>{"write-then-stall"});
        CHECK(result.body.find(www_test::status_marker(503)) != std::string::npos);
        CHECK(result.body.find("partial-response") != std::string::npos);
        CHECK(result.body.find("udho::net::ostream timed out waiting for write") != std::string::npos);
        CHECK(result.body.find("Connection: close") != std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("normal completion cancels the idle wait") {
        www_test_callbacks::clear_exception_state();

        const auto result = www_test::execute_minimal(
            www_test_callbacks::timeout_router(),
            www_test::get_request("/ok/7")
        );

        REQUIRE(www_test_callbacks::exception_invocations == std::vector<std::string>{"ok:7"});
        CHECK(result.body.find(www_test::status_marker(200)) != std::string::npos);
        CHECK(result.body.find(www_test::status_marker(503)) == std::string::npos);
        CHECK(result.active_flows == 0);
    }

    SECTION("reentry prepares a new idle wait for the next response") {
        www_test_callbacks::clear_exception_state();

        const auto result = www_test::execute_minimal(
            www_test_callbacks::timeout_router(),
            www_test::concatenate_requests({
                www_test::get_request("/ok/1"),
                www_test::get_request("/stall"),
                www_test::get_request("/ok/2")
            })
        );
        const std::vector<std::string> expected{"ok:1", "stall"};

        REQUIRE(www_test_callbacks::exception_invocations == expected);
        www_test::check_statuses(result.body, {200, 503});
        CHECK(result.body.find("Connection: close") != std::string::npos);
        CHECK(result.active_flows == 0);
    }
}

TEST_CASE("udho www framework returns not found without invoking an action", "[www][test-stream][routing][error]") {
    using namespace udho::hazo::string::literals;

    SECTION("Populated store without a view bridge") {
        test_callbacks::invocations.clear();

        auto actions =
            udho::url::slot("show"_h, &test_callbacks::show) <<
            udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}")
        ;
        auto table  = udho::url::mount("root"_h, "/", std::move(actions));
        auto router = udho::url::router(std::move(table));

        boost::asio::io_context io;
        udho::view::resources::store<> store;
        udho::pages::system::setup(store);
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type     = udho::www::test<udho::www::tags::minimal<>>;
        using framework_type = udho::www::framework<label_type>;

        auto framework = framework_type::apply(std::move(router));
        auto runtime   = framework.runtime(resources);

        boost::beast::test::stream request_stream(io,
            "GET /missing HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n");
        boost::beast::test::stream response_stream(io);
        request_stream.connect(response_stream);
        response_stream.close();

        auto& flow = runtime.spawn(std::move(request_stream));
        flow.start();
        io.run();

        CHECK(test_callbacks::invocations.empty());
        CHECK(response_stream.str().find("HTTP/1.1 404 Not Found") != std::string::npos);
        CHECK(response_stream.str().find("Connection: close") != std::string::npos);
        CHECK(runtime.count() == 0);
    }

    SECTION("Populated store with a Lua view bridge") {
        test_callbacks::invocations.clear();

        auto actions =
            udho::url::slot("show"_h, &test_callbacks::show) <<
            udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}")
        ;
        auto table  = udho::url::mount("root"_h, "/", std::move(actions));
        auto router = udho::url::router(std::move(table));

        boost::asio::io_context io;
        udho::view::data::bridges::lua lua;
        lua.init();
        udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
        udho::pages::system::setup(store);
        store.lock();
        udho::view::resources::const_store<udho::view::data::bridges::lua> resources{store};

        using label_type = udho::www::test<
            udho::www::tags::minimal<udho::view::data::bridges::lua>
        >;
        using framework_type = udho::www::framework<label_type>;

        auto framework = framework_type::apply(std::move(router));
        auto runtime   = framework.runtime(resources);

        using runtime_type = std::decay_t<decltype(runtime)>;
        lua.bind(udho::view::data::type<typename runtime_type::portal_type>{});
        lua.bind(udho::view::data::type<typename runtime_type::context_type>{});

        boost::beast::test::stream request_stream(io,
            "GET /missing HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n");
        boost::beast::test::stream response_stream(io);
        request_stream.connect(response_stream);
        response_stream.close();

        auto& flow = runtime.spawn(std::move(request_stream));
        flow.start();
        io.run();

        CHECK(test_callbacks::invocations.empty());
        CHECK(response_stream.str().find("HTTP/1.1 404 Not Found") != std::string::npos);
        CHECK(response_stream.str().find("Connection: close") != std::string::npos);
        CHECK(runtime.count() == 0);
    }

    SECTION("Empty store without a view bridge") {
        test_callbacks::invocations.clear();

        auto actions =
            udho::url::slot("show"_h, &test_callbacks::show) <<
            udho::url::regx(udho::url::verb::get, "/projects/(\\d+)", "/projects/{}")
        ;
        auto table  = udho::url::mount("root"_h, "/", std::move(actions));
        auto router = udho::url::router(std::move(table));

        boost::asio::io_context io;
        udho::view::resources::store<> store;
        store.lock();
        udho::view::resources::const_store<> resources{store};

        using label_type     = udho::www::test<udho::www::tags::minimal<>>;
        using framework_type = udho::www::framework<label_type>;

        auto framework = framework_type::apply(std::move(router));
        auto runtime   = framework.runtime(resources);

        boost::beast::test::stream request_stream(io,
            "GET /missing HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n");
        boost::beast::test::stream response_stream(io);
        request_stream.connect(response_stream);
        response_stream.close();

        auto& flow = runtime.spawn(std::move(request_stream));
        flow.start();
        io.run();

        CHECK(test_callbacks::invocations.empty());
        CHECK(response_stream.str().find("HTTP/1.1 404 Not Found") != std::string::npos);
        CHECK(response_stream.str().find("Connection: close") != std::string::npos);
        CHECK(runtime.count() == 0);
    }
}

auto url() {
    using namespace udho::hazo::string::literals;
    auto actions1 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions2 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions3 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto mount_point1 = udho::url::mount("root"_h, "/",    std::move(actions1));
    auto mount_point2 = udho::url::mount("m2"_h,   "/m2",  std::move(actions2));
    auto mount_point3 = udho::url::mount("m3"_h,   "/m3",  std::move(actions2));

    auto table      = std::move(mount_point1) /*| std::move(mount_point2) | std::move(mount_point3)*/;

    return table;
}


TEST_CASE("udho manifold www pipeline stateless", "[manifold][pipeline][www]") {
    boost::asio::io_context io;

    // { resources: assets, docroot
    udho::view::resources::store<> store;
    // populate(store)
    store.lock();
    udho::view::resources::const_store<> cstore{store};
    auto resources  = udho::www::components::resources(cstore);
    // }

    using framework_type = udho::www::framework<udho::www::stateless::rest>;
    // using endpoint_type  = typename framework_type::endpoint_type;

    auto framework = framework_type::apply(udho::url::router(url()));
    auto runtime   = framework.runtime(resources);
    auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();

    io.run_for(std::chrono::seconds(2));
}


TEST_CASE("udho manifold www pipeline stateful", "[manifold][pipeline][www]") {
    boost::asio::io_context io;

    // { session component
    using catalogue_type = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;
    catalogue_type catalogue{udho::session::storage::fs{}};
    auto session    = udho::www::components::session(catalogue);
    // }

    // { resources: views, assets, docroot
    udho::view::data::bridges::lua lua;
    lua.init();
    // udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
    udho::view::resources::store<> store{};
    // populate(store)
    store.lock();
    // udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};
    udho::view::resources::const_store<> cstore{store};
    auto resources  = udho::www::components::resources(cstore);
    // }

    // using framework_type = udho::www::framework<udho::www::stateful::lua::lazy_fs>;
    using framework_type = udho::www::framework<udho::www::stateful::lazy_fs>;
    // using endpoint_type  = typename framework_type::endpoint_type;

    auto framework = framework_type::apply(udho::url::router(url()));
    auto runtime   = framework.runtime(session, resources);
    auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    {
        boost::beast::test::stream stream_in(io);
        std::ofstream html("structure.html");
        udho::manifold::vis::html::runtime(html, runtime);
    }

    listener.start();

    io.run_for(std::chrono::seconds(5));
}
