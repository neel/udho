#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/cookies/cookie.h>
#include <boost/lexical_cast.hpp>

TEST_CASE("Cookie Construction", "[cookie]") {
    SECTION("Valid name initialization") {
        udho::cookies::cookie<std::string> c("session");
        CHECK(c.valid());
        CHECK(c.name() == "session");
    }

    SECTION("Invalid name characters") {
        udho::cookies::cookie<std::string> c("session(123");
        CHECK_FALSE(c.valid());
    }

    SECTION("Name-Value construction") {
        udho::cookies::cookie<int> c("counter", 42);
        CHECK(c.value() == 42);
        CHECK(c.secure() == false);
    }
}

TEST_CASE("Special Prefix Handling", "[cookie][security]") {
    SECTION("__Secure- prefix forces Secure") {
        udho::cookies::cookie<std::string> c("__Secure-token", "abc");
        CHECK(c.secure() == true);
    }

    SECTION("__Host- prefix CHECKments") {
        udho::cookies::cookie<std::string> c("__Host-auth", "data");
        CHECK(c.secure() == true);
        CHECK(c.path() == "/");
        CHECK_FALSE(c.domain().has_value());

        CHECK_THROWS(c.domain("example.com"));
        CHECK_THROWS(c.path("/api"));
    }
}

TEST_CASE("Attribute Setters", "[cookie][attributes]") {
    udho::cookies::cookie<std::string> c("test");

    SECTION("Path validation") {
        CHECK_THROWS(c.path("api"));
        c.path("/api");
        CHECK(*c.path() == "/api");
    }

    SECTION("Domain validation") {
        CHECK_THROWS(c.domain("example.com:3000"));
        c.domain("example.com");
        CHECK(*c.domain() == "example.com");
    }

    SECTION("SameSite None forces Secure") {
        c.same_site(udho::cookies::policy::none);
        CHECK(c.secure() == true);
    }

    SECTION("Partitioned enables Secure") {
        c.partitioned(true);
        CHECK(c.secure() == true);
    }
}

TEST_CASE("Cookie Serialization (write)", "[cookie][serialization]") {
    SECTION("Basic cookie") {
        udho::cookies::cookie<std::string> c("id", "aBc123");
        CHECK(to_string(c) == "id=aBc123");
    }

    SECTION("Full-featured cookie") {
        udho::cookies::cookie<int> c("prefs", 42);
        c.domain("example.com")
            .path("/")
            .max_age(3600)
            .http_only(true)
            .secure(true)
            .same_site(udho::cookies::policy::lax);

        auto s = to_string(c);
        CHECK(s.find("Domain=example.com") != std::string::npos);
        CHECK(s.find("SameSite=Lax") != std::string::npos);
        CHECK(s.find("Secure") != std::string::npos);
        CHECK(s.find("HttpOnly") != std::string::npos);
    }

    SECTION("Expired cookie") {
        udho::cookies::cookie<std::string> c("session", "123");
        c.remove();
        CHECK(to_string(c).find("Max-Age=0") != std::string::npos);
        CHECK(to_string(c).find("Expires=Fri, 01 Jan 1971") != std::string::npos);
    }
}

TEST_CASE("Cookie Parsing (read)", "[cookie][parsing]") {
    SECTION("Basic cookie") {
        auto c = udho::cookies::read("session=abc123");
        CHECK(c.valid());
        CHECK(c.name() == "session");
        CHECK(c.value() == "abc123");
    }

    SECTION("Cookie with attributes") {
        auto c = udho::cookies::read("id=42; Domain=example.com; Path=/; Secure; HttpOnly; SameSite=Strict");

        CHECK(c.domain() == "example.com");
        CHECK(*c.path() == "/");
        CHECK(c.secure() == true);
        CHECK(c.http_only() == true);
        CHECK(*c.same_site() == udho::cookies::policy::strict);
    }

    SECTION("Malformed cookies") {
        SECTION("Missing equals sign") {
            auto c = udho::cookies::read("session");
            CHECK_FALSE(c.valid());
        }

        SECTION("Invalid attributes") {
            auto c = udho::cookies::read("test=val; Invalid; Secure=123; SameSite=Invalid");
            CHECK(c.valid());
            CHECK(c.secure() == true);  // Secure should be set despite invalid value
            CHECK_FALSE(c.same_site().has_value());
        }
    }

    SECTION("Special prefixes during parsing") {
        auto c = udho::cookies::read("__Secure-token=abc; Secure");
        CHECK(c.valid());
        CHECK(c.secure() == true);
    }
}

TEST_CASE("Type Conversion", "[cookie][conversion]") {
    SECTION("String to numeric") {
        udho::cookies::cookie<std::string> c("count", "42");
        auto numeric = c.as<int>();
        CHECK(numeric.value() == 42);
    }

    SECTION("Numeric to string") {
        udho::cookies::cookie<int> c("version", 3);
        auto str = c.as<std::string>();
        CHECK(str.value() == "3");
    }
}

TEST_CASE("Edge Cases", "[cookie][edge]") {
    SECTION("Empty header") {
        auto c = udho::cookies::read("");
        CHECK_FALSE(c.valid());
    }

    SECTION("Cookie with empty value") {
        auto c = udho::cookies::read("session=;");
        CHECK(c.valid());
        CHECK(c.value().empty());
    }

    SECTION("Case-insensitive attributes") {
        auto c = udho::cookies::read("id=1; DOMAIN=example.com; sAMESITE=lAx");
        CHECK(c.domain() == "example.com");
        CHECK(*c.same_site() == udho::cookies::policy::lax);
    }

    SECTION("Expires parsing") {
        auto c = udho::cookies::read("test=val; Expires=Wed, 21 Oct 2025 07:28:00 GMT");
        CHECK(c.expires_at().has_value());
    }
}

TEST_CASE("Policy Enum", "[policy]") {
    SECTION("String conversion") {
        CHECK(udho::cookies::detail::same_site_str(udho::cookies::policy::none) == "None");
        CHECK(udho::cookies::detail::same_site_str(udho::cookies::policy::lax) == "Lax");
        CHECK(udho::cookies::detail::same_site_str(udho::cookies::policy::strict) == "Strict");
    }

    SECTION("Policy parsing") {
        udho::cookies::policy p;
        CHECK(udho::cookies::detail::parse_policy("None", p));
        CHECK(p == udho::cookies::policy::none);
        CHECK(udho::cookies::detail::parse_policy("LAX", p));
        CHECK(p == udho::cookies::policy::lax);
        CHECK_FALSE(udho::cookies::detail::parse_policy("Invalid", p));
    }
}
