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


TEST_CASE("View layout asset loader", "[view][asset][layout][loader]") {
    static char buffer_js[]  = "console.log(\"Hello, world!\");";
    static char buffer_js1[] = "console.log(\"Hello, Mars!\");";
    static char buffer_css[] = ".classname{color: blue}";
    static char buffer_img[] = "console.log(\"Hello, world!\");";

    const std::map<std::string, std::tuple<std::string, udho::view::resources::asset::type, std::string>> assets = {
        {"0profile1.js", {"primary", udho::view::resources::asset::type::js, buffer_js}},
        {"1profile2.js", {"primary", udho::view::resources::asset::type::js, buffer_js1}},
        {"2profile.css", {"primary", udho::view::resources::asset::type::css, buffer_css}},
        {"3profile.png", {"primary", udho::view::resources::asset::type::img, buffer_img}}
    };

    udho::view::resources::asset::store store;
    store["primary"] << udho::view::resources::asset::js("profile0.js", buffer_js, buffer_js+std::strlen(buffer_js));
    {
        auto it = assets.cbegin();
        store[std::get<0>(it->second)] << udho::view::resources::asset::js(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        store[std::get<0>(it->second)] << udho::view::resources::asset::js(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        store[std::get<0>(it->second)] << udho::view::resources::asset::css(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
        it++;
        store[std::get<0>(it->second)] << udho::view::resources::asset::img(it->first, std::get<2>(it->second).begin(), std::get<2>(it->second).end());
    }
    store.base("assets");
    store.lock();
    udho::view::resources::asset::const_store const_asset_store{store};
    udho::view::resources::asset::const_substore<udho::view::resources::asset::type::js> const_asset_store_js{const_asset_store};
    udho::view::resources::asset::const_substore<udho::view::resources::asset::type::css> const_asset_store_css{const_asset_store};
    udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::js> loader_js{const_asset_store_js};
    udho::view::tmpl::layout::asset_loader<udho::view::resources::asset::type::css> loader_css{const_asset_store_css};

    loader_js.add("primary", "1profile2.js");
    loader_css.add("primary", "2profile.css");
    {
        bool exception_thrown = false;
        try{
            loader_css.add("primary", "non-existent.css");
        } catch(const std::exception& ex){
            CHECK(std::string{ex.what()} == "Refering to asset :primary/non-existent.css which was nevered registered to the store");
            exception_thrown = true;
        }
        CHECK(exception_thrown);
    }{
        bool exception_thrown = false;
        try{
            loader_css.add("non-existent", "2profile.css");
        } catch(const std::exception& ex){
            CHECK(std::string{ex.what()} == "Refering to asset :non-existent/2profile.css which was nevered registered to the store");
            exception_thrown = true;
        }
        CHECK(exception_thrown);
    }

    boost::asio::io_context io;
    udho::net::types::headers::request request;
    {
        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get());
        loader_js.importmap(stream);
        const std::stringstream& actual_stream = fake_bridge.stream();
        std::string output = actual_stream.str();
        std::string expected_output = R"(<script type="importmap">
{
	"imports": {
		"primary/profile0.js": "/assets/primary/profile0.js",
		"primary/0profile1.js": "/assets/primary/0profile1.js",
		"primary/1profile2.js": "/assets/primary/1profile2.js",
	}
}
</script>
)";
        CHECK(output == expected_output);
    }{
        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get());
        loader_js.write(stream);
        const std::stringstream& actual_stream = fake_bridge.stream();
        std::string output = actual_stream.str();
        std::string expected_output = R"(<script src="/assets/primary/1profile2.js"></script>
)";
        CHECK(output == expected_output);
    }{
        udho::net::fake::bridge fake_bridge{request};
        udho::net::stream stream = udho::net::fake::stream::create(io, fake_bridge.get());
        loader_css.write(stream);
        const std::stringstream& actual_stream = fake_bridge.stream();
        std::string output = actual_stream.str();
        std::string expected_output = R"(<link rel="stylesheet" type="text/css" href="/assets/primary/2profile.css" media="all">
)";
        CHECK(output == expected_output);
    }
}
