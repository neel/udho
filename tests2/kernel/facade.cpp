#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/middleware/facade.h>
#include <udho/middleware/state.h>
#include <udho/middleware/evaluator.h>

namespace test {
// Custom test features
struct feature_a {};
struct feature_b {};

// Dummy state types
struct state_a {
    bool accepted() const { return true; }
    int value = 0;
};

struct state_b {
    bool accepted() const { return true; }
    std::string data;
};

// Test component implementations
struct component_a {
    using feature = feature_a;
    using state = state_a;

    template <typename Head, typename... Tail>
    state eval(const udho::middleware::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        state_a s;
        s.value = ++counter;
        return s;
    }
    mutable int counter = 0;
};

struct component_b {
    using feature = feature_b;
    using state = state_b;

    component_b() = default;
    explicit component_b(std::string d) : data(std::move(d)) {}

    template <typename Head, typename... Tail>
    state eval(const udho::middleware::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        state_b s;
        s.data = data;
        return s;
    }
    std::string data;
};

struct component_c {
    using feature = feature_b;
    using state = state_b;

    component_c() = default;
    explicit component_c(std::string d) : data(std::move(d)) {}

    template <typename Head, typename... Tail>
    state eval(const udho::middleware::states<Head, Tail...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        state_b s;
        s.data = data;
        return s;
    }
    std::string data;
};

} // namespace test

namespace udho {
namespace middleware {

template <>
struct component_traits<test::component_a>{
    static constexpr const bool prefer_reference = true;
};

template <>
struct component_traits<test::component_b>{
    static constexpr const bool prefer_reference = false;
};

template <>
struct component_traits<test::component_c>{
    static constexpr const bool prefer_reference = false;
};

}
}

TEST_CASE("Compilation") {
    test::component_a a;
    test::component_b b;

    using facade_type = udho::middleware::facade_chain<test::component_a, test::component_b, test::component_c>;
    // using states_type = udho::middleware::states<test::component_a, test::component_b, test::component_c>;

    facade_type facade(a, std::move(b), test::component_c{});
    // states_type states;

    // facade_type::result_type<test::component_a>;

    // facade.get<test::component_b>();
}

// TEST_CASE("Component wrapper construction") {
//     test::component_a a1;
//     test::component_b b1("test");

//     SECTION("Reference storage when traits require") {
//         auto wrapper = udho::middleware::component_wrapper<test::component_a>(a1);
//         REQUIRE(&wrapper.component() == &a1);
//     }

//     SECTION("Value storage when movable") {
//         auto wrapper = udho::middleware::component_wrapper<test::component_b>(test::component_b("moved"));
//         REQUIRE(wrapper.component().data == "moved");
//     }
// }

// TEST_CASE("Facade construction and retrieval") {
//     test::component_a a1;
//     test::component_b b2("moved");

//     auto facade = udho::middleware::facade<test::component_a, test::component_b>(a1, std::move(b2));

//     SECTION("Component retrieval by type") {
//         REQUIRE(&facade.get<test::component_a>().component() == &a1);
//         REQUIRE(facade.get<test::component_b>().component().data == "moved");
//     }

//     SECTION("Component retrieval by feature and index") {
//         REQUIRE(&facade.at<test::feature_a, 0>().component() == &a1);
//         REQUIRE(facade.at<test::feature_b, 0>().component().data == "moved");
//     }

//     SECTION("Component count by feature") {
//         REQUIRE(facade.count<test::feature_a>() == 1);
//         REQUIRE(facade.count<test::feature_b>() == 1);
//     }
// }

// TEST_CASE("State management") {
//     udho::middleware::states<test::component_a, test::component_b> states;
//     test::component_a a;
//     test::component_b b;

//     SECTION("Default state initialization") {
//         REQUIRE(!states.get<test::component_a>().ready());
//         REQUIRE(!states.get<test::component_b>().ready());
//     }

//     SECTION("State assignment and retrieval") {
//         test::state_a s;
//         s.value = 42;
//         states.get<test::component_a>() = std::move(s);
//         REQUIRE(states.get<test::component_a>().value().value == 42);
//     }
// }

// TEST_CASE("Evaluation workflow") {
//     // Setup components
//     test::component_a a;
//     test::component_b b("eval_test");

//     // Create facade and states
//     auto facade = udho::middleware::facade<test::component_a, test::component_b, test::component_c>(a, std::move(b));
//     auto states = udho::middleware::states<test::component_a, test::component_b, test::component_c>();

//     // Dummy request data
//     boost::asio::ip::address addr;
//     udho::net::types::headers::request req;

//     SECTION("Single feature evaluation") {
//         udho::middleware::evaluator<test::feature_a> eval(addr, req);
//         auto count = eval(facade, states);

//         REQUIRE(count == 1);
//         REQUIRE(states.get<test::component_a>().value().value == 1);
//     }

//     SECTION("Multi-feature evaluation order") {
//         udho::middleware::evaluator<test::feature_a, test::feature_b> eval(addr, req);
//         auto count = eval(facade, states);

//         REQUIRE(count == 2);
//         REQUIRE(states.get<test::component_a>().value().value == 1);
//         REQUIRE(states.get<test::component_b>().value().data == "eval_test");
//     }
// }

// TEST_CASE("Apply functionality") {
//     test::component_a a1;
//     test::component_b a2;
//     auto facade = udho::middleware::facade<test::component_a, test::component_b>(a1, std::move(a2));

//     int counter = 0;
//     auto counter_func = [&](auto& comp) {
//         counter++;
//         return true;
//     };

//     SECTION("Apply to all components of a feature") {
//         facade.apply<test::feature_a>(counter_func);
//         REQUIRE(counter == 1);
//     }
// }
