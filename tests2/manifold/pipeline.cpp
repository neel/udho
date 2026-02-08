#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <boost/asio/io_context.hpp>
#include <udho/manifold/fwd.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/features.h>
#include <udho/manifold/fabric.h>
#include <udho/manifold/evaluator.h>
#include <udho/manifold/pipeline.h>
#include <udho/manifold/config.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/params.h>
#include <udho/manifold/order.h>
#include <udho/manifold/transition.h>
#include <udho/manifold/runtime.h>
#include <udho/manifold/flow.h>
#include <udho/manifold/terminal.h>
#include <udho/utils/string_view.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

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
template <const char* Prefix, std::size_t Stage, std::size_t Idx>
constexpr auto make_tagged_feature_name() {
    constexpr std::size_t prefix_len = cstrlen(Prefix);

    constexpr std::size_t len_no_null =
        prefix_len + digits10(Stage) + 1 /*,*/ + digits10(Idx) + 1 /*>*/;

    return build_cstr<len_no_null + 1>([](writer& w) constexpr {
        w.bytes(Prefix, cstrlen(Prefix));
        w.udec(Stage);
        w.ch(',');
        w.udec(Idx);
        w.ch('>');
    });
}

// Component name uses Fs::name (string_view) so it works for Feature and XFeature.
template <std::size_t Idx, typename... Fs>
constexpr auto make_component_name() {
    constexpr std::size_t nfs = sizeof...(Fs);
    constexpr bool has_fs = (nfs != 0);

    constexpr std::size_t fs_sum = (std::size_t{0} + ... + Fs::name.size());
    constexpr std::size_t commas = has_fs ? (nfs - 1) : 0;
    constexpr std::size_t fs_block = has_fs ? (1 /*,*/ + fs_sum + commas) : 0;

    constexpr std::size_t len_no_null =
        lit_len("Component<") + digits10(Idx) + fs_block + 1 /*>*/;

    return build_cstr<len_no_null + 1>([](writer& w) constexpr {
        w.lit("Component<");
        w.udec(Idx);

        if constexpr (has_fs) {
            w.ch(',');
            bool first = true;
            auto add = [&](udho::utils::string_view sv) constexpr {
                if (!first) w.ch(',');
                first = false;
                w.sv(sv);
            };
            (add(Fs::name), ...);
        }

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

// Features with explicit stage numbers
template <std::size_t Stage, std::size_t Idx>
struct Feature{
    static constexpr const std::size_t stage = Stage;
    static constexpr const std::size_t idx = Idx;
    using result    = State;

private:
    inline static constexpr auto _name_storage = detail::make_tagged_feature_name<detail::feature_prefix, Stage, Idx>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1 };
};

// X-Features (no result) with explicit stage numbers
template <std::size_t Stage, std::size_t Idx>
struct XFeature{
    static constexpr const std::size_t stage = Stage;
    static constexpr const std::size_t idx = Idx;
private:
    inline static constexpr auto _name_storage = detail::make_tagged_feature_name<detail::xfeature_prefix, Stage, Idx>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1 };
};

template <std::size_t Idx, typename... Fs>
struct Component;

// Component template - provides whatever features are specified
template <std::size_t Idx, typename... Fs>
struct Component {
    using features = udho::manifold::features<Fs...>;

    UDHO_CONFIG_PARAM(enabled,  bool,           false   );
    UDHO_CONFIG_PARAM(param,    std::string,    "default");

    using params = udho::manifold::params<enabled, param>;

    Component(): is_default_constructed(true) {}
    Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Component(const Component&) = delete;
    Component(Component&& other) noexcept: is_default_constructed(false), message(std::move(other.message)) {}

    bool is_default_constructed;
    std::string message;

private:
    inline static constexpr auto _name_storage = detail::make_component_name<Idx, Fs...>();

public:
    inline static constexpr udho::utils::string_view name{_name_storage.data(), _name_storage.size() - 1};
};

} // namespace testing

// Alias features for readability
namespace testing {
    // Stage 0 Features
    using F00 = Feature<0, 0>;  // Stage 0, Feature 0
    using F01 = Feature<0, 1>;  // Stage 0, Feature 1

    // Stage 1 Features
    using F10 = Feature<1, 0>;  // Stage 1, Feature 0
    using F11 = Feature<1, 1>;  // Stage 1, Feature 1
    using X10 = XFeature<1, 0>; // Stage 1, XFeature 0
    using X11 = XFeature<1, 1>; // Stage 1, XFeature 1

    // Stage 2 Features
    using F20 = Feature<2, 0>;  // Stage 2, Feature 0
    using F21 = Feature<2, 1>;  // Stage 2, Feature 1
    using F22 = Feature<2, 2>;  // Stage 2, Feature 2
    using F23 = Feature<2, 3>;  // Stage 2, Feature 3
    using F24 = Feature<2, 4>;  // Stage 2, Feature 4

    // Define components with their features
    // Stage 0 components
    using C00 = Component<0, F00>;  // Provides F00 (Stage 0)
    using C01 = Component<1, F01>;  // Provides F01 (Stage 0)

    // Stage 1 components
    using C10 = Component<2, F10, X10>;  // Provides F10 and X10 (both Stage 1)
    using C11 = Component<3, F11, X11>;  // Provides F11 and X11 (both Stage 1)

    // Stage 2 components
    using C20 = Component<4, F20, F23>;  // Provides F20 and F23 (both Stage 2)
    using C21 = Component<5, F21, F24>;  // Provides F21 and F24 (both Stage 2)

    // Multi-stage component (provides features in multiple stages)
    using MSC = Component<6, F00, F11, F22>;  // Provides F00 (Stage 0), F11 (Stage 1), F22 (Stage 2)

} // namespace testing

// Facet specializations - now much clearer with explicit stage numbers
namespace udho {
namespace manifold {

// Stage 0 facets
template <>
struct facet<testing::C00, testing::F00> {
    using component_type = testing::C00;
    using feature        = testing::F00;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C00_F00(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept"};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C01, testing::F01> {
    using component_type = testing::C01;
    using feature        = testing::F01;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C01_F01(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept"};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

// Stage 1 facets
template <>
struct facet<testing::C10, testing::F10> {
    using component_type = testing::C10;
    using feature        = testing::F10;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C10_F10(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept" && _config[component_type::enabled::val].value()};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C10, testing::X10> {
    using component_type = testing::C10;
    using feature        = testing::X10;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C10_X10(param=" << _config[component_type::param::val].value() << ")[PASS]\n";
        next.pass();
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C11, testing::F11> {
    using component_type = testing::C11;
    using feature        = testing::F11;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C11_F11(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept" && _config[component_type::enabled::val].value()};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C11, testing::X11> {
    using component_type = testing::C11;
    using feature        = testing::X11;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C11_X11(param=" << _config[component_type::param::val].value() << ")[PASS]\n";
        next.pass();
    }

private:
    component_type& _component;
    const config& _config;
};

// Stage 2 facets
template <>
struct facet<testing::C20, testing::F20> {
    using component_type = testing::C20;
    using feature        = testing::F20;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C20_F20(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept" && _config[component_type::enabled::val].value()};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C20, testing::F23> {
    using component_type = testing::C20;
    using feature        = testing::F23;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C20_F23(param=" << _config[component_type::param::val].value() << ")[PASS]\n";
        result res{true}; // Always pass
        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C21, testing::F21> {
    using component_type = testing::C21;
    using feature        = testing::F21;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C21_F21(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept" && _config[component_type::enabled::val].value()};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::C21, testing::F24> {
    using component_type = testing::C21;
    using feature        = testing::F24;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "C21_F24(param=" << _config[component_type::param::val].value() << ")[PASS]\n";
        result res{true}; // Always pass
        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

// Multi-stage component facets
template <>
struct facet<testing::MSC, testing::F00> {
    using component_type = testing::MSC;
    using feature        = testing::F00;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "MSC_F00(param=" << _config[component_type::param::val].value() << ")";
        result res{_component.message == "accept"};
        stream << (res.accepted() ? "[PASS]" : "[FAIL]") << "\n";

        if(res.accepted()) {
            next.pass(std::move(res));
        } else {
            next.fail(std::move(res));
        }
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::MSC, testing::F11> {
    using component_type = testing::MSC;
    using feature        = testing::F11;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "MSC_F11(param=" << _config[component_type::param::val].value() << ")[PASS]\n";
        result res{true};
        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::MSC, testing::F22> {
    using component_type = testing::MSC;
    using feature        = testing::F22;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "MSC_F22(param=" << _config[component_type::param::val].value() << ")[PASS]\n";
        result res{true};
        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

} // namespace manifold
} // namespace udho

// Define test labels
namespace testing {
    struct Label1 {};
    struct Label2 {};
    struct Label3 {};
} // namespace testing

// Sketch specializations
namespace udho {
namespace manifold {

// Full 3-stage pipeline
template <>
struct sketch<testing::Label1> {
    using composition_type = composition<
        testing::C00,  // Stage 0: F00
        testing::C01,  // Stage 0: F01
        testing::C10,  // Stage 1: F10, X10
        testing::C11,  // Stage 1: F11, X11
        testing::C20,  // Stage 2: F20, F23
        testing::C21,  // Stage 2: F21, F24
        testing::MSC   // Multi-stage: F00(0), F11(1), F22(2)
    >;
    using order_type = order<
        // Stage 0 features
        testing::F00,  // From C00 and MSC
        testing::F01,  // From C01

        // Stage 1 features
        testing::F10,  // From C10
        testing::F11,  // From C11 and MSC
        testing::X10,  // From C10
        testing::X11,  // From C11

        // Stage 2 features
        testing::F20,  // From C20
        testing::F21,  // From C21
        testing::F23,  // From C20
        testing::F24,  // From C21
        testing::F22   // From MSC
    >;
    // static constexpr std::size_t Count = 3;
};

// Single stage pipeline
template <>
struct sketch<testing::Label2> {
    using composition_type = composition<
        testing::C00,  // Stage 0: F00
        testing::C01   // Stage 0: F01
    >;
    using order_type = order<
        testing::F00,
        testing::F01
    >;
    // static constexpr std::size_t Count = 1;
};

// Complex multi-feature pipeline
template <>
struct sketch<testing::Label3> {
    using composition_type = composition<
        testing::MSC,  // F00(0), F11(1), F22(2)
        testing::C10,  // F10(1), X10(1)
        testing::C20   // F20(2), F23(2)
    >;
    using order_type = order<
        testing::F00,  // Stage 0 from MSC
        testing::F10,  // Stage 1 from C10
        testing::X10,  // Stage 1 from C10
        testing::F11,  // Stage 1 from MSC
        testing::F20,  // Stage 2 from C20
        testing::F23,  // Stage 2 from C20
        testing::F22   // Stage 2 from MSC
    >;
    // static constexpr std::size_t Count = 3;
};

// Patch config specializations
template <typename StreamT>
struct default_transition<testing::Label1, StreamT, 0> {
    using label_type        = testing::Label1;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    static constexpr std::size_t Count = runtime_type::Count;

    using pipeline_type     = pipeline<composition_type, order_type, Count, 0>;
    using next_config_type  = typename pipeline<composition_type, order_type, Count, 1>::configs_type;

    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, next_config_type& config, Args&&... args) {
        // Example patch: modify C10's param for stage 1
        config[testing::C10::param::val] = "patched-by-stage0";
        p.next(flow, std::forward<Args>(args)...);
    }
};

template <typename StreamT>
struct default_transition<testing::Label1, StreamT, 1> {
    using label_type        = testing::Label1;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    static constexpr std::size_t Count = runtime_type::Count;

    using pipeline_type     = pipeline<composition_type, order_type, Count, 1>;
    using next_config_type  = typename pipeline<composition_type, order_type, Count, 2>::configs_type;

    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, next_config_type& config, Args&&... args) {
        // Example patch: modify C20's param for stage 2
        config[testing::C20::param::val] = "patched-by-stage1";
        p.next(flow, std::forward<Args>(args)...);
    }
};

} // namespace manifold
} // namespace udho

TEST_CASE("Pipeline System - Fabric Verification", "[manifold][pipeline][fabric]") {
    using label_type = testing::Label1;
    using sketch_type = udho::manifold::sketch<label_type>;
    using composition_type = typename sketch_type::composition_type;

    SECTION("Verify fabric stage membership") {
        // Print fabric for each stage to verify
        using fabric_0_type = composition_type::fabric_type<0>;
        using fabric_1_type = composition_type::fabric_type<1>;
        using fabric_2_type = composition_type::fabric_type<2>;

        // Verify counts
        CHECK(fabric_0_type::count<testing::F00>() == 2); // C00 and MSC provide F00
        CHECK(fabric_0_type::count<testing::F01>() == 1); // Only C01 provides F01

        CHECK(fabric_1_type::count<testing::F10>() == 1); // Only C10 provides F10
        CHECK(fabric_1_type::count<testing::F11>() == 2); // C11 and MSC provide F11
        CHECK(fabric_1_type::count<testing::X10>() == 1); // Only C10 provides X10
        CHECK(fabric_1_type::count<testing::X11>() == 1); // Only C11 provides X11

        CHECK(fabric_2_type::count<testing::F20>() == 1); // Only C20 provides F20
        CHECK(fabric_2_type::count<testing::F21>() == 1); // Only C21 provides F21
        CHECK(fabric_2_type::count<testing::F22>() == 1); // Only MSC provides F22
        CHECK(fabric_2_type::count<testing::F23>() == 1); // Only C20 provides F23
        CHECK(fabric_2_type::count<testing::F24>() == 1); // Only C21 provides F24
    }
}

TEST_CASE("Pipeline System - Basic Flow Execution", "[manifold][pipeline][basic]") {
    using label_type = testing::Label1;
    using runtime_type = udho::manifold::basic_runtime<label_type, std::stringstream>;

    SECTION("Complete pipeline execution with all accepts") {
        testing::MSC msc{"accept"};
        auto composition = runtime_type::compose(
            testing::C00{"accept"},
            testing::C01{"accept"},
            testing::C10{"accept"},
            testing::C11{"accept"},
            testing::C20{"accept"},
            testing::C21{"accept"},
            msc
        );

        std::cout << "composition: " << udho::manifold::composition_name<runtime_type::composition_type>::get() << std::endl;

        runtime_type runtime{std::move(composition)};

        // Load configuration
        nlohmann::json config_json = nlohmann::json::parse(R"({
            "Component<0,Feature<0,0>>": {"enabled": true, "param": "test-value"},
            "Component<1,Feature<0,1>>": {"enabled": true, "param": "test-value"},
            "Component<2,Feature<1,0>,XFeature<1,0>>": {"enabled": true, "param": "test-value"},
            "Component<3,Feature<1,1>,XFeature<1,1>>": {"enabled": true, "param": "test-value"},
            "Component<4,Feature<2,0>,Feature<2,3>>": {"enabled": true, "param": "test-value"},
            "Component<5,Feature<2,1>,Feature<2,4>>": {"enabled": true, "param": "test-value"},
            "Component<6,Feature<0,0>,Feature<1,1>,Feature<2,2>>": {"param": "runtime-param"}
        })");

        runtime.load(config_json);

        // Spawn and execute flow
        std::stringstream stream;
        auto flow = runtime.spawn(std::move(stream));

        CHECK(runtime.count() == 1);
        flow->start();
        CHECK(runtime.count() == 0);

        // Verify execution order and content
        std::string output = flow->stream().str();
        INFO(output);

        // Stage 0 should execute
        CHECK(output.find("C00_F00(param=test-value)[PASS]") != std::string::npos);
        CHECK(output.find("C01_F01(param=test-value)[PASS]") != std::string::npos);
        CHECK(output.find("MSC_F00(param=runtime-param)[PASS]") != std::string::npos);

        // Stage 1 should execute
        CHECK(output.find("C10_F10(param=test-value)[PASS]") == std::string::npos);
        CHECK(output.find("C10_F10(param=patched-by-stage0)[PASS]") != std::string::npos);
        CHECK(output.find("C10_X10(param=patched-by-stage0)[PASS]") != std::string::npos);
        INFO("C10 configs patched after stage 0 ends before stage 1 starts");
        CHECK(output.find("C11_F11(param=test-value)[PASS]") != std::string::npos);
        CHECK(output.find("C11_X11(param=test-value)[PASS]") != std::string::npos);
        CHECK(output.find("MSC_F11(param=runtime-param)[PASS]") != std::string::npos);

        // Stage 2 should execute
        CHECK(output.find("C20_F20(param=patched-by-stage1)[PASS]") != std::string::npos);
        CHECK(output.find("C20_F23(param=patched-by-stage1)[PASS]") != std::string::npos);
        INFO("C20 configs patched after stage 1 ends before stage 2 starts");
        CHECK(output.find("C21_F21(param=test-value)[PASS]") != std::string::npos);
        CHECK(output.find("C21_F24(param=test-value)[PASS]") != std::string::npos);
        CHECK(output.find("MSC_F22(param=runtime-param)[PASS]") != std::string::npos);
    }

    SECTION("Pipeline with early failure in stage 0") {
        testing::MSC msc{"accept"};
        auto composition = runtime_type::compose(
            testing::C00{"reject"},  // This will fail
            testing::C01{"accept"},
            testing::C10{"accept"},
            testing::C11{"accept"},
            testing::C20{"accept"},
            testing::C21{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Load config
        nlohmann::json config_json = nlohmann::json::parse(R"({
            "Component<0,Feature<0,0>>": {"enabled": true, "param": "test"},
            "Component<1,Feature<0,1>>": {"enabled": true, "param": "test"},
            "Component<2,Feature<1,0>,XFeature<1,0>>": {"enabled": true, "param": "test"},
            "Component<3,Feature<1,1>,XFeature<1,1>>": {"enabled": true, "param": "test"},
            "Component<4,Feature<2,0>,Feature<2,3>>": {"enabled": true, "param": "test"},
            "Component<5,Feature<2,1>,Feature<2,4>>": {"enabled": true, "param": "test"},
            "Component<6,Feature<0,0>,Feature<1,1>,Feature<2,2>>": {"param": "test"}
        })");

        runtime.load(config_json);

        std::stringstream stream;
        auto flow = runtime.spawn(std::move(stream));
        flow->start();

        std::string output = flow->stream().str();

        // Should have C00 failure and stop there
        CHECK(output.find("C00_F00(param=test)[FAIL]") != std::string::npos);
        CHECK(output.find("C01_F01") == std::string::npos); // Should not execute
        CHECK(output.find("C10_F10") == std::string::npos); // Should not execute
        CHECK(output.find("C20_F20") == std::string::npos); // Should not execute
    }

    SECTION("Pipeline with failure in stage 1") {
        testing::MSC msc{"accept"};
        auto composition = runtime_type::compose(
            testing::C00{"accept"},
            testing::C01{"accept"},
            testing::C10{"reject"},  // This will fail in stage 1
            testing::C11{"accept"},
            testing::C20{"accept"},
            testing::C21{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Load config
        nlohmann::json config_json = nlohmann::json::parse(R"({
            "Component<0,Feature<0,0>>": {"enabled": true, "param": "test"},
            "Component<1,Feature<0,1>>": {"enabled": true, "param": "test"},
            "Component<2,Feature<1,0>,XFeature<1,0>>": {"enabled": true, "param": "test"},
            "Component<3,Feature<1,1>,XFeature<1,1>>": {"enabled": true, "param": "test"},
            "Component<4,Feature<2,0>,Feature<2,3>>": {"enabled": true, "param": "test"},
            "Component<5,Feature<2,1>,Feature<2,4>>": {"enabled": true, "param": "test"},
            "Component<6,Feature<0,0>,Feature<1,1>,Feature<2,2>>": {"param": "test"}
        })");

        runtime.load(config_json);

        std::stringstream stream;
        auto flow = runtime.spawn(std::move(stream));
        flow->start();

        std::string output = flow->stream().str();
        INFO(output);

        // Stage 0 should execute
        CHECK(output.find("C00_F00(param=test)[PASS]") != std::string::npos);
        CHECK(output.find("C01_F01(param=test)[PASS]") != std::string::npos);
        CHECK(output.find("MSC_F00(param=test)[PASS]") != std::string::npos);

        // Stage 1 should fail at C10_F10
        CHECK(output.find("C10_F10(param=patched-by-stage0)[FAIL]") != std::string::npos);
        INFO("C10 is unconditionally patched in transision of stage 0 to stage 1");
        CHECK(output.find("C11_F11") == std::string::npos); // Should not execute
        CHECK(output.find("C20_F20") == std::string::npos); // Should not execute
    }
}

TEST_CASE("Pipeline System - Patch Configuration", "[manifold][pipeline][patch]") {
    using label_type = testing::Label1;
    using runtime_type = udho::manifold::basic_runtime<label_type, std::stringstream>;

    SECTION("Patch config modifies configuration between stages") {
        testing::MSC msc{"accept"};
        auto composition = runtime_type::compose(
            testing::C00{"accept"},
            testing::C01{"accept"},
            testing::C10{"accept"},
            testing::C11{"accept"},
            testing::C20{"accept"},
            testing::C21{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Load baseline config
        nlohmann::json config_json = nlohmann::json::parse(R"({
            "Component<0,Feature<0,0>>": {"enabled": true, "param": "initial"},
            "Component<1,Feature<0,1>>": {"enabled": true, "param": "initial"},
            "Component<2,Feature<1,0>,XFeature<1,0>>": {"enabled": true, "param": "initial"},
            "Component<3,Feature<1,1>,XFeature<1,1>>": {"enabled": true, "param": "initial"},
            "Component<4,Feature<2,0>,Feature<2,3>>": {"enabled": true, "param": "initial"},
            "Component<5,Feature<2,1>,Feature<2,4>>": {"enabled": true, "param": "initial"},
            "Component<6,Feature<0,0>,Feature<1,1>,Feature<2,2>>": {"param": "initial-param"}
        })");

        runtime.load(config_json);

        std::stringstream stream;
        auto flow = runtime.spawn(std::move(stream));
        flow->start();

        std::string output = flow->stream().str();

        // Verify patch config was applied:
        // 1. C10 should have param="patched-by-stage0" (changed by patch_config<Label1, 0>)
        CHECK(output.find("C10_F10(param=patched-by-stage0)[PASS]") != std::string::npos);
        CHECK(output.find("C10_X10(param=patched-by-stage0)[PASS]") != std::string::npos);

        // 2. C20 should have param="patched-by-stage1" (changed by patch_config<Label1, 1>)
        CHECK(output.find("C20_F20(param=patched-by-stage1)[PASS]") != std::string::npos);
        CHECK(output.find("C20_F23(param=patched-by-stage1)[PASS]") != std::string::npos);

        // 3. Other components should still have original value
        CHECK(output.find("C00_F00(param=initial)[PASS]") != std::string::npos);
        CHECK(output.find("C01_F01(param=initial)[PASS]") != std::string::npos);
        CHECK(output.find("C11_F11(param=initial)[PASS]") != std::string::npos);
        CHECK(output.find("C21_F21(param=initial)[PASS]") != std::string::npos);
    }
}

TEST_CASE("Pipeline System - Configuration Disables Components", "[manifold][pipeline][config]") {
    using label_type = testing::Label1;
    using runtime_type = udho::manifold::basic_runtime<label_type, std::stringstream>;

    SECTION("Components disabled by configuration should fail") {
        testing::MSC msc{"accept"};
        auto composition = runtime_type::compose(
            testing::C00{"accept"},
            testing::C01{"accept"},
            testing::C10{"accept"},
            testing::C11{"accept"},
            testing::C20{"accept"},
            testing::C21{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Load config with C10 disabled
        nlohmann::json config_json = nlohmann::json::parse(R"({
            "Component<0,Feature<0,0>>": {"enabled": true, "param": "test"},
            "Component<1,Feature<0,1>>": {"enabled": true, "param": "test"},
            "Component<2,Feature<1,0>,XFeature<1,0>>": {"enabled": false, "param": "test"},
            "Component<3,Feature<1,1>,XFeature<1,1>>": {"enabled": true, "param": "test"},
            "Component<4,Feature<2,0>,Feature<2,3>>": {"enabled": true, "param": "test"},
            "Component<5,Feature<2,1>,Feature<2,4>>": {"enabled": true, "param": "test"},
            "Component<6,Feature<0,0>,Feature<1,1>,Feature<2,2>>": {"param": "test"}
        })");

        runtime.load(config_json);

        std::stringstream stream;
        auto flow = runtime.spawn(std::move(stream));
        flow->start();

        std::string output = flow->stream().str();
        INFO(output);

        // C10 should fail because enabled=false
        CHECK(output.find("C10_F10(param=patched-by-stage0)[FAIL]") != std::string::npos);
        INFO("C10 is unconditionally patched in transision of stage 0 to stage 1");
        // Pipeline should stop at C10 failure
        CHECK(output.find("C11_F11") == std::string::npos);
        CHECK(output.find("C20_F20") == std::string::npos);
    }
}
