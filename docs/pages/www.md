www {#WWWPage}
=======

`udho::www` specializes udho::manifold which provides a generic compile-time component-based architecture for constructing multi-stage processing pipelines. 
In the pipeline, components expose capabilities through features, while the manifold orchestrates their feature-oriented evaluation, records intermediate results in a journal, and exposes request-scoped state and services through portals. 

udho::www provides the components, features, transitions, and runtime policies required to compose an HTTP processing pipeline. 
It supports both stateless and stateful request processing, integrating asynchronous operations, database connectivity, session management, Lua-based views, resource management, and URL routing into a unified pipeline. 
It provides multiple extension points to integrate user defined components, features and policies through www as well as through manifold system.

For the distinction between pipeline failures, captured action errors, HTTP
status exceptions, and error-page rendering, see @ref WWWErrorHandling.

```cpp
    using framework_type = udho::www::framework<udho::www::stateful::lua::lazy_fs>;

    auto framework = framework_type::apply(udho::url::router(url()));
    auto runtime   = framework.runtime(session, resources);

    auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();
```

In this example, the preset `udho::www::stateful::lua::lazy_fs` is used to define the shape of the web framework. The preset selects a stateful HTTP pipeline with filesystem-backed lazy session handling and Lua-backed view resources. A router is created with `udho::url::router` by passing the `url()` function, which returns the routing table. The routing table itself is omitted here for brevity.

```cpp
using framework_type = udho::www::framework<udho::www::stateful::lua::lazy_fs>;
```

The framework is then instantiated with the router. From this framework, a runtime is constructed by passing additional component instances that should participate in the HTTP pipeline. In this example, the session and resources components are supplied to the runtime; their construction and initialization are omitted for brevity.

```cpp
auto framework = framework_type::apply(udho::url::router(url()));
auto runtime   = framework.runtime(session, resources);
```

Finally, a listener is created with a Boost.Asio io_context, the runtime, and the listening endpoint. Calling listener.start() starts accepting HTTP connections and dispatching them through the configured www pipeline.

```cpp
auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

listener.start();
```

## Presets

Most applications can be constructed from one of the predefined presets. 
A preset selects the default HTTP pipeline, session model, resource bridge configuration, and default component composition.
Presets are convenience labels that select a predefined www pipeline shape. 
The preset itself does not construct runtime objects; instead, it chooses the tag consumed by the @ref udho::manifold::sketch specialization. 
Each preset is associated with a predined set of components.

| Preset                                        | Description                                                                                                                                                                                                             |
|-----------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `udho::www::stateless::rest`                  | Stateless HTTP framework preset without session handling or a view bridge. Suitable for REST-style endpoints and simple HTTP APIs.                                                                                      |
| `udho::www::stateless::lua`                   | Stateless HTTP framework preset with Lua-backed view resources. Suitable for applications that need rendering support without server-side sessions.                                                                     |
| `udho::www::stateful::lazy_fs`                | Stateful HTTP framework preset using filesystem-backed session storage in lazy mode. Session data is loaded and persisted only when required by the session policy.                                                     |
| `udho::www::stateful::optimistic_fs`          | Stateful HTTP framework preset using filesystem-backed session storage in optimistic mode. Suitable when session updates are expected to be lightweight and conflict-free.                                              |
| `udho::www::stateful::lazy_memfs`             | Stateful HTTP framework preset intended for memory-backed filesystem-style session storage in lazy mode. In the current source, this alias maps to `udho::session::storage::fs`.                                        |
| `udho::www::stateful::optimistic_memfs`       | Stateful HTTP framework preset intended for memory-backed filesystem-style session storage in optimistic mode. In the current source, this alias maps to `udho::session::storage::fs`.                                  |
| `udho::www::stateful::lazy_redis`             | Stateful HTTP framework preset using Redis-backed session storage in lazy mode. Suitable for deployments where session state should be shared outside the process.                                                      |
| `udho::www::stateful::optimistic_redis`       | Stateful HTTP framework preset using Redis-backed session storage in optimistic mode. Suitable for Redis-backed session workflows where updates can be committed optimistically.                                        |
| `udho::www::stateful::immediate_redis`        | Stateful HTTP framework preset using Redis-backed session storage in immediate mode. Suitable when session changes should be written eagerly.                                                                           |
| `udho::www::stateful::lua::lazy_fs`           | Stateful HTTP framework preset with Lua-backed view resources and filesystem-backed session storage in lazy mode.                                                                                                       |
| `udho::www::stateful::lua::optimistic_fs`     | Stateful HTTP framework preset with Lua-backed view resources and filesystem-backed session storage in optimistic mode.                                                                                                 |
| `udho::www::stateful::lua::lazy_memfs`        | Stateful HTTP framework preset with Lua-backed view resources, intended for memory-backed filesystem-style session storage in lazy mode. In the current source, this alias maps to `udho::session::storage::fs`.        |
| `udho::www::stateful::lua::optimistic_memfs`  | Stateful HTTP framework preset with Lua-backed view resources, intended for memory-backed filesystem-style session storage in optimistic mode. In the current source, this alias maps to `udho::session::storage::fs`.  |
| `udho::www::stateful::lua::lazy_redis`        | Stateful HTTP framework preset with Lua-backed view resources and Redis-backed session storage in lazy mode.                                                                                                            |
| `udho::www::stateful::lua::optimistic_redis`  | Stateful HTTP framework preset with Lua-backed view resources and Redis-backed session storage in optimistic mode.                                                                                                      |
| `udho::www::stateful::lua::immediate_redis`   | Stateful HTTP framework preset with Lua-backed view resources and Redis-backed session storage in immediate mode.                                                                                                       |


## Sketch

Each of these above mentioned presets are defined as `labels` as shown in the example below

```cpp
using lazy_fs = www::label<
  www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>
>;
```

The label itself contains almost no information. 
It simply identifies the desired framework configuration. 
The actual component composition and feature ordering are provided by a specialization of `udho::manifold::sketch`.
For example `udho::manifold::sketch<udho::www::stateless::rest>` provides following typedefs 

- composition_type
- order_type

This `composition_type` specifies the mandatory set of components that the composition must contain along with extension to inject more components.
The `order_type` specifies the order of the features.
As the components specify what features does it support, this order is used to order component evaluation.

## Custom Preset

The simplest method of writing a custom preset is to create a new preset like the existing one while providing extra components.
The the tag from the closest preset and append the extra components needed.

```cpp
using custom_preset = www::label<
  www::tags::stateful<udho::session::storage::fs, udho::session::modes::lazy>,
  ComponentA, 
  ComponentB,
  ...
>;
```

However, this approach doesn't provide mechanism for adding a new feature in the order_type.
Neither does it allow changing the mandatory components in the component_type typedef.
Another approach could be to add another tag for the custom preset and specialize the sketch template.

```cpp
struct custom_preset{};

struct sketch<custom_preset>{
    using composition_type = udho::manifold::composition<
        components...
    >;

    using order_type = udho::manifold::order<
        features...
    >;
};
```

## Components

Components implement the concrete functionality of the HTTP pipeline. 
Each component exposes one or more features, allowing the manifold to orchestrate their evaluation independently of the component implementation.
www module defines the necessary components to enable the HTTP pipeline.
The components and their role are mentioned below.

| Component | Description |
|---|---|
| `udho::www::components::basic_handler` | Manages active response output streams for flows and coordinates response completion callbacks. |
| `udho::www::components::db::pg` | PostgreSQL database component included in the default www sketch. |
| `udho::www::components::protocols::http` | Implements HTTP protocol handling, including request-header and body processing. |
| `udho::www::components::navigators::pretty` | Provides pretty URL navigation and request-target interpretation for routing. |
| `udho::www::components::cookies` | Loads and exposes request cookies through the www pipeline. |
| `udho::www::components::session` | Adds server-side session support for stateful presets. |
| `udho::www::components::resources` | Provides access to application resources and view-data bridges such as Lua. |
| `udho::www::components::routing` | Wraps the router, resolves matched routes, applies route-specific configuration, and invokes selected route actions. |

Out of these components `udho::www::components::basic_handler`, `udho::www::components::protocols::http`, `udho::www::components::navigators::pretty` and `udho::www::components::routing` are mandatory for processing the HTTP pipeline.
`udho::www::components::resources` is essential for processing assets as well as views.

## Features

Components expose their capabilities by providing one or more features.
Each feature is associated with a stage represented with an unsigned integer.
Following table presents the set of featyres defined in `udho::www` to facilitate the HTTP pipeline.

| Feature | Stage | Satisfied by component | Description |
|---|---:|---|---|
| `udho::www::feature::header_reader`     | 0 | `udho::www::components::protocols::http`    | Reads the incoming HTTP request header and produces the Beast request-header object. |
| `udho::www::feature::identifier`        | 0 | `udho::www::components::navigators::pretty` | Extracts routing-oriented request identity such as resource, path, extension, and query parameters. |
| `udho::www::feature::locator`           | 0 | `udho::www::components::routing`            | Resolves the identified request target into a route index. The routing component is appended by @ref udho::www::framework when the router is supplied. |
| `udho::www::feature::cookie_load`       | 1 | `udho::www::components::cookies`            | Loads request cookies and exposes them as a cookie jar. |
| `udho::www::feature::session_load`      | 1 | `udho::www::components::session`            | Loads or initializes the request session note. This component is present in stateful presets. |
| `udho::www::feature::body_reader`       | 2 | `udho::www::components::protocols::http`    | Reads the HTTP request body and exposes MIME type, body buffer, form data, byte count, error state, string conversion, and JSON parsing helpers. |
| `udho::www::feature::resources_storage` | 0 | `udho::www::components::resources`          | Marker feature for resource-storage support and view-resource access. |
| `udho::www::feature::responder`         | 2 | `udho::www::components::basic_handler`      | Marker feature for response handling. The handler also manages response output streams used by the action transition. |

## Extending the Framework

The facilities provided by `udho::www` are sufficient for building most HTTP applications. 
However, advanced customization may require extending the underlying `udho::manifold` framework. 
Typical examples include developing new components, defining new features, introducing custom evaluation policies, specializing transitions or terminals, extending the request context, or creating entirely new pipeline sketches.

These extension mechanisms are provided by `udho::manifold` and are common to all manifold-based frameworks. 
Refer to @ref ManifoldPage for a detailed discussion of the manifold architecture, extension points, and customization techniques.
