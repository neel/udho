#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/www/components/protocol.h>
#include <udho/net/protocols/protocols.h>
#include <udho/manifold/config.h>
#include <udho/manifold/journal.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/range/algorithm.hpp>

namespace sim{

template <typename ResultT>
struct expected_next{
    using result_type = ResultT;

    result_type& _result;
    std::exception_ptr& _ex;

    expected_next(result_type& res, std::exception_ptr& ex): _result(res), _ex(ex) {}

    void pass(result_type&& res) {
        _result = std::move(res);
    }
    void fail(result_type&& res) {
        _result = std::move(res);
    }

    template <typename ExceptionT, std::enable_if_t<std::is_base_of<std::exception, ExceptionT>::value, bool> = true>
    void fail(ExceptionT&& ex) {
        fail(std::make_exception_ptr(std::move(ex)));
    }

    void fail(std::exception_ptr&& exp) {
        _ex = std::move(exp);
    }

    void fail(const std::error_code& ec) {
        fail(std::system_error(ec));
    }

    std::exception_ptr& exception() { return _ex; }
};

}

TEST_CASE("udho manifold protocol", "[manifold][components][http]") {
    using stream_type      = boost::beast::test::stream; // udho::net::types::socket;
    using protocol_type    = udho::net::protocols::http<stream_type>;
    using component_type   = udho::www::components::protocol<protocol_type,stream_type>;
    using config_type      = udho::manifold::config<component_type>;
    using fabric_type      = udho::manifold::facet<component_type, udho::www::feature::header_reader>;
    using journal_type     = udho::manifold::journal<fabric_type>;
    using result_type      = udho::www::feature::header_reader::result;
    using next_type        = sim::expected_next<result_type>;

    boost::asio::io_context io_context;
    component_type component;
    config_type    config;

    SECTION("reading http header with various request types") {
        using fabric_type   = udho::manifold::facet<component_type, udho::www::feature::header_reader>;
        using journal_type  = udho::manifold::journal<fabric_type>;
        using result_type   = udho::www::feature::header_reader::result;
        using next_type     = sim::expected_next<result_type>;

        // Test Case 1: GET request with query parameters
        SECTION("GET request with query parameters") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;
            std::string request_data =
                "GET /hello/world/23?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";

            stream_type stream(io_context, request_data);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            // Run with a timeout to prevent hanging
            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.target() == "/hello/world/23?name=test&id=42&filter=active");
            CHECK(result.method() == boost::beast::http::verb::get);
            CHECK(result.version() == 11);
            CHECK(result["Host"] == "example.com");
            CHECK(result["User-Agent"] == "test-agent/1.0");
            CHECK(result["Accept"] == "application/json, text/html");
            CHECK(result["Connection"] == "keep-alive");

        }

        // Test Case 2: POST request with headers and body (though we're only reading headers)
        SECTION("POST request with headers") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;
            std::string request_data =
                "POST /api/users HTTP/1.1\r\n"
                "Host: api.example.com\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: 45\r\n"
                "Authorization: Bearer token123\r\n"
                "X-Request-ID: abcdef123456\r\n"
                "\r\n"
                "{\"name\":\"test\",\"email\":\"test@example.com\"}";

            stream_type stream(io_context, request_data);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            // Run with a timeout to prevent hanging
            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.target() == "/api/users");
            CHECK(result.method() == boost::beast::http::verb::post);
            CHECK(result["Host"] == "api.example.com");
            CHECK(result["Content-Type"] == "application/json");
            CHECK(result["Content-Length"] == "45");
            CHECK(result["Authorization"] == "Bearer token123");
            CHECK(result["X-Request-ID"] == "abcdef123456");

        }

        // Test Case 3: PUT request with special headers
        SECTION("PUT request with special headers") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;
            std::string request_data =
                "PUT /resources/123 HTTP/1.1\r\n"
                "Host: example.org\r\n"
                "If-Match: \"etag123\"\r\n"
                "If-None-Match: *\r\n"
                "X-Custom-Header: custom-value\r\n"
                "X-API-Version: 2.0\r\n"
                "\r\n";

            stream_type stream(io_context, request_data);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            // Run with a timeout to prevent hanging
            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.target() == "/resources/123");
            CHECK(result.method() == boost::beast::http::verb::put);
            CHECK(result["Host"] == "example.org");
            CHECK(result["If-Match"] == "\"etag123\"");
            CHECK(result["If-None-Match"] == "*");
            CHECK(result["X-Custom-Header"] == "custom-value");
            CHECK(result["X-API-Version"] == "2.0");

        }

        // Test Case 4: Error case - malformed request
        SECTION("Malformed request") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;
            // Create a truly malformed request - missing HTTP version
            std::string request_data =
                "GET /test\r\n"  // Missing HTTP version
                "Host: example.com\r\n"
                "Incomplete-Header: value\r\n"
                "\r\n";

            stream_type stream(io_context, request_data);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            // Run with a timeout to prevent hanging
            io_context.run_for(std::chrono::milliseconds(100));

            // Should have an error
            // CHECK(result.error());
        }

        // Test Case 5: DELETE request
        SECTION("DELETE request") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;
            std::string request_data =
                "DELETE /items/456 HTTP/1.1\r\n"
                "Host: api.example.com\r\n"
                "X-Request-ID: req-789\r\n"
                "X-API-Key: key-12345\r\n"
                "\r\n";

            stream_type stream(io_context, request_data);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            // Run with a timeout to prevent hanging
            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.target() == "/items/456");
            CHECK(result.method() == boost::beast::http::verb::delete_);
            CHECK(result["X-Request-ID"] == "req-789");
            CHECK(result["X-API-Key"] == "key-12345");

        }

        // Test Case 6: Request with cookies
        SECTION("Request with cookies") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;
            std::string request_data =
                "GET /dashboard HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Cookie: sessionid=abc123; userid=456; theme=dark\r\n"
                "Accept-Language: en-US, en; q=0.5\r\n"
                "\r\n";

            stream_type stream(io_context, request_data);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            // Run with a timeout to prevent hanging
            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.target() == "/dashboard");
            CHECK(result.method() == boost::beast::http::verb::get);
            CHECK(result["Cookie"] == "sessionid=abc123; userid=456; theme=dark");
            CHECK(result["Accept-Language"] == "en-US, en; q=0.5");

        }
    }
}

TEST_CASE("udho manifold protocol", "[manifold][components][scgi]") {
    using stream_type      = boost::beast::test::stream;
    using protocol_type    = udho::net::protocols::scgi2<stream_type>;
    using component_type   = udho::www::components::protocol<protocol_type, stream_type>;
    using config_type      = udho::manifold::config<component_type>;
    using fabric_type      = udho::manifold::facet<component_type, udho::www::feature::header_reader>;
    using journal_type     = udho::manifold::journal<fabric_type>;
    using result_type      = udho::www::feature::header_reader::result;
    using next_type        = sim::expected_next<result_type>;

    boost::asio::io_context io_context;
    component_type component;
    config_type    config;

    // Helper function to convert HTTP request to SCGI format
    auto http_to_scgi = [](const std::string& http_request) -> std::string {
        std::istringstream iss(http_request);
        std::string line;
        std::string method, path, version;

        // Parse request line
        std::getline(iss, line);
        std::istringstream request_line(line);
        request_line >> method >> path >> version;

        // Split headers from body
        std::ostringstream headers_stream, body_stream;
        bool in_body = false;
        while (std::getline(iss, line)) {
            if (line == "\r" || line.empty()) {
                in_body = true;
                continue;
            }
            if (!in_body)
                headers_stream << line << "\n";
            else
                body_stream << line << "\n";
        }
        std::string body = body_stream.str();
        if (!body.empty() && body.back() == '\n') body.pop_back(); // trim

        std::vector<char> scgi_data;

        auto add_kv = [&](const std::string& k, const std::string& v) {
            scgi_data.insert(scgi_data.end(), k.begin(), k.end());
            scgi_data.push_back('\0');
            scgi_data.insert(scgi_data.end(), v.begin(), v.end());
            scgi_data.push_back('\0');
        };

        // Required SCGI keys
        add_kv("CONTENT_LENGTH", std::to_string(body.size()));
        add_kv("SCGI", "1");
        add_kv("REQUEST_METHOD", method);
        add_kv("REQUEST_URI", path);
        add_kv("SERVER_PROTOCOL", version);

        // Parse and add HTTP headers
        std::istringstream hdrs(headers_stream.str());
        while (std::getline(hdrs, line)) {
            if (line.empty()) continue;
            size_t colon_pos = line.find(':');
            if (colon_pos == std::string::npos) continue;

            std::string header_name = line.substr(0, colon_pos);
            std::string header_value = line.substr(colon_pos + 1);
            header_value.erase(0, header_value.find_first_not_of(" \t\r\n"));
            header_value.erase(header_value.find_last_not_of(" \t\r\n") + 1);

            std::string scgi_header_name;
            if (header_name == "Content-Type")
                scgi_header_name = "CONTENT_TYPE";
            // else if (header_name == "Content-Length")
            //     continue; // already added
            else {
                scgi_header_name = "HTTP_" + header_name;
                std::replace(scgi_header_name.begin(), scgi_header_name.end(), '-', '_');
                std::transform(scgi_header_name.begin(), scgi_header_name.end(), scgi_header_name.begin(), ::toupper);
            }

            add_kv(scgi_header_name, header_value);
        }

        // Build netstring
        std::string scgi_request = std::to_string(scgi_data.size()) + ":";
        scgi_request.append(scgi_data.begin(), scgi_data.end());
        scgi_request += ",";
        scgi_request += body; // append body after netstring

        return scgi_request;
    };

    SECTION("reading scgi header with various request types") {
        // Test Case 1: GET request with query parameters
        SECTION("GET request with query parameters") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;

            std::string http_request =
                "GET /hello/world/23?name=test&id=42&filter=active HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "User-Agent: test-agent/1.0\r\n"
                "Accept: application/json, text/html\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";

            std::string scgi_request = http_to_scgi(http_request);
            stream_type stream(io_context, scgi_request);

            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            io_context.run_for(std::chrono::milliseconds(100));

            // Check the converted HTTP request
            CHECK(result.method() == boost::beast::http::verb::get);
            CHECK(result.target() == "/hello/world/23?name=test&id=42&filter=active");
            CHECK(result.version() == 11);
            CHECK(result["Host"] == "example.com");
            CHECK(result["User-Agent"] == "test-agent/1.0");
            CHECK(result["Accept"] == "application/json, text/html");
            CHECK(result["Connection"] == "keep-alive");
        }

        // Test Case 2: POST request with headers
        SECTION("POST request with headers") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;

            std::string http_request =
                "POST /api/users HTTP/1.1\r\n"
                "Host: api.example.com\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: 45\r\n"
                "Authorization: Bearer token123\r\n"
                "X-Request-ID: abcdef123456\r\n"
                "\r\n";

            std::string scgi_request = http_to_scgi(http_request);
            stream_type stream(io_context, scgi_request);

            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.method() == boost::beast::http::verb::post);
            CHECK(result.target() == "/api/users");
            CHECK(result.version() == 11);
            CHECK(result["Host"] == "api.example.com");
            CHECK(result["Content-Type"] == "application/json");
            CHECK(result["Content-Length"] == "45");
            CHECK(result["Authorization"] == "Bearer token123");
            CHECK(result["X-Request-ID"] == "abcdef123456");
        }

        // Test Case 3: PUT request with special headers
        SECTION("PUT request with special headers") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;

            std::string http_request =
                "PUT /resources/123 HTTP/1.1\r\n"
                "Host: example.org\r\n"
                "If-Match: \"etag123\"\r\n"
                "If-None-Match: *\r\n"
                "X-Custom-Header: custom-value\r\n"
                "X-API-Version: 2.0\r\n"
                "\r\n";

            std::string scgi_request = http_to_scgi(http_request);
            stream_type stream(io_context, scgi_request);

            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.method() == boost::beast::http::verb::put);
            CHECK(result.target() == "/resources/123");
            CHECK(result.version() == 11);
            CHECK(result["Host"] == "example.org");
            CHECK(result["If-Match"] == "\"etag123\"");
            CHECK(result["If-None-Match"] == "*");
            CHECK(result["X-Custom-Header"] == "custom-value");
            CHECK(result["X-API-Version"] == "2.0");
        }

        // Test Case 4: DELETE request
        SECTION("DELETE request") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;

            std::string http_request =
                "DELETE /items/456 HTTP/1.1\r\n"
                "Host: api.example.com\r\n"
                "X-Request-ID: req-789\r\n"
                "X-API-Key: key-12345\r\n"
                "\r\n";

            std::string scgi_request = http_to_scgi(http_request);
            stream_type stream(io_context, scgi_request);

            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            io_context.run_for(std::chrono::milliseconds(100));

            CHECK(result.method() == boost::beast::http::verb::delete_);
            CHECK(result.target() == "/items/456");
            CHECK(result.version() == 11);
            CHECK(result["Host"] == "api.example.com");
            CHECK(result["X-Request-ID"] == "req-789");
            CHECK(result["X-API-Key"] == "key-12345");
        }

        // Test Case 5: Error case - malformed SCGI request
        SECTION("Malformed SCGI request - no length") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;

            // Invalid SCGI format (missing length prefix)
            std::string malformed_scgi = "CONTENT_LENGTH\000\0SCGI\001\0REQUEST_METHOD\0GET\0,";

            stream_type stream(io_context, malformed_scgi);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            io_context.run();
            std::cout << result << std::endl;
            // Should have an error
            REQUIRE(!!ex);
            bool exception_caught = false;
            try{
                std::rethrow_exception(ex);
            } catch(const std::system_error& error){
                CHECK(error.code() == std::errc::operation_canceled);
                exception_caught = true;
            }
            REQUIRE(exception_caught);
        }

        SECTION("Malformed SCGI request - too long length") {
            io_context.restart();
            result_type result;
            std::exception_ptr ex;
            journal_type journal;

            // Invalid SCGI format (missing length prefix)
            std::string malformed_scgi = "658490876:CONTENT_LENGTH\000\0SCGI\001\0REQUEST_METHOD\0GET\0,";

            stream_type stream(io_context, malformed_scgi);
            fabric_type fabric{component, config, 0};
            fabric.eval(journal, next_type{result, ex}, stream);

            io_context.run();
            std::cout << result << std::endl;
            // Should have an error
            REQUIRE(!!ex);
            bool exception_caught = false;
            try{
                std::rethrow_exception(ex);
            } catch(const std::system_error& error){
                CHECK(error.code() == std::errc::invalid_argument);
                exception_caught = true;
            }
            REQUIRE(exception_caught);
        }
    }
}
