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

#include <iostream>

namespace testing {

template <std::size_t Index>
struct Feature{
    static constexpr const std::size_t idx = Index;

    static constexpr const std::size_t stage = 1;
};

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

template <>
struct Feature<3> {
    static constexpr const std::size_t stage = 0;
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

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C" << Index << "F" << feature::idx << std::endl;
        result res{_component.message == "accept"};
        if(_component.message == "accept") {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

    private:
        component_type& _component;
};

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::XComponent<Index, FeatureIndex>, F> {
    using component_type = testing::XComponent<Index, FeatureIndex>;
    using feature = F;

    facet(component_type& component): _component(component) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "X" << Index << "F" << FeatureIndex << std::endl;
        next.pass();
    }

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

    using fabric_type = composition_type::fabric_type<1>;

    using expected_fabric_type = udho::manifold::fabric<
        1,
        udho::manifold::facet<testing::Component<0>, testing::Feature<0>>,
        udho::manifold::facet<testing::XComponent<0, 1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>,
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
        using fabric_type = composition_type::fabric_type<1>;
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

        auto& facet11= fabric.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
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

        using order_type = udho::manifold::order<
            testing::Feature<0>,
            testing::Feature<1>,
            testing::Feature<2>,
            testing::Feature<3>,
            testing::Feature<4>,
            testing::Feature<5>,
            testing::Feature<6>,
            testing::Feature<7>,
            testing::Feature<8>
        >;

        using pipeline_type = udho::manifold::common_pipepine<1, order_type, composition_type>;

        SECTION("Journal OK when all facets accept") {
            testing::Component<5> component_5{"accept"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"accept"}
            );

            pipeline_type pipeline{composition};
            std::stringstream stream;

            bool evaluated = true;

            pipeline.then([&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;
                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nX0F1\nC5F1\nC5F5\nC5F6\nC6F6\n");

                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());

                const auto& statex_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(statex_51.ready());
                CHECK(statex_51.value().accepted());

                const auto& statex_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(statex_55.ready());
                CHECK(statex_55.value().accepted());

                const auto& statex_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(statex_56.ready());
                CHECK(statex_56.value().accepted());

                const auto& statex_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(statex_66.ready());
                CHECK(statex_66.value().accepted());
            }).eval(stream);

            REQUIRE(evaluated);
        }

        SECTION("Journal OK when Component 1 with feature 1 rejects") {
            testing::Component<5> component_5{"accept"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"reject"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"accept"}
            );

            pipeline_type pipeline{composition};
            std::stringstream stream;
            bool evaluated = false;

            pipeline.then([&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\n");
                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(!state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(!state_41.ready());

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(!state_51.ready());

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(!state_55.ready());

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(!state_56.ready());

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(!state_66.ready());
            }).eval(stream);

            REQUIRE(evaluated);
        }

        SECTION("Journal OK when Component 5 with feature 1, 5, 6 rejects") {
            testing::Component<5> component_5{"reject"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"accept"}
            );

            pipeline_type pipeline{composition};
            std::stringstream stream;
            bool evaluated = false;
            pipeline.then([&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nX0F1\nC5F1\n");
                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(state_51.ready());
                CHECK(!state_51.value().accepted());

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(!state_55.ready());

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(!state_56.ready());

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(!state_66.ready());
            }).eval(stream);

            REQUIRE(evaluated);
        }

        SECTION("Journal OK when last Component 6 with feature 6 rejects") {
            testing::Component<5> component_5{"accept"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"reject"}
            );

            pipeline_type pipeline{composition};
            std::stringstream stream;
            bool evaluated = false;

            pipeline.then([&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nX0F1\nC5F1\nC5F5\nC5F6\nC6F6\n");
                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(state_51.ready());
                CHECK(state_51.value().accepted());

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(state_55.ready());
                CHECK(state_55.value().accepted());

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(state_56.ready());
                CHECK(state_56.value().accepted());

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(state_66.ready());
                CHECK(!state_66.value().accepted());
            }).eval(stream);

            REQUIRE(evaluated);
        }
    }

    SECTION("Feature-based evaluation pipeline using asio io_context") {
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

        using order_type = udho::manifold::order<
            testing::Feature<0>,
            testing::Feature<1>,
            testing::Feature<2>,
            testing::Feature<3>,
            testing::Feature<4>,
            testing::Feature<5>,
            testing::Feature<6>,
            testing::Feature<7>,
            testing::Feature<8>
            >;

        using pipeline_type = udho::manifold::common_pipepine<1, order_type, composition_type>;

        SECTION("Journal OK when all facets accept") {
            testing::Component<5> component_5{"accept"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"accept"}
                );

            pipeline_type pipeline{composition};
            std::stringstream stream;

            boost::asio::io_context io;

            bool evaluated = true;

            pipeline.then(io, [&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;
                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nX0F1\nC5F1\nC5F5\nC5F6\nC6F6\n");

                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());

                const auto& statex_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(statex_51.ready());
                CHECK(statex_51.value().accepted());

                const auto& statex_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(statex_55.ready());
                CHECK(statex_55.value().accepted());

                const auto& statex_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(statex_56.ready());
                CHECK(statex_56.value().accepted());

                const auto& statex_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(statex_66.ready());
                CHECK(statex_66.value().accepted());
            }).eval(stream);

            io.run();
            REQUIRE(evaluated);
        }

        SECTION("Journal OK when Component 1 with feature 1 rejects") {
            testing::Component<5> component_5{"accept"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"reject"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"accept"}
                );

            pipeline_type pipeline{composition};
            std::stringstream stream;
            bool evaluated = false;
            boost::asio::io_context io;
            pipeline.then(io, [&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\n");
                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(!state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(!state_41.ready());

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(!state_51.ready());

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(!state_55.ready());

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(!state_56.ready());

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(!state_66.ready());
            }).eval(stream);
            io.run();
            REQUIRE(evaluated);
        }

        SECTION("Journal OK when Component 5 with feature 1, 5, 6 rejects") {
            testing::Component<5> component_5{"reject"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"accept"}
                );

            pipeline_type pipeline{composition};
            std::stringstream stream;
            bool evaluated = false;
            boost::asio::io_context io;
            pipeline.then(io, [&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nX0F1\nC5F1\n");
                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(state_51.ready());
                CHECK(!state_51.value().accepted());

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(!state_55.ready());

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(!state_56.ready());

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(!state_66.ready());
            }).eval(stream);
            io.run();

            REQUIRE(evaluated);
        }

        SECTION("Journal OK when last Component 6 with feature 6 rejects") {
            testing::Component<5> component_5{"accept"};
            composition_type composition =  composition_type::compose(
                testing::Component<0>{"accept"},
                testing::Component<1>{"accept"},
                testing::Component<2, 0>{"accept"},
                testing::Component<4, 1>{"accept"},
                component_5,
                testing::Component<6>{"reject"}
            );

            pipeline_type pipeline{composition};
            std::stringstream stream;
            bool evaluated = false;
            boost::asio::io_context io;
            pipeline.then(io, [&stream, &pipeline, &evaluated](std::variant<bool, std::exception_ptr> success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nX0F1\nC5F1\nC5F5\nC5F6\nC6F6\n");
                const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(state_51.ready());
                CHECK(state_51.value().accepted());

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(state_55.ready());
                CHECK(state_55.value().accepted());

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(state_56.ready());
                CHECK(state_56.value().accepted());

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(state_66.ready());
                CHECK(!state_66.value().accepted());
            }).eval(stream);
            io.run();
            REQUIRE(evaluated);
        }
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
