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
#include <udho/session/abstract_catalogue.h>
#include <udho/session/storage/fs.h>
#include <udho/session/catalogue.h>

using session_catalogue = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;

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
                    store[prefix] << js;
                }
                break;
            case udho::view::resources::asset::type::css:
                {
                    auto css = udho::view::resources::asset::css(key, content.begin(), content.end());
                    if(key.find("print") != std::string::npos)
                        css->media("print");
                    store[prefix] << css;
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
    store["secondary"] << udho::view::resources::asset::js("embedded_only.js", std::begin(buffer_js_module), std::end(buffer_js_module))->embedded(true);

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


    boost::asio::io_context io;
    udho::net::types::headers::request request;

    auto sessions = session_catalogue::create(udho::session::storage::fs{});

    SECTION("Importmap includes all registered javascripts except the embedded one regardless of selection") {
        udho::net::fake::bridge fake_bridge{request};

        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get(), *sessions);
        loader_js.importmap(stream);
        const std::stringstream& actual_stream = fake_bridge.stream();
        std::string output = actual_stream.str();
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
            nlohmann::json output_json = extract_json(output);
            nlohmann::json expected_json = extract_json(expected_output);
            CHECK(output_json == expected_json);
        } catch(const nlohmann::json::exception& e) {
            CHECK(false);
        }
    }

    SECTION("Embedded-only JS doesn't appear in importmap") {
        // Add the embedded-only asset to the loader
        loader_js.add("secondary", "embedded_only.js", true);

        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get(), *sessions);

        // Should still exclude from importmap
        loader_js.importmap(stream);
        const std::string output = fake_bridge.stream().str();

        CHECK(output.find("embedded_only.js") == std::string::npos);
        CHECK(output.find("module.js") != std::string::npos); // Verify non-embedded still appears
    }

    SECTION("Embedded-only JS writes correctly") {
        loader_js.add("secondary", "embedded_only.js", true);

        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get(), *sessions);

        loader_js.write(stream, true);
        const std::string output = fake_bridge.stream().str();

        CHECK(output.find("<script type=\"text/javascript\">") != std::string::npos);
        CHECK(output.find(buffer_js_module) != std::string::npos);
    }

    SECTION("JavaScript asset writing handles embedded and external correctly") {
        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get(), *sessions);

        SECTION("Non-embedded scripts generate correct link tags") {
            loader_js.write(stream, false);
            const std::string output = fake_bridge.stream().str();

            CHECK(output.find("<script src=\"/assets/primary/profile0.js\"></script>") != std::string::npos);
            CHECK(output.find("<script async src=\"/assets/primary/async_script.js\"></script>") != std::string::npos);
            CHECK(output.find("secondary/module.js") == std::string::npos);  // This one is embedded
        }

        SECTION("Embedded scripts include content with proper formatting") {
            loader_js.write(stream, true);
            const std::string output = fake_bridge.stream().str();

            // Should contain the module content
            CHECK(output.find("<script type=\"module\">\n") != std::string::npos);
            CHECK(output.find("export function example() {};") != std::string::npos);
            CHECK(output.find("</script>") != std::string::npos);

            // Should not contain non-embedded assets
            CHECK(output.find("profile0.js") == std::string::npos);
        }
    }

    SECTION("CSS asset writing handles media queries and embedding") {
        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get(), *sessions);

        SECTION("Linked CSS generates proper link tags") {
            loader_css.write(stream, false);
            const std::string output = fake_bridge.stream().str();

            CHECK(output.find("<link href=\"/assets/primary/styles.css\" rel=\"stylesheet\"") != std::string::npos);
            CHECK(output.find("media=\"all\"") != std::string::npos);
        }

        SECTION("Embedded CSS includes content with media queries") {
            loader_css.write(stream, true);
            const std::string output = fake_bridge.stream().str();

            CHECK(output.find("<style media=\"print\">") != std::string::npos);
            CHECK(output.find("@media print { .print { color: black; } }") != std::string::npos);
            CHECK(output.find("</style>") != std::string::npos);
        }
    }

    SECTION("Asset policies are properly reflected in output") {
        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get(), *sessions);

        SECTION("JS async/defer attributes") {
            loader_js.write(stream, false);
            const std::string output = fake_bridge.stream().str();

            // async_script.js should have async attribute
            CHECK(output.find("async src=\"/assets/primary/async_script.js\"") != std::string::npos);

            // profile0.js should have no special attributes
            CHECK(output.find("profile0.js\"") != std::string::npos);
            CHECK(output.find("async src=\"/assets/primary/profile0.js\"") == std::string::npos);
        }

        SECTION("Module vs nomodule handling") {
            // Add a nomodule asset
            loader_js.add("primary", "profile0.js", false);

            loader_js.write(stream, false);
            const std::string output = fake_bridge.stream().str();

            CHECK(output.find("nomodule") == std::string::npos);
            CHECK(output.find("module.js") == std::string::npos);
        }
    }

}
