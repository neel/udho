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
#include <udho/view/resources/lua.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>
#include <udho/net/context.h>
#include <udho/url/router.h>

#include "data.h"

TEST_CASE("Resource storage and retrieval of on memory resources", "[view][resource][memory]") {

    static char buffer[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
Hello World
    )TEMPLATE";

    static char buffer_js[]  = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static char buffer_img[] = "console.log('Hello, world!');";

    const std::vector<std::pair<std::string, std::pair<std::string, const char*>>> views = {
        {"view1", {"primary", buffer}},
        {"view2", {"primary", buffer}},
        {"view3", {"primary", buffer}}
    };

    const std::map<std::string, std::tuple<std::string, udho::view::resources::asset::type, std::string>> assets = {
        {"0profile1.js", {"primary", udho::view::resources::asset::type::js, buffer_js}},
        {"1profile2.js", {"primary", udho::view::resources::asset::type::js, buffer_js1}},
        {"2profile.css", {"primary", udho::view::resources::asset::type::css, buffer_css}},
        {"3profile.png", {"primary", udho::view::resources::asset::type::img, buffer_img}}
    };

    udho::view::data::bridges::lua lua;
    lua.init();

    SECTION("Lua template compilation") {
        bool res = lua.compile(udho::view::resources::tmpl::resource("profile", buffer, buffer+sizeof(buffer)), "user");
        REQUIRE(res == true);
    }

    student p;

    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};

    // Lua resources can be on memory strings, on disk files
    // TEST check with all asset sources
    // for checking on disk resources create temporary files and then provide its path as boost::filesystem::path
    store["primary"] << udho::view::resources::lua{"view1", buffer, buffer+std::strlen(buffer)}
                     << udho::view::resources::lua{"view2", buffer, buffer+std::strlen(buffer)}
                     << udho::view::resources::lua{"view3", buffer, buffer+std::strlen(buffer)};

    CHECK(3 == store.tmpl<udho::view::data::bridges::lua>().size()); // views directly added to the bridge doesn't make into the store

    // Asset resources can be on memory strings, on disk files, or remote (urls of remote sources)
    // TEST check with all asset sources
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

    CHECK(assets.size()+1 == store.assets().size());

    store.assets().base("assets");

    // TEST Creating a const_store from store should throw exception unless the store is locked.
    REQUIRE_THROWS_AS(udho::view::resources::const_store<udho::view::data::bridges::lua>{store}, std::exception);

    store.lock();
    // TEST Adding resources to a locked store should also throw exception
    {
        bool exception_fired = false;
        auto js_resource = udho::view::resources::asset::js("profile1.js", buffer_js, buffer_js+std::strlen(buffer_js));
        try{
            store["primary"] << std::move(js_resource);
        } catch(const std::exception& ex) {
            exception_fired = true;
        }
        CHECK(exception_fired);
    }

    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};

    // TEST The number of views, name and prefix of each view should be same as it was added in the store.
    udho::view::resources::tmpl::const_substore<udho::view::data::bridges::lua> lua_substore = cstore.tmpl<udho::view::data::bridges::lua>();
    // TEST check view count again
    {
        std::size_t i = 0;
        for(const udho::view::resources::tmpl::tmpl_view_registration_info& view_proxy: lua_substore){
            // std::cout << view_proxy.name() << " " << view_proxy.prefix() << std::endl;
            // TEST Check view name and prefix only, view functionality will be checked in other tests
            CHECK(view_proxy.name()   == views[i].first);
            CHECK(view_proxy.prefix() == views[i].second.first);

            ++i;
        }
    }

    // TEST The number of assets, name, type, prefix, url, mime type of each view should be same as it was added in the store.

    const udho::view::resources::asset::const_store& asset_substore = cstore.assets();
    {
        auto i = assets.begin();
        for(auto j = asset_substore.begin(); j != asset_substore.end(); ++j){
            const udho::view::resources::asset::proxy& asset = *j;
            if(asset.name() == "profile0.js"){
                // skip this entry.
                continue;
            }
            CHECK(asset.name()   == (*i).first);
            const auto& asset_desc_input = (*i).second;
            CHECK(asset.prefix() == std::get<0>(asset_desc_input));
            CHECK(asset.type()   == std::get<1>(asset_desc_input));
            if(std::get<1>(asset_desc_input) == udho::view::resources::asset::type::js){
                CHECK(asset.mime()   == "application/javascript");
            }
            CHECK(asset.url()    == udho::url::format("/assets/{}/{}",std::get<0>(asset_desc_input), (*i).first));

            // TEST asset types are enum class type{ js, css, txt, img };

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
            asset.write(stream_view);

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

            CAPTURE(response[boost::beast::http::field::content_length]);

            // TEST check mime type in response
            // TEST check content size
            CHECK(response[boost::beast::http::field::content_length] == std::to_string(std::get<2>((*i).second).size()));
            if(std::get<1>((*i).second) == udho::view::resources::asset::type::js){
                CHECK(response[boost::beast::http::field::content_type] == "application/javascript");
            }

            // TEST check output

            CHECK(body == std::get<2>((*i).second));

            ++i;
        }
    }
}

TEST_CASE("Resource storage and retrieval of on disk resources", "[view][resource][disk]") {

    static char buffer[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
Hello World
    )TEMPLATE";

    static char buffer_js[]  = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static char buffer_img[] = "console.log('Hello, world!');";

    const std::vector<std::pair<std::string, std::pair<std::string, const char*>>> views = {
        {"view1", {"primary", buffer}},
        {"view2", {"primary", buffer}},
        {"view3", {"primary", buffer}}
    };

    const std::map<std::string, std::tuple<std::string, udho::view::resources::asset::type, std::string>> assets = {
        {"profile1.js", {"primary", udho::view::resources::asset::type::js, buffer_js}},
        {"profile2.js", {"primary", udho::view::resources::asset::type::js, buffer_js1}},
        {"profile.css", {"primary", udho::view::resources::asset::type::css, buffer_css}},
        {"profile.png", {"primary", udho::view::resources::asset::type::img, buffer_img}}
    };

    for (const auto& [name, data] : assets) {
        boost::filesystem::path temp = boost::filesystem::unique_path();
        std::ofstream temp_stream(temp.c_str());
        temp_stream << std::get<2>(data);
        temp_stream.close();
    }

    udho::view::data::bridges::lua lua;
    lua.init();

    SECTION("Lua template compilation") {
        bool res = lua.compile(udho::view::resources::tmpl::resource("profile", buffer, buffer+sizeof(buffer)), "user");
        REQUIRE(res == true);
    }

    student p;

    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};

    // Lua resources can be on memory strings, on disk files
    // TEST check with all asset sources
    // for checking on disk resources create temporary files and then provide its path as boost::filesystem::path

    for (const auto& pair : views) {
        boost::filesystem::path temp = boost::filesystem::unique_path();
        std::ofstream temp_stream(temp.c_str());
        temp_stream << pair.second.second;
        temp_stream.close();

        store[pair.second.first] << udho::view::resources::lua{pair.first, temp};
    }

    CHECK(3 == store.tmpl<udho::view::data::bridges::lua>().size()); // views directly added to the bridge doesn't make into the store

    // Asset resources can be on memory strings, on disk files, or remote (urls of remote sources)
    // TEST check with all asset sources
    store["primary"] << udho::view::resources::asset::js("profile0.js", buffer_js, buffer_js+std::strlen(buffer_js));
    for (const auto& [name, data] : assets) {
        boost::filesystem::path temp = boost::filesystem::unique_path();
        std::ofstream temp_stream(temp.c_str());
        temp_stream << std::get<2>(data);
        temp_stream.close();

        switch(std::get<1>(data)){
            case udho::view::resources::asset::type::js:
                store[std::get<0>(data)] << udho::view::resources::asset::js(name, temp);
                break;
            case udho::view::resources::asset::type::css:
                store[std::get<0>(data)] << udho::view::resources::asset::css(name, temp);
                break;
            case udho::view::resources::asset::type::img:
                store[std::get<0>(data)] << udho::view::resources::asset::img(name, temp);
                break;
        }

    }

    CHECK(assets.size()+1 == store.assets().size());

    store.assets().base("assets");

    // TEST Creating a const_store from store should throw exception unless the store is locked.
    REQUIRE_THROWS_AS(udho::view::resources::const_store<udho::view::data::bridges::lua>{store}, std::exception);

    store.lock();
    // TEST Adding resources to a locked store should also throw exception
    REQUIRE_THROWS_AS(store["primary"] << udho::view::resources::asset::js("profile1.js", buffer_js, buffer_js+std::strlen(buffer_js)), std::exception);

    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};

    // TEST The number of views, name and prefix of each view should be same as it was added in the store.
    udho::view::resources::tmpl::const_substore<udho::view::data::bridges::lua> lua_substore = cstore.tmpl<udho::view::data::bridges::lua>();
    // TEST check view count again
    {
        std::size_t i = 0;
        for(const udho::view::resources::tmpl::tmpl_view_registration_info& view_proxy: lua_substore){
            // std::cout << view_proxy.name() << " " << view_proxy.prefix() << std::endl;
            // TEST Check view name and prefix only, view functionality will be checked in other tests
            CHECK(view_proxy.name()   == views[i].first);
            CHECK(view_proxy.prefix() == views[i].second.first);

            ++i;
        }
    }

    // TEST The number of assets, name, type, prefix, url, mime type of each view should be same as it was added in the store.

    const udho::view::resources::asset::const_store& asset_substore = cstore.assets();
    std::cout << asset_substore << std::endl;
    {
        auto i = assets.begin();
        for(auto j = asset_substore.begin(); j != asset_substore.end(); ++j){
            const udho::view::resources::asset::proxy& asset = *j;
            if(asset.name() == "profile0.js"){
                // skip this entry.
                continue;
            }
            CHECK(asset.name()   == (*i).first);
            CHECK(asset.prefix() == std::get<0>((*i).second));
            CHECK(asset.type()   == std::get<1>((*i).second));
            if(std::get<1>((*i).second) == udho::view::resources::asset::type::js){
                CHECK(asset.mime()   == "application/javascript");
            }
            CHECK(asset.url()    == udho::url::format("/assets/{}/{}",std::get<0>((*i).second), (*i).first));

            // TEST asset types are enum class type{ js, css, txt, img };


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
            asset.write(stream_view);

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

            CAPTURE(response[boost::beast::http::field::content_length]);

            // TEST check mime type in response
            // TEST check content size
            CHECK(response[boost::beast::http::field::content_length] == std::to_string(std::get<2>((*i).second).size()));
            if(std::get<1>((*i).second) == udho::view::resources::asset::type::js){
                CHECK(response[boost::beast::http::field::content_type] == "application/javascript");
            }


            // TEST check output

            CHECK(body == std::get<2>((*i).second));

            ++i;
        }
    }
}
