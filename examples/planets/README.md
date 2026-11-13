# Planets with standard and custom layouts

This application renders the same Lua views and C++ planet data with two layouts:

| Route | Layout | Action |
| --- | --- | --- |
| `/` | Udho standard layout | `planets::actions::home` |
| `/hello/Alice` | Udho standard layout | `planets::actions::hello` |
| `/layout` | Custom C++ presenter | `planets::actions::layout_home` |
| `/layout/hello/Alice` | Custom C++ presenter | `planets::actions::layout_hello` |

Replace `Alice` with another name to personalize the greeting. Both layout
variants share the views in `views/`, data types in `data.h`, and CSS in `assets/`.
All embedded resources use the `planets` prefix.

The custom presenter in `layout.h` defines the header, central `<main>`, and
footer markup, including their classes and shared header/footer contents. The
custom actions select it by passing the presenter as the second template
argument; the request context type is deduced:

```cpp
auto page = udho::view::tmpl::layout::attach<
    planets::layouts::placeholders,
    planets::layouts::presenter
>(context);
page.preamble().title("Planets with a custom layout");
```

The actions then select a central Lua view, assign the page data, and call
`page()` to render it. `attach` keeps the document alive until the asynchronous
response writes complete.

## Build

From the repository root, after configuring with `UDHO_BUILD_EXAMPLES=ON`:

```sh
cmake --build <build-dir> --target udho-examples-planets udho-examples-planets-https
```

Both executables link `libexamples-planets-common.a`, containing the shared
actions and data types. They use the same routes, views, CSS, and embedded
resource bundle. `http.cpp` and `https.cpp` provide the transport setup.

## Run over HTTP

From `<build-dir>/examples/planets`:

```sh
./udho-examples-planets
```

Open <http://localhost:9999/> for the standard layout or
<http://localhost:9999/layout> for the custom layout. Personalized views are at
<http://localhost:9999/hello/Alice> and
<http://localhost:9999/layout/hello/Alice>.

## Run over HTTPS

From the same directory:

```sh
./planets-https-gen.sh
./udho-examples-planets-https
```

CMake copies the certificate script beside the executables. It generates
`localhost-cert.pem` and `localhost-key.pem` in the current directory for local
development. Open <https://localhost:9999/> or
<https://localhost:9999/layout>; the personalized routes are available over HTTPS
as well.

To use existing certificate and private-key PEM files:

```sh
./udho-examples-planets-https --cert server-cert.pem --key server-key.pem
```

Both transports default to port 9999. Run them one at a time or select a different
port with `--port`. Use `--help` to see all available options:

```sh
./udho-examples-planets --help
./udho-examples-planets-https --help
```
