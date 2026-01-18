#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/manifold/components/stream.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/utils/encoding.h>
#include <iostream>
#include <boost/thread.hpp>

using stream_type     = boost::beast::test::stream;
using executor_type   = typename stream_type::executor_type;
using strand_type     = boost::asio::strand<executor_type>;
using buffered_stream = udho::manifold::detail::basic_buffered_ostream<stream_type>;
using queued_stream   = udho::manifold::detail::basic_queued_ostream<stream_type>;

TEST_CASE("udho manifold basic_stream", "[manifold][stream]") {
    boost::asio::io_context io;
    stream_type server(io);
    stream_type client(io);

    strand_type strand(server.get_executor());

    server.connect(client);

    SECTION("buffered stream") {
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};
        buffered_stream buffered_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == 6);
            }
        );

        buffered_stream.write(std::string("ABC"));
        buffered_stream.write(std::string("DEF"));
        buffered_stream.async_flush();
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }
        threads.join_all();

        CHECK(is_completed);
        std::cout << "output: " << client.str() << std::endl;
    }

    SECTION("queued stream plain - write while pumping") {
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};
        queued_stream queued_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == 17);
            }
        );

        const static std::string static_str = "0123456"; // static embedded assets such as js or images etc..

        queued_stream.write(static_str);                 // pumping started
        queued_stream.write(std::string("ABC"));         // write while pumping
        queued_stream.write(std::string("ABC"));         // write while pumping
        queued_stream.write(std::string("DEFG"));        // write while pumping
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }
        threads.join_all();

        CHECK(!is_completed);
        CHECK(udho::utils::encode::base16(client.str()) == "3031323334353641424341424344454647");

        io.restart();
        queued_stream.finish();
        boost::thread_group threads2;
        for(std::size_t i = 0; i < 4; ++i) {
            threads2.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }
        threads2.join_all();

        CHECK(is_completed);
        std::cout << "output: " << client.str() << std::endl;
    }

    SECTION("queued stream - write while pumping") {
        std::cout << std::endl;
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::chunked};
        queued_stream queued_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == /*size ascii 3+crlf*/3+ /*ABC*/3 + /*crlf*/2 + /*size ascii 4+crlf*/3+ /*DEFG*/4 + /*crlf*/2 + /*crlf+0+crlf*/5);
            }
        );

        queued_stream.write(std::string("ABC"));        // pumping started
        queued_stream.write(std::string("DEFG"));       // write while pumping
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }
        threads.join_all();

        CHECK(!is_completed);
        CHECK(udho::utils::encode::base16(client.str()) == "330d0a4142430d0a340d0a444546470d0a");

        io.restart();
        queued_stream.finish();
        boost::thread_group threads2;
        for(std::size_t i = 0; i < 4; ++i) {
            threads2.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }
        threads2.join_all();

        CHECK(is_completed);
        CHECK(udho::utils::encode::base16(client.str()) == "330d0a4142430d0a340d0a444546470d0a300d0a0d0a");
    }
}


TEST_CASE("udho manifold stream", "[manifold][stream]") {
    boost::asio::io_context io;
    stream_type server(io);
    stream_type client(io);

    server.connect(client);

    udho::net::types::headers::response headers;
    udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};
    bool is_completed = false;
    udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
       [&](boost::system::error_code ec, std::size_t bytes_written) {
           is_completed = true;
           INFO("error: " << ec.message());
           CHECK_FALSE(ec);
       }
    );

    const static std::string static_str = "0123456"; // static embedded assets such as js or images etc..
    ostream.buffering(false);
    ostream.write(static_str);                 // pumping started
    ostream.write(std::string("ABC"));         // write while pumping
    ostream.write(std::string("ABC"));         // write while pumping
    ostream.write(std::string("DEFG"));        // write while pumping

    boost::thread_group threads;
    for(std::size_t i = 0; i < 4; ++i) {
        threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
    }
    threads.join_all();

    CHECK(is_completed);
    std::cout << "output: " << client.str() << std::endl;

    // io.restart();
    // ostream.finish();
    // io.run();

    // CHECK(is_completed);
    // std::cout << "2. output: " << client.str() << std::endl;
}
