#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <boost/asio/buffer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <udho/manifold/composition.h>
#include <udho/www/components/protocol.h>
#include <udho/www/components/request.h>

namespace {

namespace feature = udho::www::feature;
using request_component = udho::www::components::request;
using protocol_component = udho::www::components::protocols::http<boost::beast::test::stream>;
using header_facet = udho::manifold::facet<protocol_component, feature::header_reader>;
using body_facet = udho::manifold::facet<protocol_component, feature::body_reader>;
using request_facet = udho::manifold::facet<request_component, feature::request_bind>;
using journal_type = udho::manifold::journal<header_facet, body_facet, request_facet>;
using composition_type = udho::manifold::composition<protocol_component, request_component>;
using full_portal = udho::manifold::portal<protocol_component, request_component>;
using request_portal = udho::manifold::portal<request_component>;

static_assert(std::is_same_v<request_portal::journal_view_type,
              udho::manifold::journal_const_view<request_facet>>);
static_assert(std::is_same_v<decltype(std::declval<const request_portal&>().request()),
              const feature::header_reader::result&>);
static_assert(std::is_same_v<decltype(std::declval<const request_portal&>().body()),
              const feature::body_reader::result&>);

feature::header_reader::result header(const std::string& target) {
    feature::header_reader::result result;
    result.method(boost::beast::http::verb::post);
    result.target(target);
    result.set(boost::beast::http::field::host, "example.com");
    return result;
}

feature::body_reader::result body(const std::string& text) {
    udho::net::protocols::body_reader_result<boost::beast::flat_buffer> result;
    boost::asio::buffer_copy(result.buffer().prepare(text.size()), boost::asio::buffer(text));
    result.buffer().commit(text.size());
    return {"text/plain", std::move(result), {}, text.size()};
}

struct store_binding {
    journal_type& journal;

    void pass(feature::request_bind::result&& result) {
        journal.at<feature::request_bind>() = std::move(result);
    }
};

struct fixture {
    protocol_component protocol;
    composition_type composition = composition_type::compose(protocol);
    composition_type::configs_type configs;
    journal_type journal;
    request_facet facet{composition.get<request_component>().component(), configs.get<request_component>(), 0};
    full_portal full{composition, configs, journal};
    request_portal request{full};
    int unused_stream = 0;

    void bind() { facet(journal, store_binding{journal}, unused_stream); }
};

}

TEST_CASE("Request binding requires both parsed results", "[manifold][components][request]") {
    fixture state;

    CHECK_FALSE(state.journal.at<feature::request_bind>().ready());
    CHECK_THROWS_AS(state.bind(), std::runtime_error);
    CHECK_FALSE(state.journal.at<feature::request_bind>().ready());

    state.journal.at<feature::header_reader>() = header("/first");
    CHECK_THROWS_AS(state.bind(), std::runtime_error);
    CHECK_FALSE(state.journal.at<feature::request_bind>().ready());

    state.journal.at<feature::body_reader>() = body("first body");
    CHECK_FALSE(state.journal.at<feature::request_bind>().ready());
    CHECK_THROWS_AS(state.request.request(), std::runtime_error);
    CHECK_THROWS_AS(state.request.body(), std::runtime_error);

    state.bind();
    REQUIRE(state.journal.at<feature::request_bind>().ready());
    CHECK(&state.request.request() == &state.journal.at<feature::header_reader>().value());
    CHECK(&state.request.body() == &state.journal.at<feature::body_reader>().value());
    CHECK(state.request.request().target() == "/first");
    CHECK(state.request.request()[boost::beast::http::field::host] == "example.com");
    CHECK(state.request.body().mime() == "text/plain");
    CHECK(state.request.body().str() == "first body");
    CHECK(state.request.body().bytes_transferred() == 10);
}

TEST_CASE("Narrowed request portal follows journal reset and rebinding", "[manifold][components][request][reentry]") {
    fixture state;
    state.journal.at<feature::header_reader>() = header("/first");
    state.journal.at<feature::body_reader>() = body("first");
    state.bind();
    CHECK(state.request.request().target() == "/first");
    CHECK(state.request.body().str() == "first");

    state.journal.clear();
    CHECK_FALSE(state.journal.at<feature::request_bind>().ready());
    CHECK_THROWS_AS(state.request.request(), std::runtime_error);
    CHECK_THROWS_AS(state.request.body(), std::runtime_error);

    state.journal.at<feature::header_reader>() = header("/second");
    state.journal.at<feature::body_reader>() = body("second body");
    CHECK_FALSE(state.journal.at<feature::request_bind>().ready());
    state.bind();

    const auto& request = state.request;
    CHECK(&request.request() == &state.journal.at<feature::header_reader>().value());
    CHECK(&request.body() == &state.journal.at<feature::body_reader>().value());
    CHECK(request.request().target() == "/second");
    CHECK(request.body().str() == "second body");
    CHECK(request.body().bytes_transferred() == 11);
}
