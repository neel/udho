#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/middleware/composition.h>

#include <udho/middleware/pipeline.h>

#include <iostream>

namespace testing {

template <std::size_t Index>
struct Feature{ };

template <std::size_t Index, int FeatureIndex = -1>
struct Component{
    using feature = Feature<(FeatureIndex >= 0) ? FeatureIndex : Index>;
    static constexpr const std::size_t component_index = Index;

    Component(): is_default_constructed(true) { }
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other): is_default_constructed(false), message(std::move(other.message)) {}

    struct state {
        bool accepted() const { return true; }
    };

    bool is_default_constructed;
    std::string message;
};

};

template <>
struct udho::middleware::component_traits<testing::Component<5>> {
    static constexpr const bool prefer_reference = true;
};

TEST_CASE("Compilation") {
    using composition_type = udho::middleware::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;

    testing::Component<5> component_5;

    composition_type composition{
        udho::middleware::default_constructed{},
        udho::middleware::default_constructed{},
        udho::middleware::default_constructed{},
        udho::middleware::default_constructed{},
        udho::middleware::default_constructed{},
        component_5,
        udho::middleware::default_constructed{}
    };

    CHECK(composition.get<testing::Component<0>>().component_index == 0);
    CHECK(composition.get<testing::Component<1>>().component_index == 1);
    CHECK(composition.get<testing::Component<2, 0>>().component_index == 2);
    CHECK(composition.get<testing::Component<3>>().component_index == 3);
    CHECK(composition.get<testing::Component<4, 1>>().component_index == 4);


    CHECK(composition.at<testing::Feature<0>, 0>().component_index == 0);
    CHECK(composition.at<testing::Feature<0>, 1>().component_index == 2);

    CHECK(composition.at<testing::Feature<1>, 0>().component_index == 1);
    CHECK(composition.at<testing::Feature<1>, 1>().component_index == 4);

    {
        auto mw = composition_type::compose(component_5);

        CHECK(mw.get<testing::Component<0>>().component_index == 0);
        CHECK(mw.get<testing::Component<1>>().component_index == 1);
        CHECK(mw.get<testing::Component<2, 0>>().component_index == 2);
        CHECK(mw.get<testing::Component<3>>().component_index == 3);
        CHECK(mw.get<testing::Component<4, 1>>().component_index == 4);

        CHECK(mw.get<testing::Component<0>>().is_default_constructed);
        CHECK(mw.get<testing::Component<1>>().is_default_constructed);
        CHECK(mw.get<testing::Component<2, 0>>().is_default_constructed);
        CHECK(mw.get<testing::Component<3>>().is_default_constructed);
        CHECK(mw.get<testing::Component<4, 1>>().is_default_constructed);
    } {
        auto mw = composition_type::compose(component_5, testing::Component<3>{"moved"}, testing::Component<1>{"moved"});

        CHECK(mw.get<testing::Component<0>>().is_default_constructed);
        CHECK(!mw.get<testing::Component<1>>().is_default_constructed);
        CHECK(mw.get<testing::Component<2, 0>>().is_default_constructed);
        CHECK(!mw.get<testing::Component<3>>().is_default_constructed);
        CHECK(mw.get<testing::Component<4, 1>>().is_default_constructed);

        CHECK(mw.get<testing::Component<1>>().message == "moved");
        CHECK(mw.get<testing::Component<3>>().message == "moved");
    }
}
