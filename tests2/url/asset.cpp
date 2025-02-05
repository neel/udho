#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>
#include <udho/net/context.h>
#include <udho/url/router.h>

#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <type_traits>
#include <curl/curl.h>
#include <udho/net/artifacts.h>

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

static size_t curl_writef(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

struct http_results{
    long code;
    std::string   body;
    std::map<std::string, std::string> headers;
};

http_results curl_fetch(CURL* curl, const std::string method, const std::string& url){
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "http");
        struct curl_slist *headers = NULL;
        std::string response_headers;
        std::string response_body;
        std::map<std::string, std::string> headers_map;
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curl_writef);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,  curl_writef);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA,      &response_headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response_body);
        res = curl_easy_perform(curl);
        long response_code = 0;
        if(res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            std::vector<std::string> header_lines;
            boost::algorithm::split(header_lines, response_headers, boost::is_any_of("\r\n"));
            for(const std::string& line: header_lines){
                std::vector<std::string> header_parts;
                boost::algorithm::split(header_parts, line, boost::is_any_of(":"));
                if(header_parts.size() >= 2){
                    headers_map.insert(std::make_pair(boost::algorithm::trim_copy(header_parts[0]), boost::algorithm::trim_copy(header_parts[1])));
                }
            }
        }
        return http_results{response_code, response_body, headers_map};
}


TEST_CASE("Accessing assets through router via HTTP requests", "[router][asset]") {
    static char buffer_js [] = "console.log('Hello, world!');";
    static char buffer_js1[] = "console.log('Hello, Mars!');";
    static char buffer_css[] = ".classname{color: blue}";
    static unsigned char buffer_img[] = {
        0x47,0x49,0x46,0x38,0x39,0x61,0x04,0x00,0x04,0x00,0xA1,0x01,0x00,0x00,0x00,0x00,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x21,0xF9,0x04,0x01,0x0A,0x00,0x02,
        0x00,0x2C,0x00,0x00,0x00,0x00,0x04,0x00,0x04,0x00,0x00,0x02,0x05,0x44,0x7C,0x67,
        0xB8,0x05,0x00,0x3B
    };

    udho::view::resources::store<> resources;
    resources["primary"] << udho::view::resources::asset::js ("0profile1.js", std::begin(buffer_js),  std::end(buffer_js) );
    resources["primary"] << udho::view::resources::asset::js ("1profile2.js", std::begin(buffer_js1), std::end(buffer_js1));
    resources["primary"] << udho::view::resources::asset::css("2profile.css", std::begin(buffer_css), std::end(buffer_css));
    resources["primary"] << udho::view::resources::asset::img("3profile.gif", std::begin(buffer_img), std::end(buffer_img))->mime("image/gif");

    CHECK(4 == resources.assets().size());

    resources.assets().base("assets");

    // TEST Creating a const_store from store should throw exception unless the store is locked.
    REQUIRE_THROWS_AS(udho::view::resources::const_store{resources}, std::exception);

    resources.lock();
    // TEST Adding resources to a locked store should also throw exception
    REQUIRE_THROWS_AS(resources["primary"] << udho::view::resources::asset::js("profile1.js", buffer_js, buffer_js+std::strlen(buffer_js)), std::exception);

    udho::view::resources::const_store cstore{resources};

    auto router = udho::url::router(cstore.assets());

    std::cout << router << std::endl;

    boost::asio::io_service service;

    auto server = udho::net::server<http_listener>(service, 9000);
    auto artifacts  = udho::net::artifacts{router, resources};

    server.run(artifacts);

    std::thread thread([&]{
        service.run();
    });


    CHECK(0 == 0);

    CURL* curl;
    curl = curl_easy_init();
    CHECK(curl != 0x0);

    SECTION("HTTP Response js0") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/0profile1.js");
        CHECK(results.code == 200);
        CHECK(results.body == buffer_js);
        CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
        CHECK(results.headers["Content-Type"] == "application/javascript");
    }

    SECTION("HTTP Response js1") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/1profile2.js");
        CHECK(results.code == 200);
        CHECK(results.body == buffer_js1);
        CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
        CHECK(results.headers["Content-Type"] == "application/javascript");
    }

    SECTION("HTTP Response css") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/2profile.css");
        CHECK(results.code == 200);
        CHECK(results.body == buffer_css);
        CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
        CHECK(results.headers["Content-Type"] == "text/css");
    }

    SECTION("HTTP Response img") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/assets/primary/3profile.gif");
        CHECK(results.code == 200);
        // CHECK(results.body == gif);
        // CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
        // CHECK(results.headers["Content-Type"] == "text/css");
    }

    curl_easy_cleanup(curl);

    // server.stop();


    thread.join();
}
