#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>
#include <udho/manifold/composition.h>
#include <udho/manifold/fabric.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/config.h>
#include <udho/hazo/map/element.h>
#include <udho/manifold/evaluator.h>
#include <udho/manifold/pipeline.h>

#include <nlohmann/json.hpp>
#include <iostream>

namespace testing {

template <int... X>
struct name_generator{
    static constexpr const char name[sizeof...(X)+2] ={'C', ((X > 9 ? 65 : 48)+X)..., 0};
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
    using features  = udho::manifold::features<Feature<FeatureIndex>>;

    UDHO_CONFIG_PARAM(enabled,  bool,           false   );
    UDHO_CONFIG_PARAM(p1,       std::string,    "p1"    );
    UDHO_CONFIG_PARAM(p2,       std::uint32_t,  0       );
    UDHO_CONFIG_PARAM(p3,       std::string,    "p3"    );
    UDHO_CONFIG_PARAM(p4,       std::string,     ""     );

    static constexpr const char* name = name_generator<Index, FeatureIndex>::name;

    struct Name: udho::hazo::element<Name , std::string>{
        using base_element = udho::hazo::element<Name , std::string>;

        inline Name(): base_element("FullName") {}
        inline explicit Name(const std::string& value): base_element(value) {}
        Name(const Name&) = default;
        Name(Name&&) = default;
        using base_element::operator=;
        inline static constexpr auto key() {
            using namespace udho::hazo::string::literals;
            return "Name"_h;
        }
    };

    using params = udho::manifold::params<enabled, p1, p2, p3, p4>;

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
struct Component<0, 2> {
    using features  = udho::manifold::features<Feature<2>>;

    static constexpr const std::string_view name = name_generator<0, 2>::name;

    using params = udho::manifold::params<>;

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

    UDHO_CONFIG_PARAM(enabled,  bool,           false   );
    UDHO_CONFIG_PARAM(p1,       std::string,    "p1"    );
    UDHO_CONFIG_PARAM(p2,       std::uint32_t,  0       );
    UDHO_CONFIG_PARAM(p3,       std::string,    "p3"    );
    UDHO_CONFIG_PARAM(p4,       std::string,     ""     );

    static constexpr const std::string_view name = name_generator<5, 5>::name;

    using params = udho::manifold::params<enabled, p1, p2, p3, p4>;

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

    UDHO_CONFIG_PARAM(enabled,  bool,           false   );
    UDHO_CONFIG_PARAM(p1,       std::string,    "p1"    );
    UDHO_CONFIG_PARAM(p2,       std::uint32_t,  0       );
    UDHO_CONFIG_PARAM(p3,       std::string,    "p3"    );
    UDHO_CONFIG_PARAM(p4,       std::string,     ""     );

    static constexpr const char name[3] = {'x', 'c', 0};

    using params = udho::manifold::params<enabled, p1, p2, p3, p4>;

    static constexpr const std::size_t component_index = Index;
    static constexpr const int feature_index = FeatureIndex;

    XComponent(): is_default_constructed(true) {}
    XComponent(const std::string& msg): is_default_constructed(false), message(msg) {}
    XComponent(const XComponent&) = delete;
    XComponent(XComponent&& other) noexcept : is_default_constructed(false), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;
};

using F0 = Feature<0>;
using F1 = Feature<1>;
using F2 = Feature<2>;
using F3 = Feature<3>;
using F4 = Feature<4>;
using F5 = Feature<5>;
using F6 = Feature<6>;

using C00 = Component<0>;
using C01 = Component<0, 1>;
using C02 = Component<0, 2>;
using C11 = Component<1>;
using C22 = Component<2>;
using C20 = Component<2, 0>;
using C33 = Component<3>;
using C44 = Component<4>;
using C41 = Component<4, 1>;
using C55 = Component<5>;
using C66 = Component<6>;

}

namespace udho {
namespace manifold {

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::Component<Index, FeatureIndex>, F> {
    using component_type = testing::Component<Index, FeatureIndex>;
    using feature        = F;
    using result         = testing::State;

    facet(component_type& component, const typename component_type::params& params): _component(component), _params(params) {}

    template <typename... Components>
    result eval(const udho::manifold::journal<Components...>& journal, const boost::asio::ip::address& address, const udho::net::types::headers::request& request) const {
        return result(_component.message == "accept");
    }

private:
    component_type& _component;
    typename component_type::params& _params;
};

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::XComponent<Index, FeatureIndex>, F> {
    using component_type = testing::XComponent<Index, FeatureIndex>;
    using feature = F;

    facet(component_type& component, const typename component_type::params& params): _component(component), _params(params) {}

private:
    component_type& _component;
    typename component_type::params& _params;
};


}
}

template <>
struct udho::manifold::component_traits<testing::Component<5>> {
    static constexpr const bool shared = true;
    using result = testing::State;
    using params = udho::manifold::params<>;
};


TEST_CASE("config instance", "[manifold][config]") {
    udho::manifold::configs<testing::C02> configs_c02;
    udho::manifold::configs<testing::C00> configs_c00;
    udho::manifold::configs<testing::C00, testing::C11> configs_c00_11;
}

TEST_CASE("manifold components params & config", "[manifold][config][params]") {
    SECTION("save and load params from json") {
        using params_type = testing::C00::params;

        params_type params;
        nlohmann::json params_json;
        params.save(params_json);
        CHECK(params_json == nlohmann::json::parse(R"({"enabled":false,"p1":"p1","p2":0,"p3":"p3","p4":""})"));

        nlohmann::json saved_params = nlohmann::json::parse(R"({"enabled":true,"p1":"vp1","p2":1,"p3":"vp3","p4":"vp4"})");
        params.load(saved_params);
        CHECK(params[testing::C00::enabled::val].value());
        CHECK(params[testing::C00::p1::val] == "vp1");
        CHECK(params[testing::C00::p2::val] == 1);
        CHECK(params[testing::C00::p3::val] == "vp3");
        CHECK(params[testing::C00::p4::val] == "vp4");
    }

    SECTION("save and load config from json") {
        using config_type = udho::manifold::config<testing::C00>;
        config_type config;
        nlohmann::json config_json = nlohmann::json::object();
        config.save(config_json);
        CHECK(config_json == nlohmann::json::parse(R"({"C00": {"enabled":false,"p1":"p1","p2":0,"p3":"p3","p4":""}})"));

        nlohmann::json saved_config = nlohmann::json::parse(R"({"C00": {"enabled":true,"p1":"vp1","p2":1,"p3":"vp3","p4":"vp4"}})");
        config.load(saved_config);
        CHECK(config[testing::C00::enabled::val].value());
        CHECK(config[testing::C00::p1::val] == "vp1");
        CHECK(config[testing::C00::p2::val] == 1);
        CHECK(config[testing::C00::p3::val] == "vp3");
        CHECK(config[testing::C00::p4::val] == "vp4");
    }

    SECTION("save and load empty params from json") {
        using params_type = udho::manifold::params<>;

        params_type params;
        nlohmann::json params_json = nlohmann::json::array();
        params.save(params_json);
        CHECK(params_json == nlohmann::json::object());

        nlohmann::json saved_params = nlohmann::json::parse(R"({"Name":"Neel Basu","hostname":"127.0.0.1","password":"","port":5432,"username":"postgres"})");
        params.load(saved_params);
    }

}
