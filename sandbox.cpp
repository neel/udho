#include <iostream>
#include <udho/view/trie.h>
#include <udho/view/sections.h>
#include <udho/view/scope.h>
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
    void f0(udho::net::stream context){
        context << "Hello X::f0";
        context.finish();
    }

    int f1(udho::net::stream context, int a, const std::string& b, const double& c){
        context << "Hello X::f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
    }
};


struct info{
    std::string name;
    double      value;
    std::uint32_t _x;

    inline double x() const { return _x; }
    inline void setx(const std::uint32_t& v) { _x = v; }

    inline info() {
        name = "Hello";
        value = 42;
        _x = 43;
    }

    void print(){
        std::cout << "name: " << name << " value: " << value  << std::endl;
    }

    friend auto prototype(udho::view::data::type<info>){
        using namespace udho::view::data;

        return assoc(
            mvar("name",  &info::name),
            cvar("value", &info::value),
            fvar("x",     &info::x, &info::setx),
            func("print", &info::print)
        ).as("info");
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

Hello <?= d.name ?>

<?:score udho.view() ?>

<# Some comments that will be ignored #>

<@ verbatim block @>

)TEMPLATE";

int main(){

    udho::view::detail::trie trie;

    std::string ab = "ab",
                abc = "abc",
                abcdef = "abcdef",
                xycdef = "xycdef";

    trie.add(ab, 101);
    trie.add(abc, 102);
    trie.add(abcdef, 103);
    trie.add(xycdef, 104);

    std::string subject = "hello ab I am a string abcd abcdef and then abcxycdef";
    auto begin = subject.begin();
    auto end = subject.end();

    auto pos = begin;
    while(pos != end){
        auto tpos = trie.next(pos, end);
        pos = tpos.token_end;
        std::cout << trie[tpos.token_id] << std::endl;
        std::string buff;
        std::copy(tpos.token_begin, tpos.token_end, std::back_inserter(buff));
        std::cout << "buff: " << std::distance(tpos.token_begin, tpos.token_end) << " " << buff << std::endl;
    }

    std::cout << "hello world" << std::endl;

    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<info>{});
    bool res = lua.compile(udho::view::resources::resource::view("script.lua", buffer, buffer+sizeof(buffer)), "");
    // std::cout << "compilation result " << res << std::endl;
    info inf;
    inf.name = "NAME";
    inf.value = 42.42;
    inf._x    = 42;

    {
        auto tstart = std::chrono::high_resolution_clock::now();
        std::string output;
        lua.exec("script.lua", "", inf, output);
        auto tend = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = tend - tstart;
        // std::string output = lua.eval("script.lua", inf);
        std::cout << output << std::endl;
        std::cout << "Execution time: " << std::fixed << std::setprecision(8) << duration.count() << " seconds" << std::endl;
    }{
        auto tstart = std::chrono::high_resolution_clock::now();
        std::string output;
        lua.exec("script.lua", "", inf, output);
        auto tend = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = tend - tstart;
        // std::string output = lua.eval("script.lua", inf);
        std::cout << output << std::endl;
        std::cout << "Execution time: " << std::fixed << std::setprecision(8) << duration.count() << " seconds" << std::endl;
    }


    boost::filesystem::path temp = boost::filesystem::unique_path();
    {
        std::ofstream temp_stream(temp.c_str());
        temp_stream << buffer;
        temp_stream.close();
    }

    udho::view::resources::store<udho::view::data::bridges::lua> resources{lua};
    // auto& primary = resources.primary();
    resources << udho::view::resources::resource::view("temp", temp);

    std::cout << "see views below " << resources.views.count() << std::endl;
    for(const auto& res: resources.views){
        std::cout << res.name() << std::endl;
    }
    // std::cout << "view output" << std::endl <<primary.view("temp").eval(inf).str() << std::endl;
    auto temp_view = resources.views["temp"];
    auto results = temp_view(inf);
    std::cout << "resources.views[temp](inf).str() " << std::endl;
    std::cout << resources.views("temp", inf).str() << std::endl;

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

    boost::asio::io_service service;
    auto server     = http_server{service, 9000};
    auto artifacts  = udho::net::artifacts{router, resources};
    server.run(artifacts);

    service.run();

}
