#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/middleware/composition.h>

#include <iostream>

namespace testing {

template <std::size_t Index>
struct Feature{};

template <std::size_t Index, int FeatureIndex = -1>
struct Component{
    using feature = Feature<(FeatureIndex >= 0) ? FeatureIndex : Index>;
    static constexpr const std::size_t component_index = Index;

    struct state {
        bool accepted() const { return true; }
    };
};

};


TEST_CASE("Compilation") {
    using composition_type = udho::middleware::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>
        >;

    composition_type composition;

    CHECK(composition.get<testing::Component<0>>().component_index == 0);
    CHECK(composition.get<testing::Component<1>>().component_index == 1);
    CHECK(composition.get<testing::Component<2, 0>>().component_index == 2);
    CHECK(composition.get<testing::Component<3>>().component_index == 3);
    CHECK(composition.get<testing::Component<4, 1>>().component_index == 4);


    CHECK(composition.at<testing::Feature<0>, 0>().component_index == 0);
    CHECK(composition.at<testing::Feature<0>, 1>().component_index == 2);

    CHECK(composition.at<testing::Feature<1>, 0>().component_index == 1);
    CHECK(composition.at<testing::Feature<1>, 1>().component_index == 4);
}
