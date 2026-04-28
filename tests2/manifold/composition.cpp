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
#include <udho/manifold/config.h>
#include <iostream>

namespace testing {

namespace detail{
constexpr std::size_t digits10(std::size_t n) {
    std::size_t d = 1;
    while (n >= 10) { n /= 10; ++d; }
    return d;
}

template <std::size_t N>
constexpr std::size_t lit_len(const char (&)[N]) { return N - 1; } // exclude '\0'

constexpr std::size_t cstrlen(const char* s) {
    std::size_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

// ---- writer that appends into a char buffer at compile time ----
struct writer {
    char* out;
    std::size_t pos = 0;

    constexpr explicit writer(char* p) : out(p) {}

    constexpr void ch(char c) { out[pos++] = c; }

    template <std::size_t N>
    constexpr void lit(const char (&s)[N]) {
        for (std::size_t i = 0; i < N - 1; ++i) out[pos++] = s[i];
    }

    constexpr void bytes(const char* s, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) out[pos++] = s[i];
    }

    constexpr void sv(udho::utils::string_view s) {
        for (std::size_t i = 0; i < s.size(); ++i) out[pos++] = s[i];
    }

    // writes decimal digits of n (no '\0')
    constexpr void udec(std::size_t n) {
        char tmp[20] = {};            // enough for 64-bit size_t (max 20 digits)
        std::size_t len = 0;
        do {
            tmp[len++] = static_cast<char>('0' + (n % 10));
            n /= 10;
        } while (n != 0);

        for (std::size_t i = 0; i < len; ++i)
            out[pos++] = tmp[len - 1 - i];
    }
};

// Build a null-terminated buffer of exactly Len chars (Len includes '\0')
template <std::size_t Len, typename F>
constexpr std::array<char, Len> build_cstr(F f) {
    std::array<char, Len> a{};
    writer w{a.data()};
    f(w);
    w.ch('\0');
    return a;
}

// Prefix storage used as NTTP pointers (const char*) in C++17.
// Must have external linkage -> "inline constexpr" at namespace scope is OK in C++17.
inline constexpr char feature_prefix[]  = "Feature<";
inline constexpr char xfeature_prefix[] = "XFeature<";

// Unified tagged feature name factory.
template <const char* Prefix, std::size_t Idx>
constexpr auto make_tagged_feature_name() {
    constexpr std::size_t prefix_len = cstrlen(Prefix);

    constexpr std::size_t len_no_null = prefix_len + digits10(Idx) + 1 /*>*/;

    return build_cstr<len_no_null + 1>([](writer& w) constexpr {
        w.bytes(Prefix, cstrlen(Prefix));
        w.udec(Idx);
        w.ch('>');
    });
}

// Component name uses Fs::name (string_view) so it works for Feature.
inline constexpr char component_prefix[]  = "Component<";
inline constexpr char xcomponent_prefix[] = "XComponent<";
template <const char* Prefix, std::size_t Idx, std::size_t Fi>
constexpr auto make_component_name() {
    constexpr std::size_t len_no_null = cstrlen(Prefix) + digits10(Idx) + 1 /*,*/ + lit_len("Feature<") + digits10(Fi) + 1 /*>*/ + 1/*>*/;

    return build_cstr<len_no_null + 1>([&](writer& w) constexpr {
        w.bytes(Prefix, cstrlen(Prefix));
        w.udec(Idx);
        w.ch(',');
        w.lit("Feature<");
        w.udec(Fi);
        w.ch('>');
        w.ch('>');
    });
}

}

struct State {
    State() = delete;
    State(const State&) = default;
    inline explicit State(bool val) : _value(val) {
        static std::size_t counter = 0;
        _counter = ++counter;
    }
    inline bool accepted() const { return _value; }

    bool _value;
    std::size_t _counter;
};


template <std::size_t Index>
struct Feature{
    static constexpr const std::size_t idx = Index;
    static constexpr const std::size_t stage = 1;
    using result    = State;

private:
    inline static constexpr auto _name_storage = detail::make_tagged_feature_name<detail::feature_prefix, Index>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1 };
};

template <std::size_t Index>
struct XFeature{
    static constexpr const std::size_t idx = Index;
    static constexpr const std::size_t stage = 1;

private:
    inline static constexpr auto _name_storage = detail::make_tagged_feature_name<detail::feature_prefix, Index>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1 };
};


template <std::size_t Index, int FeatureIndex = Index>
struct Component {
    using features  = udho::manifold::features<Feature<FeatureIndex>>;

    static constexpr const std::size_t component_index = Index;
    static constexpr const int feature_index = FeatureIndex;

    Component(): is_default_constructed(true) {}
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other) noexcept : is_default_constructed(std::move(other.is_default_constructed)), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;

private:
    inline static constexpr auto _name_storage = detail::make_component_name<detail::component_prefix, Index, FeatureIndex>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1};
};

template <>
struct Component<5, 5> {
    using features  = udho::manifold::features<Feature<1>, Feature<5>, Feature<6>>;

    static constexpr const std::size_t component_index = 5;
    static constexpr const int feature_index = 5;

    Component(): is_default_constructed(true) {}
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other) noexcept : is_default_constructed(other.is_default_constructed), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;

    inline static constexpr udho::utils::string_view name = "Component<Feature<1>, Feature<5>, Feature<6>>";
};

template <std::size_t Index, int FeatureIndex = Index>
struct XComponent {
    using features = udho::manifold::features<XFeature<FeatureIndex>>;

    static constexpr const std::size_t component_index = Index;
    static constexpr const int feature_index = FeatureIndex;

    XComponent(): is_default_constructed(true) {}
    XComponent(const std::string& msg): is_default_constructed(false), message(msg) {}
    XComponent(const XComponent&) = delete;
    XComponent(XComponent&& other) noexcept : is_default_constructed(other.is_default_constructed), message(std::move(other.message))  { }

    bool is_default_constructed;
    std::string message;

private:
    inline static constexpr auto _name_storage = detail::make_component_name<detail::xcomponent_prefix, Index, FeatureIndex>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1};
};

template <>
struct Feature<3> {
    static constexpr const std::size_t stage = 0;

private:
    inline static constexpr auto _name_storage = detail::make_tagged_feature_name<detail::feature_prefix, 3>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1 };
};

}

namespace udho {
namespace manifold {

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::Component<Index, FeatureIndex>, F> {
    using component_type = testing::Component<Index, FeatureIndex>;
    using feature        = F;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

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
        const config& _config;
};

template <std::size_t Index, int FeatureIndex, typename F>
struct facet<testing::XComponent<Index, FeatureIndex>, F> {
    using component_type = testing::XComponent<Index, FeatureIndex>;
    using feature        = F;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "X" << Index << "F" << FeatureIndex << std::endl;
        next.pass();
    }

    private:
        component_type& _component;
        const config& _config;
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

        CHECK(composition.at<testing::XFeature<1>, 0>().component().component_index == 0);
        CHECK(composition.at<testing::Feature<1>, 0>().component().component_index == 1);
        CHECK(composition.at<testing::Feature<1>, 1>().component().component_index == 4);
        CHECK(composition.at<testing::Feature<1>, 2>().component().component_index == 5);
        CHECK(composition.count<testing::XFeature<1>>() == 1);
        CHECK(composition.count<testing::Feature<1>>() == 3);
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

    using configs_type = udho::manifold::configs<
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
        udho::manifold::facet<testing::XComponent<0, 1>, testing::XFeature<1>>,
        udho::manifold::facet<testing::Component<1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>,
        udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<1>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<5>>,
        udho::manifold::facet<testing::Component<5>, testing::Feature<6>>,
        udho::manifold::facet<testing::Component<6>, testing::Feature<6>>
    >;
    static_assert(std::is_same_v<expected_fabric_type, fabric_type>);

    configs_type conf;
    fabric_type fabric{composition, conf, 0};

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

    SECTION("Initially all facets in the journal are unevaluated") {
        journal_type journal;
        CHECK(!journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        CHECK(!journal.at<testing::Feature<0>>().ready());
        CHECK(!journal.at<testing::Feature<0>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>>().ready());
        CHECK(!journal.at<testing::Feature<1>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>, 2>().ready());
        CHECK(!journal.at<testing::Feature<5>>().ready());
        CHECK(!journal.at<testing::Feature<6>, 0>().ready());
        CHECK(!journal.at<testing::Feature<6>, 1>().ready());
    }

    SECTION("Setting first facet result makes it ready and keeps all unevaluated") {
        journal_type journal;

        auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
        state_00 = testing::State{true};
        CHECK(state_00.ready());
        CHECK(state_00.value().accepted());

        CHECK(journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        CHECK(journal.at<testing::Feature<0>>().ready());
        CHECK(!journal.at<testing::Feature<0>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>>().ready());
        CHECK(!journal.at<testing::Feature<1>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>, 2>().ready());
        CHECK(!journal.at<testing::Feature<5>>().ready());
        CHECK(!journal.at<testing::Feature<6>, 0>().ready());
        CHECK(!journal.at<testing::Feature<6>, 1>().ready());

        CHECK(journal.at<testing::Feature<0>>().value().accepted());
    }

    SECTION("Setting first facet result rejected makes it ready and keeps all unevaluated") {
        journal_type journal;

        auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
        state_00 = testing::State{false};
        CHECK(state_00.ready());
        CHECK(!state_00.value().accepted());

        CHECK(journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        CHECK(journal.at<testing::Feature<0>>().ready());
        CHECK(!journal.at<testing::Feature<0>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>>().ready());
        CHECK(!journal.at<testing::Feature<1>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>, 2>().ready());
        CHECK(!journal.at<testing::Feature<5>>().ready());
        CHECK(!journal.at<testing::Feature<6>, 0>().ready());
        CHECK(!journal.at<testing::Feature<6>, 1>().ready());

        CHECK(!journal.at<testing::Feature<0>>().value().accepted());
    }

    SECTION("Setting another one facet result makes it ready and keeps all unevaluated") {
        journal_type journal;

        auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
        state_51 = testing::State{true};
        CHECK(state_51.ready());
        CHECK(state_51.value().accepted());

        CHECK(!journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>().ready());
        CHECK(!journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        CHECK(!journal.at<testing::Feature<0>>().ready());
        CHECK(!journal.at<testing::Feature<0>, 1>().ready());
        CHECK(!journal.at<testing::Feature<1>>().ready());
        CHECK(!journal.at<testing::Feature<1>, 1>().ready());
        CHECK(journal.at<testing::Feature<1>, 2>().ready());
        CHECK(!journal.at<testing::Feature<5>>().ready());
        CHECK(!journal.at<testing::Feature<6>, 0>().ready());
        CHECK(!journal.at<testing::Feature<6>, 1>().ready());
    }

    SECTION("Initially all facets in the journal are evaluated") {
        journal_type journal;

        journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>() = testing::State{true};
        journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>() = testing::State{true};

        CHECK(journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        CHECK(journal.at<testing::Feature<0>>().ready());
        CHECK(journal.at<testing::Feature<0>, 1>().ready());
        CHECK(journal.at<testing::Feature<1>>().ready());
        CHECK(journal.at<testing::Feature<1>, 1>().ready());
        CHECK(journal.at<testing::Feature<1>, 2>().ready());
        CHECK(journal.at<testing::Feature<5>>().ready());
        CHECK(journal.at<testing::Feature<6>, 0>().ready());
        CHECK(journal.at<testing::Feature<6>, 1>().ready());

        CHECK(journal.at<testing::Feature<0>>().value().accepted());
        CHECK(journal.at<testing::Feature<0>, 1>().value().accepted());
        CHECK(journal.at<testing::Feature<1>>().value().accepted());
        CHECK(journal.at<testing::Feature<1>, 1>().value().accepted());
        CHECK(journal.at<testing::Feature<1>, 2>().value().accepted());
        CHECK(journal.at<testing::Feature<5>>().value().accepted());
        CHECK(journal.at<testing::Feature<6>, 0>().value().accepted());
        CHECK(journal.at<testing::Feature<6>, 1>().value().accepted());
    }

    SECTION("Initially all facets in the journal are evaluated but rejected") {
        journal_type journal;

        journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>() = testing::State{false};
        journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>() = testing::State{false};

        CHECK(journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>().ready());
        CHECK(journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>().ready());

        CHECK(journal.at<testing::Feature<0>>().ready());
        CHECK(journal.at<testing::Feature<0>, 1>().ready());
        CHECK(journal.at<testing::Feature<1>>().ready());
        CHECK(journal.at<testing::Feature<1>, 1>().ready());
        CHECK(journal.at<testing::Feature<1>, 2>().ready());
        CHECK(journal.at<testing::Feature<5>>().ready());
        CHECK(journal.at<testing::Feature<6>, 0>().ready());
        CHECK(journal.at<testing::Feature<6>, 1>().ready());

        CHECK(!journal.at<testing::Feature<0>>().value().accepted());
        CHECK(!journal.at<testing::Feature<0>, 1>().value().accepted());
        CHECK(!journal.at<testing::Feature<1>>().value().accepted());
        CHECK(!journal.at<testing::Feature<1>, 1>().value().accepted());
        CHECK(!journal.at<testing::Feature<1>, 2>().value().accepted());
        CHECK(!journal.at<testing::Feature<5>>().value().accepted());
        CHECK(!journal.at<testing::Feature<6>, 0>().value().accepted());
        CHECK(!journal.at<testing::Feature<6>, 1>().value().accepted());
    }
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
        using conffigs_type = udho::manifold::configs<
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

        // journal_type journal;

        boost::asio::ip::address address;
        udho::net::types::headers::request request;

        conffigs_type configs;
        fabric_type fabric{composition, configs, 0};

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
            testing::XFeature<1>,
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


            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;

            bool evaluated = true;

            pipeline.then([&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;
                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nC5F1\nX0F1\nC5F5\nC5F6\nC6F6\n");

                // const auto& journal = pipeline.journal();

                const auto& state_00 = journal.get<udho::manifold::facet<testing::Component<0>, testing::Feature<0>>>();
                CHECK(state_00.ready());
                CHECK(state_00.value().accepted());
                CHECK(state_00->_counter == journal.at<testing::Feature<0>>()->_counter);

                const auto& state_20 = journal.get<udho::manifold::facet<testing::Component<2, 0>, testing::Feature<0>>>();
                CHECK(state_20.ready());
                CHECK(state_20.value().accepted());
                CHECK(state_20->_counter == journal.at<testing::Feature<0>, 1>()->_counter);

                const auto& state_11 = journal.get<udho::manifold::facet<testing::Component<1>, testing::Feature<1>>>();
                CHECK(state_11.ready());
                CHECK(state_11.value().accepted());
                CHECK(state_11->_counter == journal.at<testing::Feature<1>, 0>()->_counter);

                const auto& state_41 = journal.get<udho::manifold::facet<testing::Component<4, 1>, testing::Feature<1>>>();
                CHECK(state_41.ready());
                CHECK(state_41.value().accepted());
                CHECK(state_41->_counter == journal.at<testing::Feature<1>, 1>()->_counter);

                const auto& state_51 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<1>>>();
                CHECK(state_51.ready());
                CHECK(state_51.value().accepted());
                CHECK(state_51->_counter == journal.at<testing::Feature<1>, 2>()->_counter);

                const auto& state_55 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<5>>>();
                CHECK(state_55.ready());
                CHECK(state_55.value().accepted());
                CHECK(state_55->_counter == journal.at<testing::Feature<5>>()->_counter);

                const auto& state_56 = journal.get<udho::manifold::facet<testing::Component<5>, testing::Feature<6>>>();
                CHECK(state_56.ready());
                CHECK(state_56.value().accepted());
                CHECK(state_56->_counter == journal.at<testing::Feature<6>, 0>()->_counter);

                const auto& state_66 = journal.get<udho::manifold::facet<testing::Component<6>, testing::Feature<6>>>();
                CHECK(state_66.ready());
                CHECK(state_66.value().accepted());
                CHECK(state_66->_counter == journal.at<testing::Feature<6>, 1>()->_counter);

                CHECK(success);
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;
            bool evaluated = false;

            pipeline.then([&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\n");
                // const auto& journal = pipeline.journal();

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

                CHECK(!success);
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;
            bool evaluated = false;
            pipeline.then([&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nC5F1\n");
                // const auto& journal = pipeline.journal();

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

                CHECK(!success);
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;
            bool evaluated = false;

            pipeline.then([&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nC5F1\nX0F1\nC5F5\nC5F6\nC6F6\n");
                // const auto& journal = pipeline.journal();

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

                CHECK(!success);
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
            testing::XFeature<1>,
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;

            boost::asio::io_context io;

            bool evaluated = true;

            pipeline.then(io, [&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;
                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nC5F1\nX0F1\nC5F5\nC5F6\nC6F6\n");

                // const auto& journal = pipeline.journal();

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
                CHECK(state_66.value().accepted());

                CHECK(success);
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;
            bool evaluated = false;
            boost::asio::io_context io;
            pipeline.then(io, [&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\n");
                // const auto& journal = pipeline.journal();

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

                CHECK(!success);
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;
            bool evaluated = false;
            boost::asio::io_context io;
            pipeline.then(io, [&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nC5F1\n");
                // const auto& journal = pipeline.journal();

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

                CHECK(!success);
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

            using configs_type  = pipeline_type::configs_type;
            using full_journal_type = pipeline_type::full_journal_type;

            full_journal_type journal;
            configs_type configs;
            pipeline_type pipeline{composition, configs, journal, 0};
            std::stringstream stream;
            bool evaluated = false;
            boost::asio::io_context io;
            pipeline.then(io, [&stream, &pipeline, &journal, &evaluated](udho::manifold::evaluation_result success){
                evaluated = true;

                CHECK(stream.str() == "C0F0\nC2F0\nC1F1\nC4F1\nC5F1\nX0F1\nC5F5\nC5F6\nC6F6\n");
                // const auto& journal = pipeline.journal();

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

                CHECK(!success);
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
