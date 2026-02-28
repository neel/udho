#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/net/ostream.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/utils/encoding.h>
#include <iostream>
#include <boost/thread.hpp>
#include <udho/net/protocols/http.h>

using stream_type     = boost::beast::test::stream;
using executor_type   = typename stream_type::executor_type;

udho::net::types::headers::request make_request(std::initializer_list<std::pair<boost::beast::http::field, std::string>> fields) {
    udho::net::types::headers::request req;
    for (auto& f : fields)
        req.set(f.first, f.second);
    return req;
}

using test_reader = udho::net::protocols::h11_body_reader<boost::beast::flat_buffer, stream_type>;


// Plain body tests begin
// -----------------------------------------------------------------------------

TEST_CASE("udho net HTTP 1.1 body reader - plain body", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    std::string plain_body = "Hello";

    stream_type stream_in(io);

    auto req = make_request({
        {boost::beast::http::field::content_length, "5"},
        {boost::beast::http::field::content_type, "text/plain"}
    });

    boost::beast::flat_buffer hbuff;          // empty header buffer

    stream_in.append(plain_body);
    auto reader = std::make_shared<test_reader>(req, stream_in);

    using buffer_type = boost::beast::flat_buffer;

    buffer_type result_buf;
    boost::system::error_code result_ec;
    std::size_t result_bytes = 0;
    bool completed = false;

    reader->start(
        [&](buffer_type&& buf, boost::system::error_code ec, std::size_t bytes) {
            result_buf = std::move(buf);
            result_ec = ec;
            result_bytes = bytes;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_bytes == 5);

    udho::utils::string_view output(static_cast<const char *>(result_buf.data().data()), 5);

    CHECK(output == plain_body);
}

TEST_CASE("udho net HTTP 1.1 body reader - with leftover in header buffer", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    std::string body = "Hello";
    std::string leftover = "Extra";

    stream_type stream(io);
    stream.append(body);

    boost::beast::flat_buffer hbuff;
    hbuff.commit(boost::asio::buffer_copy(hbuff.prepare(leftover.size()), boost::asio::buffer(leftover)));

    auto req = make_request({
        {boost::beast::http::field::content_length, std::to_string(leftover.size() + body.size())}
    });

    auto reader = std::make_shared<test_reader>(req, stream);

    boost::beast::flat_buffer result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_buf.size() == leftover.size()+body.size());
    CHECK(std::string(static_cast<const char*>(result_buf.data().data()), result_buf.size()) == leftover+body);
}

TEST_CASE("udho net HTTP 1.1 body reader - zero length", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    stream_type stream(io);
    auto req = make_request({{boost::beast::http::field::content_length, "0"}});

    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t bytes) {
            result_ec = ec;
            CHECK(bytes == 0);
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();
    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
}

TEST_CASE("udho net HTTP 1.1 body reader - content length exceeds limit", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    std::string body = "Too big";
    stream_type stream(io);
    stream.append(body);

    auto req = make_request({{boost::beast::http::field::content_length, std::to_string(body.size())}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 3   // limit 3 < 7
    );

    io.run();
    REQUIRE(completed);
    REQUIRE(result_ec == boost::system::errc::value_too_large);
}

TEST_CASE("udho net HTTP 1.1 body reader - content exceeds content length", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    std::string body = "Too big Content";
    stream_type stream(io);
    stream.append(body);

    auto req = make_request({{boost::beast::http::field::content_length, std::to_string(body.size() -8)}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::beast::flat_buffer result_buf;
    boost::system::error_code result_ec;
    std::size_t bytes_read = 0;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&& buf, boost::system::error_code ec, std::size_t len) {
            result_buf = std::move(buf);
            result_ec = ec;
            bytes_read = len;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();
    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);

    CHECK(bytes_read == body.size() -8);

    udho::utils::string_view output(static_cast<const char*>(result_buf.data().data()), bytes_read);
    CHECK(output == "Too big");
}

// -----------------------------------------------------------------------------
// Plain body tests end


// Multipart form data tests (plain, non-chunked) begin
// -----------------------------------------------------------------------------
TEST_CASE("udho net HTTP 1.1 body reader - multipart - single field", "[net][reader][h11][multipart]") {
    boost::asio::io_context io;
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"field1\"\r\n"
                          "\r\n"
                          "value1\r\n"
                          "--" + boundary + "--\r\n";

    stream_type stream(io);
    stream.append(body);

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::content_length, std::to_string(body.size())}
    });

    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    std::size_t result_bytes = 0;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t bytes) {
            result_ec = ec;
            result_bytes = bytes;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_bytes == body.size());

    const auto& fields = reader->fields();
    REQUIRE(fields.size() == 1);
    auto it = fields.find("field1");
    REQUIRE(it != fields.end());
    CHECK(std::holds_alternative<std::string>(it->second));
    CHECK(std::get<std::string>(it->second) == "value1");
}

TEST_CASE("udho net HTTP 1.1 body reader - multipart - multiple fields", "[net][reader][h11][multipart]") {
    boost::asio::io_context io;
    std::string boundary = "boundary123";
    std::string body =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"text1\"\r\n"
                          "\r\n"
                          "Hello\r\n"
                          "--" + boundary + "\r\n"
                     "Content-Disposition: form-data; name=\"text2\"\r\n"
                     "\r\n"
                     "World\r\n"
                     "--" + boundary + "--\r\n";

    stream_type stream(io);
    stream.append(body);

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::content_length, std::to_string(body.size())}
    });

    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
        );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);

    const auto& fields = reader->fields();
    REQUIRE(fields.size() == 2);
    CHECK(std::get<std::string>(fields.find("text1")->second) == "Hello");
    CHECK(std::get<std::string>(fields.find("text2")->second) == "World");
}

TEST_CASE("udho net HTTP 1.1 body reader - multipart - file upload", "[net][reader][h11][multipart][upload]") {
    boost::asio::io_context io;
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string file_content = "This is a test file.\nWith two lines.";
    std::string body =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"upload\"; filename=\"test.txt\"\r\n"
                          "Content-Type: text/plain\r\n"
                          "\r\n"
        + file_content + "\r\n"
                         "--" + boundary + "--\r\n";

    stream_type stream(io);
    stream.append(body);

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::content_length, std::to_string(body.size())}
    });

    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);

    const auto& fields = reader->fields();
    REQUIRE(fields.size() == 1);
    auto it = fields.find("upload");
    REQUIRE(it != fields.end());
    REQUIRE(std::holds_alternative<boost::filesystem::path>(it->second));
    boost::filesystem::path file_path = std::get<boost::filesystem::path>(it->second);
    REQUIRE(boost::filesystem::exists(file_path));
    CHECK(boost::filesystem::file_size(file_path) == file_content.size());

    std::ifstream f(file_path.string());
    std::string read_content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(read_content == file_content);

    boost::filesystem::remove(file_path);
}

TEST_CASE("udho net HTTP 1.1 body reader - multipart - missing boundary parameter", "[net][reader][h11][multipart][error]") {
    boost::asio::io_context io;
    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data"},
        {boost::beast::http::field::content_length, "100"}
    });

    stream_type stream(io);
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();
    REQUIRE(completed);
    REQUIRE(result_ec == boost::system::errc::protocol_error);
}

// -----------------------------------------------------------------------------
// Multipart form data tests (plain, non-chunked) end

// Chunked transfer encoding tests begin
// -----------------------------------------------------------------------------
TEST_CASE("udho net HTTP 1.1 body reader - chunked - single chunk", "[net][reader][h11][chunked]") {
    boost::asio::io_context io;
    std::string body = "Hello";
    std::string chunked_data = "5\r\nHello\r\n0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::beast::flat_buffer result_buf;
    boost::system::error_code result_ec;
    std::size_t result_bytes = 0;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&& buf, boost::system::error_code ec, std::size_t bytes) {
            result_buf = std::move(buf);
            result_ec = ec;
            result_bytes = bytes;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_bytes == body.size());

    std::string output(static_cast<const char*>(result_buf.data().data()), result_buf.size());
    CHECK(output == body);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked - multiple chunks", "[net][reader][h11][chunked]") {
    boost::asio::io_context io;
    std::string body = "HelloWorld";
    std::string chunked_data = "5\r\nHello\r\n5\r\nWorld\r\n0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({{boost::beast::http::field::transfer_encoding, "chunked"}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::beast::flat_buffer result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_buf.size() == body.size());
    CHECK(std::string(static_cast<const char*>(result_buf.data().data()), result_buf.size()) == body);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked - with trailers", "[net][reader][h11][chunked][trailers]") {
    boost::asio::io_context io;
    std::string body = "Data";
    std::string chunked_data = "4\r\nData\r\n0\r\nExpires: never\r\nX-Custom: foo\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({{boost::beast::http::field::transfer_encoding, "chunked"}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::beast::flat_buffer result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
        );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_buf.size() == body.size());
    CHECK(std::string(static_cast<const char*>(result_buf.data().data()), result_buf.size()) == body);
    // Trailers are ignored by the reader; we just ensure no error
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked - zero-length body", "[net][reader][h11][chunked]") {
    boost::asio::io_context io;
    std::string chunked_data = "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({{boost::beast::http::field::transfer_encoding, "chunked"}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::beast::flat_buffer result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();

    REQUIRE(completed);
    REQUIRE_FALSE(result_ec);
    CHECK(result_buf.size() == 0);
}

// -----------------------------------------------------------------------------
// Chunked transfer encoding tests end


// Error and Edge cases begin
// -----------------------------------------------------------------------------
TEST_CASE("udho net HTTP 1.1 body reader - error - both content-length and chunked", "[net][reader][h11][error]") {
    boost::asio::io_context io;
    auto req = make_request({
        {boost::beast::http::field::content_length, "5"},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    stream_type stream(io);
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();
    REQUIRE(completed);
    REQUIRE(result_ec == boost::system::errc::protocol_error);
}

TEST_CASE("udho net HTTP 1.1 body reader - error - invalid content-length string", "[net][reader][h11][error]") {
    boost::asio::io_context io;
    auto req = make_request({{boost::beast::http::field::content_length, "abc"}});

    stream_type stream(io);
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
        );

    io.run();
    REQUIRE(completed);
    REQUIRE(result_ec == boost::system::errc::invalid_argument);
}

TEST_CASE("udho net HTTP 1.1 body reader - error - premature EOF", "[net][reader][h11][error]") {
    boost::asio::io_context io;
    std::string body = "Hello";
    stream_type stream(io);
    stream.append(body); // only 5 bytes, but content-length says 10

    auto req = make_request({{boost::beast::http::field::content_length, "10"}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 30, 1024
    );

    io.run();
    REQUIRE(completed);
    std::cout << "result_ec: " << result_ec.message() << std::endl;
    // Should get eof error or short read error (implementation dependent)
    REQUIRE(result_ec == boost::asio::error::operation_aborted);
}

TEST_CASE("udho net HTTP 1.1 body reader - error - timeout", "[net][reader][h11][timeout]") {
    boost::asio::io_context io;
    stream_type stream(io);
    // Don't append any data; the read will hang until timeout

    auto req = make_request({{boost::beast::http::field::content_length, "5"}});
    boost::beast::flat_buffer hbuff;
    auto reader = std::make_shared<test_reader>(req, stream);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](boost::beast::flat_buffer&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff, 1, 1024 // 1 second timeout
        );

    io.run_for(std::chrono::seconds(2));
    REQUIRE(completed);
    // Should get operation_aborted because the stream was terminated by timeout
    REQUIRE(result_ec == boost::asio::error::operation_aborted);
}
// -----------------------------------------------------------------------------
// Error and Edge cases end
