Manifold {#ManifoldPage}
========================

`udho::manifold` is the compile-time orchestration layer used by udho to build staged processing systems. It provides a generic component architecture where independently defined components expose capabilities through features, while the manifold system discovers those capabilities, orders their evaluation, records intermediate results, and exposes request-scoped state through structured views.

Most application code does not interact with `udho::manifold` directly. Instead, higher-level modules such as @ref WWWPage specialize it for a concrete domain. For example, `udho::www` uses manifold to construct an HTTP pipeline from protocol, routing, cookie, session, resource, and response-handling components. The purpose of this page is to explain the architecture behind that mechanism: what manifold is, how its parts fit together, and which concepts are involved when extending the framework.

@image html manifold-architecture.png 

@tableofcontents

## Design Overview {#page_manifold_design_overview}

The manifold system is built around a small set of cooperating constructs. A [component](#page_manifold_component) is a concrete object that participates in the system. It exposes one or more [features](#page_manifold_feature), where each feature represents a capability that can be evaluated in a pipeline stage. For every component-feature pair, a [facet](#page_manifold_facet) defines how that component implements that feature.

A @ref udho::manifold::composition "composition" stores the component instances that form the runtime system. From this composition, manifold derives stage-specific [fabrics](#page_manifold_fabric), each of which selects the facets that must be evaluated in a particular pipeline stage. During execution, a [flow](#page_manifold_flow) moves through these stages, while a @ref udho::manifold::journal "journal" records intermediate results produced by feature evaluation. Component configuration is stored in @ref udho::manifold::configs "configs", allowing each component to receive type-safe runtime parameters.

The following table summarizes the core constructs.

| Construct | Role |
|---|---|
| [Feature](#page_manifold_features) | A capability tag associated with a numeric stage, that can be provided by one or more components and evaluated in a pipeline stage. |
| [Component](#page_manifold_components) | A component provides one or more features and may define configuration parameters. |
| [Composition](#page_manifold_composition) | A composition is a type-safe collection of component wrappers that form the runtime system. |
| [Facet](#page_manifold_facets) | Given a component `C` provides features `F1`, `F2`, `F3`, a facet provides a per-feature interface to the component through specialization. The component `C` should be complemented with 3 facets `<C, F1>`, `<C, F2>`, `<C, F3>` |
| [Fabric](#page_manifold_fabric) | A stage-specific view that instantiates and owns the facets relevant to a pipeline stage. |
| [Pipeline](#page_manifold_pipeline) | Pipeline evaluates each facet in a fabric sequentially in the feature order. Each stage has a dedicated pipeline. |
| [Flow](#page_manifold_flow) | A single execution instance moving through the multiple pipelines dedicated to each stage. |
| @ref udho::manifold::journal "Journal" | A flow-scoped result store for values produced during feature evaluation. |
| @ref udho::manifold::configs "Configs" | A collection of component-specific configuration objects. |

These constructs form the core of manifold. Other types such as labels, orders, sketches, transitions, terminals, portals, and contexts organize, customize, or expose this core machinery, but the basic execution model is defined by the relationship between components, features, facets, fabrics, flows, journals, and configs.

The figure below shows the same idea in the context of a web pipeline. The upper part shows the runtime composition: a fixed set of components and their configuration. The lower part shows a flow: a single execution instance moving through stage-specific fabrics. As facets are evaluated, their results are recorded into the journal, and the flow advances until it finishes, re-enters, or aborts.

## Components {#page_manifold_components}

A component is a concrete object that participates in a manifold pipeline. Components provide behavior by declaring the set of features they support.

A component may also define configuration parameters. These parameters are collected into a component-specific @ref udho::manifold::config object and later grouped into a @ref udho::manifold::configs collection for the full runtime.

Conceptually, a component describes the capabilities it provides to the pipeline.

A component usually contributes the following information:

| Item | Purpose |
|---|---|
| `features` | The list of features provided by the component. |
| `params` | The list of configuration parameters supported by the component. |
| `name` | A human-readable or serialization-friendly component name. |
| Component instance | The actual runtime object stored in the composition. |

```cpp
struct AuthenticationComponent {
    using features = udho::manifold::features<
        feature::filter,
        feature::session
    >;

    UDHO_CONFIG_PARAM(enabled, bool, true);
    UDHO_CONFIG_PARAM(timeout, int, 5000);

    using params = udho::manifold::params<enabled, timeout>;
    static constexpr const char* name = "auth";
};
```

Components are not evaluated directly by the pipeline. Their features are evaluated through facets.

## Features {#page_manifold_features}

A feature is a type-level description of a capability.

A feature does not implement behavior by itself. Instead, it declares what kind of capability exists and, optionally, what result type is produced when the capability is evaluated.

A feature normally defines:

| Item | Purpose |
|---|---|
| `stage` | The pipeline stage in which the feature participates. |
| `name` | A human-readable feature name used for diagnostics and visualization. |
| `result` | Optional result type stored in the journal after evaluation. |

```cpp
namespace feature {

struct filter {
    static constexpr const std::size_t stage = 0;
    static constexpr const std::string_view name = "filter";

    struct result {
        bool accepted;
    };
};

struct cache {
    static constexpr const std::size_t stage = 1;
    static constexpr const std::string_view name = "cache";

    // No result type: this feature performs side effects only.
};

}
```

Features are the primary decoupling mechanism in manifold. Components advertise the features they provide, while the pipeline is ordered in terms of features rather than concrete component instances.

This means a pipeline can be expressed in terms of features rather than concrete components. In other words, it can evaluate feature `F` at stage `S` without directly calling component `C`.

The mapping from feature to component is resolved statically from the composition and component feature lists.

## Facets {#page_manifold_facets}

A facet is the implementation of a feature for a specific component.

Where a feature says *what capability exists*, a facet says *how this component provides that capability*.

A facet is specialized for a pair:

```cpp
udho::manifold::facet<ComponentT, FeatureT>
```

During pipeline evaluation, manifold constructs or accesses the facet corresponding to a component-feature pair and invokes it. The facet receives access to the component, its configuration, the journal, and the continuation object used to advance or fail evaluation.

```cpp
namespace udho {
namespace manifold {

template <>
struct facet<AuthenticationComponent, feature::filter> {
    using component_type = AuthenticationComponent;
    using feature        = feature::filter;
    using result         = typename feature::result;
    using config         = udho::manifold::config<component_type>;

    facet(component_type& component, const config& conf, std::size_t id)
        : _component(component), _config(conf) {}

    template <typename JournalT, typename NextT, typename RequestT>
    void operator()(const JournalT& journal, NextT&& next, RequestT& request) const {
        if (!_config[component_type::enabled::val].value()) {
            next.pass(result{true});
            return;
        }

        if (authenticate(request)) {
            next.pass(result{true});
        } else {
            next.fail(result{false});
        }
    }

private:
    component_type& _component;
    const config&   _config;
};

}
}
```

A facet is responsible for one of the following outcomes:

| Outcome | Meaning |
|---|---|
| `next.pass(...)` | The feature evaluation succeeded. If the feature has a result, the result is stored in the journal. |
| `next.fail(...)` | The feature evaluation failed. The flow enters the error path. |
| Exception capture | An exception may be propagated through the manifold error mechanism. |

Facets are the main implementation point for new component behavior.

## Composition {#page_manifold_composition}

A @ref udho::manifold::composition is the type-safe collection of component instances used by the runtime.

The composition is statically typed. Its component set is known at compile time, which allows manifold to resolve feature providers, stage membership, and typed access without dynamic lookup.

```cpp
using composition_type = udho::manifold::composition<
    AuthenticationComponent,
    LoggingComponent,
    CacheComponent
>;
```

A composition supports two important access patterns:

| Access pattern | Purpose |
|---|---|
| `get<ComponentT>()` | Access the wrapper for a specific component type. |
| `at<FeatureT, Index>()` | Access the component that provides a specific feature occurrence. |

```cpp
auto& auth_wrapper = composition.get<AuthenticationComponent>();
auto& auth         = auth_wrapper.component();

auto& first_filter_provider = composition.at<feature::filter, 0>().component();
```

The `Index` in feature-based access is needed because multiple components may provide the same feature. The first provider is index `0`, the next is index `1`, and so on.

Composition also manages ownership. A component can be owned, borrowed, or default constructed.

```cpp
AuthenticationComponent auth;
LoggingComponent logger;

// Borrow externally owned components.
auto comp1 = composition_type::compose(auth, logger);

// Move an owned component into the composition.
auto comp2 = composition_type::compose(
    AuthenticationComponent{},
    logger
);

// Omitted default-constructible components are constructed by the composition.
auto comp3 = composition_type::compose(auth);
```

Borrowed components must outlive the composition that references them.

## Configuration {#page_manifold_configuration}

Each component may define a set of type-safe parameters. Manifold stores those parameters in @ref udho::manifold::config objects and groups them into a @ref udho::manifold::configs collection.

Configuration has three main purposes:

| Purpose | Description |
|---|---|
| Type-safe access | Parameters are accessed by parameter keys rather than string lookup. |
| Runtime modification | Values may be changed before or during pipeline execution. |
| Serialization | Configurations can be saved to and loaded from JSON. |

```cpp
UDHO_CONFIG_PARAM(enabled, bool, true);
UDHO_CONFIG_PARAM(timeout, int, 5000);

struct AuthenticationComponent {
    using features = udho::manifold::features<feature::filter>;
    using params   = udho::manifold::params<enabled, timeout>;

    static constexpr udho::utils::string_view name = "auth";
};
```

```cpp
udho::manifold::configs<AuthenticationComponent> configs;

configs[AuthenticationComponent::enabled::val] = true;
configs[AuthenticationComponent::timeout::val] = 3000;
```

Configuration is component-scoped. This prevents unrelated components from accidentally sharing parameter names or accessing each other's settings without an explicit type-level relationship.

Configurations can also be serialized.

```cpp
nlohmann::json json = nlohmann::json::object();

configs.save(json);
configs.load(json);
```

In staged systems, configuration may also be patched between pipeline stages. This is how a transition can use an earlier result, such as route lookup, to adjust the configuration used by later features.

## Journal {#page_manifold_journal}

The @ref udho::manifold::journal stores the results produced during feature evaluation.

If a feature defines a `result` type, each facet that evaluates that feature gets a corresponding result entry in the journal. Before evaluation, the entry is empty. After successful evaluation, the result becomes available.

The journal enables later stages, transitions, terminals, portals, and user code to inspect what earlier features produced.

Conceptually, the journal records what has already been evaluated in the current flow and which results it produced.

Journal access is type-safe and feature-oriented:

| Access pattern | Purpose |
|---|---|
| `at<FeatureT>()` | Access the first result for a feature. |
| `at<FeatureT, Index>()` | Access a specific feature result occurrence. |
| `get<FacetT>()` | Access the result associated with a specific facet. |

```cpp
auto& filter_result = journal.at<feature::filter>();

if (filter_result.ready()) {
    const auto& value = filter_result.value();

    if (value.accepted) {
        // Feature evaluation succeeded.
    }
}
```

The journal is scoped to a flow. It represents the state of one execution instance, not global runtime state.

## Fabric {#page_manifold_fabric}

A fabric is a stage-specific view of the composition.

The full composition may contain many components, but a single pipeline stage only needs the components that provide features belonging to that stage. A fabric selects those relevant component-feature pairs and presents them to the pipeline evaluator.

Conceptually, the fabric identifies which facets should be evaluated in a given stage.

```cpp
using stage0_fabric = typename composition_type::template fabric_type<0>;
using stage1_fabric = typename composition_type::template fabric_type<1>;
```

This is mostly an internal construct, but it is important for understanding the architecture. It is the bridge between the static composition and the staged pipeline evaluator.

## Pipeline {#page_manifold_pipeline}

A pipeline evaluates the features belonging to one stage.

The full system is divided into numbered stages. Within each stage, features are evaluated according to the order declared by @ref udho::manifold::order. For each feature, the pipeline evaluates the facets provided by the components selected into the stage fabric.

```cpp
using order_type = udho::manifold::order<
    feature::filter,    // stage 0
    feature::cache,     // stage 1
    feature::responder  // stage 2
>;
```

A pipeline stage has three responsibilities:

| Responsibility | Description |
|---|---|
| Select | Use the fabric to determine which facets belong to the stage. |
| Evaluate | Invoke each facet in order. |
| Propagate | Continue on success or enter the error path on failure. |

```cpp
using pipeline_type = udho::manifold::common_pipeline<
    0,
    order_type,
    composition_type
>;

pipeline_type pipeline{composition, configs, journal};

pipeline
    .then([](auto result) {
        // Called when this stage completes.
    })
    .eval(request);
```

Pipeline evaluation is continuation-based. A facet does not return directly to the pipeline with a status value. Instead, it calls `next.pass(...)` or `next.fail(...)`, allowing both synchronous and asynchronous implementations to fit into the same model.

## Flow {#page_manifold_flow}

A flow is a single execution instance of a manifold runtime.

The runtime owns the shared composition and baseline configuration. A flow owns or references the per-execution state needed to move through the pipeline stages, including the journal and patched configuration state.

In an HTTP system, a flow corresponds to request processing on a connection. A completed flow may terminate, abort, or re-enter depending on the terminal policy. For example, the `www` terminal allows re-entry so that a persistent HTTP connection can process another request.

A flow is responsible for:

| Responsibility | Description |
|---|---|
| Starting evaluation | Begin the first pipeline stage. |
| Advancing stages | Move from one stage to the next through transitions. |
| Handling failure | Route evaluation failures to the terminal. |
| Re-entry or termination | Decide whether execution should continue after completion. |

```cpp
auto flow = runtime.spawn();

flow->start(stream);
```

When asynchronous execution is used, the runtime and pipeline can be driven through Boost.Asio.

```cpp
flow->start(io_context, stream);

io_context.run();
```

## Runtime {#page_manifold_runtime}

A runtime owns the long-lived state of a manifold-based system.

It is constructed from a sketch-derived composition type and feature order. The runtime stores the component composition and baseline configuration, then creates flows to process individual execution instances.

Conceptually, the runtime defines the components and policies that shape this processing system. The flow then captures what is happening in a particular execution.

```cpp
using runtime_type = udho::manifold::basic_runtime<MyLabel, StreamT>;

runtime_type runtime{component_a, component_b};

auto flow = runtime.spawn();
```

This separation allows shared infrastructure to remain stable while each execution receives its own journal and flow state.

## Sketch and Order {#page_manifold_sketch_order}

A @ref udho::manifold::sketch is a compile-time description of a complete manifold system.

A sketch associates a label with:

| Type | Purpose |
|---|---|
| `composition_type` | The component set required by the system. |
| `order_type` | The ordered feature list evaluated by the pipeline. |

The label itself is usually a lightweight type. It identifies the kind of system being built. The sketch specialization contains the actual architectural definition.

```cpp
struct MyLabel {};

template <>
struct udho::manifold::sketch<MyLabel> {
    using composition_type = udho::manifold::composition<
        AuthenticationComponent,
        CacheComponent,
        LoggingComponent
    >;

    using order_type = udho::manifold::order<
        feature::filter,
        feature::cache,
        feature::log
    >;
};
```

The @ref udho::manifold::order type lists features in evaluation order. Each feature also declares a stage. Together, the feature order and feature stages determine how the runtime builds its staged pipelines.

This is how higher-level modules specialize manifold. For example, `udho::www` defines labels and sketches for stateless and stateful HTTP pipelines.

## Transitions {#page_manifold_transitions}

A transition is a hook between pipeline stages.

Transitions allow a domain-specific framework to inspect the current flow state, read journal results, update configs, and decide how to continue. They are one of the main ways a manifold specialization adds behavior that is not simply a feature evaluation.

For example, in a web pipeline, a transition may use a route lookup result from an earlier stage to reconfigure later components before the request body is read or the route action is invoked.

Conceptually, a transition describes what should happen after one stage completes and before the next one begins.

Transitions are specialized by label, stream type, and stage.

```cpp
template <typename LabelT, typename StreamT, std::size_t Stage>
struct default_transition;

template <typename StreamT>
struct default_transition<MyLabel, StreamT, 1> {
    template <typename FlowT, typename PipelineT, typename ConfigsT, typename... Args>
    static void apply(
        FlowT& flow,
        PipelineT& pipeline,
        ConfigsT& configs,
        StreamT& stream,
        Args&&... args
    ) {
        // Inspect journal/configuration and decide how to continue.
        pipeline.next(flow, stream, std::forward<Args>(args)...);
    }
};
```

## Terminal Policy {#page_manifold_terminal}

A terminal defines what happens when a flow completes or fails.

The default terminal can simply terminate the flow. A specialized terminal may restart the flow, re-enter request processing, render error responses, abort a connection, or perform cleanup.

Conceptually, a terminal describes what should happen at the boundary of a flow.

In `udho::www`, the terminal is specialized to support HTTP behavior: successful flows may re-enter for persistent connections, while errors may produce HTTP error responses or abort the flow.

```cpp
template <>
struct udho::manifold::basic_terminal<MyLabel, StreamT> {
    using runtime_type = udho::manifold::basic_runtime<MyLabel, StreamT>;
    using flow_type    = typename runtime_type::flow_type;

    template <typename CompositionT, typename ConfigsT, typename JournalT>
    basic_terminal(CompositionT& composition, ConfigsT& configs, const JournalT& journal) {
    }

    bool reenter(StreamT& stream) {
        return false;
    }

    template <typename... Args>
    void internal_error(
        udho::manifold::evaluation_result result,
        flow_type& flow,
        StreamT& stream,
        Args&&... args
    ) {
        flow.abort();
    }
};
```

## Portal and Context {#page_manifold_portal_context}

A portal is a request-scoped access layer over the composition, configs, and journal.

It exposes selected components and their associated configuration and journal view through typed accessors. This allows user code or framework code to interact with the current execution state without directly manipulating the full runtime internals.

A context builds on top of this idea. It combines the portal with additional execution-specific state, such as an output stream or flow identifier. Higher-level modules may expose their own context aliases so application code receives a domain-specific interface.

In short:

| Construct | Role |
|---|---|
| Portal | Typed access to components, configs, and journal results. |
| Context | Domain-specific execution object built around a portal. |

```cpp
portal_type portal(composition, configs, journal);

context_type context(ostream, portal, flow.id());
```

For web applications, the context passed to route actions is the main user-facing object. It is backed by manifold, but exposed through the `www` layer.

## Error Handling {#page_manifold_error_handling}

Manifold uses explicit continuation-based error propagation.

A facet can report failure by calling `next.fail(...)`. The failure may carry a feature result, an error state, or a captured exception depending on the evaluator path. Once a failure is reported, the current pipeline stage stops normal progression and the flow enters the configured error path.

```cpp
if (accepted) {
    next.pass(result{true});
} else {
    next.fail(result{false});
}
```

Exceptions can also be propagated through the same evaluation path.

```cpp
try {
    perform_operation();
    next.pass();
} catch (...) {
    next.fail(std::current_exception());
}
```

Error handling is completed by the terminal policy. This separation is important:

| Layer | Responsibility |
|---|---|
| Facet | Detects local success or failure. |
| Pipeline | Propagates the success or failure. |
| Flow | Routes failure to the terminal. |
| Terminal | Decides the final outcome: abort, restart, render error, or terminate. |

This model allows generic manifold code to remain independent of domain-specific error behavior.

## Ownership Model {#page_manifold_ownership_model}

Manifold supports mixed component ownership.

Some components are naturally owned by the runtime. Others may represent external services, database handles, resource managers, or application objects that should be borrowed. Composition construction supports both cases.

| Mode | Description |
|---|---|
| Default constructed | The composition constructs the component. |
| Moved | The component is moved into the composition and owned by it. |
| Borrowed | The composition stores a reference to an externally managed object. |

```cpp
ExternalService service;

// Borrowed: user manages lifetime.
auto comp1 = composition_type::compose(service);

// Owned: composition receives the moved object.
auto comp2 = composition_type::compose(InternalComponent{});

// Mixed ownership.
auto comp3 = composition_type::compose(
    service,
    InternalComponent{}
);
```

Borrowed components must outlive the composition that references them.

## Extension Points {#page_manifold_extension_points}

Manifold is primarily an extension framework. Higher-level modules such as `udho::www` are built by specializing manifold concepts.

Typical extension points include:

| Extension | Use case |
|---|---|
| New component | Add a new service or capability provider to a pipeline. |
| New feature | Introduce a new capability that can be ordered and evaluated. |
| New facet | Implement an existing or new feature for a component. |
| New config specialization | Validate or customize component configuration. |
| New sketch | Define a new pipeline shape from a label. |
| New transition | Add behavior between pipeline stages. |
| New terminal | Customize flow completion and error behavior. |
| New accessor or portal behavior | Expose higher-level request-scoped APIs to user code. |

When extending `udho::www`, many changes can be made at the `www` layer by adding components or choosing a preset. Deeper changes, such as adding a new feature stage, defining a new evaluation policy, or changing flow behavior, are manifold-level extensions.

## API Reference {#page_manifold_api_reference}

| API | Description |
|---|---|
| @ref udho::manifold::features | Type-list of features provided by a component. |
| @ref udho::manifold::facet | Component-feature implementation point. |
| @ref udho::manifold::composition | Type-safe component collection. |
| @ref udho::manifold::composition_view | Non-owning typed view over selected composition components. |
| @ref udho::manifold::wrapper | Ownership wrapper used by compositions. |
| @ref udho::manifold::config | Component-specific configuration object. |
| @ref udho::manifold::configs | Configuration collection for a component composition. |
| @ref udho::manifold::configs_view | Non-owning typed view over selected component configs. |
| @ref udho::manifold::journal | Flow-scoped result store. |
| @ref udho::manifold::journal_const_view | Const typed view over selected journal results. |
| @ref udho::manifold::fabric | Stage-specific component and facet view. |
| @ref udho::manifold::order | Compile-time feature evaluation order. |
| @ref udho::manifold::pipeline | Stage evaluator. |
| @ref udho::manifold::flow | Single execution instance created by a runtime. |
| @ref udho::manifold::basic_runtime | Runtime owning composition and baseline configuration. |
| @ref udho::manifold::sketch | Compile-time definition of a runtime composition and feature order. |
| @ref udho::manifold::portal | Request-scoped access layer over components, configs, and journal results. |
| @ref udho::manifold::basic_context | Generic context object built around a portal. |
| @ref udho::manifold::basic_terminal | Flow completion and error policy. |
