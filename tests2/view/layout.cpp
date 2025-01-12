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

#include "data.h"

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

// TEST_CASE("View layout normal functionalities", "[view][lua][layout]") {
//     udho::view::data::bridges::lua lua;
//     lua.init();
//     lua.bind(udho::view::data::type<tabulate::Table>{});
//     lua.bind(udho::view::data::type<udho::net::context<udho::view::data::bridges::lua>>{});
//     info inf;
//     inf.name = "NAME";
//     inf.value = 42.42;
//     inf._x    = 42;
//
//     boost::filesystem::path temp = boost::filesystem::unique_path();
//     {
//         std::ofstream temp_stream(temp.c_str());
//         temp_stream << buffer_router;
//         temp_stream.close();
//     }
//
//     udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};
//     resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("temp", temp));
//     resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("temp2", buffer_store, buffer_store+sizeof(buffer_store)));
//     resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("mini",  buffer_mini, buffer_mini+sizeof(buffer_mini)));
//
//     std::string js_str = "console.log(\"Hello World\")";
//
//     resource_store.assets().add("primary", udho::view::resources::asset::js("hello.js", js_str.begin(), js_str.end())->self().is_async(true) );
//     resource_store.lock();
//
//     udho::view::resources::const_store<udho::view::data::bridges::lua> resource_store_proxy{resource_store};
//     udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js> loader{resource_store_proxy.js()};
// }


TEST_CASE("udho view layout regular functionalities", "[view][layout]") {
    CHECK(0 == 0);
}
