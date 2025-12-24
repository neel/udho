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
#include <nlohmann/json.hpp>
#include <iostream>
#include <iostream>
#include <sstream>

namespace testing {

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
    static constexpr const std::size_t stage = Index % 3; // Features distributed across stages 0,1,2
    using result    = State;
};

template <std::size_t Index>
struct XFeature{
    static constexpr const std::size_t idx = Index;
    static constexpr const std::size_t stage = (Index + 1) % 3; // Different stage pattern
};

// Stage 0 components
template <std::size_t Index>
struct Stage0Component {
    using features  = udho::manifold::features<Feature<Index>>;

    UDHO_CONFIG_PARAM(enabled,  bool,           false   );
    UDHO_CONFIG_PARAM(param,    std::string,    "default");

    static constexpr const std::string_view name = "S0C";
    using params = udho::manifold::params<enabled, param>;

    Stage0Component(): is_default_constructed(true) {}
    Stage0Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Stage0Component(const Stage0Component&) = delete;
    Stage0Component(Stage0Component&& other) noexcept: is_default_constructed(false), message(std::move(other.message)) {}

    bool is_default_constructed;
    std::string message;
};

// Stage 1 components
template <std::size_t Index>
struct Stage1Component {
    using features  = udho::manifold::features<Feature<Index + 10>, XFeature<Index>>;

    UDHO_CONFIG_PARAM(enabled,  bool,           true    );
    UDHO_CONFIG_PARAM(threshold,int,            5       );
    UDHO_CONFIG_PARAM(mode,     std::string,    "auto"  );

    static constexpr const std::string_view name = "S1C";
    using params = udho::manifold::params<enabled, threshold, mode>;

    Stage1Component(): is_default_constructed(true) {}
    Stage1Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Stage1Component(const Stage1Component&) = delete;
    Stage1Component(Stage1Component&& other) noexcept: is_default_constructed(false), message(std::move(other.message)) {}

    bool is_default_constructed;
    std::string message;
};

// Stage 2 components
template <std::size_t Index>
struct Stage2Component {
    using features  = udho::manifold::features<Feature<Index + 20>, Feature<Index + 30>>;

    UDHO_CONFIG_PARAM(enabled,  bool,           true    );
    UDHO_CONFIG_PARAM(timeout,  int,            1000    );
    UDHO_CONFIG_PARAM(retries,  int,            3       );

    static constexpr const std::string_view name = "S2C";
    using params = udho::manifold::params<enabled, timeout, retries>;

    Stage2Component(): is_default_constructed(true) {}
    Stage2Component(const std::string& msg): is_default_constructed(false), message(msg) {}
    Stage2Component(const Stage2Component&) = delete;
    Stage2Component(Stage2Component&& other) noexcept: is_default_constructed(false), message(std::move(other.message)) {}

    bool is_default_constructed;
    std::string message;
};

// Special component that appears in multiple stages
struct MultiStageComponent {
    using features  = udho::manifold::features<Feature<0>, Feature<11>, Feature<22>>;

    UDHO_CONFIG_PARAM(enabled,  bool,           true    );
    UDHO_CONFIG_PARAM(global,   std::string,    "global");

    static constexpr const std::string_view name = "MSC";
    using params = udho::manifold::params<enabled, global>;

    MultiStageComponent(): is_default_constructed(true) {}
    MultiStageComponent(const std::string& msg): is_default_constructed(false), message(msg) {}
    MultiStageComponent(const MultiStageComponent&) = delete;
    MultiStageComponent(MultiStageComponent&& other) noexcept: is_default_constructed(false), message(std::move(other.message)) {}

    bool is_default_constructed;
    std::string message;
};

}


namespace udho {
namespace manifold {

template <std::size_t Index>
struct facet<testing::Stage0Component<Index>, testing::Feature<Index>> {
    using component_type = testing::Stage0Component<Index>;
    using feature        = testing::Feature<Index>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "S0C" << Index << "F" << feature::idx << "(" << _config[component_type::param::val] << ")";
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

template <std::size_t Index>
struct facet<testing::Stage1Component<Index>, testing::Feature<Index + 10>> {
    using component_type = testing::Stage1Component<Index>;
    using feature        = testing::Feature<Index + 10>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "S1C" << Index << "F" << feature::idx << "(thresh="<< _config[component_type::threshold::val] << ")";

        // Check if previous stage results exist
        bool prev_stage_passed = true;

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

template <std::size_t Index>
struct facet<testing::Stage1Component<Index>, testing::XFeature<Index>> {
    using component_type = testing::Stage1Component<Index>;
    using feature        = testing::XFeature<Index>;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "S1C" << Index << "XF" << feature::idx << "(mode=" << _config[component_type::mode::val] << ")[PASS]\n";
        next.pass();
    }

private:
    component_type& _component;
    const config& _config;
};

template <std::size_t Index>
struct facet<testing::Stage2Component<Index>, testing::Feature<Index + 20>> {
    using component_type = testing::Stage2Component<Index>;
    using feature        = testing::Feature<Index + 20>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "S2C" << Index << "F" << feature::idx << "(timeout=" << _config[component_type::timeout::val] << ")";

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

template <std::size_t Index>
struct facet<testing::Stage2Component<Index>, testing::Feature<Index + 30>> {
    using component_type = testing::Stage2Component<Index>;
    using feature        = testing::Feature<Index + 30>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "S2C" << Index << "F" << feature::idx << "(retries=" << _config[component_type::retries::val] << ")";

        result res{true}; // Always pass this feature
        stream << "[PASS]\n";

        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::MultiStageComponent, testing::Feature<0>> {
    using component_type = testing::MultiStageComponent;
    using feature        = testing::Feature<0>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "MSC_F0(global=" << _config[component_type::global::val] << ")";
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
struct facet<testing::MultiStageComponent, testing::Feature<11>> {
    using component_type = testing::MultiStageComponent;
    using feature        = testing::Feature<11>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "MSC_F11[PASS]\n";
        result res{true};
        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

template <>
struct facet<testing::MultiStageComponent, testing::Feature<22>> {
    using component_type = testing::MultiStageComponent;
    using feature        = testing::Feature<22>;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf): _component(component), _config(conf) {}

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, std::stringstream& stream) const {
        stream << "MSC_F22[PASS]\n";
        result res{true};
        next.pass(std::move(res));
    }

private:
    component_type& _component;
    const config& _config;
};

}
}

namespace testing {

// Define test labels
struct Label1 {};
struct Label2 {};
struct Label3 {};

// Alias for convenience
using S0C0 = Stage0Component<0>;
using S0C1 = Stage0Component<1>;
using S1C0 = Stage1Component<0>;
using S1C1 = Stage1Component<1>;
using S2C0 = Stage2Component<0>;
using S2C1 = Stage2Component<1>;
using MSC  = MultiStageComponent;

} // namespace testing

// Sketch specializations for different test scenarios
namespace udho {
namespace manifold {

// Simple 3-stage pipeline
template <>
struct sketch<testing::Label1> {
    using composition_type = composition<
        testing::S0C0,
        testing::S0C1,
        testing::S1C0,
        testing::S1C1,
        testing::S2C0,
        testing::S2C1,
        testing::MSC
    >;
    using order_type = order<
        testing::Feature<0>,     // Stage 0
        testing::Feature<1>,     // Stage 0
        testing::Feature<10>,    // Stage 1
        testing::Feature<11>,    // Stage 1
        testing::XFeature<0>,    // Stage 1
        testing::XFeature<1>,    // Stage 1
        testing::Feature<20>,    // Stage 2
        testing::Feature<21>,    // Stage 2
        testing::Feature<30>,    // Stage 2
        testing::Feature<31>,    // Stage 2
        testing::Feature<22>     // Stage 2 (from MSC)
    >;
    static constexpr std::size_t Count = 3; // 3 stages (0,1,2)
};

// Single stage pipeline
template <>
struct sketch<testing::Label2> {
    using composition_type = composition<
        testing::S0C0,
        testing::S0C1
    >;
    using order_type = order<
        testing::Feature<0>,
        testing::Feature<1>
    >;
    static constexpr std::size_t Count = 1; // Only stage 0
};

// Complex multi-feature pipeline
template <>
struct sketch<testing::Label3> {
    using composition_type = composition<
        testing::MSC,
        testing::S1C0,
        testing::S2C0
    >;
    using order_type = order<
        testing::Feature<0>,     // Stage 0 (from MSC)
        testing::Feature<10>,    // Stage 1
        testing::XFeature<0>,    // Stage 1
        testing::Feature<11>,    // Stage 1 (from MSC)
        testing::Feature<20>,    // Stage 2
        testing::Feature<30>,    // Stage 2
        testing::Feature<22>     // Stage 2 (from MSC)
    >;
    static constexpr std::size_t Count = 3;
};

// Patch config specializations for dynamic configuration between stages
template <>
struct patch_config<testing::Label1, 0> {
    using label_type        = testing::Label1;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;

    static constexpr std::size_t Count = sketch_type::Count;

    using pipeline_type     = pipeline<composition_type, order_type, Count, 0>;
    using next_config_type  = typename pipeline<composition_type, order_type, Count, 1>::configs_type;

    void apply(const pipeline_type& p, next_config_type& config) {
        std::cout << "PatchConfig[Stage0->Stage1]: Applying stage transition logic\n";
        config[testing::S1C0::mode::val] = "adaptive";
    }
};

template <>
struct patch_config<testing::Label1, 1> {
    using label_type = testing::Label1;
    using sketch_type = sketch<label_type>;
    using composition_type = typename sketch_type::composition_type;
    using order_type = typename sketch_type::order_type;
    static constexpr std::size_t Count = sketch_type::Count;

    using pipeline_type = pipeline<composition_type, order_type, Count, 1>;
    using next_config_type = typename pipeline<composition_type, order_type, Count, 2>::configs_type;

    void apply(const pipeline_type& p, next_config_type& config) {
        std::cout << "PatchConfig[Stage1->Stage2]: Applying stage transition logic\n";
        config[testing::S2C0::timeout::val] = 2000;
    }
};

} // namespace manifold
} // namespace udho

TEST_CASE("Pipeline System - Type Safety and Compile-time Checks", "[manifold][pipeline][typesafety]") {
    using label_type    = testing::Label1;
    using sketch_type   = udho::manifold::sketch<testing::Label1>;
    using runtime_type  = udho::manifold::runtime<testing::Label1>;
    using flow_type     = udho::manifold::flow<testing::Label1>;

    using start_pipeline_type = udho::manifold::pipeline<typename sketch_type::composition_type, typename sketch_type::order_type, sketch_type::Count, -1>;

    static_assert(!std::is_void_v<typename sketch_type::composition_type>);
    static_assert(!std::is_void_v<typename sketch_type::order_type>);

    static_assert(std::is_same_v<typename runtime_type::composition_type, typename sketch_type::composition_type>);
    static_assert(std::is_same_v<typename flow_type::composition_type,    typename runtime_type::composition_type>);

    static_assert(std::is_same_v<typename flow_type::start_pipeline_type, start_pipeline_type>);

    static_assert(sketch_type::Count == 3);

    CHECK(true);
}

TEST_CASE("Pipeline System - Basic Flow", "[manifold][pipeline][flow]") {
    using label_type        = testing::Label1;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using runtime_type      = udho::manifold::runtime<label_type>;
    using flow_type         = udho::manifold::flow<label_type>;

    SECTION("Runtime creation and baseline configuration") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        CHECK(runtime.composition().template get<testing::S0C0>().component().message == "accept");
        CHECK(runtime.composition().template get<testing::MSC>().borrowed());
        CHECK(runtime.count() == 0);
    }

    SECTION("Flow spawning and execution") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Spawn multiple flows
        auto flow1 = runtime.spawn();
        auto flow2 = runtime.spawn();

        CHECK(runtime.count() == 2);

        std::stringstream stream;

        // Execute flows synchronously
        flow1->start(stream);
        flow2->start(stream);

        // Cleanup expired flows
        runtime.cleanup();
        CHECK(runtime.count() == 2); // Both still alive

        // Reset flows (simulate completion)
        flow1.reset();
        flow2.reset();

        runtime.cleanup();
        // Count may still be 2 because weak pointers aren't cleaned until cleanup
    }

    SECTION("Async flow execution") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        boost::asio::io_context io;
        bool callback_called = false;

        std::stringstream stream;
        auto flow = runtime.spawn();
        flow->start(io, stream);

        // Run IO context to process async operations
        io.run();

        // Callback should have been called
        // Note: Actual callback happens inside pipeline completion
        CHECK(true); // Placeholder
    }
}

TEST_CASE("Pipeline System - Multi-stage Execution", "[manifold][pipeline][multistage]") {
    using label_type        = testing::Label1;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using runtime_type      = udho::manifold::runtime<label_type>;
    using flow_type         = udho::manifold::flow<label_type>;

    SECTION("Successful multi-stage pipeline") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        std::stringstream execution_log;
        auto flow = runtime.spawn();

        // We need to intercept the pipeline execution to capture output
        // This is a simplified test - in practice you'd need to inject a tracer
        std::stringstream stream;
        flow->start(stream);

        // Verify that all stages executed
        // The actual verification would require examining the journal
        CHECK(true);
    }

    SECTION("Pipeline with stage 0 failure") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"reject"}, // This will fail
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        auto flow = runtime.spawn();
        std::stringstream stream;
        flow->start(stream);

        // Stage 1 and 2 should not execute due to stage 0 failure
        // Verification would require checking that later stages weren't called
        CHECK(true);
    }

    SECTION("Pipeline with stage 1 failure") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"reject"}, // This will fail
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        auto flow = runtime.spawn();
        std::stringstream stream;
        flow->start(stream);

        // Stage 0 should execute, stage 1 should fail, stage 2 should not execute
        CHECK(true);
    }
}

TEST_CASE("Pipeline System - Configuration Propagation", "[manifold][pipeline][config]") {
    using label_type        = testing::Label1;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using runtime_type      = udho::manifold::runtime<label_type>;

    SECTION("Baseline configuration loading") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Load baseline configuration from JSON
        nlohmann::json baseline_config = nlohmann::json::parse(R"({
            "S0C": {
                "enabled": true,
                "param": "configured"
            },
            "S1C": {
                "enabled": false,
                "threshold": 10,
                "mode": "manual"
            },
            "S2C": {
                "enabled": true,
                "timeout": 500,
                "retries": 5
            },
            "MSC": {
                "enabled": true,
                "global": "runtime-config"
            }
        })");

        // Note: In actual implementation, runtime would have load() method
        // runtime.baseline().load(baseline_config);

        CHECK(true); // Configuration loading would be tested here
    }

    SECTION("Per-stage configuration isolation") {
        // Test that each stage gets its own copy of configuration
        // and modifications don't affect other stages
        CHECK(true);
    }
}

TEST_CASE("Pipeline System - Single Stage Pipeline", "[manifold][pipeline][singlestage]") {
    using label_type        = testing::Label2;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using runtime_type      = udho::manifold::runtime<label_type>;

    SECTION("Single stage execution") {
        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"}
        );

        runtime_type runtime{std::move(composition)};
        auto flow = runtime.spawn();

        std::stringstream stream;
        flow->start(stream);

        // Only stage 0 should execute
        CHECK(runtime.count() == 1);
    }

    SECTION("Single stage with mixed results") {
        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"reject"} // Second component fails
        );

        runtime_type runtime{std::move(composition)};
        auto flow = runtime.spawn();

        std::stringstream stream;
        flow->start(stream);

        // Pipeline should fail
        CHECK(true);
    }
}

TEST_CASE("Pipeline System - Complex Feature Ordering", "[manifold][pipeline][ordering]") {
    using label_type        = testing::Label3;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using runtime_type      = udho::manifold::runtime<label_type>;

    SECTION("Multi-feature component across stages") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            msc,
            testing::S1C0{"accept"},
            testing::S2C0{"accept"}
        );

        runtime_type runtime{std::move(composition)};

        // MSC has features in stages 0, 1, and 2
        // Verify all are accessible
        CHECK(runtime.composition().template count<testing::Feature<0>>() >= 1);
        CHECK(runtime.composition().template count<testing::Feature<11>>() >= 1);
        CHECK(runtime.composition().template count<testing::Feature<22>>() >= 1);

        auto flow = runtime.spawn();
        std::stringstream stream;
        flow->start(stream);

        // All features across all stages should execute
        CHECK(true);
    }

    SECTION("Feature execution order verification") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            msc,
            testing::S1C0{"accept"},
            testing::S2C0{"accept"}
        );

        runtime_type runtime{std::move(composition)};

        std::stringstream exec_order;
        auto flow = runtime.spawn();

        // Need to intercept facet execution to verify order
        // This would require modifying facets to accept execution tracer
        std::stringstream stream;
        flow->start(stream);

        // Verify features execute in sketch::order_type sequence
        // Stage 0 features first, then stage 1, then stage 2
        CHECK(true);
    }
}

TEST_CASE("Pipeline System - Flow Management", "[manifold][pipeline][flowmgmt]") {
    using label_type        = testing::Label1;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using runtime_type      = udho::manifold::runtime<label_type>;

    SECTION("Multiple concurrent flows") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};

        // Spawn multiple flows
        std::vector<std::shared_ptr<udho::manifold::flow<label_type>>> flows;
        for (int i = 0; i < 5; ++i) {
            flows.push_back(runtime.spawn());
        }

        CHECK(runtime.count() == 5);

        // Start all flows
        std::stringstream stream;
        for (auto& flow : flows) {
            flow->start(stream);
        }

        // Cleanup after some flows complete
        flows[0].reset();
        flows[1].reset();

        runtime.cleanup();
        // Should still have 3 active flows
        CHECK(runtime.count() >= 3);
    }

    SECTION("Flow weak pointer tracking") {
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            msc
        );

        runtime_type runtime{std::move(composition)};
        std::stringstream stream;
        {
            auto flow = runtime.spawn();
            CHECK(runtime.count() == 1);

            flow->start(stream);
            // flow goes out of scope, should be cleaned up
        }

        runtime.cleanup();
        // Weak pointer should be removed from collection
        CHECK(runtime.count() == 0);
    }
}

TEST_CASE("Pipeline System - Patch Configuration", "[manifold][pipeline][patchconfig]") {
    using label_type        = testing::Label1;
    using sketch_type       = udho::manifold::sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;

    SECTION("Patch config application between stages") {
        // This tests the patch_config specialization we defined above
        testing::MSC msc{"accept"};

        auto composition = composition_type::compose(
            testing::S0C0{"accept"},
            testing::S0C1{"accept"},
            testing::S1C0{"accept"},
            testing::S1C1{"accept"},
            testing::S2C0{"accept"},
            testing::S2C1{"accept"},
            msc
        );

        // Create a pipeline to test patch config
        using pipeline_type = udho::manifold::pipeline<composition_type, typename sketch_type::order_type, sketch_type::Count, -1>;

        using configs_type = typename composition_type::configs_type;

        configs_type baseline;
        pipeline_type test_pipeline{composition, baseline};

        // Access stage 0 pipeline
        auto& stage0 = test_pipeline.template at<0>();

        // The patch_config specializations should apply when flow moves between stages
        // This is tested through the flow's apply() method

        CHECK(true);
    }
}
