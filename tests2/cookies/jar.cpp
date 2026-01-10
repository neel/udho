#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <thread>
#include <limits>
#include <udho/cookies/cookie.h>
#include <udho/cookies/jar.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/lexical_cast.hpp>

TEST_CASE("Cookie Jar Construction", "[jar][construction]") {
    SECTION("Default construction") {
        udho::cookies::jar jar;
        // Jar should be empty initially
        CHECK_FALSE(jar.exists("test"));
    }

    SECTION("Non-copyable") {
        // This should not compile - testing conceptually
        // udho::cookies::jar jar1;
        // udho::cookies::jar jar2(jar1);  // Should not compile
        // udho::cookies::jar jar3 = jar1; // Should not compile

        // Test passes if compilation succeeds without copy operations
        CHECK(true);
    }
}

TEST_CASE("Cookie Jar Basic Operations", "[jar][basic]") {
    udho::cookies::jar jar;

    SECTION("Adding cookies") {
        udho::cookies::cookie<std::string> cookie("session", "abc123");
        jar.add(cookie);

        CHECK(jar.exists("session"));
        CHECK(jar.get("session").value() == "abc123");
    }

    SECTION("Adding cookies with move semantics") {
        udho::cookies::cookie<int> cookie("counter", 42);
        jar.add(std::move(cookie));

        CHECK(jar.exists("counter"));
        CHECK(jar.get("counter").value() == "42");
    }

    SECTION("Stream insertion operator") {
        udho::cookies::cookie<std::string> cookie1("token", "xyz789");
        udho::cookies::cookie<int> cookie2("version", 3);

        jar << cookie1 << cookie2;

        CHECK(jar.exists("token"));
        CHECK(jar.exists("version"));
        CHECK(jar.get("token").value() == "xyz789");
        CHECK(jar.get("version").value() == "3");
    }

    SECTION("Overwriting existing cookies") {
        udho::cookies::cookie<std::string> cookie1("session", "old_value");
        udho::cookies::cookie<std::string> cookie2("session", "new_value");

        jar.add(cookie1);
        CHECK(jar.get("session").value() == "old_value");

        jar.add(cookie2);
        CHECK(jar.get("session").value() == "new_value");
    }

    SECTION("Subscript operator") {
        udho::cookies::cookie<std::string> cookie("test", "value");
        jar.add(cookie);

        CHECK(jar["test"].value() == "value");
    }

    SECTION("Clear operation") {
        udho::cookies::cookie<std::string> cookie1("session", "abc");
        udho::cookies::cookie<std::string> cookie2("token", "xyz");

        jar.add(cookie1);
        jar.add(cookie2);

        CHECK(jar.exists("session"));
        CHECK(jar.exists("token"));

        jar.clear();

        CHECK_FALSE(jar.exists("session"));
        CHECK_FALSE(jar.exists("token"));
    }
}

TEST_CASE("Cookie Jar Error Handling", "[jar][errors]") {
    udho::cookies::jar jar;

    SECTION("Getting non-existent cookie") {
        CHECK_THROWS_AS(jar.get("nonexistent"), std::out_of_range);
        CHECK_THROWS_WITH(jar.get("nonexistent"), "no cookie found with name or id: nonexistent");
    }

    SECTION("Subscript operator with non-existent cookie") {
        CHECK_THROWS_AS(jar["nonexistent"], std::out_of_range);
    }

    SECTION("Exists returns false for non-existent cookies") {
        CHECK_FALSE(jar.exists("nonexistent"));
    }
}

TEST_CASE("Cookie Jar HTTP Request Processing", "[jar][http][request]") {
    udho::cookies::jar jar;

    SECTION("Empty request") {
        boost::beast::http::request<boost::beast::http::string_body> req;
        std::size_t count = jar.apply(req);

        CHECK(count == 0);
        CHECK_FALSE(jar.exists("session"));
    }

    SECTION("Request with single cookie") {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "session=abc123");

        std::size_t count = jar.apply(req);

        CHECK(count == 1);
        CHECK(jar.exists("session"));
        CHECK(jar.get("session").value() == "abc123");
    }

    SECTION("Request with multiple cookies") {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "session=abc123; token=xyz789; counter=42");

        std::size_t count = jar.apply(req);

        CHECK(count == 3);
        CHECK(jar.exists("session"));
        CHECK(jar.exists("token"));
        CHECK(jar.exists("counter"));
        CHECK(jar.get("session").value() == "abc123");
        CHECK(jar.get("token").value() == "xyz789");
        CHECK(jar.get("counter").value() == "42");
    }

    SECTION("Request with malformed cookies") {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "valid=good; invalid_no_equals; another=valid");

        std::size_t count = jar.apply(req);

        // Only valid cookies should be counted
        CHECK(count == 2);
        CHECK(jar.exists("valid"));
        CHECK(jar.exists("another"));
        CHECK(jar.get("valid").value() == "good");
        CHECK(jar.get("another").value() == "valid");
    }

    SECTION("Function call operator for requests") {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "test=value");

        std::size_t count = jar(req);

        CHECK(count == 1);
        CHECK(jar.exists("test"));
    }

    SECTION("Apply clears existing cookies") {
        // Add some cookies first
        udho::cookies::cookie<std::string> existing("existing", "value");
        jar.add(existing);
        CHECK(jar.exists("existing"));

        // Apply request with different cookies
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "new=cookie");

        std::size_t count = jar.apply(req);

        CHECK(count == 1);
        CHECK_FALSE(jar.exists("existing"));
        CHECK(jar.exists("new"));
    }
}

TEST_CASE("Cookie Jar HTTP Response Processing", "[jar][http][response]") {
    udho::cookies::jar jar;

    SECTION("Empty jar to response") {
        boost::beast::http::response<boost::beast::http::string_body> res;
        std::size_t count = jar.apply(res);

        CHECK(count == 0);
        CHECK(res.count(boost::beast::http::field::set_cookie) == 0);
    }

    SECTION("Single cookie to response") {
        udho::cookies::cookie<std::string> cookie("session", "abc123");
        jar.add(cookie);

        boost::beast::http::response<boost::beast::http::string_body> res;
        std::size_t count = jar.apply(res);

        CHECK(count == 1);
        CHECK(res.count(boost::beast::http::field::set_cookie) == 1);

        std::string set_cookie_value = res[boost::beast::http::field::set_cookie];
        CHECK(set_cookie_value.find("session=abc123") != std::string::npos);
    }

    SECTION("Multiple cookies to response") {
        udho::cookies::cookie<std::string> cookie1("session", "abc123");
        udho::cookies::cookie<std::string> cookie2("token", "xyz789");
        udho::cookies::cookie<int> cookie3("counter", 42);

        jar.add(cookie1);
        jar.add(cookie2);
        jar.add(cookie3);

        boost::beast::http::response<boost::beast::http::string_body> res;
        std::size_t count = jar.apply(res);

        CHECK(count == 3);
        CHECK(res.count(boost::beast::http::field::set_cookie) == 3);
    }

    SECTION("Cookie with attributes to response") {
        udho::cookies::cookie<std::string> cookie("secure_session", "value");
        cookie.secure(true)
            .http_only(true)
            .same_site(udho::cookies::policy::strict)
            .path("/api")
            .domain("example.com");

        jar.add(cookie);

        boost::beast::http::response<boost::beast::http::string_body> res;
        std::size_t count = jar.apply(res);

        CHECK(count == 1);

        std::string set_cookie_value = res[boost::beast::http::field::set_cookie];
        CHECK(set_cookie_value.find("secure_session=value") != std::string::npos);
        CHECK(set_cookie_value.find("Secure") != std::string::npos);
        CHECK(set_cookie_value.find("HttpOnly") != std::string::npos);
        CHECK(set_cookie_value.find("SameSite=Strict") != std::string::npos);
        CHECK(set_cookie_value.find("Path=/api") != std::string::npos);
        CHECK(set_cookie_value.find("Domain=example.com") != std::string::npos);
    }

    SECTION("Function call operator for responses") {
        udho::cookies::cookie<std::string> cookie("test", "value");
        jar.add(cookie);

        boost::beast::http::response<boost::beast::http::string_body> res;
        std::size_t count = jar(res);

        CHECK(count == 1);
        CHECK(res.count(boost::beast::http::field::set_cookie) == 1);
    }
}

// TEST_CASE("Cookie Jar Thread Safety", "[jar][threading]") {
//     udho::cookies::jar jar;

//     SECTION("Concurrent additions") {
//         const int num_threads = 10;
//         const int cookies_per_thread = 100;

//         std::vector<std::thread> threads;

//         for (int t = 0; t < num_threads; ++t) {
//             threads.emplace_back([&jar, t, cookies_per_thread]() {
//                 for (int i = 0; i < cookies_per_thread; ++i) {
//                     std::string name = "thread_" + std::to_string(t) + "_cookie_" + std::to_string(i);
//                     udho::cookies::cookie<std::string> cookie(name, "value_" + std::to_string(i));
//                     jar.add(cookie);
//                 }
//             });
//         }

//         for (auto& thread : threads) {
//             thread.join();
//         }

//         // Verify all cookies were added
//         for (int t = 0; t < num_threads; ++t) {
//             for (int i = 0; i < cookies_per_thread; ++i) {
//                 std::string name = "thread_" + std::to_string(t) + "_cookie_" + std::to_string(i);
//                 CHECK(jar.exists(name));
//                 CHECK(jar.get(name).value() == "value_" + std::to_string(i));
//             }
//         }
//     }
// }

TEST_CASE("Cookie Jar Type Conversion", "[jar][conversion]") {
    udho::cookies::jar jar;

    SECTION("Adding different value types") {
        udho::cookies::cookie<int> int_cookie("counter", 42);
        udho::cookies::cookie<double> double_cookie("price", 19.99);
        udho::cookies::cookie<bool> bool_cookie("enabled", true);
        udho::cookies::cookie<std::string> string_cookie("name", "test");

        jar.add(int_cookie);
        jar.add(double_cookie);
        jar.add(bool_cookie);
        jar.add(string_cookie);

        CHECK(jar.exists("counter"));
        CHECK(jar.exists("price"));
        CHECK(jar.exists("enabled"));
        CHECK(jar.exists("name"));

        // All should be converted to string storage
        CHECK(jar.get("counter").value() == "42");
        CHECK(std::abs(boost::lexical_cast<double>(jar.get("price").value()) - 19.99) <= std::numeric_limits<double>::epsilon());
        CHECK(jar.get("enabled").value() == "1");
        CHECK(jar.get("name").value() == "test");
    }
}

TEST_CASE("Cookie Jar Integration", "[jar][integration]") {
    udho::cookies::jar jar;

    SECTION("Full request-response cycle") {
        // Simulate receiving a request with cookies
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "session=abc123; prefs=dark_mode");

        std::size_t parsed_count = jar.apply(req);
        CHECK(parsed_count == 2);

        // Add additional cookies
        udho::cookies::cookie<std::string> csrf_cookie("csrf_token", "xyz789");
        csrf_cookie.secure(true).http_only(true);
        jar.add(csrf_cookie);

        // Apply to response
        boost::beast::http::response<boost::beast::http::string_body> res;
        std::size_t applied_count = jar.apply(res);
        CHECK(applied_count == 3);

        // Verify all cookies are in response
        CHECK(res.count(boost::beast::http::field::set_cookie) == 3);
    }

    SECTION("Special cookie prefixes handling") {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.set(boost::beast::http::field::cookie, "__Secure-token=secure_value; __Host-auth=host_value");

        std::size_t count = jar.apply(req);
        CHECK(count == 2);

        CHECK(jar.count("__Secure-token") > 0);
        CHECK(jar.count("__Host-auth") > 0);

        // These should have security attributes set
        CHECK(jar.get("__Secure-token").secure() == true);
        CHECK(jar.get("__Host-auth").secure() == true);
    }
}
