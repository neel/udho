#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/manifold/composition.h>
#include <udho/manifold/features.h>
#include <udho/manifold/fabric.h>
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
    // using feature   = Feature<FeatureIndex>;
    using features  = udho::manifold::features<Feature<FeatureIndex>>;
    using result    = State;

    static constexpr const std::size_t component_index = Index;
    static constexpr const int feature_index = FeatureIndex;

    Component(): is_default_constructed(true) {}
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other) noexcept : is_default_constructed(false), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;
};

template <>
struct Component<5, 5> {
    using features  = udho::manifold::features<Feature<1>, Feature<5>, Feature<6>>;
    using result    = State;

    Component(): is_default_constructed(true) {}
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other) noexcept : is_default_constructed(false), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;
};

template <std::size_t Index, int FeatureIndex = Index>
struct XComponent {
    // using feature  = Feature<FeatureIndex>;
    using features = udho::manifold::features<Feature<FeatureIndex>>;

    static constexpr const std::size_t component_index = Index;
    static constexpr const int feature_index = FeatureIndex;

    XComponent(): is_default_constructed(true) {}
    XComponent(const std::string& msg): is_default_constructed(false), message(msg) {}
    XComponent(const XComponent&) = delete;
    XComponent(XComponent&& other) noexcept : is_default_constructed(false), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;
};

}

namespace udho {
namespace manifold {

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::Component<Index, FeatureIndex>, F> {
    using component_type = testing::Component<Index, FeatureIndex>;
    using feature        = F;
    using result         = typename testing::Component<Index, FeatureIndex>::result;

    facet(component_type& component): _component(component) {}

    template <typename... Components>
    result eval(const udho::manifold::journal<Components...>& journal, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        return result(_component.message == "accept");
    }

    private:
        component_type& _component;
};

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::XComponent<Index, FeatureIndex>, F> {
    using component_type = testing::XComponent<Index, FeatureIndex>;
    using feature = F;

    facet(component_type& component): _component(component) {}

    private:
        component_type& _component;
};


}
}

template <>
struct udho::manifold::component_traits<testing::Component<5>> {
    static constexpr const bool shared = true;
    using result = testing::State;
    using params = udho::manifold::params<>;
};

// {

// namespace lib{

// template <typename... D>
// struct fabric{};

// template <typename C, typename F>
// struct facet{};

// template <typename ComponentT, typename... Features>
// struct features{
//     using fabric_type = fabric<facet<ComponentT, Features>...>;
// };

// }

// //-----



// namespace detail {

// template <typename... FacetsSet>
// struct flatten;

// template <typename... Facets>
// struct flattened{
//     using type = lib::fabric<Facets ...>;
// };

// template <typename L, typename R>
// struct combined;

// template <typename... X, typename... Y>
// struct combined<flattened<X...>, flattened<Y...>>{
//     using type = flattened<X..., Y...>;
// };

// template <typename... Facets, typename... Rest>
// struct flatten<lib::fabric<Facets...>, Rest...> {
//     using type = flattened<Facets...>;
//     using rest = typename flatten<Rest...>::combined;
//     using combined = typename combined<type, rest>::type;
// };

// template <>
// struct flatten<>{
//     using type = flattened<>;
//     using combined = flattened<>;
// };

// template <typename... Components>
// struct flatten_all{
//     using type = typename flatten<typename Components::features::fabric_type...>::combined;
// };

// }

// namespace x {

// template <int I>
// struct F{};

// struct C1{
//     using features = lib::features<C1, F<1>, F<2>, F<3>>;
// };

// struct C2{
//     using features = lib::features<C2, F<1>, F<5>>;
// };

// struct C3{
//     using features = lib::features<C3, F<3>, F<8>>;
// };

// struct C4{
//     using features = lib::features<C4, F<4>>;
// };

// }

// int main() {

//     using flattened_type = typename detail::flatten_all<x::C1, x::C2, x::C4, x::C3>::type;

//     flattened_type::xyz();
//     // x::C1::features::fabric_type::xyz();

//     return 0;
// }


// }

TEST_CASE("manifold composition Construction & Composition", "[manifold][composition]") {
    using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::XComponent<0, 1>,
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

        CHECK(composition.at<testing::Feature<1>, 0>().component().component_index == 0);
        CHECK(composition.at<testing::Feature<1>, 1>().component().component_index == 1);
        CHECK(composition.at<testing::Feature<1>, 2>().component().component_index == 4);
        CHECK(composition.count<testing::Feature<1>>() == 4);
        CHECK(composition.count<testing::Feature<5>>() == 1);
        CHECK(composition.count<testing::Feature<6>>() == 2);
    }

    SECTION("Ordered Composition") {
        {
            auto composition = composition_type::compose(component_5);

            CHECK(composition.get<testing::Component<0>>().component().component_index == 0);
            CHECK(composition.get<testing::Component<1>>().component().component_index == 1);
            CHECK(composition.get<testing::Component<2, 0>>().component().component_index == 2);
            CHECK(composition.get<testing::Component<3>>().component().component_index == 3);
            CHECK(composition.get<testing::Component<4, 1>>().component().component_index == 4);

            CHECK(composition.get<testing::Component<0>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<1>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<2, 0>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<3>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<4, 1>>().component().is_default_constructed);
            CHECK(!composition.get<testing::Component<5>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<6>>().component().is_default_constructed);
        } {
            auto composition = composition_type::compose(testing::Component<1>{"moved"}, testing::Component<3>{"moved"}, component_5);

            CHECK(composition.get<testing::Component<0>>().component().component_index == 0);
            CHECK(composition.get<testing::Component<1>>().component().component_index == 1);
            CHECK(composition.get<testing::Component<2, 0>>().component().component_index == 2);
            CHECK(composition.get<testing::Component<3>>().component().component_index == 3);
            CHECK(composition.get<testing::Component<4, 1>>().component().component_index == 4);

            CHECK(composition.get<testing::Component<0>>().component().is_default_constructed);
            CHECK(!composition.get<testing::Component<1>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<2, 0>>().component().is_default_constructed);
            CHECK(!composition.get<testing::Component<3>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<4, 1>>().component().is_default_constructed);
            CHECK(!composition.get<testing::Component<5>>().component().is_default_constructed);
            CHECK(composition.get<testing::Component<6>>().component().is_default_constructed);
        }
    }

    SECTION("Unordered Composition") {
        auto composition = composition_type::compose(component_5, testing::Component<3>{"moved"}, testing::Component<1>{"moved"});

        CHECK(composition.get<testing::Component<0>>().component().is_default_constructed);
        CHECK(!composition.get<testing::Component<1>>().component().is_default_constructed);
        CHECK(composition.get<testing::Component<2, 0>>().component().is_default_constructed);
        CHECK(!composition.get<testing::Component<3>>().component().is_default_constructed);
        CHECK(composition.get<testing::Component<4, 1>>().component().is_default_constructed);

        CHECK(composition.get<testing::Component<1>>().component().message == "moved");
        CHECK(composition.get<testing::Component<3>>().component().message == "moved");
    }
}

TEST_CASE("manifold facet Construction & Composition", "[manifold][fabric]") {
    using composition_type = udho::manifold::composition<
        testing::Component<0>,
        testing::XComponent<0, 1>,
        testing::Component<1>,
        testing::Component<2, 0>,
        testing::Component<3>,
        testing::Component<4, 1>,
        testing::Component<5>,
        testing::Component<6>
    >;

    testing::Component<5> component_5{"C5"};

    auto composition = composition_type::compose(component_5);

    using fabric_type = composition_type::fabric_type;
    using expected_fabric_type = udho::manifold::fabric<
        udho::manifold::facet<testing::Component<0>, testing::Feature<0>>,
        udho::manifold::facet<testing::XComponent<0, 1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>,
        udho::manifold::facet<testing::Component<3>, testing::Feature<3>>,
        udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<5>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<6>>,
        udho::manifold::facet<testing::Component<6>, testing::Feature<6>>
    >;
    static_assert(std::is_same_v<expected_fabric_type, fabric_type>);

    fabric_type fabric{composition};

    using journal_type = udho::manifold::detail::journal_for_fabric<fabric_type>::type;
    using expected_journal_type = udho::manifold::journal<
        udho::manifold::facet<testing::Component<0>, testing::Feature<0>>,
        udho::manifold::facet<testing::Component<1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>,
        udho::manifold::facet<testing::Component<3>, testing::Feature<3>>,
        udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<5>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<6>>,
        udho::manifold::facet<testing::Component<6>, testing::Feature<6>>
    >;
    static_assert(std::is_same_v<expected_journal_type, journal_type>);

    journal_type journal;
}

TEST_CASE("manifold Pipeline", "[manifold][pipeline]") {
    SECTION("eval & journal basic operations") {
        using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::XComponent<0, 1>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;
        using fabric_type = composition_type::fabric_type;

        using journal_type = udho::manifold::detail::journal_for_fabric<fabric_type>::type;

        testing::Component<5> component_5;

        auto composition = composition_type::compose(
                testing::XComponent<0, 1>{},
                testing::Component<0>{"accept"},  // Will evaluate to true
                testing::Component<1>{"reject"},  // Will evaluate to false
                component_5
            );

        journal_type journal;

        boost::asio::ip::address address;
        udho::net::types::headers::request request;

        fabric_type fabric{composition};

        bool s0 = fabric.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().eval(journal, address, request);
        bool s1 = fabric.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().eval(journal, address, request);

        CHECK(s0);
        CHECK(!s1);

        CHECK(journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
    }


    SECTION("Feature-based evaluation pipeline") {
        using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::XComponent<0, 1>,
            testing::Component<5>,
            testing::Component<6>
         >;
        using fabric_type = composition_type::fabric_type;

        testing::Component<5> component_5;

        composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
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

        using pipeline_type = udho::manifold::pipeline<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::XComponent<0, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;

        pipeline_type pipeline{composition};
        evaluator_type evaluator(address, request);
        std::size_t count = pipeline(std::move(evaluator));

        // Should have evaluated 4 components (2 for Feature<0>, 2 for Feature<1>)
        CHECK(count == 4);

        // Verify journal
        CHECK(pipeline.journal().get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>()->accepted());
        CHECK(pipeline.journal().get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>()->accepted());
        CHECK(pipeline.journal().get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>()->accepted());
        CHECK(pipeline.journal().get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>()->accepted());

        // Components without features shouldn't be evaluated
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<3>, testing::Feature<3>>>().ready());
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());
    }

    SECTION("Feature-based evaluation pipeline stops after one component rejects") {
        using composition_type = udho::manifold::composition<
            testing::Component<0>,
            testing::Component<1>,
            testing::Component<2, 0>,
            testing::Component<3>,
            testing::Component<4, 1>,
            testing::XComponent<0, 1>,
            testing::Component<5>,
            testing::Component<6>
        >;
        using fabric_type = composition_type::fabric_type;
        using pipeline_type  = composition_type::pipeline_type;

        testing::Component<5> component_5;

        composition_type composition =  composition_type::compose(
            testing::Component<0>{"accept"},
            testing::Component<1>{"reject"},
            testing::Component<2, 0>{"accept"},
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

        pipeline_type pipeline{composition};
        evaluator_type evaluator(address, request);
        std::size_t count = pipeline(std::move(evaluator));

        // Should have evaluated 4 components (2 for Feature<0>, 2 for Feature<1>)
        CHECK(count == 2);

        // Verify journal
        CHECK(pipeline.journal().get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>()->accepted());
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>()->accepted());
        CHECK(pipeline.journal().get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>()->accepted());
        CHECK_THROWS_WITH(
            (pipeline.journal().get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>()->accepted()),
            Catch::Matchers::ContainsSubstring("unevaluated", Catch::CaseSensitive::No)
        );

        // Components without features shouldn't be evaluated
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<3>, testing::Feature<3>>>().ready());
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(!pipeline.journal().get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        // verify that components that don't have result do not exist in the journal type
        //pipeline.journal().get<testing::XComponent<0, 1>>();
    }
}

TEST_CASE("manifold Extra", "[manifold]") {
    SECTION("Error handling in result access") {
        udho::manifold::result_wrapper<testing::State, testing::Feature<0>> result;
        CHECK(!result.ready());

        // Accessing unready result should throw
        CHECK_THROWS_AS(result.value(), std::runtime_error);
        CHECK_THROWS_AS(*result, std::runtime_error);
        CHECK_THROWS_AS(result.operator->(), std::runtime_error);

        // After assignment, should be accessible
        result = testing::State{true};
        CHECK(result.ready());
        CHECK(result.value()._value == true);
        CHECK((*result)._value == true);
        CHECK(result->accepted() == true);  // Using operator->
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
