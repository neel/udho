#include <catch2/catch_test_macros.hpp>
#include <udho/logging/message.h>

using namespace udho::logging::params;

TEST_CASE("Log message binary serialisation roundtrip", "[logging][binary]") {
    auto now = std::chrono::system_clock::now();

    udho::logging::message original;
    original[local_id::val] = 100ULL;
    original[timestamp::val] = now;
    original[severity::val] = udho::logging::severity::warning;
    original[thread::val] = 400ULL;
    original[process::val] = 500U;
    original[subsystem::val] = "subsys";
    original[message::val] = "A log message";
    original[file::val] = "source.cpp";
    original[function::val] = "someFunction";
    original[line::val] = 600ULL;

    original[request_id::val] = "req-999";
    original[status_code::val] = 404U;

    // Serialise to binary
    auto buffer = original.save();
    REQUIRE(buffer.size() == original.byte_size());

    // Deserialise into a new record
    udho::logging::message loaded;
    bool success = loaded.load(buffer.data(), buffer.size());
    REQUIRE(success);

    // Verify all fields
    REQUIRE(loaded[local_id::val] == 100ULL);
    REQUIRE(loaded[timestamp::val] == now);
    REQUIRE(loaded[severity::val] == udho::logging::severity::warning);
    REQUIRE(loaded[thread::val] == 400ULL);
    REQUIRE(loaded[process::val] == 500U);
    REQUIRE(loaded[subsystem::val] == "subsys");
    REQUIRE(loaded[message::val] == "A log message");
    REQUIRE(loaded[file::val] == "source.cpp");
    REQUIRE(loaded[function::val] == "someFunction");
    REQUIRE(loaded[line::val] == 600ULL);

    REQUIRE(loaded[request_id::val].value().has_value());
    REQUIRE(loaded[request_id::val].value() == "req-999");
    REQUIRE(loaded[status_code::val].value().has_value());
    REQUIRE(loaded[status_code::val].value() == 404U);
}

TEST_CASE("Log message binary deserialisation rejects corrupted data", "[logging][binary][negative]") {
    auto now = std::chrono::system_clock::now();

    udho::logging::message original;
    original[local_id::val] = 1ULL;
    original[timestamp::val] = now;
    original[severity::val] = udho::logging::severity::warning;
    original[thread::val] = 4ULL;
    original[process::val] = 5U;
    original[subsystem::val] = "test";
    original[message::val] = "msg";
    original[file::val] = "f";
    original[function::val] = "func";
    original[line::val] = 6ULL;

    auto buffer = original.save();

    SECTION("Truncated buffer (missing length field)") {
        std::vector<std::uint8_t> short_buf(buffer.begin(), buffer.begin() + 2);
        udho::logging::message loaded;
        REQUIRE_FALSE(loaded.load(short_buf.data(), short_buf.size()));
    }

    SECTION("Length field too large") {
        // Corrupt the length to exceed buffer size
        std::uint32_t fake_len = 9999;
        std::memcpy(buffer.data(), &fake_len, sizeof(fake_len));
        udho::logging::message loaded;
        REQUIRE_FALSE(loaded.load(buffer.data(), buffer.size()));
    }

    SECTION("Truncated inside payload") {
        // Keep only part of the payload after length
        std::uint32_t len;
        std::memcpy(&len, buffer.data(), sizeof(len));
        std::vector<std::uint8_t> partial(buffer.data(), buffer.data() + sizeof(len) + len/2);
        udho::logging::message loaded;
        REQUIRE_FALSE(loaded.load(partial.data(), partial.size()));
    }

    SECTION("Corrupt presence byte for optional") {
        // Find the offset where an optional starts (after all mandatory)
        // Instead of guessing, we can create a record with an optional and then corrupt that byte.
        udho::logging::message with_opt;
        with_opt[local_id::val] = 1;
        with_opt[timestamp::val] = now;
        with_opt[severity::val] = udho::logging::severity::warning;
        with_opt[thread::val] = 4;
        with_opt[process::val] = 5;
        with_opt[subsystem::val] = "a";
        with_opt[message::val] = "b";
        with_opt[file::val] = "c";
        with_opt[function::val] = "d";
        with_opt[line::val] = 6;
        auto buf = with_opt.save();
        // The presence byte for module is after all mandatory fields.
        // Mandatory total size = 4 (len) + 8+8+4+8+4+8 (arithmetic) + (4+1)+(4+1)+(4+1)+(4+1) strings
        // That's 4 + 40 + 4*4 + 4*1? Wait, string lengths: each string has 4-byte length + data. With data="a","b","c","d" each length 1, so each string contributes 4+1=5, four strings =20. So total mandatory = 4+40+20 = 64. So the presence byte is at offset 64.
        // Corrupt it
        if (buf.size() > 64) {
            buf[64] = 0x02; // invalid presence byte (should be 0 or 1)
            udho::logging::message loaded;
            REQUIRE_FALSE(loaded.load(buf.data(), buf.size()));
        }
    }
}

TEST_CASE("Log message binary deserialisation with absent optional fields", "[logging][binary]") {
    auto now = std::chrono::system_clock::now();

    udho::logging::message original;
    original[local_id::val] = 1;
    original[timestamp::val] = now;
    original[severity::val] = udho::logging::severity::warning;
    original[thread::val] = 4;
    original[process::val] = 5;
    original[subsystem::val] = "";
    original[message::val] = "";
    original[file::val] = "";
    original[function::val] = "";
    original[line::val] = 6;
    // No optional fields set

    auto buffer = original.save();
    udho::logging::message loaded;
    REQUIRE(loaded.load(buffer.data(), buffer.size()));

    // Check that optionals are still empty
    REQUIRE_FALSE(loaded[request_id::val].value().has_value());
    REQUIRE_FALSE(loaded[flow_id::val].value().has_value());
    REQUIRE_FALSE(loaded[session_id::val].value().has_value());
    REQUIRE_FALSE(loaded[user_id::val].value().has_value());

    REQUIRE_FALSE(loaded[client::val].value().has_value());
    REQUIRE_FALSE(loaded[host::val].value().has_value());
    REQUIRE_FALSE(loaded[method::val].value().has_value());
    REQUIRE_FALSE(loaded[uri::val].value().has_value());
    REQUIRE_FALSE(loaded[route::val].value().has_value());
    REQUIRE_FALSE(loaded[query::val].value().has_value());
    REQUIRE_FALSE(loaded[agent::val].value().has_value());

    REQUIRE_FALSE(loaded[status_code::val].value().has_value());
    REQUIRE_FALSE(loaded[bytes_sent::val].value().has_value());
    REQUIRE_FALSE(loaded[latency::val].value().has_value());
    REQUIRE_FALSE(loaded[retry_count::val].value().has_value());
}
