#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <cstdint>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

#include <udho/manifold/composition.h>
#include <udho/manifold/configs_view.h>
#include <udho/manifold/context.h>
#include <udho/manifold/journal_view.h>

#include <udho/url/url.h>

#include <udho/view/bridges/lua.h>
#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/resources/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/presenter.h>
#include <udho/view/tmpl/layout/renderer.h>

#include <udho/www/components/handler.h>
#include <udho/www/components/resources.h>
#include <udho/www/components/routing.h>

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

namespace {

static char template_mapped[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
<span class="mapped-view"><?= d.value ?>:<?= d.ordinal ?></span>
)TEMPLATE";

static char template_explicit[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
<span class="explicit-view"><?= d.value ?>:<?= d.ordinal ?></span>
)TEMPLATE";

static char template_assets[] = R"TEMPLATE(
<?! vars('d', 'ctx') include.js("renderer", "external.js") include.css("renderer", "external.css") embed.js("renderer", "embedded.js") embed.css("renderer", "embedded.css") ?>
<span class="asset-view"><?= d.value ?></span>
)TEMPLATE";

static char external_js[] = "window.rendererExternal = true;";
static char embedded_js[] = "window.rendererEmbedded = true;";
static char external_css[] = ".renderer-external { display: block; }";
static char embedded_css[] = ".renderer-embedded { display: inline; }";

struct renderer_data {
    std::string value;
    std::uint32_t ordinal;

    friend auto metatype(udho::view::data::type<renderer_data>) {
        using namespace udho::view::data;

        return assoc("renderer_data"),
               cvar("value", &renderer_data::value),
               cvar("ordinal", &renderer_data::ordinal);
    }
};

using namespace udho::www::components;
using namespace udho::manifold;

using stream_type = boost::beast::test::stream;
using handler_type = basic_handler<stream_type>;

void unused_route(basic_context<stream_type, handler_type> context) {
    context << "unused";
    context.finish();
}

} // namespace

TEST_CASE("View layout renderer", "[view][layout][renderer]") {
    boost::asio::io_context io;

    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<renderer_data>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};

    resource_store["renderer"]
        << udho::view::resources::lua{
               "mapped", std::begin(template_mapped), std::end(template_mapped)}
        << udho::view::resources::lua{
               "explicit", std::begin(template_explicit), std::end(template_explicit)}
        << udho::view::resources::lua{
               "assets", std::begin(template_assets), std::end(template_assets)}
        << udho::view::resources::asset::js(
               "external.js", std::begin(external_js), std::end(external_js))
        << udho::view::resources::asset::js(
               "embedded.js", std::begin(embedded_js), std::end(embedded_js))
        << udho::view::resources::asset::css(
               "external.css", std::begin(external_css), std::end(external_css))
        << udho::view::resources::asset::css(
               "embedded.css", std::begin(embedded_css), std::end(embedded_css));

    resource_store.assets().base("assets");
    resource_store.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua>
        resource_store_proxy{resource_store};

    using namespace udho::hazo::string::literals;

    auto router = udho::url::router(
        udho::url::root(
            udho::url::slot("unused"_h, &unused_route)
                << udho::url::home(udho::url::verb::get)),
        resource_store_proxy.assets());

    auto handler_component =
        udho::www::components::basic_handler<stream_type>(router.table().summary());
    auto routing_component = udho::www::components::routing(std::move(router));
    auto resource_component = udho::www::components::resources(resource_store_proxy);

    using composition_type = udho::manifold::composition<
        std::decay_t<decltype(handler_component)>,
        std::decay_t<decltype(routing_component)>,
        std::decay_t<decltype(resource_component)>>;
    using configs_type = typename composition_type::configs_type;
    using journal_type = typename udho::manifold::detail::get_journal_for_full_fabric<
        composition_type>::type;

    auto composition = composition_type::compose(
        handler_component, routing_component, resource_component);

    configs_type configs;
    journal_type journal;
    auto portal = udho::manifold::portal(composition, configs, journal);

    boost::beast::test::stream stream_in(io);
    boost::beast::test::stream stream_out(io);
    stream_in.connect(stream_out);

    udho::net::test_ostream stream(
        stream_in,
        [&](boost::system::error_code ec, std::size_t) {
            CHECK_FALSE(ec);
        },
        [&](udho::net::test_ostream& output) {
            output.finish();
        });

    auto context = udho::manifold::basic_context(stream, portal, 0);

    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = udho::view::tmpl::layout::placeholders;

    auto response_body = [&]() {
        io.run();

        const std::string output = stream_out.str();
        CAPTURE(output);

        boost::beast::http::response_parser<boost::beast::http::string_body> parser;
        parser.eager(true);

        boost::beast::error_code error;
        parser.put(boost::asio::buffer(output), error);

        CHECK_FALSE(error);
        CHECK(parser.is_done());

        boost::beast::http::response<boost::beast::http::string_body> response =
            parser.release();
        return response.body();
    };

    SECTION("Single-valued and multi-valued renderers expose the same render interface") {
        auto page = layout::create<layout::placeholders::standard>(context);

        auto single = page[placeholders::central];
        auto multiple = page[placeholders::left];

        using single_renderer = decltype(single);
        using multiple_renderer = decltype(multiple);

        static_assert(std::is_same_v<
                      decltype(std::declval<single_renderer&>().render(
                          std::declval<renderer_data>())),
                      single_renderer&>);
        static_assert(std::is_same_v<
                      decltype(std::declval<single_renderer&>().render(
                          std::declval<const std::string&>(),
                          std::declval<renderer_data>())),
                      single_renderer&>);

        static_assert(std::is_same_v<
                      decltype(std::declval<multiple_renderer&>().render(
                          std::declval<renderer_data>())),
                      multiple_renderer&>);
        static_assert(std::is_same_v<
                      decltype(std::declval<multiple_renderer&>().render(
                          std::declval<const std::string&>(),
                          std::declval<renderer_data>())),
                      multiple_renderer&>);

        CHECK_FALSE(single.exists());
        CHECK_FALSE(multiple.exists());

        single = "interface";
        page();

        CHECK(response_body().find("<body>interface</body>")
              != std::string::npos);
    }

    SECTION("A mapped view assigns rendered output to a single-valued placeholder") {
        auto page = layout::create<layout::placeholders::standard>(context);

        page.properties(placeholders::central).view("lua://renderer/mapped");

        auto central = page[placeholders::central];
        renderer_data data{"single", 1};

        central = data;

        CHECK(central.exists());
        CHECK(central.count() == 1);

        page();
        const std::string body = response_body();

        CHECK(body.find("<span class=\"mapped-view\">single:1</span>")
              != std::string::npos);
    }

    SECTION("A mapped view appends rendered output to a multi-valued placeholder") {
        auto page = layout::create<layout::placeholders::standard>(context);

        page.properties(placeholders::left).view("lua://renderer/mapped");

        auto left = page[placeholders::left];
        left.render(renderer_data{"first", 1});
        left += renderer_data{"second", 2};

        CHECK(left.exists());
        CHECK(left.count() == 2);

        page();
        const std::string body = response_body();

        const std::size_t first =
            body.find("<span class=\"mapped-view\">first:1</span>");
        const std::size_t second =
            body.find("<span class=\"mapped-view\">second:2</span>");

        REQUIRE(first != std::string::npos);
        REQUIRE(second != std::string::npos);
        CHECK(first < second);
    }

    SECTION("An explicit view address renders a single value without storing a mapping") {
        auto page = layout::create<layout::placeholders::standard>(context);

        CHECK(page.properties(placeholders::central).view().empty());

        auto central = page[placeholders::central];
        central.render(
            "lua://renderer/explicit", renderer_data{"single-explicit", 3});

        CHECK(page.properties(placeholders::central).view().empty());
        CHECK(central.exists());
        CHECK(central.count() == 1);

        page();
        const std::string body = response_body();

        CHECK(body.find(
                  "<span class=\"explicit-view\">single-explicit:3</span>")
              != std::string::npos);
    }

    SECTION("An explicit view address appends multiple rendered values") {
        auto page = layout::create<layout::placeholders::standard>(context);

        CHECK(page.properties(placeholders::right).view().empty());

        auto right = page[placeholders::right];
        right.render(
                 "lua://renderer/explicit", renderer_data{"first-explicit", 4})
            .render(
                 "lua://renderer/explicit", renderer_data{"second-explicit", 5});

        CHECK(page.properties(placeholders::right).view().empty());
        CHECK(right.exists());
        CHECK(right.count() == 2);

        page();
        const std::string body = response_body();

        const std::size_t first = body.find(
            "<span class=\"explicit-view\">first-explicit:4</span>");
        const std::size_t second = body.find(
            "<span class=\"explicit-view\">second-explicit:5</span>");

        REQUIRE(first != std::string::npos);
        REQUIRE(second != std::string::npos);
        CHECK(first < second);
    }

    SECTION("An explicit view address overrides the configured mapping for that render") {
        auto page = layout::create<layout::placeholders::standard>(context);

        page.properties(placeholders::central).view("lua://renderer/mapped");

        auto central = page[placeholders::central];
        central.render(
            "lua://renderer/explicit", renderer_data{"override", 6});

        CHECK(page.properties(placeholders::central).view()
              == "lua://renderer/mapped");

        page();
        const std::string body = response_body();

        CHECK(body.find("<span class=\"explicit-view\">override:6</span>")
              != std::string::npos);
        CHECK(body.find("<span class=\"mapped-view\">override:6</span>")
              == std::string::npos);
    }

    SECTION("Assets requested by a rendered view are forwarded to the layout") {
        auto page = layout::create<layout::placeholders::standard>(context);

        page.properties(placeholders::central).view("lua://renderer/assets");
        page[placeholders::central] = renderer_data{"assets", 7};

        page();
        const std::string body = response_body();

        CHECK(body.find("<span class=\"asset-view\">assets</span>")
              != std::string::npos);

        CHECK(body.find("src=\"/assets/renderer/external.js\"")
              != std::string::npos);
        CHECK(body.find("href=\"/assets/renderer/external.css\"")
              != std::string::npos);

        CHECK(body.find("window.rendererEmbedded = true;")
              != std::string::npos);
        CHECK(body.find(".renderer-embedded { display: inline; }")
              != std::string::npos);
    }
}
