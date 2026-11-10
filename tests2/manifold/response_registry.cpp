#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <udho/url/summary.h>
#include <udho/www/components/connection.h>
#include <udho/www/context.h>
#include <udho/www/response_registry.h>

static_assert(udho::www::context<>::template has<udho::www::components::connection>::value);
static_assert(std::is_same<
    udho::www::context<>,
    udho::manifold::basic_context<udho::www::components::connection>
>::value);

TEST_CASE("response registry completes hooks in FIFO order", "[www][response-registry]") {
    udho::url::summary::router summary;
    udho::www::response_registry responses(summary);
    std::vector<int> completed;

    responses.attach(17);
    REQUIRE(responses.add_hook(17, [&completed] { completed.push_back(1); }));
    REQUIRE(responses.add_hook(17, [&completed] { completed.push_back(2); }));

    responses.complete(17);
    CHECK(completed == std::vector<int>{1, 2});

    SECTION("the attached flow accepts hooks for its next response") {
        REQUIRE(responses.add_hook(17, [&completed] { completed.push_back(3); }));
        responses.complete(17);
        CHECK(completed == std::vector<int>{1, 2, 3});
    }

    SECTION("a hook can register the next response hook without deadlocking") {
        bool next_hook_registered = false;
        REQUIRE(responses.add_hook(17, [&responses, &completed, &next_hook_registered] {
            completed.push_back(3);
            next_hook_registered = responses.add_hook(17, [&completed] { completed.push_back(4); });
        }));

        responses.complete(17);
        CHECK(next_hook_registered);
        CHECK(completed == std::vector<int>{1, 2, 3});

        responses.complete(17);
        CHECK(completed == std::vector<int>{1, 2, 3, 4});
    }
}

TEST_CASE("response registry isolates hook failures and detach", "[www][response-registry]") {
    udho::url::summary::router summary;
    udho::www::response_registry responses(summary);
    bool second_hook_ran = false;

    responses.attach(23);
    REQUIRE(responses.add_hook(23, [] { throw std::runtime_error{"hook failure"}; }));
    REQUIRE(responses.add_hook(23, [&second_hook_ran] { second_hook_ran = true; }));

    CHECK_NOTHROW(responses.complete(23));
    CHECK(second_hook_ran);

    auto retained = std::make_shared<int>(42);
    std::weak_ptr<int> released = retained;
    REQUIRE(responses.add_hook(23, [retained] {}));
    retained.reset();

    responses.detach(23);
    CHECK(released.expired());
    CHECK_FALSE(responses.add_hook(23, [] {}));
}

TEST_CASE("connection owns and exposes response completion state", "[www][connection]") {
    udho::url::summary::router summary;
    udho::www::components::connection connection(summary);
    bool completed = false;

    CHECK(&connection.summary() == &connection.responses().summary());

    connection.responses().attach(31);
    REQUIRE(connection.add_hook(31, [&completed] { completed = true; }));
    connection.responses().complete(31);
    CHECK(completed);

    connection.responses().detach(31);
    CHECK_FALSE(connection.add_hook(31, [] {}));
}
