#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <string>
#include <type_traits>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>

#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/presenter.h>


namespace {

std::string parse_response_body(const std::string& output){
    CAPTURE(output);

    boost::beast::http::response_parser<boost::beast::http::string_body> parser;
    parser.eager(true);

    boost::beast::error_code error;
    parser.put(boost::asio::buffer(output), error);

    REQUIRE_FALSE(error);
    REQUIRE(parser.is_done());

    boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
    return response.body();
}

template <typename Presenter>
std::string render_presenter_to_body(const Presenter& presenter){
    boost::asio::io_context io;

    boost::beast::test::stream stream_in(io);
    boost::beast::test::stream stream_out(io);

    stream_in.connect(stream_out);

    udho::net::test_ostream stream(
        stream_in,
        [&](boost::system::error_code ec, std::size_t) {
            CHECK_FALSE(ec);
        },
        [&](udho::net::test_ostream& s){
            s.finish();
        }
    );
    stream.prepare();
    udho::net::ostream_view stream_view = stream.view();

    presenter(stream_view);
    stream_view.finish();

    io.run();

    return parse_response_body(stream_out.str());
}

template <typename ResourceStore>
void add_asset(ResourceStore& store, const std::string& prefix, const std::string& name, udho::view::resources::asset::type type, const std::string& content){
    switch(type){
        case udho::view::resources::asset::type::js:
            store[prefix] << udho::view::resources::asset::js(name, content.begin(), content.end());
            break;

        case udho::view::resources::asset::type::css:
            store[prefix] << udho::view::resources::asset::css(name, content.begin(), content.end());
            break;

        case udho::view::resources::asset::type::img:
            store[prefix] << udho::view::resources::asset::img(name, content.begin(), content.end());
            break;

        default:
            break;
    }
}

template <typename ResourceStore>
void add_presenter_assets(ResourceStore& store){
    static const std::string external_js  = "console.log('external presenter js');";
    static const std::string embedded_js  = "console.log('embedded presenter js');";
    static const std::string external_css = ".external_presenter{color: blue}";
    static const std::string embedded_css = ".embedded_presenter{color: red}";

    add_asset(store, "primary", "external.js",  udho::view::resources::asset::type::js,  external_js);
    add_asset(store, "primary", "external.css", udho::view::resources::asset::type::css, external_css);

    {
        auto js = udho::view::resources::asset::js("embedded.js", embedded_js.begin(), embedded_js.end());
        js->embedded(true);
        store["primary"] << std::move(js);
    } {
        auto css = udho::view::resources::asset::css("embedded.css", embedded_css.begin(), embedded_css.end());
        css->media("print");
        store["primary"] << std::move(css);
    }
}

template <typename ConfigureDocument>
std::string render_default_document(ConfigureDocument&& configure_document, bool with_assets = false){
    udho::view::data::bridges::lua lua;
    lua.init();

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};

    if(with_assets){
        add_presenter_assets(resource_store);
    }

    resource_store.assets().base("assets");
    resource_store.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> const_resource_store{resource_store};

    udho::view::tmpl::layout::standard_document document{const_resource_store};

    configure_document(document);

    udho::view::tmpl::layout::default_presenter<udho::view::tmpl::layout::standard_document> presenter{document};

    return render_presenter_to_body(presenter);
}

struct custom_standard_presenter: udho::view::tmpl::layout::default_presenter<udho::view::tmpl::layout::standard_document, custom_standard_presenter> {
    using document_type = udho::view::tmpl::layout::standard_document;
    using base_type     = udho::view::tmpl::layout::default_presenter<document_type, custom_standard_presenter>;

    explicit custom_standard_presenter(const document_type& document): base_type(document) {}

    template <typename Stream>
    void render(Stream& stream) const {
        const std::string content = "Custom presenter body";

        stream << "<section id=\"custom-body\">";
        stream.write(content.c_str(), content.size(), true);
        stream << "</section>";
    }
};

template <typename ConfigureDocument>
std::string render_custom_document(ConfigureDocument&& configure_document, bool with_assets = false){
    udho::view::data::bridges::lua lua;
    lua.init();

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};

    if(with_assets){
        add_presenter_assets(resource_store);
    }

    resource_store.assets().base("assets");
    resource_store.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> const_resource_store{resource_store};

    udho::view::tmpl::layout::standard_document document{const_resource_store};

    configure_document(document);

    custom_standard_presenter presenter{document};

    return render_presenter_to_body(presenter);
}

} // namespace


TEST_CASE("View layout default presenter", "[view][layout][presenter]") {
    using namespace udho::view::tmpl::layout;
    namespace placeholders = udho::view::tmpl::layout::placeholders;

    SECTION("Renders the default HTML envelope around placeholder body") {
        const std::string body = render_default_document([](standard_document& document){
            document.preamble().title("Presenter title");
            document[placeholders::central] = "Presenter body";
        });

        CAPTURE(body);

        CHECK(body.find("<!doctype html><html>") != std::string::npos);
        CHECK(body.find("<head>") != std::string::npos);
        CHECK(body.find("<title>Presenter title</title>") != std::string::npos);
        CHECK(body.find("</head>") != std::string::npos);
        CHECK(body.find("<body>") != std::string::npos);
        CHECK(body.find("Presenter body") != std::string::npos);
        CHECK(body.find("</body></html>") != std::string::npos);

        const auto head_open  = body.find("<head>");
        const auto title      = body.find("<title>Presenter title</title>");
        const auto head_close = body.find("</head>");
        const auto body_open  = body.find("<body>");
        const auto content    = body.find("Presenter body");
        const auto body_close = body.find("</body>");

        REQUIRE(head_open  != std::string::npos);
        REQUIRE(title      != std::string::npos);
        REQUIRE(head_close != std::string::npos);
        REQUIRE(body_open  != std::string::npos);
        REQUIRE(content    != std::string::npos);
        REQUIRE(body_close != std::string::npos);

        CHECK(head_open < title);
        CHECK(title < head_close);
        CHECK(head_close < body_open);
        CHECK(body_open < content);
        CHECK(content < body_close);
    }

    SECTION("Uses preamble fields while opening html and rendering head") {
        const std::string body = render_default_document([](standard_document& document){
            document.preamble()
            .title("Preamble title")
                .doclang("en")
                .xmlns("http://www.w3.org/1999/xhtml")
                .dir("ltr")
                .classes("app shell");

            document.preamble().meta.property("description", "Presenter meta description");
            document.preamble().meta.property("og:title",    "Presenter OpenGraph title");
            document.preamble().meta.property(meta_tags::refresh, "10");

            document[placeholders::central] = "Main";
        });

        CAPTURE(body);

        CHECK(body.find("<!doctype html><html lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\" dir=\"ltr\" class=\"app shell\">") != std::string::npos);
        CHECK(body.find("<title>Preamble title</title>") != std::string::npos);
        CHECK(body.find("<meta http-equiv=\"refresh\" content=\"10\" />") != std::string::npos);
        CHECK(body.find("<meta name=\"description\" content=\"Presenter meta description\" />") != std::string::npos);
        CHECK(body.find("<meta property=\"og:title\" content=\"Presenter OpenGraph title\" />") != std::string::npos);
        CHECK(body.find("<body>Main</body>") != std::string::npos);
    }

    SECTION("Respects disabled doctype") {
        const std::string body = render_default_document([](standard_document& document){
            document.preamble().doctype(false).title("No doctype");

            document[placeholders::central] = "No doctype body";
        });

        CAPTURE(body);

        CHECK(body.find("<!doctype html>") == std::string::npos);
        CHECK(body.rfind("<html>", 0) == 0);
        CHECK(body.find("<title>No doctype</title>") != std::string::npos);
        CHECK(body.find("<body>No doctype body</body></html>") != std::string::npos);
    }

    SECTION("Renders standard placeholders in document placeholder order") {
        const std::string body = render_default_document([](standard_document& document){
            document.preamble().title("Placeholder order");

            document[placeholders::header]  = "H";
            document[placeholders::left]   += "L1";
            document[placeholders::left]   += "L2";
            document[placeholders::central] = "C";
            document[placeholders::right]  += "R1";
            document[placeholders::right]  += "R2";
            document[placeholders::footer]  = "F";
        });

        CAPTURE(body);

        CHECK(body.find("<body>HL1L2CR1R2F</body>") != std::string::npos);
    }

    SECTION("Wraps single and multi placeholders with placeholder properties") {
        const std::string body = render_default_document([](standard_document& document){
            document.preamble().title("Placeholder properties");

            document.properties(placeholders::header).tag("header").classes("header_class").id("header_id");

            document.properties(placeholders::left)
                .tag("aside")
                .classes("left_class")
                .id("left_id");

            document.properties(placeholders::central)
                .tag("main")
                .classes("central_class")
                .id("central_id");

            document.properties(placeholders::right)
                .tag("aside")
                .classes("right_class")
                .id("right_id");

            document.properties(placeholders::footer)
                .tag("footer")
                .classes("footer_class")
                .id("footer_id");

            document[placeholders::header]  = "H";
            document[placeholders::left]   += "L1";
            document[placeholders::left]   += "L2";
            document[placeholders::central] = "C";
            document[placeholders::right]  += "R1";
            document[placeholders::right]  += "R2";
            document[placeholders::footer]  = "F";
        });

        CAPTURE(body);

        const std::string expected =
            "<body>"
            "<header class=\"header_class\" id=\"header_id\">H</header>"
            "<aside class=\"left_class\" id=\"left_id\">L1L2</aside>"
            "<main class=\"central_class\" id=\"central_id\">C</main>"
            "<aside class=\"right_class\" id=\"right_id\">R1R2</aside>"
            "<footer class=\"footer_class\" id=\"footer_id\">F</footer>"
            "</body>";

        CHECK(body.find(expected) != std::string::npos);
    }

    SECTION("Skips missing placeholders even when properties are configured") {
        const std::string body = render_default_document([](standard_document& document){
            document.preamble().title("Missing placeholders");

            document.properties(placeholders::header)
                .tag("header")
                .classes("header_class")
                .id("header_id");

            document.properties(placeholders::left)
                .tag("aside")
                .classes("left_class")
                .id("left_id");

            document.properties(placeholders::central)
                .tag("main")
                .classes("central_class")
                .id("central_id");

            document.properties(placeholders::right)
                .tag("aside")
                .classes("right_class")
                .id("right_id");

            document.properties(placeholders::footer)
                .tag("footer")
                .classes("footer_class")
                .id("footer_id");

            document[placeholders::header]  = "H";
            document[placeholders::central] = "C";
            document[placeholders::footer]  = "F";
        });

        CAPTURE(body);

        const std::string expected =
            "<body>"
            "<header class=\"header_class\" id=\"header_id\">H</header>"
            "<main class=\"central_class\" id=\"central_id\">C</main>"
            "<footer class=\"footer_class\" id=\"footer_id\">F</footer>"
            "</body>";

        CHECK(body.find(expected) != std::string::npos);
        CHECK(body.find("left_id")  == std::string::npos);
        CHECK(body.find("right_id") == std::string::npos);
    }
}


TEST_CASE("View layout default presenter asset placement", "[view][layout][presenter][asset]") {
    using namespace udho::view::tmpl::layout;
    namespace placeholders = udho::view::tmpl::layout::placeholders;

    const std::string body = render_default_document([](standard_document& document){
        document.preamble().title("Presenter assets");

        document.js().add ("primary", "external.js",  false);
        document.js().add ("primary", "embedded.js",  true);
        document.css().add("primary", "external.css", false);
        document.css().add("primary", "embedded.css", true);

        document[placeholders::central] = "Asset body";
    }, true);

    CAPTURE(body);

    const auto head_open  = body.find("<head>");
    const auto head_close = body.find("</head>");
    const auto body_open  = body.find("<body>");
    const auto body_close = body.find("</body>");

    REQUIRE(head_open  != std::string::npos);
    REQUIRE(head_close != std::string::npos);
    REQUIRE(body_open  != std::string::npos);
    REQUIRE(body_close != std::string::npos);

    const auto importmap   = body.find("<script type=\"importmap\">");
    const auto external_js = body.find("<script src=\"/assets/primary/external.js\"></script>");
    const auto external_css = body.find("<link href=\"/assets/primary/external.css\" rel=\"stylesheet\" type=\"text/css\" media=\"all\">");
    const auto embedded_css = body.find("<style media=\"print\">");
    const auto asset_body   = body.find("Asset body");
    const auto embedded_js  = body.find("console.log('embedded presenter js');");

    REQUIRE(importmap    != std::string::npos);
    REQUIRE(external_js  != std::string::npos);
    REQUIRE(external_css != std::string::npos);
    REQUIRE(embedded_css != std::string::npos);
    REQUIRE(asset_body   != std::string::npos);
    REQUIRE(embedded_js  != std::string::npos);

    CHECK(head_open < importmap);
    CHECK(importmap < head_close);

    CHECK(head_open < external_js);
    CHECK(external_js < head_close);

    CHECK(head_open < external_css);
    CHECK(external_css < head_close);

    CHECK(head_open < embedded_css);
    CHECK(embedded_css < head_close);

    CHECK(body_open < asset_body);
    CHECK(asset_body < embedded_js);
    CHECK(embedded_js < body_close);

    CHECK(body.find("\"primary/external.js\": \"/assets/primary/external.js\"") != std::string::npos);
    CHECK(body.find("\"primary/embedded.js\":") == std::string::npos);
}


TEST_CASE("View layout custom presenter", "[view][layout][presenter][custom]") {
    using namespace udho::view::tmpl::layout;
    namespace placeholders = udho::view::tmpl::layout::placeholders;

    static_assert(
        std::is_base_of<
            basic_presenter<standard_document>,
            custom_standard_presenter
            >::value,
        "Custom presenter must derive from basic_presenter<standard_document>"
        );

    SECTION("Custom presenter supplies body content while preserving the common envelope") {
        const std::string body = render_custom_document([](standard_document& document){
            document.preamble().title("Custom presenter");
            document[placeholders::central] = "Default placeholder body should not appear";
        });

        CAPTURE(body);

        CHECK(body.find("<!doctype html><html>") != std::string::npos);
        CHECK(body.find("<title>Custom presenter</title>") != std::string::npos);
        CHECK(body.find("<body><section id=\"custom-body\">Custom presenter body</section></body></html>") != std::string::npos);

        CHECK(body.find("Default placeholder body should not appear") == std::string::npos);
    }

    SECTION("Custom presenter still uses common head and body-close asset handling") {
        const std::string body = render_custom_document([](standard_document& document){
            document.preamble().title("Custom presenter assets");

            document.js().add ("primary", "external.js",  false);
            document.js().add ("primary", "embedded.js",  true);
            document.css().add("primary", "external.css", false);
            document.css().add("primary", "embedded.css", true);

            document[placeholders::central] = "Default placeholder body should not appear";
        }, true);

        CAPTURE(body);

        const auto head_close = body.find("</head>");
        const auto body_open  = body.find("<body>");
        const auto custom     = body.find("<section id=\"custom-body\">Custom presenter body</section>");
        const auto embedded_js = body.find("console.log('embedded presenter js');");
        const auto body_close = body.find("</body>");

        REQUIRE(head_close  != std::string::npos);
        REQUIRE(body_open   != std::string::npos);
        REQUIRE(custom      != std::string::npos);
        REQUIRE(embedded_js != std::string::npos);
        REQUIRE(body_close  != std::string::npos);

        CHECK(body.find("<script src=\"/assets/primary/external.js\"></script>") != std::string::npos);
        CHECK(body.find("<link href=\"/assets/primary/external.css\" rel=\"stylesheet\" type=\"text/css\" media=\"all\">") != std::string::npos);
        CHECK(body.find("<style media=\"print\">") != std::string::npos);

        CHECK(head_close < body_open);
        CHECK(body_open < custom);
        CHECK(custom < embedded_js);
        CHECK(embedded_js < body_close);

        CHECK(body.find("Default placeholder body should not appear") == std::string::npos);
    }
}