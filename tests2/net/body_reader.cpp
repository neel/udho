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
#include <boost/beast/core/buffers_to_string.hpp>

using stream_type     = boost::beast::test::stream;
using executor_type   = typename stream_type::executor_type;

udho::net::types::headers::request make_request(std::initializer_list<std::pair<boost::beast::http::field, std::string>> fields) {
    udho::net::types::headers::request req;
    for (auto& f : fields)
        req.set(f.first, f.second);
    return req;
}

using test_reader = udho::net::protocols::h11::body_reader<boost::beast::flat_buffer, stream_type>;
using buffer_type = boost::beast::flat_buffer;


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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream_in, config);

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
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    CHECK(result_bytes == 5);

    std::string output = boost::beast::buffers_to_string(result_buf.data());

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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));
    auto reader = std::make_shared<test_reader>(req, stream, config);

    buffer_type result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    CHECK(result_buf.size() == leftover.size()+body.size());
    std::string output = boost::beast::buffers_to_string(result_buf.data());
    CHECK(output == leftover+body);
}

TEST_CASE("udho net HTTP 1.1 body reader - zero length", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    stream_type stream(io);
    auto req = make_request({{boost::beast::http::field::content_length, "0"}});

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t bytes) {
            result_ec = ec;
            CHECK(bytes == 0);
            completed = true;
        },
        hbuff
    );

    io.run();
    CHECK(completed);
    CHECK_FALSE(result_ec);
}

TEST_CASE("udho net HTTP 1.1 body reader - content length exceeds limit", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    std::string body = "Too big";
    stream_type stream(io);
    stream.append(body);

    auto req = make_request({{boost::beast::http::field::content_length, std::to_string(body.size())}});
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(3).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff   // limit 3 < 7
    );

    io.run();
    CHECK(completed);
    CHECK(result_ec == boost::system::errc::value_too_large);
}

TEST_CASE("udho net HTTP 1.1 body reader - content exceeds content length", "[net][reader][h11][plain]") {
    boost::asio::io_context io;
    std::string body = "Too big Content";
    stream_type stream(io);
    stream.append(body);

    auto req = make_request({{boost::beast::http::field::content_length, std::to_string(body.size() -8)}});
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    buffer_type result_buf;
    boost::system::error_code result_ec;
    std::size_t bytes_read = 0;
    bool completed = false;

    reader->start(
        [&](buffer_type&& buf, boost::system::error_code ec, std::size_t len) {
            result_buf = std::move(buf);
            result_ec = ec;
            bytes_read = len;
            completed = true;
        },
        hbuff
    );

    io.run();
    CHECK(completed);
    CHECK_FALSE(result_ec);

    CHECK(bytes_read == body.size() -8);

    std::string output = boost::beast::buffers_to_string(result_buf.data());
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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    std::size_t result_bytes = 0;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t bytes) {
            result_ec = ec;
            result_bytes = bytes;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    std::cout << "error: " << result_ec.message() << std::endl;
    CHECK_FALSE(result_ec);
    CHECK(result_bytes == body.size());

    const auto& fields = reader->fields();
    CHECK(fields.size() == 1);
    auto it = fields.find("field1");
    CHECK(it != fields.end());
    CHECK(it->second.is_string());
    CHECK(it->second.string() == "value1");
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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));
    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    std::cout << "error: " << result_ec.message() << std::endl;
    CHECK_FALSE(result_ec);

    const auto& fields = reader->fields();
    CHECK(fields.size() == 2);
    CHECK(fields.find("text1")->second.string() == "Hello");
    CHECK(fields.find("text2")->second.string() == "World");
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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);

    const auto& fields = reader->fields();
    CHECK(fields.size() == 1);
    auto it = fields.find("upload");
    CHECK(it != fields.end());
    CHECK(it->second.is_path());
    udho::utils::filesystem::path file_path = it->second.path();
    CHECK(udho::utils::filesystem::exists(file_path));
    CHECK(udho::utils::filesystem::file_size(file_path) == file_content.size());

    std::ifstream f(file_path.string());
    std::string read_content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(read_content == file_content);

    udho::utils::filesystem::remove(file_path);
}

TEST_CASE("udho net HTTP 1.1 body reader - multipart - missing boundary parameter", "[net][reader][h11][multipart][error]") {
    boost::asio::io_context io;
    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data"},
        {boost::beast::http::field::content_length, "100"}
    });

    stream_type stream(io);
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();
    CHECK(completed);
    CHECK(result_ec == boost::system::errc::protocol_error);
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
    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

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
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    CHECK(result_bytes == body.size());

    std::string output = boost::beast::buffers_to_string(result_buf.data());
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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    buffer_type result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    CHECK(result_buf.size() == body.size());
    std::string output = boost::beast::buffers_to_string(result_buf.data());
    CHECK(output == body);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked - with trailers", "[net][reader][h11][chunked][trailers]") {
    boost::asio::io_context io;
    std::string body = "Data";
    std::string chunked_data = "4\r\nData\r\n0\r\nExpires: never\r\nX-Custom: foo\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({{boost::beast::http::field::transfer_encoding, "chunked"}});
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));
    auto reader = std::make_shared<test_reader>(req, stream, config);

    buffer_type result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    CHECK(result_buf.size() == body.size());
    std::string output = boost::beast::buffers_to_string(result_buf.data());
    CHECK(output == body);
    // Trailers are ignored by the reader; we just ensure no error
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked - zero-length body", "[net][reader][h11][chunked]") {
    boost::asio::io_context io;
    std::string chunked_data = "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({{boost::beast::http::field::transfer_encoding, "chunked"}});
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    buffer_type result_buf;
    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&& buf, boost::system::error_code ec, std::size_t) {
            result_buf = std::move(buf);
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
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

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();
    CHECK(completed);
    CHECK(result_ec == boost::system::errc::protocol_error);
}

TEST_CASE("udho net HTTP 1.1 body reader - error - invalid content-length string", "[net][reader][h11][error]") {
    boost::asio::io_context io;
    auto req = make_request({{boost::beast::http::field::content_length, "abc"}});

    stream_type stream(io);
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();
    CHECK(completed);
    CHECK(result_ec == boost::system::errc::invalid_argument);
}

TEST_CASE("udho net HTTP 1.1 body reader - error - premature EOF", "[net][reader][h11][error]") {
    boost::asio::io_context io;
    std::string body = "Hello";
    stream_type stream(io);
    stream.append(body); // only 5 bytes, but content-length says 10

    auto req = make_request({{boost::beast::http::field::content_length, "10"}});
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();
    CHECK(completed);
    std::cout << "result_ec: " << result_ec.message() << std::endl;
    // Should get eof error or short read error (implementation dependent)
    CHECK(result_ec == boost::asio::error::operation_aborted);
}

TEST_CASE("udho net HTTP 1.1 body reader - error - timeout", "[net][reader][h11][timeout]") {
    boost::asio::io_context io;
    stream_type stream(io);
    // Don't append any data; the read will hang until timeout

    auto req = make_request({{boost::beast::http::field::content_length, "5"}});
    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(1));
    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff // 1 second timeout
    );

    io.run_for(std::chrono::seconds(2));
    CHECK(completed);
    // Should get operation_aborted because the stream was terminated by timeout
    CHECK(result_ec == boost::asio::error::operation_aborted);
}

// -----------------------------------------------------------------------------
// Error and Edge cases end


// Chunked multipart form data tests begin
// -----------------------------------------------------------------------------

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - single field", "[net][reader][h11][chunked][multipart]") {
    boost::asio::io_context io;
    std::string boundary = "----Boundary123";
    std::string field_value = "value1";
    std::string part =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"field1\"\r\n"
                          "\r\n"
        + field_value + "\r\n";
    std::string final_boundary = "--" + boundary + "--\r\n";
    std::string body = part + final_boundary;

    // Chunk the body into one chunk
    std::ostringstream chunked_data;
    chunked_data << std::hex << body.size() << "\r\n" << body << "\r\n"
                 << "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data.str());

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    std::size_t result_bytes = 0;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t bytes) {
            result_ec = ec;
            result_bytes = bytes;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    // Total bytes read should be the chunked representation size, not the raw body size.
    // We can check that the total consumed bytes (from _bytes_consumed) matches the chunked data size.
    // Since we don't expose that, we can verify fields instead.
    const auto& fields = reader->fields();
    CHECK(fields.size() == 1);
    auto it = fields.find("field1");
    CHECK(it != fields.end());
    CHECK(it->second.is_string());
    CHECK(it->second.string() == field_value);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - multiple fields across chunks", "[net][reader][h11][chunked][multipart]") {
    boost::asio::io_context io;
    std::string boundary = "boundary456";
    std::string part1 =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"text1\"\r\n"
                          "\r\n"
                          "Hello\r\n";
    std::string part2 =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"text2\"\r\n"
                          "\r\n"
                          "World\r\n";
    std::string final_boundary = "--" + boundary + "--\r\n";
    std::string body = part1 + part2 + final_boundary;

    // Split into two chunks: first chunk contains part1 and beginning of part2,
    // second chunk contains rest of part2 and final boundary.
    std::size_t split_pos = part1.size() + 5; // arbitrary split inside part2
    std::string chunk1 = body.substr(0, split_pos);
    std::string chunk2 = body.substr(split_pos);

    std::ostringstream chunked_data;
    chunked_data << std::hex << chunk1.size() << "\r\n" << chunk1 << "\r\n"
                 << std::hex << chunk2.size() << "\r\n" << chunk2 << "\r\n"
                 << "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data.str());

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);

    const auto& fields = reader->fields();
    CHECK(fields.size() == 2);
    CHECK(fields.find("text1")->second.string() == "Hello");
    CHECK(fields.find("text2")->second.string() == "World");
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - boundary split across chunks", "[net][reader][h11][chunked][multipart][edge]") {
    boost::asio::io_context io;
    std::string boundary = "split-boundary";
    std::string field_value = "important data";
    std::string part =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"field1\"\r\n"
                          "\r\n"
        + field_value + "\r\n";
    std::string final_boundary = "--" + boundary + "--\r\n";
    std::string body = part + final_boundary;

    // Split right in the middle of the boundary string to test partial boundary detection.
    std::size_t boundary_pos = body.find(boundary);
    std::size_t split_in_boundary = boundary_pos + boundary.size() / 2;
    std::string chunk1 = body.substr(0, split_in_boundary);
    std::string chunk2 = body.substr(split_in_boundary);

    std::ostringstream chunked_data;
    chunked_data << std::hex << chunk1.size() << "\r\n" << chunk1 << "\r\n"
                 << std::hex << chunk2.size() << "\r\n" << chunk2 << "\r\n"
                 << "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data.str());

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);

    const auto& fields = reader->fields();
    CHECK(fields.size() == 1);
    CHECK(fields.find("field1")->second.string() == field_value);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - file upload", "[net][reader][h11][chunked][multipart][upload]") {
    boost::asio::io_context io;
    std::string boundary = "file-boundary";
    std::string file_content = "This is a test file.\nWith two lines.";
    std::string part =
        "--" + boundary + "\r\n"
                          "Content-Disposition: form-data; name=\"upload\"; filename=\"test.txt\"\r\n"
                          "Content-Type: text/plain\r\n"
                          "\r\n"
        + file_content + "\r\n";
    std::string final_boundary = "--" + boundary + "--\r\n";
    std::string body = part + final_boundary;

    // Chunk the body arbitrarily
    std::size_t chunk_size = 10; // small chunks to stress streaming
    std::ostringstream chunked_data;
    for (std::size_t i = 0; i < body.size(); i += chunk_size) {
        std::string chunk = body.substr(i, chunk_size);
        chunked_data << std::hex << chunk.size() << "\r\n" << chunk << "\r\n";
    }
    chunked_data << "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data.str());

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);

    const auto& fields = reader->fields();
    CHECK(fields.size() == 1);
    auto it = fields.find("upload");
    CHECK(it != fields.end());
    CHECK(it->second.is_path());
    udho::utils::filesystem::path file_path = it->second.path();
    CHECK(udho::utils::filesystem::exists(file_path));
    CHECK(udho::utils::filesystem::file_size(file_path) == file_content.size());

    std::ifstream f(file_path.string());
    std::string read_content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(read_content == file_content);

    udho::utils::filesystem::remove(file_path);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - zero parts", "[net][reader][h11][chunked][multipart]") {
    boost::asio::io_context io;
    std::string boundary = "empty-boundary";
    std::string body = "--" + boundary + "--\r\n"; // no parts, just final boundary

    std::ostringstream chunked_data;
    chunked_data << std::hex << body.size() << "\r\n" << body << "\r\n"
                 << "0\r\n\r\n";

    stream_type stream(io);
    stream.append(chunked_data.str());

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK_FALSE(result_ec);
    CHECK(reader->fields().empty());
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - malformed chunk header", "[net][reader][h11][chunked][multipart][error]") {
    boost::asio::io_context io;
    std::string boundary = "error-boundary";
    std::string body = "--" + boundary + "--\r\n";
    std::string chunked_data = "ZZ\r\n" + body + "\r\n0\r\n\r\n"; // invalid hex size

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;

    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(30));

    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run();

    CHECK(completed);
    CHECK(result_ec == boost::system::errc::protocol_error);
}

TEST_CASE("udho net HTTP 1.1 body reader - chunked multipart - missing final chunk", "[net][reader][h11][chunked][multipart][error]") {
    boost::asio::io_context io;
    std::string boundary = "missing-final";
    std::string body = "--" + boundary + "--\r\n";
    std::string chunked_data = std::to_string(body.size()) + "\r\n" + body + "\r\n"; // no final 0 chunk

    stream_type stream(io);
    stream.append(chunked_data);

    auto req = make_request({
        {boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary},
        {boost::beast::http::field::transfer_encoding, "chunked"}
    });

    boost::beast::flat_buffer hbuff;
    udho::net::detail::body_parser_config config;
    config.total_content_limit(1024).total_timeout(std::chrono::seconds(5));
    auto reader = std::make_shared<test_reader>(req, stream, config);

    boost::system::error_code result_ec;
    bool completed = false;

    reader->start(
        [&](buffer_type&&, boost::system::error_code ec, std::size_t) {
            result_ec = ec;
            completed = true;
        },
        hbuff
    );

    io.run_for(std::chrono::seconds(10)); // should time out or get eof
    // Expect either eof or operation_aborted (timeout) – depends on stream behavior
    CHECK(completed);
    CHECK(result_ec != boost::system::errc::success);
}

// -----------------------------------------------------------------------------
// Chunked multipart form data tests end

