#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <sstream>
#include <string>

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>

#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/placeholder.h>

namespace {

// using meta_properties_type = udho::view::tmpl::layout::meta_tags::properties_type;

// meta_properties_type& standard_meta(udho::view::tmpl::layout::meta_tags& tags){ return static_cast<meta_properties_type&>(tags); }

// const meta_properties_type& standard_meta(const udho::view::tmpl::layout::meta_tags& tags){ return static_cast<const meta_properties_type&>(tags); }

std::size_t count_occurrences(const std::string& text, const std::string& needle){
    if(needle.empty()) return 0;

    std::size_t count = 0;
    std::size_t pos   = 0;

    while((pos = text.find(needle, pos)) != std::string::npos){
        ++count;
        pos += needle.size();
    }

    return count;
}

template <typename ResourceStore>
void add_test_asset(ResourceStore& store, const std::string& prefix, const std::string& name, udho::view::resources::asset::type type, const std::string& content){
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

} // namespace


TEST_CASE("View layout meta tags", "[view][layout][document][meta]") {
    using namespace udho::view::tmpl::layout;

    SECTION("Empty meta tags write no output") {
        meta_tags tags;

        std::stringstream stream;
        tags.write(stream);

        CHECK(stream.str().empty());
        CHECK(tags.empty());
    }

    SECTION("http-equiv properties store and write using their mapped names") {
        meta_tags tags;

        CHECK_FALSE(tags.property(meta_tags::content_security_policy).has_value());
        CHECK_FALSE(tags[meta_tags::refresh].has_value());

        tags.property(meta_tags::content_security_policy, "default-src 'self'");
        tags.property(meta_tags::content_type,            "text/html; charset=utf-8");
        tags.property(meta_tags::default_style,           "main");
        tags.property(meta_tags::x_ua_compatible,         "IE=edge");
        tags.property(meta_tags::refresh,                 "30");

        REQUIRE(tags.property(meta_tags::content_security_policy).has_value());
        REQUIRE(tags.property(meta_tags::content_type).has_value());
        REQUIRE(tags.property(meta_tags::default_style).has_value());
        REQUIRE(tags.property(meta_tags::x_ua_compatible).has_value());
        REQUIRE(tags[meta_tags::refresh].has_value());

        CHECK(tags.property(meta_tags::content_security_policy).value() == "default-src 'self'");
        CHECK(tags.property(meta_tags::content_type).value()            == "text/html; charset=utf-8");
        CHECK(tags.property(meta_tags::default_style).value()           == "main");
        CHECK(tags.property(meta_tags::x_ua_compatible).value()         == "IE=edge");
        CHECK(tags[meta_tags::refresh].value()                          == "30");

        std::stringstream stream;
        tags.write(stream);

        CHECK(stream.str() ==
            "<meta http-equiv=\"content-security-policy\" content=\"default-src 'self'\" />\n"
            "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n"
            "<meta http-equiv=\"default-style\" content=\"main\" />\n"
            "<meta http-equiv=\"x-ua-compatible\" content=\"IE=edge\" />\n"
            "<meta http-equiv=\"refresh\" content=\"30\" />\n"
        );
    }

    SECTION("Standard and OpenGraph properties write expected meta tags") {
        meta_tags tags;

        tags.property("charset",     "utf-8");
        tags.property("description", "Layout document test");
        tags.property("keywords",    "layout,document,meta");
        tags.property("og:title",    "OpenGraph title");

        CHECK(tags.count("charset")     == 1);
        CHECK(tags.count("description") == 1);
        CHECK(tags.count("keywords")    == 1);
        CHECK(tags.count("og:title")    == 1);

        std::stringstream stream;
        tags.write(stream);

        const std::string output = stream.str();
        CAPTURE(output);

        CHECK(output ==
            "<meta charset=\"utf-8\" />\n"
            "<meta name=\"charset\" content=\"utf-8\" />\n"
            "<meta name=\"description\" content=\"Layout document test\" />\n"
            "<meta name=\"keywords\" content=\"layout,document,meta\" />\n"
            "<meta property=\"og:title\" content=\"OpenGraph title\" />\n"
        );

        CHECK(count_occurrences(output, "charset") == 2);
    }

    SECTION("Mixed standard, OpenGraph and http-equiv properties write in grouped order") {
        meta_tags tags;

        tags.property(meta_tags::x_ua_compatible, "IE=edge");
        tags.property(meta_tags::refresh,         "10");

        tags.property("description", "Mixed meta test");
        tags.property("og:title",    "Mixed OG title");

        std::stringstream stream;
        tags.write(stream);

        const std::string output = stream.str();
        CAPTURE(output);

        CHECK(output.find("<meta http-equiv=\"x-ua-compatible\" content=\"IE=edge\" />") != std::string::npos);
        CHECK(output.find("<meta http-equiv=\"refresh\" content=\"10\" />") != std::string::npos);
        CHECK(output.find("<meta name=\"description\" content=\"Mixed meta test\" />") != std::string::npos);
        CHECK(output.find("<meta property=\"og:title\" content=\"Mixed OG title\" />") != std::string::npos);

        CHECK(output.find("http-equiv=\"x-ua-compatible\"") < output.find("name=\"description\""));
        CHECK(output.find("http-equiv=\"refresh\"")         < output.find("name=\"description\""));
    }
}


TEST_CASE("View layout document preamble", "[view][layout][document][preamble]") {
    using namespace udho::view::tmpl::layout;

    SECTION("Default preamble state") {
        document_preamble preamble;

        CHECK(preamble.doctype());
        CHECK(preamble.doclang().empty());
        CHECK(preamble.xmlns().empty());
        CHECK(preamble.dir().empty());
        CHECK(preamble.classes().empty());
        CHECK(preamble.title().empty());

        std::stringstream stream;
        preamble.meta.write(stream);

        CHECK(stream.str().empty());
        CHECK(preamble.meta.empty());
    }

    SECTION("Setters store values and support chaining") {
        document_preamble preamble;

        document_preamble& returned = preamble
                                        .doctype(false)
                                        .doclang("en-US")
                                        .xmlns("http://www.w3.org/1999/xhtml")
                                        .dir("ltr")
                                        .classes("app shell")
                                        .title("Document title");

        CHECK(&returned == &preamble);

        CHECK_FALSE(preamble.doctype());
        CHECK(preamble.doclang() == "en-US");
        CHECK(preamble.xmlns()   == "http://www.w3.org/1999/xhtml");
        CHECK(preamble.dir()     == "ltr");
        CHECK(preamble.classes() == "app shell");
        CHECK(preamble.title()   == "Document title");
    }

    SECTION("Preamble exposes meta_tags storage") {
        document_preamble preamble;

        preamble.meta.property(meta_tags::refresh, "5");
        preamble.meta.property("description", "Preamble description");

        REQUIRE(preamble.meta.property(meta_tags::refresh).has_value());
        REQUIRE(preamble.meta.property("description").has_value());

        CHECK(preamble.meta.property(meta_tags::refresh).value() == "5");
        CHECK(preamble.meta.property("description").value() == "Preamble description");

        std::stringstream stream;
        preamble.meta.write(stream);

        const std::string output = stream.str();
        CAPTURE(output);

        CHECK(output.find("<meta http-equiv=\"refresh\" content=\"5\" />") != std::string::npos);
        CHECK(output.find("<meta name=\"description\" content=\"Preamble description\" />") != std::string::npos);
    }
}


TEST_CASE("View layout basic document integration", "[view][layout][document]") {
    using namespace udho::view::tmpl::layout;

    static char buffer_js[]  = "console.log('document js');";
    static char buffer_css[] = ".document{color: blue}";
    static char buffer_img[] = "fake image data";

    udho::view::data::bridges::lua lua;
    lua.init();

    udho::view::resources::store<udho::view::data::bridges::lua> resource_store{lua};

    add_test_asset(resource_store, "primary", "document.js",  udho::view::resources::asset::type::js,  buffer_js);
    add_test_asset(resource_store, "primary", "document.css", udho::view::resources::asset::type::css, buffer_css);
    add_test_asset(resource_store, "primary", "document.png", udho::view::resources::asset::type::img, buffer_img);

    resource_store.assets().base("assets");
    resource_store.lock();

    udho::view::resources::const_store<udho::view::data::bridges::lua> const_resource_store{resource_store};

    standard_document document{const_resource_store};

    namespace placeholders = udho::view::tmpl::layout::placeholders;

    SECTION("Document exposes preamble and default body tag") {
        CHECK(document.preamble().doctype());
        CHECK(document.preamble().title().empty());

        document.preamble()
            .title("Document integration")
            .doclang("en")
            .classes("document-root");

        CHECK(document.preamble().title()   == "Document integration");
        CHECK(document.preamble().doclang() == "en");
        CHECK(document.preamble().classes() == "document-root");

        document.preamble().meta.property("description", "Document integration test");

        REQUIRE(document.preamble().meta.property("description").has_value());
        CHECK(document.preamble().meta.property("description").value() == "Document integration test");

        CHECK(document.body().tag()   == "body");
        CHECK(document.body().open()  == "<body>");
        CHECK(document.body().close() == "</body>");
    }

    SECTION("Document exposes placeholder storage") {
        CHECK_FALSE(document[placeholders::central].exists());
        CHECK_FALSE(document[placeholders::left].exists());
        CHECK_FALSE(document[placeholders::footer].exists());

        document[placeholders::central] = "Central";
        document[placeholders::left] += "Left 1";
        document[placeholders::left] += "Left 2";
        document[placeholders::footer] = "Footer";

        REQUIRE(document[placeholders::central].exists());
        REQUIRE(document[placeholders::left].exists());
        REQUIRE(document[placeholders::footer].exists());

        CHECK(document[placeholders::central].count() == 1);
        CHECK(document[placeholders::left].count()    == 2);
        CHECK(document[placeholders::footer].count()  == 1);

        CHECK(document[placeholders::central].value() == "Central");
        CHECK(document[placeholders::left][0]         == "Left 1");
        CHECK(document[placeholders::left][1]         == "Left 2");
        CHECK(document[placeholders::footer].value()  == "Footer");
    }

    SECTION("Document wires JavaScript and CSS loaders to the resource store") {
        CHECK(document.js().add("primary", "document.js", false));
        CHECK_FALSE(document.js().add("primary", "document.js", false));

        CHECK(document.css().add("primary", "document.css", false));
        CHECK_FALSE(document.css().add("primary", "document.css", false));

        CHECK_THROWS_AS(document.js().add("primary", "missing.js", false), std::out_of_range);
        CHECK_THROWS_AS(document.css().add("primary", "missing.css", false), std::out_of_range);
    }

    SECTION("Const document access exposes const preamble, loaders, body and placeholders") {
        document.preamble().title("Const document");
        document[placeholders::central] = "Const central";

        const standard_document& const_document = document;

        CHECK(const_document.preamble().title() == "Const document");

        CHECK(const_document.body().tag()   == "body");
        CHECK(const_document.body().open()  == "<body>");
        CHECK(const_document.body().close() == "</body>");

        REQUIRE(const_document[placeholders::central].exists());
        CHECK(const_document[placeholders::central].count() == 1);
        CHECK(const_document[placeholders::central].value() == "Const central");

        CHECK_NOTHROW(const_document.js());
        CHECK_NOTHROW(const_document.css());
    }
}