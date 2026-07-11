Routing {#RoutingPage}
===========

The routing module maps an incoming HTTP method and URL path to a C++ callback. It is designed around a compact, declarative syntax:

```cpp
udho::url::slot("label"_h, &callback) << udho::url::pattern_factory(udho::url::verb::get, ...)
```

A route is made by binding a **slot** to a **match pattern**. A slot names and wraps the callback. A match pattern describes the HTTP verb, URL pattern, URL argument extraction, and URL-generation replacement. Routes can be chained with `|`, grouped under a mount point, and finally given to a router.

Most user code should include the umbrella header:

```cpp
#include <udho/url/url.h>

using namespace udho::hazo::string::literals;
```

A minimal root router looks like this:

```cpp
void home(udho::www::context<> ctx) {
    ctx << "home";
    ctx.finish();
}

int article(udho::www::context<> ctx, int id, const std::string& slug) {
    ctx << "article " << id << ": " << slug;
    ctx.finish();
    return 0;
}

auto router = udho::url::router(
    udho::url::root(
          udho::url::slot("home"_h,    &home)    << udho::url::home(udho::url::verb::get)
        | udho::url::slot("article"_h, &article) << udho::url::scan(udho::url::verb::get, "/article/{:d}/{}", "/article/{}/{}")
    )
);
```

With this router, `GET /` invokes `home(ctx)`, and `GET /article/42/routing` invokes `article(ctx, 42, "routing")`. The `ctx` argument is supplied externally by the web framework; the remaining callback arguments are filled from the URL captures.

A slightly larger example with a sub-mount and a filesystem explorer:

```cpp
void index(udho::www::context<> ctx) {
    ctx << "index";
    ctx.finish();
}

void profile(udho::www::context<> ctx, std::string user) {
    ctx << "profile " << user;
    ctx.finish();
}

void admin(udho::www::context<> ctx) {
    ctx << "admin";
    ctx.finish();
}

auto public_routes =
      udho::url::slot("index"_h,   &index)   << udho::url::home(udho::url::verb::get)
    | udho::url::slot("profile"_h, &profile) << udho::url::regx(udho::url::verb::get, "/profile/(\\w+)", "/profile/{}");

auto admin_routes =
      udho::url::slot("admin"_h, &admin) << udho::url::fixed(udho::url::verb::get, "/dashboard");

auto router = udho::url::router(
      udho::url::root(std::move(public_routes))
    | udho::url::mount("admin"_h, "/admin", std::move(admin_routes)),
    udho::url::explorers::registry{udho::url::explorers::files{"docroot", std::filesystem::path{"public"}}}
);
```

Here `GET /profile/alice` reaches the public route, `GET /admin/dashboard` reaches the mounted admin route, and unmatched `GET` paths can fall back to the static file explorer.

## Routes

A **route** is an action: a callback slot plus a URL match object.

```cpp
auto show_user = udho::url::slot("show_user"_h, &show_user_handler) << udho::url::scan(udho::url::verb::get, "/users/{:d}", "/users/{}");
```

The routing module intentionally makes the route declaration read left-to-right:

1. choose a route label,
2. bind the callback,
3. attach a match pattern,
4. optionally attach route options to the match pattern.

Preferred style:

```cpp
auto route = udho::url::slot("profile"_h, &profile) << udho::url::scan(udho::url::verb::get, "/profile/{}", "/profile/{}").options(opt::layout("main"), opt::auth(true));
```

That keeps the slot, match, replacement, HTTP verb, and route options in one place.

### Chain of routes

Routes are chained with `operator|`. The result is an `action_table`.

```cpp
auto routes =
      udho::url::slot("home"_h,    &home)    << udho::url::home(udho::url::verb::get)
    | udho::url::slot("about"_h,   &about)   << udho::url::fixed(udho::url::verb::get, "/about")
    | udho::url::slot("article"_h, &article) << udho::url::scan(udho::url::verb::get, "/article/{:d}/{}", "/article/{}/{}");
```

The reverse form is also available:

```cpp
auto route = udho::url::fixed(udho::url::verb::get, "/about") >> udho::url::slot("about"_h, &about);
```

In normal documentation and examples, prefer the forward form:

```cpp
udho::url::slot("label"_h, &callback) << udho::url::match_factory(...)
```

A chain can be extended by chaining another route or another `action_table`:

```cpp
auto all_routes = public_routes | admin_routes;
```

### Building blocks of a route

| Building block    | Meaning                                                                                                     | Typical API |
| ----------------- | ----------------------------------------------------------------------------------------------------------- | --- |
| HTTP verb         | The request method a route accepts. The alias is `udho::url::verb`, backed by `boost::beast::http::verb`.   | `udho::url::verb::get`, `udho::url::verb::post`, etc. |
| Pattern / match   | The object that checks the path, extracts URL arguments, and generates URLs from replacement strings.       | `home(...)`, `fixed(...)`, `regx(...)`, `scan(...)` |
| Replacement       | URL-generation template. It is not necessarily identical to the matching pattern.                           | `"/article/{}/{}"` |
| Slot              | A named callback wrapper. It records the route label/key and wraps a free function or member function.      | `udho::url::slot("name"_h, &fn)` or `udho::url::slot("name"_h, &T::fn, &object)` |
| Label / key       | Compile-time route identifier. Used for route lookup, summaries, and URL generation.                        | `"show"_h`, `"profile"_h` |
| Action            | A complete route formed by `slot << match` or `match >> slot`.                                              | `udho::url::slot("show"_h, &show) << udho::url::scan(...)` |
| Mount point       | A named group of routes under a base URL path.                                                              | `udho::url::mount("api"_h, "/api", std::move(routes))` |
| Root mount point  | The mount point for `/`.                                                                                    | `udho::url::root(std::move(routes))` |
| Router            | The object that searches mount points, invokes actions, serves explorers, and exposes summaries.            | `udho::url::router(std::move(mounts))` |

### Slots

A slot wraps a callback and assigns a compile-time label.

```cpp
auto route = udho::url::slot("f0"_h, &f0) << udho::url::home(udho::url::verb::get);
```

For member functions, pass the member pointer and the object pointer:

```cpp
struct controller {
    void dashboard(udho::www::context<> ctx) {
        ctx << "dashboard";
        ctx.finish();
    }
};

controller c;
auto route = udho::url::slot("dashboard"_h, &controller::dashboard, &c) << udho::url::fixed(udho::url::verb::get, "/dashboard");
```

The slot label becomes the stable name of the route. Use it for URL generation and summary lookup.

### Match patterns

A match pattern stores:

- the HTTP verb,
- the matching format,
- the replacement format,
- any route-local options.

The public pattern factories are:

| Factory | Format name in summary | Matches | Captures | Replacement behavior | Use when |
| --- | --- | --- | --- | --- | --- |
| `udho::url::home(method)` | `home` | The mount root. It matches `/` and the empty string after mount-prefix stripping. | No captures. | Always generates `/`. | Home/index route for a mount point. |
| `udho::url::fixed(method, pattern)` | `fixed` | Exact string match. | No captures. | Generates `pattern`. | Static endpoints such as `/health` or `/about`. |
| `udho::url::fixed(method, pattern, replacement)` | `fixed` | Exact string match. | No captures. | Generates `replacement`. | Exact match where the generated URL should be normalized or different. |
| `udho::url::regx(method, regex, replacement)` | `regex` | `std::regex_match` against the subject path. | Regex capture groups converted to callback argument types. | Generates `replacement` using `{}` placeholders. | Flexible matching that needs regular expressions. |
| `udho::url::scan(method, pattern)` | `p1729` | scnlib / P1729-style scan format. | Format fields converted to callback argument types. | Generates `pattern`. | Typed URL parameters with a readable route pattern. |
| `udho::url::scan(method, pattern, replacement)` | `p1729` | scnlib / P1729-style scan format. | Format fields converted to callback argument types. | Generates `replacement` using `{}` placeholders. | Typed URL parameters with a separate generation format. |

Examples:

```cpp
auto r0 = udho::url::slot("home"_h,  &home)  << udho::url::home(udho::url::verb::get);
auto r1 = udho::url::slot("about"_h, &about) << udho::url::fixed(udho::url::verb::get, "/about");
auto r2 = udho::url::slot("user"_h,  &user)  << udho::url::regx(udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}");
auto r3 = udho::url::slot("data"_h,  &data)  << udho::url::scan(udho::url::verb::get, "/data/{:d}/{:f}/{}", "/data/{}/{}/{}");
```

For non-home patterns, both the pattern and replacement must start with `/`. For mount paths, the path must start with `/`; a non-root mount path must not end with `/`.

### Actions

An action is the result of combining a slot and a match:

```cpp
auto action = udho::url::slot("article"_h, &article) << udho::url::scan(udho::url::verb::get, "/article/{:d}/{}", "/article/{}/{}");
```

An action can:

```cpp
bool matched = action.find(udho::url::verb::get, std::string{"/article/42/routing"});
bool invoked = action.invoke(udho::url::verb::get, std::string{"/article/42/routing"}, ctx);
std::string href = action(42, "routing");
```

The arguments explicitly supplied to `invoke(...)` become the leading callback arguments. The URL captures fill the remaining callback arguments. That is how the framework can pass the request context first and then append URL parameters.

### Mount points

A mount point attaches a chain of routes to a base path.

```cpp
auto api = udho::url::mount("api"_h, "/api", std::move(api_routes));
auto root = udho::url::root(std::move(root_routes));
```

A route pattern is relative to the mount point. If the mounted action uses pattern `/users/{:d}` under mount `/api`, the full URL is `/api/users/42`.

A mount point supports route lookup, invocation, keyed action access, and full mounted URL generation:

```cpp
auto api = udho::url::mount("api"_h, "/api", std::move(routes));

bool found = api.find(udho::url::verb::get, std::string{"/users/42"});
bool done  = api.invoke(udho::url::verb::get, std::string{"/users/42"}, ctx);

auto& show = api["show_user"_h];
std::string href1 = api("show_user"_h, 42);
std::string href2 = api.fill("show_user"_h, std::make_tuple(42));
```

`api("show_user"_h, 42)` includes the mount path. If the replacement is `/users/{}`, the generated URL is `/api/users/42`.

### Routers

A router owns one mount point, a table of mount points, or only explorers.

```cpp
auto router = udho::url::router(
      udho::url::root(std::move(public_routes))
    | udho::url::mount("admin"_h, "/admin", std::move(admin_routes))
);
```

The main lookup/invocation API is:

```cpp
bool exists = router.find(udho::url::verb::get, std::string{"/admin/dashboard"});
auto index = router.index_of(udho::url::verb::get, std::string{"/admin/dashboard"});

if (index.valid()) {
    router.invoke_at(index, ctx);
}
```

`index_of()` is useful when request dispatch needs to resolve the route once and then use the resolved index for both configuration and invocation.

The returned route index has these possible meanings:

| State | Meaning |
| --- | --- |
| `none` | No route or explorer matched. |
| `action` | A mounted route action matched. |
| `registry` | A static explorer matched. This fallback is used for `GET` requests. |

## Summary

The summary API is the runtime-friendly view of the routing table. It is useful for:

- debug output,
- generated route listing pages,
- templates/views that need links by route name,
- documentation or diagnostics without exposing the huge compile-time router type.

A router summary is obtained from the concrete router:

```cpp
const udho::url::summary::router& summary = router.summary();
```

Typical traversal:

```cpp
for (const auto& [mount_name, mount] : router.summary()) {
    std::cout << mount_name << " mounted at " << mount.path() << '\n';

    for (const auto& [route_key, action] : mount) {
        const auto& slot  = action.slot();
        const auto& match = action.match();

        std::cout << "  "
                  << match.method() << ' '
                  << match.pattern() << " -> "
                  << match.replacement() << " ["
                  << match.format() << "] "
                  << slot.key() << ' '
                  << slot.symbol() << '\n';
    }
}
```

Summary types expose this view:

| Type | API | Meaning |
| --- | --- | --- |
| `udho::url::summary::router` | `size()`, `begin()`, `end()`, `route(name)`, `operator[](name)` | Collection of summarized mount points. |
| `udho::url::summary::mount_point` | `name()`, `path()`, `size()`, `begin()`, `end()`, `url(key)`, `operator[](key)` | Summarized mount point and its actions. |
| `udho::url::summary::action` | `slot()`, `match()` | Summary of one route action. |
| `udho::url::summary::slot` | `key()`, `symbol()`, `nargs()` | Route label, callback symbol, callback argument count. |
| `udho::url::summary::match` | `method()`, `format()`, `pattern()`, `replacement()` | HTTP verb and URL pattern details. |

URL generation through the summary uses the stored replacement string:

```cpp
std::string href = router.summary()["root"].url("article")(42, "routing");
```

Important distinction: `summary::mount_point::url(key)` uses the action replacement string. It does not prepend the mount path by itself. When you need the full mounted URL from a concrete mount point, use the concrete mount point API:

```cpp
std::string full_href = api_mount("show_user"_h, 42);
```

When you only have the summary, combine the mount path and replacement-generated URL explicitly if needed.

## Options

Options attach route-local configuration to a match. The use case is: first resolve the route, then apply the matched route’s options into a larger runtime configuration object before invoking the callback.

This is useful for per-route settings such as layout selection, navigation style, authentication flags, page title behavior, session behavior, or any framework component configuration stored in a `hazo::map_d`-compatible object.

Define option keys using `HAZO_ELEMENT`:

```cpp
namespace opt {
    HAZO_ELEMENT(layout, std::string);
    HAZO_ELEMENT(auth,   bool);
    HAZO_ELEMENT(title,  std::string);
}
```

Create an option set directly:

```cpp
auto opts = udho::url::options(opt::layout("main"), opt::auth(true), opt::title("Profile"));
```

Attach options to a matcher. Preferred route style keeps the options at the end of the same route expression:

```cpp
auto route = udho::url::slot("profile"_h, &profile) << udho::url::scan(udho::url::verb::get, "/profile/{}", "/profile/{}").options(opt::layout("main"), opt::auth(true), opt::title("Profile"));
```

The `.options(...)` call returns a new matcher with the same verb, pattern, and replacement, but with the supplied option set.

Apply the options after route lookup:

```cpp
using runtime_config = udho::hazo::map_d<opt::layout, opt::auth, opt::title, opt::other>;

runtime_config config;
auto index = router.index_of(udho::url::verb::get, std::string{"/profile/alice"});

if (index.valid()) {
    router.reconfigure_for(index, config);
    router.invoke_at(index, ctx);
}
```

The option set only modifies keys present in the route’s option type. Other fields in the larger runtime config are left unchanged. Empty options are valid and apply nothing.

Direct option API:

```cpp
auto opts = udho::url::options(opt::layout("main"), opt::auth(true));

std::size_t n = opts.length;
auto layout = opts[opt::layout::val];

runtime_config config;
std::size_t changed = opts.apply(config);
```

## Explorer

Explorers are the static-resource side of the router. They are used when a request path does not match an action route but may correspond to a file, embedded asset, or directory listing.

The explorer layer is useful for:

- serving files from a document root,
- serving embedded assets from a resource store,
- combining several static sources under one router,
- generating directory/listing pages,
- applying MIME types to filesystem responses.

### Explorer model

All explorers implement the `udho::url::explorers::abstract_explorer` interface:

```cpp
struct abstract_explorer {
    const std::string& label() const;

    virtual bool exists(const std::string& subject) const = 0;
    virtual bool is_subset(const std::string& subject) const = 0;
    virtual bool cat(const std::string& subject, udho::net::ostream_view& stream) const = 0;
    virtual bool ls(const std::string& subject, udho::pages::system::data::listings& data) const;
};
```

The high-level meanings are:

| Function | Use |
| --- | --- |
| `label()` | Human-readable source name used in listings. |
| `exists(subject)` | True when `subject` is a concrete resource this explorer can serve. |
| `is_subset(subject)` | True when `subject` is a directory/prefix that can be listed. |
| `cat(subject, stream)` | Write the resource body to the network stream. |
| `ls(subject, data)` | Add listing entries for the directory/prefix. |

### Filesystem explorer

`udho::url::explorers::files` serves files from a filesystem document root.

```cpp
std::filesystem::path docroot = std::filesystem::current_path() / "public";
udho::url::explorers::files files{"docroot", docroot};

auto router = udho::url::router(std::move(files));
```

It also inherits `udho::url::mime_registry`, so MIME mappings can be extended before building the router:

```cpp
udho::url::explorers::files files{"docroot", docroot};
files.mime("myapp", "application/x-myapp");

auto router = udho::url::router(std::move(files));
```

The MIME registry is extension based and case-insensitive. If no extension mapping exists, the filesystem explorer falls back to automatic MIME detection.

### Asset explorer

`udho::url::explorers::assets` serves embedded assets from an asset `const_store`.

```cpp
resources.assets().base("assets");
resources.lock();

udho::view::resources::const_store<bridge_type> cstore{resources};

auto router = udho::url::router(
    udho::url::explorers::assets{"assets", cstore.assets()}
);
```

Use this for resources bundled into the application rather than files loaded from the deployment filesystem.

### Registry

`udho::url::explorers::registry` owns one or more explorers and is the object used by routers for static fallback.

```cpp
auto registry = udho::url::explorers::registry{
    udho::url::explorers::files{"docroot", docroot},
    udho::url::explorers::assets{"assets", cstore.assets()},
    udho::url::explorers::files{"alternate", alternate}
};

auto router = udho::url::router(udho::url::root(std::move(routes)), std::move(registry));
```

You can also pass explorers directly:

```cpp
auto router = udho::url::router(
    udho::url::explorers::files{"docroot", docroot},
    udho::url::explorers::assets{"assets", cstore.assets()}
);
```

Or combine action routes and static resources through router overloads:

```cpp
auto router = udho::url::router(
    udho::url::root(std::move(routes)),
    udho::url::explorers::registry{udho::url::explorers::files{"docroot", docroot}}
);
```

The registry checks explorers in registration order for concrete resources. This makes precedence explicit: put higher-priority static sources earlier.

For directory-like paths, listing data may be collected from every explorer that treats the subject as a subset/prefix. This allows a generated listing page to show entries from multiple static sources.

### Explorer-only router

A router can be created with no action routes, only static resources:

```cpp
auto router = udho::url::router(
    udho::url::explorers::files{"docroot", docroot},
    udho::url::explorers::assets{"assets", cstore.assets()}
);
```

This is useful for pure static serving, tests, and resource-store inspection.

### Route router with explorer fallback

A normal application router can combine dynamic routes and static fallback:

```cpp
auto router = udho::url::router(
      udho::url::root(std::move(routes))
    | udho::url::mount("admin"_h, "/admin", std::move(admin_routes)),
    udho::url::explorers::registry{
        udho::url::explorers::files{"docroot", docroot},
        udho::url::explorers::assets{"assets", cstore.assets()}
    }
);
```

Request handling then becomes:

```cpp
auto index = router.index_of(udho::url::verb::get, target);

if (index.valid()) {
    router.reconfigure_for(index, config);
    router.invoke_at(index, ctx);
} else {
    // framework can render 404 / route listing
}
```

For action matches, `invoke_at()` dispatches the callback. For registry matches, `invoke_at()` serves the file or generated listing through the explorer registry.

