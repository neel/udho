#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/asset/store.h>
#include <boost/variant.hpp>
#include <udho/net/ostream.h>
#include <udho/url/router.h>
#include <boost/asio/buffer.hpp>

static char buffer_js[]  = "console.log('Hello, world!');";
static char buffer_js1[] = "console.log('Hello, Mars!');";
static char buffer_css[] = ".classname{color: blue}";
static unsigned char buffer_img[] = {
    0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x04, 0x00, 0x04, 0x00, 0x80, 0x01,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x04, 0x00, 0x00, 0x02, 0x05, 0x44, 0x7c, 0x67, 0xb8, 0x05,
    0x00, 0x3b
};

TEST_CASE("Asset iteration using different indexes", "[view][resource][asset]") {
    udho::view::resources::asset::store store;

    store["a"]      << udho::view::resources::asset::js ("aworld.js",  std::begin(buffer_js),   std::end(buffer_js)  );
    store["a"]      << udho::view::resources::asset::js ("amars.js",   std::begin(buffer_js1),  std::end(buffer_js1) );
    store["a"]      << udho::view::resources::asset::css("astyle.css", std::begin(buffer_css),  std::end(buffer_css) );
    store["a"]      << udho::view::resources::asset::img("aimg.gif",   std::begin(buffer_img),  std::end(buffer_img) );

    store["b"]      << udho::view::resources::asset::js ("bworld.js",  std::begin(buffer_js),   std::end(buffer_js)  );
    store["b"]      << udho::view::resources::asset::js ("bmars.js",   std::begin(buffer_js1),  std::end(buffer_js1) );
    store["b"]      << udho::view::resources::asset::css("bstyle.css", std::begin(buffer_css),  std::end(buffer_css) );
    store["b"]      << udho::view::resources::asset::img("bimg.gif",   std::begin(buffer_img),  std::end(buffer_img) );

    store["c/d"]    << udho::view::resources::asset::js ("cdworld.js", std::begin(buffer_js),   std::end(buffer_js)  );
    store["c/d"]    << udho::view::resources::asset::js ("cdmars.js",  std::begin(buffer_js1),  std::end(buffer_js1) );
    store["c/d"]    << udho::view::resources::asset::css("cdstyle.css",std::begin(buffer_css),  std::end(buffer_css) );
    store["c/d"]    << udho::view::resources::asset::img("cdimg.gif",  std::begin(buffer_img),  std::end(buffer_img) );

    store["c"]      << udho::view::resources::asset::js ("cworld.js",  std::begin(buffer_js),   std::end(buffer_js)  );
    store["c"]      << udho::view::resources::asset::js ("cmars.js",   std::begin(buffer_js1),  std::end(buffer_js1) );
    store["c"]      << udho::view::resources::asset::css("cstyle.css", std::begin(buffer_css),  std::end(buffer_css) );
    store["c"]      << udho::view::resources::asset::img("cimg.gif",   std::begin(buffer_img),  std::end(buffer_img) );

    store.base("assets");
    store.lock();

    udho::view::resources::asset::const_store cstore{store};

    auto check_type = [&cstore](udho::view::resources::asset::type type, const std::string& extension, std::size_t expected_num_resources){
        auto begin = cstore.begin(type);
        auto end   = cstore.end(type);
        auto size  = cstore.size(type);

        REQUIRE(size == expected_num_resources);

        std::size_t counter = 0;
        for(auto it = begin; it != end; ++it){
            CAPTURE(it->prefix(), it->name());
            CHECK(it->type() == type);
            CHECK(boost::ends_with(it->name(), extension));
            ++counter;
        }
        REQUIRE(counter == size);

        return counter;
    };

    SECTION("Iteration by type") {
        check_type(udho::view::resources::asset::type::js,  ".js", 8);
        check_type(udho::view::resources::asset::type::css, ".css", 4);
        check_type(udho::view::resources::asset::type::img, ".gif", 4);
    }

    auto check_prefix = [&cstore](const std::string& prefix){
        auto begin = cstore.begin(prefix);
        auto end   = cstore.end(prefix);
        auto size  = cstore.size(prefix);

        std::vector<std::string> names;
        for(auto it = begin; it != end; ++it){
            CAPTURE(it->prefix(), it->name());
            CHECK  (it->prefix() == prefix);
            names.push_back(it->name());
        }
        REQUIRE(names.size() == size);
        REQUIRE(boost::ends_with(names[0], "js"));
        REQUIRE(boost::ends_with(names[1], "js"));
        REQUIRE(boost::ends_with(names[2], "css"));
        REQUIRE(boost::ends_with(names[3], "gif"));
    };

    SECTION("Iteration by prefix") {
        check_prefix("a");
        check_prefix("b");
        check_prefix("c");
        check_prefix("c/d");
    }

    auto check_prefix_type = [&cstore](const std::string& prefix, udho::view::resources::asset::type type, const std::string& ext, std::size_t expected_size){
        auto begin = cstore.begin(prefix, type);
        auto end   = cstore.end  (prefix, type);
        auto size  = cstore.size (prefix, type);

        CHECK(size == expected_size);

        std::vector<std::string> names;
        for(auto it = begin; it != end; ++it){
            CAPTURE(it->prefix(), it->name());
            CHECK  (it->prefix() == prefix);
            CHECK  (it->type() == type);
            names.push_back(it->name());
        }
        CHECK(names.size() == size);
        for(const auto& name: names){
            REQUIRE(boost::ends_with(name, ext));
        }

        REQUIRE(expected_size == names.size());
    };

    SECTION("Iteration by prefix and type") {
        check_prefix_type("a", udho::view::resources::asset::type::js,  "js",  2);
        check_prefix_type("a", udho::view::resources::asset::type::css, "css", 1);
        check_prefix_type("a", udho::view::resources::asset::type::img, "gif", 1);

        check_prefix_type("b", udho::view::resources::asset::type::js,  "js",  2);
        check_prefix_type("b", udho::view::resources::asset::type::css, "css", 1);
        check_prefix_type("b", udho::view::resources::asset::type::img, "gif", 1);

        check_prefix_type("c", udho::view::resources::asset::type::js,  "js",  2);
        check_prefix_type("c", udho::view::resources::asset::type::css, "css", 1);
        check_prefix_type("c", udho::view::resources::asset::type::img, "gif", 1);

        check_prefix_type("c/d", udho::view::resources::asset::type::js,  "js",  2);
        check_prefix_type("c/d", udho::view::resources::asset::type::css, "css", 1);
        check_prefix_type("c/d", udho::view::resources::asset::type::img, "gif", 1);
    }

    SECTION("Prefix proxy"){
        auto prefixes = cstore.prefixes();

        std::vector<std::string> prefixes_names;
        std::vector<std::size_t> prefixes_sizes;
        for(const auto& p: prefixes){
            prefixes_names.push_back(p.prefix());
            prefixes_sizes.push_back(p.size());

            CAPTURE(p.prefix(), p.size());
            REQUIRE(p.size() == 4);

            std::vector<std::string> names;
            for(const auto& r: p){
                CAPTURE(r.prefix(), r.name());
                CHECK  (r.prefix() == p.prefix());
                names.push_back(r.name());
            }
            REQUIRE(names.size() == p.size());
            REQUIRE(boost::ends_with(names[0], "js"));
            REQUIRE(boost::ends_with(names[1], "js"));
            REQUIRE(boost::ends_with(names[2], "css"));
            REQUIRE(boost::ends_with(names[3], "gif"));
        }

        REQUIRE(prefixes_names.size() == prefixes_sizes.size());
        REQUIRE(prefixes_names.size() == 4);
        REQUIRE(prefixes_names[0] == "a");
        REQUIRE(prefixes_names[1] == "b");
        REQUIRE(prefixes_names[2] == "c");
        REQUIRE(prefixes_names[3] == "c/d");
    }

    SECTION("Find resource by address") {
        REQUIRE(cstore.find("/assets/c/cworld.js") != cstore.cend());
        REQUIRE(cstore.find("/assets/c/d/cdworld.js") != cstore.cend());

        REQUIRE(cstore.find("/assets")    == cstore.cend());
        REQUIRE(cstore.find("/assets/")   == cstore.cend());
        REQUIRE(cstore.find("/assets//")  == cstore.cend());
        REQUIRE(cstore.find("/assets/a")  == cstore.cend());
        REQUIRE(cstore.find("/assets/a/") == cstore.cend());
    }

    SECTION("Retrieve assets through HTTP requests") {
        {
            udho::view::resources::asset::const_substore<udho::view::resources::asset::type::js> substore{cstore};
            std::size_t counter = 0;
            for(const auto& asset: substore){
                std::string url = asset.url();
                CAPTURE(url);

                boost::asio::io_context io;
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
                udho::net::ostream_view stream_view = stream.view();
                cstore.serve(stream_view, url);

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

                CAPTURE(response[boost::beast::http::field::content_type]);
                REQUIRE(response[boost::beast::http::field::content_type] == "application/javascript");
                if(counter % 2 == 0) {
                    REQUIRE(std::equal(body.begin(), body.end(), std::begin(buffer_js)));
                } else {
                    REQUIRE(std::equal(body.begin(), body.end(), std::begin(buffer_js1)));
                }

                counter++;
            }
        }{
            udho::view::resources::asset::const_substore<udho::view::resources::asset::type::css> substore{cstore};
            for(const auto& asset: substore){
                std::string url = asset.url();
                CAPTURE(url);

                boost::asio::io_context io;
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
                udho::net::ostream_view stream_view = stream.view();
                cstore.serve(stream_view, url);

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


                CAPTURE(response[boost::beast::http::field::content_type]);
                REQUIRE(response[boost::beast::http::field::content_type] == "text/css");
                REQUIRE(std::equal(body.begin(), body.end(), std::begin(buffer_css)));
            }
        }{
            udho::view::resources::asset::const_substore<udho::view::resources::asset::type::img> substore{cstore};
            for(const auto& asset: substore){
                std::string url = asset.url();
                CAPTURE(url);

                boost::asio::io_context io;
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
                udho::net::ostream_view stream_view = stream.view();
                cstore.serve(stream_view, url);

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

                CAPTURE(response[boost::beast::http::field::content_type]);
                REQUIRE(response[boost::beast::http::field::content_type] == "image/gif");
                REQUIRE(std::equal(body.begin(), body.end(), std::begin(buffer_img), [](const char& l, const unsigned char& r){ return static_cast<unsigned char>(l) == r; }));
            }
        }

    }
}
