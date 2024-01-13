#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <string>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <type_traits>
#include <curl/curl.h>
#include <udho/url/url.h>
#include <boost/algorithm/string.hpp>

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

void chunk3(udho::net::context context){
    context << "Chunk 3 (Final)";
    context.finish();
}

void chunk2(udho::net::context context){
    context << "chunk 2";
    context.flush(std::bind(&chunk3, context));
}

void chunk(udho::net::context context){
    context.encoding(udho::net::types::transfer::encoding::chunked);
    context << "Chunk 1";
    context.flush(std::bind(&chunk2, context));
}

void f0(udho::net::context context){
    context << "Hello f0";
    context.finish();
}

int f1(udho::net::context context, int a, const std::string& b, const double& c){
        context << "Hello f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
}

struct X{
    void f0(udho::net::context context){
        context << "Hello X::f0";
        context.finish();
    }

    int f1(udho::net::context context, int a, const std::string& b, const double& c){
        context << "Hello X::f1 ";
        context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
        context.finish();
        return a+b.size()+c;
    }
};


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

TEST_CASE("udho network", "[net]") {
    CHECK(1 == 1);

    using namespace udho::hazo::string::literals;


    X x;
    auto router = udho::url::router(
        udho::url::root(
            udho::url::slot("f0"_h,  &f0)         << udho::url::home  (udho::url::verb::get)                                                  |
            udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed (udho::url::verb::get, "/x/f0", "/x/f0")                                |
            udho::url::slot("chunked"_h,  &chunk) << udho::url::fixed (udho::url::verb::get, "/chunk")
        ) |
        udho::url::mount("b"_h, "/b",
            udho::url::slot("f1"_h,  &f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")      |
            udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx  (udho::url::verb::get, "/x/f1/(\\d+)/(\\w+)/(\\d+\\.\\d)", "/x/f1/{}/{}/{}")
        )
    );

    CHECK(router["b"_h]("f1"_h, 24, "Hello", 42) == "/b/f1/24/Hello/42");

    boost::asio::io_service service;

    auto server = udho::net::server<http_listener>(service, std::move(router), 9000);

    server.run();

    std::thread thread([&]{
        service.run();
    });

    CURL* curl;
    curl = curl_easy_init();
    CHECK(curl != 0x0);

    SECTION("HTTP Response home") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/");
        CHECK(results.code == 200);
        CHECK(results.body == "Hello f0");
        CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
    }

    SECTION("HTTP Response home") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/x/f0");
        CHECK(results.code == 200);
        CHECK(results.body == "Hello X::f0");
        CHECK(results.headers["Transfer-Encoding"] == "plain,plain");
    }

    SECTION("HTTP Response from mountpoint") {
        http_results results_f1 = curl_fetch(curl, "GET", "http://localhost:9000/b/f1/10/hello/42");
        CHECK(results_f1.code == 200);
        CHECK(results_f1.body == "Hello f1 a: 10, b: hello, c: 42");
        CHECK(results_f1.headers["Transfer-Encoding"] == "plain,plain");

        http_results results_xf1 = curl_fetch(curl, "GET", "http://localhost:9000/b/x/f1/567/ping/42.8");
        CHECK(results_xf1.code == 200);
        CHECK(results_xf1.body == "Hello X::f1 a: 567, b: ping, c: 42.8");
        CHECK(results_xf1.headers["Transfer-Encoding"] == "plain,plain");
    }

    SECTION("HTTP Chunked Response") {
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/chunk");
        CHECK(results.code == 200);
        CHECK(results.body == "Chunk 1chunk 2Chunk 3 (Final)");
        CHECK(results.headers["Transfer-Encoding"] == "chunked,plain");
    }

    curl_easy_cleanup(curl);

    server.stop();


    thread.join();
}
