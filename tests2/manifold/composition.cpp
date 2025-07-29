#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
// #include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_contains.hpp>
// #include <catch2/matchers/catch_matchers_string.hpp>

#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/manifold/composition.h>
#include <udho/manifold/evaluator.h>
#include <udho/manifold/pipeline.h>

namespace testing {

template <std::size_t Index>
struct Feature{ };

struct State {
    State() = delete;

    inline explicit State(bool val) : _value(val) {}
    inline bool accepted() const { return _value; }

    bool _value;
};

template <std::size_t Index, int FeatureIndex = Index>
struct Component {
    using feature = Feature<FeatureIndex>;
    using state   = State;

    static constexpr const std::size_t component_index = Index;
    static constexpr const int feature_index = FeatureIndex;

    Component(): is_default_constructed(true) {}
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other) noexcept : is_default_constructed(false), message(std::move(other.message))  { }

    template <typename... Components>
    state eval(const udho::manifold::states<Components...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        return state(message == "accept");
    }

    bool is_default_constructed;
    mutable std::string message;  // mutable for testing move semantics
};

}

template <>
struct udho::manifold::component_traits<testing::Component<5>> {
    static constexpr const bool prefer_reference = true;
};

TEST_CASE("manifold Construction & Composition") {
    using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;

    testing::Component<5> component_5{"C5"};

    SECTION("Basic Construction") {
        composition_type composition{
            udho::manifold::default_constructed{},
            udho::manifold::default_constructed{},
            udho::manifold::default_constructed{},
            udho::manifold::default_constructed{},
            udho::manifold::default_constructed{},
            component_5,
            udho::manifold::default_constructed{}
        };

        CHECK(composition.get<testing::Component<0>>().component().component_index == 0);
        CHECK(composition.get<testing::Component<1>>().component().component_index == 1);
        CHECK(composition.get<testing::Component<2, 0>>().component().component_index == 2);
        CHECK(composition.get<testing::Component<3>>().component().component_index == 3);
        CHECK(composition.get<testing::Component<4, 1>>().component().component_index == 4);


        CHECK(composition.at<testing::Feature<0>, 0>().component().component_index == 0);
        CHECK(composition.at<testing::Feature<0>, 1>().component().component_index == 2);
        CHECK(composition.count<testing::Feature<0>>() == 2);

        CHECK(composition.at<testing::Feature<1>, 0>().component().component_index == 1);
        CHECK(composition.at<testing::Feature<1>, 1>().component().component_index == 4);
        CHECK(composition.count<testing::Feature<1>>() == 2);
    }

    SECTION("Ordered Composition") {
        {
            auto mw = composition_type::compose(component_5);

            CHECK(mw.get<testing::Component<0>>().component().component_index == 0);
            CHECK(mw.get<testing::Component<1>>().component().component_index == 1);
            CHECK(mw.get<testing::Component<2, 0>>().component().component_index == 2);
            CHECK(mw.get<testing::Component<3>>().component().component_index == 3);
            CHECK(mw.get<testing::Component<4, 1>>().component().component_index == 4);

            CHECK(mw.get<testing::Component<0>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<1>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<2, 0>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<3>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<4, 1>>().component().is_default_constructed);
            CHECK(!mw.get<testing::Component<5>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<6>>().component().is_default_constructed);
        } {
            auto mw = composition_type::compose(testing::Component<1>{"moved"}, testing::Component<3>{"moved"}, component_5);

            CHECK(mw.get<testing::Component<0>>().component().component_index == 0);
            CHECK(mw.get<testing::Component<1>>().component().component_index == 1);
            CHECK(mw.get<testing::Component<2, 0>>().component().component_index == 2);
            CHECK(mw.get<testing::Component<3>>().component().component_index == 3);
            CHECK(mw.get<testing::Component<4, 1>>().component().component_index == 4);

            CHECK(mw.get<testing::Component<0>>().component().is_default_constructed);
            CHECK(!mw.get<testing::Component<1>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<2, 0>>().component().is_default_constructed);
            CHECK(!mw.get<testing::Component<3>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<4, 1>>().component().is_default_constructed);
            CHECK(!mw.get<testing::Component<5>>().component().is_default_constructed);
            CHECK(mw.get<testing::Component<6>>().component().is_default_constructed);
        }
    }

    SECTION("Unordered Composition") {
        auto mw = composition_type::compose(component_5, testing::Component<3>{"moved"}, testing::Component<1>{"moved"});

        CHECK(mw.get<testing::Component<0>>().component().is_default_constructed);
        CHECK(!mw.get<testing::Component<1>>().component().is_default_constructed);
        CHECK(mw.get<testing::Component<2, 0>>().component().is_default_constructed);
        CHECK(!mw.get<testing::Component<3>>().component().is_default_constructed);
        CHECK(mw.get<testing::Component<4, 1>>().component().is_default_constructed);

        CHECK(mw.get<testing::Component<1>>().component().message == "moved");
        CHECK(mw.get<testing::Component<3>>().component().message == "moved");
    }
}

TEST_CASE("manifold Pipeline") {
    SECTION("eval & states basic operations") {
        using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;

        using states_type = udho::manifold::states<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;


        testing::Component<5> component_5;

        auto mw = composition_type::compose(
                testing::Component<0>{"accept"},  // Will evaluate to true
                testing::Component<1>{"reject"},  // Will evaluate to false
                component_5
            );

        states_type states;

        boost::asio::ip::address address;
        udho::net::types::headers::request request;

        bool s0 = mw.get<testing::Component<0>>().eval(states, address, request);
        bool s1 = mw.get<testing::Component<1>>().eval(states, address, request);

        CHECK(s0);
        CHECK(!s1);

        CHECK(states.get<testing::Component<0>>().ready());
        CHECK(states.get<testing::Component<1>>().ready());
        CHECK(!states.get<testing::Component<2, 0>>().ready());
    }

    SECTION("Feature-based evaluation pipeline") {
        using pipeline_type = udho::manifold::pipeline<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
         >;

        testing::Component<5> component_5;

        pipeline_type pipeline(
            testing::Component<0>{"accept"},
            testing::Component<1>{"accept"},
            testing::Component<2, 0>{"accept"},
            udho::manifold::default_constructed{},
            testing::Component<4, 1>{"accept"},
            component_5,
            testing::Component<6>{"accept"}
        );

        // Mock request objects
        boost::asio::ip::address address;
        udho::net::types::headers::request request;

        // Create evaluator for Feature<0> and Feature<1>
        using evaluator_type = udho::manifold::evaluator<
            testing::Feature<0>,
            testing::Feature<1>
        >;

        evaluator_type evaluator(address, request);
        std::size_t count = pipeline(std::move(evaluator));

        // Should have evaluated 4 components (2 for Feature<0>, 2 for Feature<1>)
        CHECK(count == 4);

        // Verify states
        CHECK(pipeline.states().get<testing::Component<0>>()->accepted());
        CHECK(pipeline.states().get<testing::Component<1>>()->accepted());
        CHECK(pipeline.states().get<testing::Component<2, 0>>()->accepted());
        CHECK(pipeline.states().get<testing::Component<4, 1>>()->accepted());

        // Components without features shouldn't be evaluated
        CHECK(!pipeline.states().get<testing::Component<3>>().ready());
        CHECK(!pipeline.states().get<testing::Component<5>>().ready());
        CHECK(!pipeline.states().get<testing::Component<6>>().ready());
    }

    SECTION("Feature-based evaluation pipeline stops after one component rejects") {
        using pipeline_type = udho::manifold::pipeline<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;

        testing::Component<5> component_5;

        pipeline_type pipeline(
            testing::Component<0>{"accept"},
            testing::Component<1>{"reject"},
            testing::Component<2, 0>{"accept"},
            udho::manifold::default_constructed{},
            testing::Component<4, 1>{"accept"},
            component_5,
            testing::Component<6>{"accept"}
        );

        // Mock request objects
        boost::asio::ip::address address;
        udho::net::types::headers::request request;

        // Create evaluator for Feature<0> and Feature<1>
        using evaluator_type = udho::manifold::evaluator<
            testing::Feature<0>,
            testing::Feature<1>
        >;

        evaluator_type evaluator(address, request);
        std::size_t count = pipeline(std::move(evaluator));

        // Should have evaluated 4 components (2 for Feature<0>, 2 for Feature<1>)
        CHECK(count == 2);

        // Verify states
        CHECK(pipeline.states().get<testing::Component<0>>()->accepted());
        CHECK(!pipeline.states().get<testing::Component<1>>()->accepted());
        CHECK(pipeline.states().get<testing::Component<2, 0>>()->accepted());
        CHECK_THROWS_WITH(
            (pipeline.states().get<testing::Component<4, 1>>()->accepted()),
            Catch::Matchers::EndsWith("unevaluated state")
        );

        // Components without features shouldn't be evaluated
        CHECK(!pipeline.states().get<testing::Component<4, 1>>().ready());
        CHECK(!pipeline.states().get<testing::Component<3>>().ready());
        CHECK(!pipeline.states().get<testing::Component<5>>().ready());
        CHECK(!pipeline.states().get<testing::Component<6>>().ready());
    }
}

TEST_CASE("manifold Extra") {
    SECTION("Error handling in state access") {
        udho::manifold::state_wrapper<testing::State, testing::Feature<0>> state;
        CHECK(!state.ready());

        // Accessing unready state should throw
        CHECK_THROWS_AS(state.value(), std::runtime_error);
        CHECK_THROWS_AS(*state, std::runtime_error);
        CHECK_THROWS_AS(state.operator->(), std::runtime_error);

        // After assignment, should be accessible
        state = testing::State{true};
        CHECK(state.ready());
        CHECK(state.value()._value == true);
        CHECK((*state)._value == true);
        CHECK(state->accepted() == true);  // Using operator->
    }

    SECTION("Component with reference storage semantics") {
        testing::Component<5> non_movable{"non_movable"};

        using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;

        composition_type composition = composition_type::compose(non_movable);
        auto& wrapper = composition.get<testing::Component<5>>();

        CHECK(wrapper.component().message == "non_movable");
        wrapper.component().message = "modified";
        CHECK(non_movable.message == "modified");  // Should reference original

        // Should not be movable
        auto moved = std::move(composition);
        CHECK(moved.get<testing::Component<5>>().component().message == "modified");
        CHECK(non_movable.message == "modified");  // Still references original
    }
}
