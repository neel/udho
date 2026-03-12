#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/www/components/navigator.h>
#include <udho/manifold/config.h>
#include <udho/manifold/journal.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/www/components/protocol.h>

TEST_CASE("udho manifold component navigator - pretty url policy", "[manifold][components][navigator][policy]") {
    SECTION("multiple query parameters") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/hello/world/23?name=test&id=42&filter=active"};
        policy.extract(result, input);
        CHECK(result.resource() == "/hello/world/23");
        CHECK(result.params().count("name") == 1);
        CHECK(result.params().find("name")->second == "test");
        CHECK(result.params().find("id")->second == "42");
        CHECK(result.params().size() == 3);
        CHECK(result.params().find("filter")->second == "active");
    }

    SECTION("empty query parameters") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/test?empty=&valid=value"};
        policy.extract(result, input);
        CHECK(result.resource() == "/test");
        CHECK(result.params().find("empty")->second == "");
        CHECK(result.params().find("valid")->second == "value");
    }

    SECTION("URL encoded parameters") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/path?name=John%20Doe&city=New%20York"};
        policy.extract(result, input);
        CHECK(result.params().find("name")->second == "John Doe");
        CHECK(result.params().find("city")->second == "New York");
    }

    SECTION("no query parameters") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/simple/path"};
        policy.extract(result, input);
        CHECK(result.resource() == "/simple/path");
        CHECK(result.params().empty());
    }

    SECTION("multiple ampersands") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/test?&&key=value&&"};
        policy.extract(result, input);
        CHECK(result.params().size() == 4); // Correct?
        CHECK(result.params().find("key")->second == "value");
    }

    SECTION("file extension detection") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/document.html"};
        policy.extract_path(result, input);
        CHECK(result.resource() == "/document");
        CHECK(result.extension() == "html");
    }

    SECTION("no extension") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/path/no/extension"};
        policy.extract_path(result, input);
        CHECK(result.resource() == "/path/no/extension");
        CHECK(result.extension().empty());
    }

    SECTION("malformed query parameters") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/test?=novalue&key=value&="};
        policy.extract(result, input);
        CHECK(result.params().count("") == 2); // Correct?
        CHECK(result.params().find("")->second == "novalue");
        CHECK(result.params().find("key")->second == "value");
    }

    SECTION("special characters in parameters") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/api?query=a%2Bb%3Dc%26d%3De"};
        policy.extract(result, input);
        CHECK(result.params().find("query")->second == "a+b=c&d=e");
    }

    SECTION("multiple dots in path") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/path.with.many.dots.json"};
        policy.extract_path(result, input);
        CHECK(result.resource() == "/path.with.many.dots");
        CHECK(result.extension() == "json");
    }

    SECTION("trailing question mark") {
        udho::www::components::pretty_url_policy policy;
        udho::www::feature::identifier::result result;
        udho::utils::string_view input{"/test?"};
        policy.extract(result, input);
        CHECK(result.resource() == "/test");
        CHECK(result.params().empty());
    }
}

namespace sim{

template <typename ResultT>
struct expected_next{
    using result_type = ResultT;

    result_type& _result;
    std::exception_ptr& _ex;

    expected_next(result_type& res, std::exception_ptr& ex): _result(res), _ex(ex) {}

    void pass(result_type&& res) {
        _result = std::move(res);
    }
    void fail(result_type&& res) {
        _result = std::move(res);
    }

    template <typename Exception, std::enable_if_t<!std::is_same_v<Exception, result_type>, bool> = true>
    void fail(Exception&& ex) {
        fail(std::make_exception_ptr(std::move(ex)));
    }

    void fail(std::exception_ptr ex) {
        _ex = ex;
    }

    std::exception_ptr& exception() { return _ex; }
};

}

TEST_CASE("udho manifold component navigator", "[manifold][components][navigator]") {
    using navigator_component_type   = udho::www::components::navigator<udho::www::components::pretty_url_policy>;
    using navigator_config_type      = udho::manifold::config<navigator_component_type>;

    navigator_component_type component;
    navigator_config_type    config;

    SECTION("reading http header with various request types") {
        using stream_type               = boost::beast::test::stream; // udho::net::types::socket;
        using protocol_type             = udho::net::protocols::http<stream_type>;
        using protocol_component_type   = udho::www::components::protocol<protocol_type, stream_type>;
        using protocol_facet_type       = udho::manifold::facet<protocol_component_type, udho::www::feature::header_reader>;
        using fabric_type               = udho::manifold::facet<navigator_component_type, udho::www::feature::identifier>;
        using journal_type              = udho::manifold::journal<protocol_facet_type, fabric_type>;
        using result_type               = udho::www::feature::identifier::result;
        using next_type                 = sim::expected_next<result_type>;
        using request_type              = udho::net::types::headers::request;

        request_type request;
        request.method(boost::beast::http::verb::get);
        request.target("/hello/world/23.html?name=test&id=42&filter=active");

        fabric_type fabric{component, config, 0};
        result_type result;
        std::exception_ptr ex;
        journal_type journal;

        CHECK(journal.count<udho::www::feature::header_reader>() == 1);

        journal.at<udho::www::feature::header_reader, 0>() = std::move(request);

        int x;
        fabric.eval(journal, next_type{result, ex}, x);

        CHECK(result.resource() == "/hello/world/23");
        CHECK(result.extension() == "html");

        const auto& params = result.params();
        CHECK(params.count("name") == 1);
        CHECK(params.count("id") == 1);
        CHECK(params.count("filter") == 1);
    }
}
