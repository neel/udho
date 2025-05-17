#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <string>
#include <udho/utils/encoding.h>

TEST_CASE("Cookie Encoding/Decoding", "[encoding][cookie]") {
    SECTION("Encode basic characters") {
        std::string input = "user@example.com";
        auto encoded = udho::utils::encode::cookie(input);
        CHECK(encoded == "user%40example.com");
    }

    SECTION("Encode special characters") {
        std::string input = "name=John Doe;age=30";
        auto encoded = udho::utils::encode::cookie(input);
        CHECK(encoded == "name%3DJohn%20Doe%3Bage%3D30");
    }

    SECTION("Decode cookie string") {
        std::string input = "city%3DNew%20York%2B%25";
        auto decoded = udho::utils::decode::cookie(input);
        CHECK(decoded == "city=New York+%");
    }

    SECTION("Round trip") {
        std::string original = "session_id=ae3$%&*_!~";
        auto encoded = udho::utils::encode::cookie(original);
        auto decoded = udho::utils::decode::cookie(encoded);
        CHECK(decoded == original);
    }
}

TEST_CASE("URL Encoding/Decoding", "[encoding][url]") {
    SECTION("Encode spaces") {
        std::string input = "hello world";
        auto encoded = udho::utils::encode::url(input);
        CHECK(encoded == "hello+world");
    }

    SECTION("Encode special characters") {
        std::string input = "file name??.jpg";
        auto encoded = udho::utils::encode::url(input);
        CHECK(encoded == "file+name%3F%3F.jpg");
    }

    SECTION("Decode URL string") {
        std::string input = "price%3D100%2B%25";
        auto decoded = udho::utils::decode::url(input);
        CHECK(decoded == "price=100+%");
    }

    SECTION("Round trip with binary data") {
        std::string original;
        original.push_back('\0');  // Null byte
        original += "binary";
        original.push_back(0x01); // SOH
        original.push_back(0x02); // STX
        original += "data";

        auto encoded = udho::utils::encode::url(original);
        auto decoded = udho::utils::decode::url(encoded);

        CHECK(decoded == original);
    }
}

TEST_CASE("Base64 Encoding/Decoding", "[encoding][base64]") {
    SECTION("Encode basic string") {
        std::string input = "Hello World";
        auto encoded = udho::utils::encode::base64(input);
        CHECK(encoded == "SGVsbG8gV29ybGQ=");
    }

    SECTION("Decode basic string") {
        std::string input = "SGVsbG8gV29ybGQ=";
        auto decoded = udho::utils::decode::base64(input);
        CHECK(decoded == "Hello World");
    }

    SECTION("Binary data handling") {
        std::string input(1, '\xff');
        input += "\x00\x01\x02";
        auto encoded = udho::utils::encode::base64(input);
        auto decoded = udho::utils::decode::base64(encoded);
        CHECK(decoded == input);
    }

    SECTION("Padded input handling") {
        CHECK(udho::utils::decode::base64("SGVsbG8gV29ybGQ") == "Hello World");
        CHECK(udho::utils::decode::base64("SGVsbG8gV29ybGQ=") == "Hello World");
        CHECK_THROWS_AS(udho::utils::decode::base64("SGVsbG8gV29ybGQ=="), std::invalid_argument);
    }

    SECTION("Invalid input handling") {
        CHECK_THROWS_AS(udho::utils::decode::base64("SGVsbG8!V29ybGQ="), std::invalid_argument);
        CHECK_THROWS_AS(udho::utils::decode::base64("SGVsbG8=V29ybGQ="), std::invalid_argument);
    }
}

TEST_CASE("Base64 URL Encoding/Decoding", "[encoding][base64_url]") {
    SECTION("Encode URL-safe") {
        std::string input = "Hello World~";
        auto encoded = udho::utils::encode::base64_url(input);
        CHECK(encoded == "SGVsbG8gV29ybGR-");
    }

    SECTION("Decode URL-safe") {
        std::string input = "SGVsbG8gV29ybGR-";
        auto decoded = udho::utils::decode::base64_url(input);
        CHECK(decoded == "Hello World~");
    }

    SECTION("Padding handling") {
        std::string input = "padding";
        auto encoded = udho::utils::encode::base64_url(input);

        // Verify encoded string has no padding
        CHECK(encoded == "cGFkZGluZw");

        // Verify round-trip decoding
        CHECK(udho::utils::decode::base64_url(encoded) == input);

        // Verify with added padding
        CHECK(udho::utils::decode::base64_url(encoded + "=") == input);
        CHECK(udho::utils::decode::base64_url(encoded + "==") == input);
    }
}

TEST_CASE("Base16 Encoding/Decoding", "[encoding][base16]") {
    SECTION("Encode to lowercase") {
        std::string input = "Hello";
        auto encoded = udho::utils::encode::base16(input);
        CHECK(encoded == "48656c6c6f");
    }

    SECTION("Decode case-insensitive") {
        std::string input = "48656C6C6F";
        auto decoded = udho::utils::decode::base16(input);
        CHECK(decoded == "Hello");
    }

    SECTION("Invalid hex handling") {
        CHECK_THROWS_AS(udho::utils::decode::base16("48656z6c6f"), boost::algorithm::hex_decode_error);
        CHECK_THROWS_AS(udho::utils::decode::base16("486"), boost::algorithm::hex_decode_error);
    }
}

TEST_CASE("Edge Cases", "[encoding]") {
    SECTION("Empty strings") {
        CHECK(udho::utils::encode::cookie("") == "");
        CHECK(udho::utils::decode::base64("") == "");
    }

    SECTION("All special characters") {
        std::string input = "!*'();:@&=+$,/?#[]";
        auto url_encoded = udho::utils::encode::url(input);
        CHECK(url_encoded == "!*'()%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D");

        auto cookie_encoded = udho::utils::encode::cookie(input);
        CHECK(cookie_encoded == "%21%2A%27%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D");
    }
}
