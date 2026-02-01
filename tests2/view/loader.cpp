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
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/parser.hpp>

TEST_CASE("View layout asset loader", "[view][asset][layout][loader]") {
    static char buffer_js[]       = "console.log(\"Hello, world!\");";
    static char buffer_js_async[] = "console.log(\"Async script\");";
    static char buffer_js_module[]= "export function example() {};";
    static char buffer_css[]      = ".classname{color: blue}";
    static char buffer_css_print[]= "@media print { .print { color: black; } }";
    static char buffer_img[]      = "fake image data";

    const std::map<std::string, std::tuple<std::string, udho::view::resources::asset::type, std::string>> assets = {
        {"profile0.js",    {"primary", udho::view::resources::asset::type::js,  buffer_js}},
        {"async_script.js",{"primary", udho::view::resources::asset::type::js,  buffer_js_async}},
        {"module.js",      {"secondary", udho::view::resources::asset::type::js,  buffer_js_module}},
        {"styles.css",     {"primary", udho::view::resources::asset::type::css, buffer_css}},
        {"print.css",      {"primary", udho::view::resources::asset::type::css, buffer_css_print}},
        {"image.png",      {"primary", udho::view::resources::asset::type::img, buffer_img}}
    };

    udho::view::resources::asset::store store;

    auto add_asset = [&](const auto& entry) {
        const auto& [key, value] = entry;
        const auto& [prefix, type, content] = value;

        switch(type) {
            case udho::view::resources::asset::type::js:
                {
                    auto js = udho::view::resources::asset::js(key, content.begin(), content.end());
                    if(key.find("async") != std::string::npos)
                        js->is_async(true);
                    if(key.find("module") != std::string::npos)
                        js->is_module(true);
                    store[prefix] << std::move(js);
                }
                break;
            case udho::view::resources::asset::type::css:
                {
                    auto css = udho::view::resources::asset::css(key, content.begin(), content.end());
                    if(key.find("print") != std::string::npos)
                        css->media("print");
                    store[prefix] << std::move(css);
                }
                break;
            case udho::view::resources::asset::type::img:
                store[prefix] << udho::view::resources::asset::img(key, content.begin(), content.end());
                break;
            default:
                break;
            }

    };

    for(const auto& asset : assets) {
        add_asset(asset);
    }

    auto embedded_only_res = udho::view::resources::asset::js("embedded_only.js", std::begin(buffer_js_module), std::end(buffer_js_module));
    embedded_only_res->embedded(true);
    store["secondary"] << std::move(embedded_only_res);

    store.base("assets");
    store.lock();

    udho::view::resources::asset::const_store const_asset_store{store};

    udho::view::resources::asset::const_substore<udho::view::resources::asset::type::js> const_asset_store_js{const_asset_store};
    udho::view::resources::asset::const_substore<udho::view::resources::asset::type::css> const_asset_store_css{const_asset_store};

    udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js> loader_js{const_asset_store_js};
    udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css> loader_css{const_asset_store_css};

    loader_js.add("primary",    "profile0.js",      false);   // Non-embedded
    loader_js.add("primary",    "async_script.js",  false);   // Non-embedded with async
    loader_js.add("secondary",  "module.js",        true);    // Embedded module
    loader_css.add("primary",   "styles.css",       false);   // Linked CSS
    loader_css.add("primary",   "print.css",        true);    // Embedded CSS

    SECTION("Accessing non existent resources throw exception") {
        {
            bool exception_thrown = false;
            try{
                loader_css.add("primary", "non-existent.css");
            } catch(const std::exception& ex){
                CHECK(std::string{ex.what()} == "Refering to asset :primary/non-existent.css which was never registered to the store");
                exception_thrown = true;
            }
            CHECK(exception_thrown);
        }{
            bool exception_thrown = false;
            try{
                loader_css.add("non-existent", "2profile.css");
            } catch(const std::exception& ex){
                CHECK(std::string{ex.what()} == "Refering to asset :non-existent/2profile.css which was never registered to the store");
                exception_thrown = true;
            }
            CHECK(exception_thrown);
        }
    }

    SECTION("Importmap includes all registered javascripts except the embedded one regardless of selection") {
        boost::asio::io_context io;
        boost::beast::test::stream stream_in(io);
        boost::beast::test::stream stream_out(io);
        stream_in.connect(stream_out);

        udho::net::test_ostream stream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                CHECK_FALSE(ec);
            }
        );
        udho::net::ostream_view stream_view = stream.view();
        loader_js.importmap(stream_view);

        stream.finish();
        io.run();

        std::string output = stream_out.str();
        CAPTURE(output);
        std::size_t crlf_pos = output.find("\r\n");
        CAPTURE(crlf_pos);
        boost::beast::http::response_parser<boost::beast::http::string_body> parser;
        parser.eager(true);
        boost::beast::error_code error;
        parser.put(boost::asio::buffer(output), error);

        CHECK(!error);
        CHECK(parser.is_done());

        boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
        std::string body = response.body();

        std::string expected_output = R"(<script type="importmap">
        {
            "imports": {
                "primary/profile0.js": "/assets/primary/profile0.js",
                "primary/async_script.js": "/assets/primary/async_script.js",
                "secondary/module.js": "/assets/secondary/module.js"
            }
        }
        </script>
        )";

        auto extract_json = [](const std::string& str) {
            size_t start = str.find('{');
            size_t end = str.rfind('}');
            if (start == std::string::npos || end == std::string::npos) return nlohmann::json();
            return nlohmann::json::parse(str.substr(start, end - start + 1));
        };
        try {
            CAPTURE(body);
            nlohmann::json output_json = extract_json(body);
            nlohmann::json expected_json = extract_json(expected_output);
            CHECK(output_json == expected_json);
        } catch(const nlohmann::json::exception& e) {
            CHECK(false);
        }
    }

    SECTION("Embedded-only JS doesn't appear in importmap") {
        // Add the embedded-only asset to the loader
        loader_js.add("secondary", "embedded_only.js", true);

        boost::asio::io_context io;
        boost::beast::test::stream stream_in(io);
        boost::beast::test::stream stream_out(io);
        stream_in.connect(stream_out);

        udho::net::test_ostream stream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                CHECK_FALSE(ec);
            }
        );
        udho::net::ostream_view stream_view = stream.view();

        // Should still exclude from importmap
        loader_js.importmap(stream_view);

        stream.finish();
        io.run();

        std::string output = stream_out.str();
        CAPTURE(output);
        std::size_t crlf_pos = output.find("\r\n");
        CAPTURE(crlf_pos);
        boost::beast::http::response_parser<boost::beast::http::string_body> parser;
        parser.eager(true);
        boost::beast::error_code error;
        parser.put(boost::asio::buffer(output), error);

        CHECK(!error);
        CHECK(parser.is_done());

        boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
        std::string body = response.body();
        CHECK(body.find("embedded_only.js") == std::string::npos);
        CHECK(body.find("module.js") != std::string::npos); // Verify non-embedded still appears
    }

    SECTION("Embedded-only JS writes correctly") {
        loader_js.add("secondary", "embedded_only.js", true);

        boost::asio::io_context io;
        boost::beast::test::stream stream_in(io);
        boost::beast::test::stream stream_out(io);
        stream_in.connect(stream_out);

        udho::net::test_ostream stream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                CHECK_FALSE(ec);
            }
        );
        udho::net::ostream_view stream_view = stream.view();

        loader_js.write(stream_view, true);

        stream.finish();
        io.run();

        std::string output = stream_out.str();
        CAPTURE(output);
        std::size_t crlf_pos = output.find("\r\n");
        CAPTURE(crlf_pos);
        boost::beast::http::response_parser<boost::beast::http::string_body> parser;
        parser.eager(true);
        boost::beast::error_code error;
        parser.put(boost::asio::buffer(output), error);

        CHECK(!error);
        CHECK(parser.is_done());

        boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
        std::string body = response.body();

        CHECK(body.find("<script type=\"text/javascript\">") != std::string::npos);
        CHECK(body.find(buffer_js_module) != std::string::npos);
    }

    SECTION("JavaScript asset writing handles embedded and external correctly") {
        boost::asio::io_context io;
        boost::beast::test::stream stream_in(io);
        boost::beast::test::stream stream_out(io);
        stream_in.connect(stream_out);

        udho::net::test_ostream stream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                CHECK_FALSE(ec);
            }
        );
        udho::net::ostream_view stream_view = stream.view();

        SECTION("Non-embedded scripts generate correct link tags") {
            loader_js.write(stream_view, false);

            stream.finish();
            io.run();

            std::string output = stream_out.str();
            CAPTURE(output);
            std::size_t crlf_pos = output.find("\r\n");
            CAPTURE(crlf_pos);
            boost::beast::http::response_parser<boost::beast::http::string_body> parser;
            parser.eager(true);
            boost::beast::error_code error;
            parser.put(boost::asio::buffer(output), error);

            CHECK(!error);
            CHECK(parser.is_done());

            boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
            std::string body = response.body();

            CHECK(body.find("<script src=\"/assets/primary/profile0.js\"></script>") != std::string::npos);
            CHECK(body.find("<script async src=\"/assets/primary/async_script.js\"></script>") != std::string::npos);
            CHECK(body.find("secondary/module.js") == std::string::npos);  // This one is embedded
        }

        SECTION("Embedded scripts include content with proper formatting") {
            loader_js.write(stream_view, true);

            stream.finish();
            io.run();

            std::string output = stream_out.str();
            CAPTURE(output);
            std::size_t crlf_pos = output.find("\r\n");
            CAPTURE(crlf_pos);
            boost::beast::http::response_parser<boost::beast::http::string_body> parser;
            parser.eager(true);
            boost::beast::error_code error;
            parser.put(boost::asio::buffer(output), error);

            CHECK(!error);
            CHECK(parser.is_done());

            boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
            std::string body = response.body();

            // Should contain the module content
            CHECK(body.find("<script type=\"module\">\n") != std::string::npos);
            CHECK(body.find("export function example() {};") != std::string::npos);
            CHECK(body.find("</script>") != std::string::npos);

            // Should not contain non-embedded assets
            CHECK(body.find("profile0.js") == std::string::npos);
        }
    }

    SECTION("CSS asset writing handles media queries and embedding") {
        boost::asio::io_context io;
        boost::beast::test::stream stream_in(io);
        boost::beast::test::stream stream_out(io);
        stream_in.connect(stream_out);

        udho::net::test_ostream stream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                CHECK_FALSE(ec);
            }
        );
        udho::net::ostream_view stream_view = stream.view();

        SECTION("Linked CSS generates proper link tags") {
            loader_css.write(stream_view, false);

            stream.finish();
            io.run();

            std::string output = stream_out.str();
            CAPTURE(output);
            std::size_t crlf_pos = output.find("\r\n");
            CAPTURE(crlf_pos);
            boost::beast::http::response_parser<boost::beast::http::string_body> parser;
            parser.eager(true);
            boost::beast::error_code error;
            parser.put(boost::asio::buffer(output), error);

            CHECK(!error);
            CHECK(parser.is_done());

            boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
            std::string body = response.body();

            CHECK(body.find("<link href=\"/assets/primary/styles.css\" rel=\"stylesheet\"") != std::string::npos);
            CHECK(body.find("media=\"all\"") != std::string::npos);
        }

        SECTION("Embedded CSS includes content with media queries") {
            loader_css.write(stream_view, true);

            stream.finish();
            io.run();

            std::string output = stream_out.str();
            CAPTURE(output);
            std::size_t crlf_pos = output.find("\r\n");
            CAPTURE(crlf_pos);
            boost::beast::http::response_parser<boost::beast::http::string_body> parser;
            parser.eager(true);
            boost::beast::error_code error;
            parser.put(boost::asio::buffer(output), error);

            CHECK(!error);
            CHECK(parser.is_done());

            boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
            std::string body = response.body();

            CHECK(body.find("<style media=\"print\">") != std::string::npos);
            CHECK(body.find("@media print { .print { color: black; } }") != std::string::npos);
            CHECK(body.find("</style>") != std::string::npos);
        }
    }

    SECTION("Asset policies are properly reflected in output") {
        boost::asio::io_context io;
        boost::beast::test::stream stream_in(io);
        boost::beast::test::stream stream_out(io);
        stream_in.connect(stream_out);

        udho::net::test_ostream stream(stream_in,
           [&](boost::system::error_code ec, std::size_t) {
               CHECK_FALSE(ec);
           }
        );

        udho::net::ostream_view stream_view = stream.view();

        SECTION("JS async/defer attributes") {
            loader_js.write(stream_view, false);

            stream.finish();
            io.run();

            std::string output = stream_out.str();
            CAPTURE(output);
            std::size_t crlf_pos = output.find("\r\n");
            CAPTURE(crlf_pos);
            boost::beast::http::response_parser<boost::beast::http::string_body> parser;
            parser.eager(true);
            boost::beast::error_code error;
            parser.put(boost::asio::buffer(output), error);

            CHECK(!error);
            CHECK(parser.is_done());

            boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
            std::string body = response.body();

            // async_script.js should have async attribute
            CHECK(body.find("async src=\"/assets/primary/async_script.js\"") != std::string::npos);

            // profile0.js should have no special attributes
            CHECK(body.find("profile0.js\"") != std::string::npos);
            CHECK(body.find("async src=\"/assets/primary/profile0.js\"") == std::string::npos);
        }

        SECTION("Module vs nomodule handling") {
            // Add a nomodule asset
            loader_js.add("primary", "profile0.js", false);

            loader_js.write(stream_view, false);

            stream.finish();
            io.run();

            std::string output = stream_out.str();
            CAPTURE(output);
            std::size_t crlf_pos = output.find("\r\n");
            CAPTURE(crlf_pos);
            boost::beast::http::response_parser<boost::beast::http::string_body> parser;
            parser.eager(true);
            boost::beast::error_code error;
            parser.put(boost::asio::buffer(output), error);

            CHECK(!error);
            CHECK(parser.is_done());

            boost::beast::http::response<boost::beast::http::string_body> response = parser.release();
            std::string body = response.body();

            CHECK(body.find("nomodule") == std::string::npos);
            CHECK(body.find("module.js") == std::string::npos);
        }
    }

}
