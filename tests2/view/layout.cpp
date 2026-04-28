#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>
#include <udho/url/detail/format.h>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/presenter.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/url/url.h>
#include "data.h"
#include <udho/session/abstract_catalogue.h>
#include <udho/session/storage/fs.h>
#include <udho/session/catalogue.h>

#include <udho/manifold/composition.h>
#include <udho/manifold/composition_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/manifold/configs_view.h>
#include <udho/www/components/routing.h>
#include <udho/www/components/handler.h>
#include <udho/manifold/context.h>

// using session_catalogue = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;

static char buffer_router[] = R"TEMPLATE(
<?! vars(\"d\", \"ctx\") ?>

<?
for k, m in ctx.routes:pairs() do
    echo(k, m.size)

    for i, u in m:pairs() do
        echo(i, u)
        echo(\"\n\")
    end
    echo(\"\n\")
    echo(\"\n\")
end
?>

<?= ctx.routes[\"b\"][\"f1\"]:replace(1, 2, 3) ?>

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
<?! vars(\"d\", \"ctx\") ?>

Embedding view
<?= ctx:view("primary", "mini"):render() ?>

)TEMPLATE";

static char buffer_mini[] = R"TEMPLATE(
<?! vars(\"d\", \"ctx\"); whitespace(false) ?>

Mount points (<?= ctx.routes.size ?>)
==================
<? for label, mountpoint in ctx.routes:pairs() do ?>
    <? echo(\"\n\") ?>
    <?= string.format("%s -> %s (%d)", label, mountpoint.path, mountpoint.size) ?>
    <?
        echo(\"\n\")
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
    <?= \"No Javascript Assets added\" ?>
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
    <?= \"No CSS Assets added\" ?>
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
    <?= \"No Image Assets added\" ?>
<? else ?>
    <? local table = udho.Tabulate.new() ?>
    <? for i, img in ctx.resources.img:ipairs() do ?>
        <? table:add(img.prefix, img.name, string.format("/%s/%s", img.prefix, img.name)) ?>
    <? end ?>

    <?= table ?>
<? end ?>

)TEMPLATE";


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

using namespace udho::www::components;
using namespace udho::manifold;

using stream_type      = boost::beast::test::stream;
using handler = basic_handler<stream_type>;

namespace callbacks{

void chunk3(basic_context<stream_type, handler> context){
    context << "Chunk 3 (Final)";
    context.finish();
}

void chunk2(basic_context<stream_type, handler> context){
    context << "chunk 2";
    chunk3(context);
}

void chunk(basic_context<stream_type, handler> context){
    context.encoding(udho::net::types::transfer::encoding::chunked);
    context << "Chunk 1";
    chunk2(context);
}

void f0(basic_context<stream_type, handler> context){
    context << "Hello f0";
    context.finish();
}

int f1(basic_context<stream_type, handler> context, int a, const std::string& b, const double& c){
        context << "Hello f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
}

struct X{
    void f0(basic_context<stream_type, handler, resources<udho::view::data::bridges::lua>> context){
        const auto& store = context.portal().resources();

        udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> proxy = store.template view<udho::view::data::bridges::lua>("primary", "temp");

        info inf;
        inf.name = "NAME";
        inf.value = 42.42;
        inf._x    = 42;

        proxy(inf, context);

        context << "Hello X::f0";
        context << context.portal().route("f0").name();
        context.finish();
        std::cout << context.portal().route("f0").name() << std::endl;
    }

    int f1(basic_context<stream_type, handler> context, int a, const std::string& b, const double& c){
        context << "Hello X::f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
    }
};

}

TEST_CASE("udho view layout regular functionalities", "[view][layout]") {
    CHECK(0 == 0);

    static char buffer_js[]  = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static char buffer_img[] = "console.log('Hello, world!');";

    student p;

    boost::asio::io_context io;

    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<tabulate::Table>{});
    // lua.bind(udho::view::data::type<udho::net::context<udho::view::data::bridges::lua>>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};

    const std::map<std::string, std::tuple<std::string, udho::view::resources::asset::type, std::string>> assets = {
        {"0profile1.js", {"primary", udho::view::resources::asset::type::js, buffer_js}},
        {"1profile2.js", {"primary", udho::view::resources::asset::type::js, buffer_js1}},
        {"2profile.css", {"primary", udho::view::resources::asset::type::css, buffer_css}},
        {"3profile.png", {"primary", udho::view::resources::asset::type::img, buffer_img}}
    };

    {
        auto it = assets.cbegin();
        resource_store[std::get<0>(it->second)] << udho::view::resources::asset::js(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        resource_store[std::get<0>(it->second)] << udho::view::resources::asset::js(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        resource_store[std::get<0>(it->second)] << udho::view::resources::asset::css(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        resource_store[std::get<0>(it->second)] << udho::view::resources::asset::img(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
    }
    resource_store.assets().base("assets");
    resource_store.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> resource_store_proxy{resource_store};
    udho::view::resources::tmpl::const_substore<udho::view::data::bridges::lua> tmpl_lua = resource_store_proxy.tmpl<udho::view::data::bridges::lua>();
    // udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> ctx_explorer = tmpl_lua.view("primary", "ctx_explorer");

    using namespace udho::hazo::string::literals;

    callbacks::X x;
    auto router = udho::url::router(
          udho::url::root(
                udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home  (udho::url::verb::get)
              | udho::url::slot("xf0"_h, &callbacks::X::f0, &x)  << udho::url::fixed (udho::url::verb::get, "/x/f0", "/x/f0")
              | udho::url::slot("chunked"_h,  &callbacks::chunk) << udho::url::fixed (udho::url::verb::get, "/chunk")
          )
        | udho::url::mount("b"_h, "/b",
              udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")
            | udho::url::slot("xf1"_h, &callbacks::X::f1, &x)  << udho::url::regx  (udho::url::verb::get, "/x/f1/(\\d+)/(\\w+)/(\\d+\\.\\d)", "/x/f1/{}/{}/{}")
        ),
        resource_store_proxy.assets()
    );

    auto handler_component  = udho::www::components::basic_handler<stream_type>(router.table().summary());
    auto routing_component  = udho::www::components::routing(std::move(router));
    auto resource_component = udho::www::components::resources(resource_store_proxy);

    using composition_type  = udho::manifold::composition<
        std::decay_t<decltype(handler_component)>,
        std::decay_t<decltype(routing_component)>,
        std::decay_t<decltype(resource_component)>
    >;
    using configs_type      = typename composition_type::configs_type;
    using journal_type      = typename udho::manifold::detail::get_journal_for_full_fabric<composition_type>::type;

    auto composition = composition_type::compose(handler_component, routing_component, resource_component);

    configs_type configs;
    journal_type journal;

    auto portal = udho::manifold::portal(composition, configs, journal);

    // udho::net::types::headers::request request;

    boost::beast::test::stream stream_in(io);
    boost::beast::test::stream stream_out(io);
    stream_in.connect(stream_out);

    udho::net::test_ostream stream(stream_in,
        [&](boost::system::error_code ec, std::size_t) {
            CHECK_FALSE(ec);
        },
        [&](udho::net::test_ostream& s){
            s.finish();
        }
    );

    auto context = udho::manifold::basic_context(stream, portal, 0);

    // udho::net::ostream_view stream_view = stream.view();


    {
        // udho::view::tmpl::layout::standard_layout<context_type> layout{context};
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        layout.preamble().title("Page title");
        namespace placeholders = udho::view::tmpl::layout::placeholders;
        layout[placeholders::central] = "Hello";

        layout();

    }

    io.run();

    std::string output = stream_out.str();
    std::cout << output << std::endl;
}
