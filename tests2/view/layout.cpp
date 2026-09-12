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

#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/parser.hpp>


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

    auto router = udho::url::router(
          udho::url::root(
                udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home  (udho::url::verb::get)
              | udho::url::slot("chunked"_h,  &callbacks::chunk) << udho::url::fixed (udho::url::verb::get, "/chunk")
          )
        | udho::url::mount("b"_h, "/b",
              udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")
            | udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")
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
    stream.prepare();
    auto context = udho::manifold::basic_context(stream, portal, 0);

    // udho::net::ostream_view stream_view = stream.view();


    // udho::net::ostream_view stream_view = stream.view();

    namespace placeholders = udho::view::tmpl::layout::placeholders;

    auto response_body = [&](){
        io.run();

        std::string output = stream_out.str();
        CAPTURE(output);

        boost::beast::http::response_parser<boost::beast::http::string_body> parser;
        parser.eager(true);

        boost::beast::error_code error;
        parser.put(boost::asio::buffer(output), error);

        CHECK(!error);
        CHECK(parser.is_done());

        boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
        return response.body();
    };

    SECTION("Layout renders content assigned through a single-valued renderer") {
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        auto central = layout[placeholders::central];

        CHECK_FALSE(central.exists());
        CHECK(central.count() == 0);

        layout.preamble().title("Page title");
        central = "Hello";

        CHECK(central.exists());
        CHECK(central.count() == 1);

        layout();

        std::string body = response_body();

        CHECK(body.find("<title>Page title</title>") != std::string::npos);
        CHECK(body.find("<body>Hello</body>") != std::string::npos);
    }

    SECTION("Layout appends content through multi-valued renderers") {
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        auto left  = layout[placeholders::left];
        auto right = layout[placeholders::right];

        CHECK_FALSE(left.exists());
        CHECK(left.count() == 0);
        CHECK_FALSE(right.exists());
        CHECK(right.count() == 0);

        left += "L1";
        left += "L2";

        layout[placeholders::central] = "C";

        right += "R1";
        right += "R2";

        CHECK(left.exists());
        CHECK(left.count() == 2);
        CHECK(right.exists());
        CHECK(right.count() == 2);

        layout();

        std::string body = response_body();

        CHECK(body.find("<body>L1L2CR1R2</body>") != std::string::npos);
    }

    SECTION("Layout forwards placeholder properties to the document and presenter") {
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        layout.properties(placeholders::header).classes("header_class").id("header_id");
        layout.properties(placeholders::left).classes("left_class").id("left_id");
        layout.properties(placeholders::central).classes("central_class").id("central_id");
        layout.properties(placeholders::right).classes("right_class").id("right_id");
        layout.properties(placeholders::footer).classes("footer_class").id("footer_id");

        layout[placeholders::header] = "H";
        layout[placeholders::left] += "L1";
        layout[placeholders::left] += "L2";
        layout[placeholders::central] = "C";
        layout[placeholders::right] += "R1";
        layout[placeholders::right] += "R2";
        layout[placeholders::footer] = "F";

        layout();

        std::string body = response_body();

        CAPTURE(body);

        CHECK(body.find(
            "<body>"
                "<div class=\"header_class\" id=\"header_id\">H</div>"
                "<div class=\"left_class\" id=\"left_id\">L1L2</div>"
                "<div class=\"central_class\" id=\"central_id\">C</div>"
                "<div class=\"right_class\" id=\"right_id\">R1R2</div>"
                "<div class=\"footer_class\" id=\"footer_id\">F</div>"
            "</body>"
        ) != std::string::npos);
    }

    SECTION("Layout forwards preamble configuration") {
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        layout.preamble()
            .title("Page title")
            .doclang("en")
            .xmlns("http://www.w3.org/1999/xhtml")
            .dir("ltr")
            .classes("app shell");

        layout.preamble().meta.property("description", "Layout integration test");

        layout[placeholders::central] = "Hello";

        layout();

        std::string body = response_body();

        CHECK(body.find(
            "<!doctype html>"
            "<html lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\" dir=\"ltr\" class=\"app shell\">"
        ) != std::string::npos);

        CHECK(body.find("<title>Page title</title>") != std::string::npos);
        CHECK(body.find("<meta name=\"description\" content=\"Layout integration test\" />") != std::string::npos);
        CHECK(body.find("<body>Hello</body>") != std::string::npos);
    }

    SECTION("Layout forwards asset selections to its document loaders") {
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        CHECK(layout.js().add("primary", "0profile1.js", false));
        CHECK(layout.js().add("primary", "1profile2.js", true));
        CHECK(layout.css().add("primary", "2profile.css", false));

        layout[placeholders::central] = "Hello";

        layout();

        std::string body = response_body();

        CHECK(body.find("<script type=\"importmap\">") != std::string::npos);
        CHECK(body.find("\"primary/0profile1.js\": \"/assets/primary/0profile1.js\"") != std::string::npos);
        CHECK(body.find("\"primary/1profile2.js\": \"/assets/primary/1profile2.js\"") != std::string::npos);

        CHECK(body.find("<script src=\"/assets/primary/0profile1.js\"></script>") != std::string::npos);
        CHECK(body.find("href=\"/assets/primary/2profile.css\"") != std::string::npos);

        CHECK(body.find("<body>Hello") != std::string::npos);
        CHECK(body.find("console.log('Hello, Mars!');") != std::string::npos);
        CHECK(body.find("</script>\n</body>") != std::string::npos);
    }

    SECTION("Calling layout more than once does not present the document more than once") {
        auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

        layout.preamble().title("Page title");
        layout[placeholders::central] = "Hello";

        layout();
        layout();

        std::string body = response_body();

        CHECK(body.find("<title>Page title</title>") != std::string::npos);
        CHECK(body.find("<body>Hello</body>") != std::string::npos);

        std::size_t first = body.find("Hello");
        REQUIRE(first != std::string::npos);

        CHECK(body.find("Hello", first + 1) == std::string::npos);
    }



    io.run();

    std::string output = stream_out.str();
    std::cout << output << std::endl;
}
