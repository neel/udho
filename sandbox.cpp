#include <iostream>
#include <udho/view/tmpl/detail/trie.h>
#include <udho/view/tmpl/sections.h>
#include <udho/view/data/data.h>
#include <udho/view/resources/store.h>
#include <udho/view/bridges/lua.h>
#include <udho/hazo/string/basic.h>
#include <stdio.h>
#include <complex>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem.hpp>
#include <udho/net/artifacts.h>
#include <udho/url/url.h>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <udho/net/context.h>
#include <type_traits>
#include <curl/curl.h>
#include <udho/url/url.h>
#include <boost/algorithm/string.hpp>
#include <udho/net/artifacts.h>
#include <tabulate/table.hpp>
#include <udho/view/tmpl/layout/layout.h>

struct subinfo{
    std::string desc = "DESC";

    friend auto metatype(udho::view::data::type<subinfo>){
        using namespace udho::view::data;

        return assoc("subinfo"),
            mvar("desc",  &subinfo::desc);
    }
};

struct info{
    std::string name;
    double      value;
    std::uint32_t _x;
    std::vector<subinfo> subs;

    inline double x() const { return _x; }
    inline void setx(const std::uint32_t& v) { _x = v; }

    const subinfo& operator[](const std::size_t& i) const {
        return subs.at(i);
    }
    std::size_t size() const { return subs.size(); }

    std::vector<subinfo>::const_iterator begin() const { return subs.begin(); }
    std::vector<subinfo>::const_iterator end() const { return subs.end(); }

    inline info() {
        name = "Hello";
        value = 42;
        _x = 43;
        subs.push_back(subinfo{});
        subs.push_back(subinfo{});
        subs.push_back(subinfo{});
    }

    void print(){
        std::cout << "name: " << name << " value: " << value  << std::endl;
    }

    friend auto metatype(udho::view::data::type<info>){
        using namespace udho::view::data;

        return assoc("info"),
            index(&info::operator[], &info::size),
            iter(&info::begin, &info::end),
            mvar("name",  &info::name),
            cvar("value", &info::value),
            fvar("x",     &info::x, &info::setx),
            mvar("sub",   &info::subs),
            func("print", &info::print);
    }
};

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;
using http_server     = udho::net::server<http_listener>;
using scgi_server     = udho::net::server<scgi_listener>;

void chunk3(udho::net::stream context){
    context << "Chunk 3 (Final)";
    context.finish();
}

void chunk2(udho::net::stream context){
    context << "chunk 2";
    context.flush(std::bind(&chunk3, context));
}

void chunk(udho::net::stream context){
    context.encoding(udho::net::types::transfer::encoding::chunked);
    context << "Chunk 1";
    context.flush(std::bind(&chunk2, context));
}

void f0(udho::net::stream context){
    context << "Hello f0";
    context.finish();
}

int f1(udho::net::stream context, int a, const std::string& b, const double& c){
        context << "Hello f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
}

struct X{
    void f0(udho::net::context<udho::view::data::bridges::lua> context){
        using context_type = udho::net::context<udho::view::data::bridges::lua>;
        using store_type   = typename context_type::resource_store;

        const store_type& store = context.resources();

        udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> proxy = store.view<udho::view::data::bridges::lua>("primary", "temp");

        info inf;
        inf.name = "NAME";
        inf.value = 42.42;
        inf._x    = 42;

        proxy(inf, context);

        context << "Hello X::f0";
        context << context.route("f0").name();
        context.finish();
        std::cout << context.route("f0").name() << std::endl;
    }

    int f1(udho::net::stream context, int a, const std::string& b, const double& c){
        context << "Hello X::f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
    }
};

static char buffer_router[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>

<?
for k, m in ctx.routes:pairs() do
    echo(k, m.size)

    for i, u in m:pairs() do
        echo(i, u)
        echo('\n')
    end
    echo('\n')
    echo('\n')
end
?>

<?= ctx.routes['b']['f1']:replace(1, 2, 3) ?>

<? if jit then ?>
LuaJIT is being used
LuaJIT version: <?= jit.version ?>
<? else ?>
LuaJIT is not being used
<? end ?>

Hello <?= d[1].desc ?>

<?
sub = udho.subinfo.new()
sub.desc = "Changed Desc"
d[1] = sub
?>

Hello <?= d.sub[1].desc ?>


<?
for i, value in d:ipairs() do
    echo(i, value.desc)
end
?>

<?:score udho.view() ?>

<# Some comments that will be ignored #>

<@ verbatim block @>

)TEMPLATE";

static char buffer_store[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>

Embedding view
<?= ctx:view("primary", "mini"):render() ?>

)TEMPLATE";

static char buffer_mini[] = R"TEMPLATE(
<?! vars('d', 'ctx'); whitespace(false) ?>

Mount points (<?= ctx.routes.size ?>)
==================
<? for label, mountpoint in ctx.routes:pairs() do ?>
    <? echo('\n') ?>
    <?= string.format("%s -> %s (%d)", label, mountpoint.path, mountpoint.size) ?>
    <?
        echo('\n')
        local table = udho.Tabulate.new()
        for name, pattern in mountpoint:pairs() do
            table:add(name, pattern)
        end
    ?>

<?= table ?>
<? end ?>


Javascript Assets (<?= ctx.resources.js.size ?>)
=======================
<? if ctx.resources.js.size == 0 then ?>
    <?= 'No Javascript Assets added' ?>
<? else ?>
    <? local table = udho.Tabulate.new() ?>
    <? for i, js in ctx.resources.js:ipairs() do ?>
        <? table:add(js.prefix, js.name, js.url) ?>
    <? end ?>

    <?= table ?>
<? end ?>


CSS Assets (<?= ctx.resources.css.size ?>)
================
<? if ctx.resources.css.size == 0 then ?>
    <?= 'No CSS Assets added' ?>
<? else ?>
    <? local table = udho.Tabulate.new() ?>
    <? for i, css in ctx.resources.css:ipairs() do ?>
        <? table:add(css.prefix, css.name, string.format("/%s/%s", css.prefix, css.name)) ?>
    <? end ?>

    <?= table ?>
<? end ?>


Image Assets (<?= ctx.resources.img.size ?>)
=================
<? if ctx.resources.img.size == 0 then ?>
    <?= 'No Image Assets added' ?>
<? else ?>
    <? local table = udho.Tabulate.new() ?>
    <? for i, img in ctx.resources.img:ipairs() do ?>
        <? table:add(img.prefix, img.name, string.format("/%s/%s", img.prefix, img.name)) ?>
    <? end ?>

    <?= table ?>
<? end ?>

)TEMPLATE";

struct Base{
    std::string value = "VALUE";
};

struct MyClass: Base {
    void modify() {
        // Modify the object
        std::cout << "modify" << std::endl;
    }
    void inspect() const {
        // Inspect the object without modifying it
        std::cout << "inspect" << std::endl;
    }
};

template <typename Data>
void run(sol::protected_function& view_fnc, const Data& data){
    sol::protected_function_result result = view_fnc(data);
    if (!result.valid()) {
        sol::error err = result;
        throw std::runtime_error("Error during function call: " + std::string(err.what()));
    } else {
        // Check if the return value can be converted to a string
        if (result.get_type() == sol::type::string) {
            std::string output = result;
            std::cout << "Function returned: " << output << std::endl;
        }
    }
}

int main(){
    // // // sol::state lua;
    // // // lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::utf8);
    // // //
    // // // auto type = lua.new_usertype<MyClass>("MyClass",
    // // //     "modify", &MyClass::modify,
    // // //     "inspect", &MyClass::inspect
    // // // );
    // // //
    // // // type.set("value",   &MyClass::value);
    // // //
    // // // static char buffer[] = R"TEMPLATE(
    // // // return function(obj)
    // // //     print(obj.value)
    // // //     obj:inspect()
    // // //     print("inspect called")
    // // //     obj:modify()
    // // //     print("modify called")
    // // //     return "hello"
    // // // end
    // // // )TEMPLATE";
    // // //
    // // // sol::load_result load_result = lua.load_buffer(buffer, strlen(buffer));
    // // // if (!load_result.valid()) {
    // // //     sol::error err = load_result;
    // // //     throw std::runtime_error("Error loading script: " + std::string(err.what()));
    // // // }
    // // //
    // // // sol::protected_function view =  load_result.get<sol::protected_function>();
    // // // sol::protected_function_result view_result = view();
    // // // if (!view_result.valid()) {
    // // //     sol::error err = view_result;
    // // //     throw std::runtime_error("Error during function extraction: " + std::string(err.what()));
    // // // }
    // // //
    // // // sol::protected_function view_fnc = view_result;
    // // // MyClass data;
    // // // run(view_fnc, data);
    // // // // const MyClass data;
    // // // // sol::protected_function_result result = view_fnc(data);
    // // // // if (!result.valid()) {
    // // // //     sol::error err = result;
    // // // //     throw std::runtime_error("Error during function call: " + std::string(err.what()));
    // // // // } else {
    // // // //     // Check if the return value can be converted to a string
    // // // //     if (result.get_type() == sol::type::string) {
    // // // //         std::string output = result;
    // // // //         std::cout << "Function returned: " << output << std::endl;
    // // // //     } else {
    // // // //         // Provide more information about the actual type returned
    // // // //         std::cout << "Function returned a non-string value. Actual type: " << sol::type_name(lua, result.get_type()) << std::endl;
    // // // //     }
    // // // // }
    // // // std::cout << "Lua function called successfully." << std::endl;
    // // // return 0;

    // std::cout << "udho::view::data::has_metatype<subinfo>::value " << udho::view::data::has_metatype<subinfo>::value << std::endl;
    //
    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<tabulate::Table>{});
    // lua.bind(udho::view::data::type<udho::url::summary::mount_point::url_proxy>{});
    // lua.bind(udho::view::data::type<udho::net::proxy_wrapper<udho::view::data::bridges::lua, udho::view::data::bridges::lua>>{});
    // lua.bind(udho::view::data::type<udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua>>{});
    // // lua.bind(udho::view::data::type<subinfo>{});
    // // lua.bind(udho::view::data::type<info>{});
    // bool res = lua.compile(udho::view::resources::resource::view("script.lua", buffer, buffer+sizeof(buffer)), "");
    // // std::cout << "compilation result " << res << std::endl;
    //
    info inf;
    inf.name = "NAME";
    inf.value = 42.42;
    inf._x    = 42;
    //
    // // udho::view::data::from_json(inf, nlohmann::json::parse(R"({"name":"NAME NAME","value":45.42,"x":43.0})"));
    // // std::cout << "Look HERE " << udho::view::data::to_json(inf) << std::endl;
    //
    // {
    //     auto tstart = std::chrono::high_resolution_clock::now();
    //     std::string output;
    //     lua.exec("script.lua", "", inf, output);
    //     auto tend = std::chrono::high_resolution_clock::now();
    //     std::chrono::duration<double> duration = tend - tstart;
    //     // std::string output = lua.eval("script.lua", inf);
    //     std::cout << output << std::endl;
    //     std::cout << "Execution time: " << std::fixed << std::setprecision(8) << duration.count() << " seconds" << std::endl;
    // }{
    //     auto tstart = std::chrono::high_resolution_clock::now();
    //     std::string output;
    //     lua.exec("script.lua", "", inf, output);
    //     auto tend = std::chrono::high_resolution_clock::now();
    //     std::chrono::duration<double> duration = tend - tstart;
    //     // std::string output = lua.eval("script.lua", inf);
    //     std::cout << output << std::endl;
    //     std::cout << "Execution time: " << std::fixed << std::setprecision(8) << duration.count() << " seconds" << std::endl;
    // }
    //
    //
    boost::filesystem::path temp = boost::filesystem::unique_path();
    {
        std::ofstream temp_stream(temp.c_str());
        temp_stream << buffer_router;
        temp_stream.close();
    }
    //
    // udho::view::resources::store<udho::view::data::bridges::lua> resources{lua};
    // auto& primary = resources.primary();
    // resources << udho::view::resources::resource::view("temp", temp);
    //
    // std::cout << "see views below " << resources.views.count() << std::endl;
    // for(const auto& res: resources.views){
    //     std::cout << res.name() << std::endl;
    // }
    // // std::cout << "view output" << std::endl <<primary.view("temp").eval(inf).str() << std::endl;
    // auto temp_view = resources.views["temp"];
    // auto results = temp_view(inf);
    // std::cout << "resources.views[temp](inf).str() " << std::endl;
    // std::cout << resources.views("temp", inf).str() << std::endl;

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};
    resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("temp", temp));
    resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("temp2", buffer_store, buffer_store+sizeof(buffer_store)));
    resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("mini",  buffer_mini, buffer_mini+sizeof(buffer_mini)));

    std::string js_str = "console.log('Hello World')";

    resource_store.assets().add("primary", udho::view::resources::asset::js("hello.js", js_str.begin(), js_str.end())->self().is_async(true) );
    resource_store.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> resource_store_proxy{resource_store};

    udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js> loader{resource_store_proxy.js()};


    using namespace udho::hazo::string::literals;

    X x;
    auto router = udho::url::router(
          udho::url::root(
                udho::url::slot("f0"_h,  &f0)         << udho::url::home  (udho::url::verb::get)
              | udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed (udho::url::verb::get, "/x/f0", "/x/f0")
              | udho::url::slot("chunked"_h,  &chunk) << udho::url::fixed (udho::url::verb::get, "/chunk")
          )
        | udho::url::mount("b"_h, "/b",
              udho::url::slot("f1"_h,  &f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")
            | udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx  (udho::url::verb::get, "/x/f1/(\\d+)/(\\w+)/(\\d+\\.\\d)", "/x/f1/{}/{}/{}")
        ),
        resource_store_proxy.assets()
    );

    std::cout << "Router: " << std::endl << router << std::endl;



    // auto tmpl_lua = resource_store_proxy.tmpl<udho::view::data::bridges::lua>();
    // std::cout << "see views below " << tmpl_lua.size("primary") << std::endl;
    // for(auto i = tmpl_lua.begin("primary"); i != tmpl_lua.end("primary"); ++i){
    //     std::cout << i->name() << std::endl;
    // }
    // udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> view = tmpl_lua("primary", "temp");
    // std::cout << view(inf).str() << std::endl;

    // udho::view::resources::tmpl::multi_substore_readonly_prefixed<udho::view::data::bridges::lua> multi_store_readonly_prefixed{resource_store_proxy._tmpls_proxy, "primary"};
    // auto tmpls_lua_prefixed = multi_store_readonly_prefixed.substore<udho::view::data::bridges::lua>();
    // std::cout << "see views below " << tmpls_lua_prefixed.size() << std::endl;

    // udho::view::resources::const_store_prefixed<udho::view::data::bridges::lua> resource_store_proxy_prefixed = resource_store_proxy["primary"];
    std::cout << resource_store_proxy.js().size() << std::endl;
    udho::view::resources::tmpl::const_substore<udho::view::data::bridges::lua> tmpl_lua = resource_store_proxy.tmpl<udho::view::data::bridges::lua>();
    std::cout << "see views below " << tmpl_lua.size() << std::endl;
    for(auto i = tmpl_lua.begin(); i != tmpl_lua.end(); ++i){
        std::cout << i->name() << std::endl;
    }
    udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> view_prefixed = tmpl_lua.view("primary", "temp");
    udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> view_store    = tmpl_lua.view("primary", "temp2");


    boost::asio::io_service io;
    auto server     = http_server{io, 9000};
    auto artifacts  = udho::net::artifacts(router, resource_store);

    udho::net::types::headers::request  request;
    udho::net::fake::context<udho::view::data::bridges::lua> fake_context_generator{request};
    udho::net::context<udho::view::data::bridges::lua> context = fake_context_generator.create(io, router, resource_store_proxy);


    std::cout << view_prefixed(inf, context).str() << std::endl;
    std::cout << view_store(inf, context).str() << std::endl;


    // const auto& summary = router.summary();
    // std::cout << summary.size() << std::endl;
    // std::cout << view_prefixed(inf, summary).str() << std::endl;
    //
    // std::cout << view_store(inf, resource_store_proxy).str() << std::endl;

    server.run(artifacts);

    // service.run();
    io.run();

}
