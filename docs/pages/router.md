Router {#RouterPage}
=======

After registering the resources that application will add its own routes to the routing table as shown in teh next code listing.
However the store has to be finalized before the server can be started.
Because, after the server starts, no resources can be added to it.

```c++
store.finalize();
```

Now we pass the assets of the store to the router.

```c++
namespace url = udho::url;
namespace net = udho::net;

using namespace udho::hazo::string::literals;

auto router = url::router(
      url::root(
            url::slot("f0"_h,  &f0)         << url::home  (url::verb::get)
          | url::slot("chunked"_h,  &chunk) << url::fixed (url::verb::get, "/chunk")
      )
    | url::mount("b"_h, "/b",
          url::slot("f1"_h,  &f1)         << url::regx  (url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")
        | url::slot("xf1"_h, &X::f1, &x)  << url::regx  (url::verb::get, "/x/f1/(\\d+)/(\\w+)/(\\d+\\.\\d)", "/x/f1/{}/{}/{}")
    ),
    | url::mount("_m"_h, "/m", app.routes()),
    store.assets()
);

store.assets().base("/assets");                   // The document root from where the assets (js, css etc...) would be served.
auto artifacts  = net::artifacts(router, store);
```

You can view the routing table by printing the router.
Finally you create artifacts that coprises of the router and the assets of the store.


```c++
std::cout << "Routering Table: " << std::endl << router << std::endl;
```

The server runs on boost asio io_context using the artifacts.

```c++
boost::asio::io_service io;

auto server = udho::net::http_server{io, "0.0.0.0", 9000};

server.run(artifacts);

io.run();
```

## Slot

The views are executed using the context passed to the slots bounded with the url patterns. 
Given a context of type `udho::net::context<bridges::lua>` the views can be rendered by calling the render method.

```c++
void slot(net::context<bridges::lua> context, std::size_t id){
  user_data data = ...                  // fetch user data using id
  view::layouts::standard layout;       // layout will provide a bigger template in which the view contents will be plugged in
  context.view<bridges::lua>("profile", "users").render(data, layout);
}
```
