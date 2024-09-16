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


struct subinfo{
    std::string desc = "DESC";

    friend auto prototype(udho::view::data::type<subinfo>){
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

    inline info() {
        name = "Hello";
        value = 42;
        _x = 43;
        subs.push_back(subinfo{});
    }

    void print(){
        std::cout << "name: " << name << " value: " << value  << std::endl;
    }

    friend auto prototype(udho::view::data::type<info>){
        using namespace udho::view::data;

        return assoc("info"),
            mvar("name",  &info::name),
            cvar("value", &info::value),
            fvar("x",     &info::x, &info::setx),
            mvar("sub",   &info::subs),
            func("print", &info::print);
    }
};

static char buffer[] = R"TEMPLATE(
<?! register "views.user.badge"; lang "lua" ?>

<? if jit then ?>
LuaJIT is being used
LuaJIT version: <?= jit.version ?>
<? else ?>
LuaJIT is not being used
<? end ?>

Hello <?= d.sub[1].desc ?>

<?:score udho.view() ?>

<# Some comments that will be ignored #>

<@ verbatim block @>

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

    // std::cout << "udho::view::data::has_prototype<subinfo>::value " << udho::view::data::has_prototype<subinfo>::value << std::endl;
    //
    udho::view::data::bridges::lua lua;
    lua.init();
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
        temp_stream << buffer;
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

    using namespace udho::hazo::string::literals;

    X x;
    auto router = udho::url::router(
        udho::url::root(
            udho::url::slot("f0"_h,  &f0)         << udho::url::home  (udho::url::verb::get)                                                  |
            udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed (udho::url::verb::get, "/x/f0", "/x/f0")                                |
            udho::url::slot("chunked"_h,  &chunk) << udho::url::fixed (udho::url::verb::get, "/chunk")
        ) |
        udho::url::mount("b"_h, "/b",
            udho::url::slot("f1"_h,  &f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")      |
            udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx  (udho::url::verb::get, "/x/f1/(\\d+)/(\\w+)/(\\d+\\.\\d)", "/x/f1/{}/{}/{}")
        )
    );

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};
    resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::resource::view("temp", temp));
    resource_store.lock();

    udho::view::resources::store_readonly<udho::view::data::bridges::lua> resource_store_proxy{resource_store};
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

    udho::view::resources::store_readonly_prefixed<udho::view::data::bridges::lua> resource_store_proxy_prefixed{resource_store_proxy, "primary"};
    std::cout << resource_store_proxy_prefixed.js().size() << std::endl;
    auto tmpl_lua_prefixed = resource_store_proxy_prefixed.tmpl<udho::view::data::bridges::lua>();
    std::cout << "see views below " << tmpl_lua_prefixed.size() << std::endl;
    for(auto i = tmpl_lua_prefixed.begin(); i != tmpl_lua_prefixed.end(); ++i){
        std::cout << i->name() << std::endl;
    }
    udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> view_prefixed = tmpl_lua_prefixed["temp"];
    std::cout << view_prefixed(inf).str() << std::endl;

    boost::asio::io_service service;
    auto server     = http_server{service, 9000};
    auto artifacts  = udho::net::artifacts<decltype(router), udho::view::resources::store<udho::view::data::bridges::lua> >{router, resource_store};
    // server.run(artifacts);

    // service.run();

}
