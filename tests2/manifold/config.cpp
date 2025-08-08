#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/manifold/composition.h>
#include <udho/manifold/mediator.h>
#include <udho/manifold/state.h>
#include <udho/manifold/config.h>
#include <udho/manifold/evaluator.h>
#include <udho/manifold/pipeline.h>

#include <nlohmann/json.hpp>
#include <iostream>

namespace testing{

UDHO_CONFIG_PARAM(enabled,  bool,           false       );
UDHO_CONFIG_PARAM(hostname, std::string,    "localhost" );
UDHO_CONFIG_PARAM(port,     std::uint32_t,  3306        );
UDHO_CONFIG_PARAM(username, std::string,    "root"      );
UDHO_CONFIG_PARAM(password, std::string,    ""          );

struct Name: udho::hazo::element<Name , std::string>{
    inline Name(): element("FullName") {}
    inline explicit Name(const std::string& value): element(value) {}
    Name(const Name&) = default;
    Name(Name&&) = default;
    using element::operator=;
    inline static constexpr auto key() {
        using namespace udho::hazo::string::literals;
        return "Name"_h;
    }
};

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

    bool is_default_constructed;
    std::string message;
};

template <std::size_t Index, int FeatureIndex = Index>
struct XComponent {
    using feature = Feature<FeatureIndex>;

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

namespace udho::manifold {

template <std::size_t Index, int FeatureIndex>
struct delegate<testing::Component<Index, FeatureIndex>> {
    using component_type = testing::Component<Index, FeatureIndex>;
    using state_type     = typename testing::Component<Index, FeatureIndex>::state;

    delegate(component_type& component): _component(component) {}

    template <typename... Components>
    state_type eval(const udho::manifold::states<Components...>& states, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        return state_type(_component.message == "accept");
    }

private:
    component_type& _component;
};

template <std::size_t Index, int FeatureIndex>
struct delegate<testing::XComponent<Index, FeatureIndex>> {
    using component_type = testing::XComponent<Index, FeatureIndex>;

    delegate(component_type& component): _component(component) {}

private:
    component_type& _component;
};

}

template <>
struct udho::manifold::component_traits<testing::Component<5>> {
    static constexpr const bool shared = true;
    using state = testing::State;
};


TEST_CASE("manifold components params", "[manifold][config][params]") {
    testing::hostname hostname{"localhost"};
    testing::port port{3306};

    SECTION("save and load from json") {
        using config_type = udho::manifold::params<
            testing::hostname,
            testing::port,
            testing::username,
            testing::password,
            testing::Name
        >;

        config_type config;
        nlohmann::json obj;
        config.save(obj);

        nlohmann::json saved_config = nlohmann::json::parse(R"({"Name":"Neel Basu","hostname":"127.0.0.1","password":"","port":5432,"username":"postgres"})");
        config.load(saved_config);
    }

    SECTION("save and load empty params from json") {
        using config_type = udho::manifold::params<>;

        config_type config;
        nlohmann::json obj;
        config.save(obj);

        nlohmann::json saved_config = nlohmann::json::parse(R"({"Name":"Neel Basu","hostname":"127.0.0.1","password":"","port":5432,"username":"postgres"})");
        config.load(saved_config);
    }
}

TEST_CASE("manifold components config", "[manifold][config]") {
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

    using delegates_type = composition_type::delegates_type;

    using expected_states_type = udho::manifold::detail::states_for_composition<composition_type>::type;

    using states_type = udho::manifold::states<
        testing::Component<0>,
        testing::Component<1>,
        testing::Component<2, 0>,
        testing::Component<3>,
        testing::Component<4, 1>,
        testing::Component<5>,
        testing::Component<6>
        >;

    static_assert(std::is_same_v<expected_states_type, states_type>);

    testing::Component<5> component_5;

    auto composition = composition_type::compose(
        testing::XComponent<0, 1>{},
        testing::Component<0>{"accept"},  // Will evaluate to true
        testing::Component<1>{"reject"},  // Will evaluate to false
        component_5
    );

    states_type states;

    boost::asio::ip::address address;
    udho::net::types::headers::request request;

    delegates_type delegates{composition};
}
