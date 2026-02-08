#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/bridges/lua.h>
#include <udho/view/data/data.h>
#include <udho/url/url.h>
#include <tabulate/table.hpp>
#include <nlohmann/json.hpp>

#include "data.h"

#include <udho/session/abstract_catalogue.h>
#include <udho/session/storage/fs.h>
#include <udho/session/catalogue.h>

#include <udho/manifold/composition.h>
#include <udho/manifold/composition_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/manifold/configs_view.h>
#include <udho/manifold/components/routing.h>
#include <udho/manifold/context.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/store.h>
#include <udho/manifold/components/resources.h>

using session_catalogue = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;

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

using namespace udho::manifold::components;
using namespace udho::manifold;

namespace callbacks{

using stream_type      = boost::beast::test::stream;
using handler = basic_handler<stream_type>;

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

        udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> proxy = store.view<udho::view::data::bridges::lua>("primary", "temp");

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

TEST_CASE("Lua Context Interop", "[view][lua][context][interop]") {
    static char buffer[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
{
    "router": {
        "size": <?= ctx.portal.routes.size ?>,
        "mountpoints": [
            <? local is_first = true ?>
            <? for label, mountpoint in ctx.portal.routes:pairs() do ?>
            <? if not is_first then ?>,<? end ?>
            {
                "label": "<?= label ?>",
                "path" : "<?= mountpoint.path ?>",
                "size" :  <?= mountpoint.size ?>,
                "routes": [
                    <? local is_first_route = true ?>
                    <? for name, action in mountpoint:pairs() do ?>
                    <? if not is_first_route then ?>,<? end ?>
                    {
                        "name": "<?= name ?>",
                        "pattern": "<?= action.match.replacement ?>"
                    }
                    <? is_first_route = false ?>
                    <? end ?>
                ]
            }
            <? is_first = false ?>
            <? end ?>
        ]
    },
    "resources": {
        "js": {
            "size": <?= ctx.portal.resources.js.size ?>,
            "resources": [
                <? local is_first = true ?>
                <? for i, js in ctx.portal.resources.js:ipairs() do ?>
                <? if not is_first then ?>,<? end ?>
                {
                    "prefix": "<?= js.prefix ?>",
                    "name"  : "<?= js.name ?>",
                    "url"   : "<?= js.url ?>"
                }
                <? is_first = false ?>
                <? end ?>
            ]
        },
        "css": {
            "size": <?= ctx.portal.resources.css.size ?>,
            "resources": [
                <? local is_first = true ?>
                <? for i, css in ctx.portal.resources.css:ipairs() do ?>
                <? if not is_first then ?>,<? end ?>
                {
                    "prefix": "<?= css.prefix ?>",
                    "name"  : "<?= css.name ?>",
                    "url"   : "<?= css.url ?>"
                }
                <? is_first = false ?>
                <? end ?>
            ]
        },
        "img": {
            "size": <?= ctx.portal.resources.img.size ?>,
            "resources": [
                <? local is_first = true ?>
                <? for i, img in ctx.portal.resources.img:ipairs() do ?>
                <? if not is_first then ?>,<? end ?>
                {
                    "prefix": "<?= img.prefix ?>",
                    "name"  : "<?= img.name ?>",
                    "url"   : "<?= img.url ?>"
                }
                <? is_first = false ?>
                <? end ?>
            ]
        }
    }
}
)TEMPLATE";

    static char buffer_js[]  = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static char buffer_img[] = "console.log('Hello, world!');";

    student p;

    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<tabulate::Table>{});
    lua.bind(udho::view::data::type<udho::url::summary::router>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};
    resource_store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("ctx_explorer",  buffer, buffer+sizeof(buffer)));

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
    udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> ctx_explorer = tmpl_lua.view("primary", "ctx_explorer");

    boost::asio::io_context io;

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

    using stream_type = udho::net::test_ostream;;

    auto handler_component  = udho::manifold::components::basic_handler<callbacks::stream_type>(router.table().summary());
    auto routing_component  = udho::manifold::components::routing(std::move(router));
    auto resource_component = udho::manifold::components::resources(resource_store_proxy);

    using composition_type  = udho::manifold::composition<
        std::decay_t<decltype(handler_component)>,
        std::decay_t<decltype(routing_component)>,
        std::decay_t<decltype(resource_component)>
    >;

    using context_type      = typename udho::manifold::detail::get_context_for_composition<stream_type, composition_type>::type;
    using configs_type      = typename composition_type::configs_type;
    using journal_type      = typename udho::manifold::detail::get_journal_for_full_fabric<composition_type>::type;

    auto composition = composition_type::compose(handler_component, routing_component, resource_component);

    configs_type configs;
    journal_type journal;

    auto portal = udho::manifold::portal(composition, configs, journal);
    boost::beast::test::stream stream_in(io);
    boost::beast::test::stream stream_out(io);
    stream_in.connect(stream_out);

    stream_type stream(stream_in,
       [&](boost::system::error_code ec, std::size_t) {
           CHECK_FALSE(ec);
       }
    );

    auto context = udho::manifold::basic_context(stream, portal, 0);

    lua.bind(udho::view::data::type<std::decay_t<decltype(context)>>{});

    std::string output = ctx_explorer(p, context).str();
    // std::cout << output << std::endl;
    nlohmann::json output_json = nlohmann::json::parse(output);

    std::string expected_output = R"(
    {
        "router": {
            "size": 2,
            "mountpoints": [{
                "label": "b",
                "path": "/b",
                "size": 2,
                "routes": [{
                    "name": "f1",
                    "pattern": "/f1/{}/{}/{}"
                },{
                    "name": "xf1",
                    "pattern": "/x/f1/{}/{}/{}"
                }]
            },{
                "label": "root",
                "path": "/",
                "size": 3,
                "routes": [{
                    "name": "chunked",
                    "pattern": "/chunk"
                },{
                    "name": "f0",
                    "pattern": "/"
                },{
                    "name": "xf0",
                    "pattern": "/x/f0"
                }]
            }
        ]},"resources": {
            "js": {
                "size": 2,
                "resources": [{
                    "prefix": "primary",
                    "name": "0profile1.js",
                    "url": "/assets/primary/0profile1.js"
                },{
                    "prefix": "primary",
                    "name": "1profile2.js",
                    "url": "/assets/primary/1profile2.js"
                }]
            }, "css": {
                "size": 1,
                "resources": [{
                    "prefix": "primary",
                    "name": "2profile.css",
                    "url": "/assets/primary/2profile.css"
                }]
            }, "img": {
                "size": 1,
                "resources": [{
                    "prefix": "primary",
                    "name": "3profile.png",
                    "url": "/assets/primary/3profile.png"
                }]
            }
        }
    }
    )";

    nlohmann::json expected_json = nlohmann::json::parse(expected_output);
    CHECK(output_json.dump() == expected_json.dump());

    SECTION("Check Router") {
        auto& router = output_json["router"];
        REQUIRE(router["size"] == 2);

        SECTION("Mountpoint b") {
            auto& b = router["mountpoints"][0];
            CHECK(b["label"] == "b");
            CHECK(b["path"] == "/b");
            CHECK(b["size"] == 2);

            SECTION("Routes in b") {
                CHECK(b["routes"][0]["name"] == "f1");
                CHECK(b["routes"][0]["pattern"] == "/f1/{}/{}/{}");
                CHECK(b["routes"][1]["name"] == "xf1");
                CHECK(b["routes"][1]["pattern"] == "/x/f1/{}/{}/{}");
            }
        }

        SECTION("Mountpoint root") {
            auto& root = router["mountpoints"][1];
            CHECK(root["label"] == "root");
            CHECK(root["path"] == "/");
            CHECK(root["size"] == 3);

            SECTION("Routes in root") {
                CHECK(root["routes"][0]["name"] == "chunked");
                CHECK(root["routes"][0]["pattern"] == "/chunk");
                CHECK(root["routes"][1]["name"] == "f0");
                CHECK(root["routes"][1]["pattern"] == "/");
                CHECK(root["routes"][2]["name"] == "xf0");
                CHECK(root["routes"][2]["pattern"] == "/x/f0");
            }
        }
    }

    SECTION("Check Resources") {
        auto& resources = output_json["resources"];

        SECTION("JS Resources") {
            auto& js = resources["js"];
            REQUIRE(js["size"] == 2);
            CHECK(js["resources"][0]["prefix"] == "primary");
            CHECK(js["resources"][0]["name"] == "0profile1.js");
            CHECK(js["resources"][0]["url"] == "/assets/primary/0profile1.js");
            CHECK(js["resources"][1]["prefix"] == "primary");
            CHECK(js["resources"][1]["name"] == "1profile2.js");
            CHECK(js["resources"][1]["url"] == "/assets/primary/1profile2.js");
        }

        SECTION("CSS Resources") {
            auto& css = resources["css"];
            REQUIRE(css["size"] == 1);
            CHECK(css["resources"][0]["prefix"] == "primary");
            CHECK(css["resources"][0]["name"] == "2profile.css");
            CHECK(css["resources"][0]["url"] == "/assets/primary/2profile.css");
        }

        SECTION("IMG Resources") {
            auto& img = resources["img"];
            REQUIRE(img["size"] == 1);
            CHECK(img["resources"][0]["prefix"] == "primary");
            CHECK(img["resources"][0]["name"] == "3profile.png");
            CHECK(img["resources"][0]["url"] == "/assets/primary/3profile.png");
        }
    }
}
