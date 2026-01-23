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

struct TestType {
    int value;
    friend std::ostream& operator<<(std::ostream& os, const TestType& t) {
        return os << "TestType:" << t.value;
    }
};

template <std::size_t N>
struct multithreaded_io{
    using executor_type = boost::asio::io_context::executor_type;
    using guard_type    = boost::asio::executor_work_guard<executor_type>;

    multithreaded_io(boost::asio::io_context& io): _io(io), _guard(boost::asio::make_work_guard(io)) {
        run();
    }

    void restart() {
        _io.restart();
    }

    void run() {
        for(std::size_t i = 0; i < N; ++i) {
            _threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(_io)));
        }
    }

    void reset() {
        _guard.reset();
    }

    void join() {
        reset();
        _threads.join_all();
    }

private:
    boost::asio::io_context& _io;
    boost::thread_group      _threads;
    guard_type               _guard;
};

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
        io.restart();
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
        io.restart();
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
        io.restart();
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


TEST_CASE("udho manifold composite stream", "[manifold][stream]") {
    boost::asio::io_context io;
    stream_type server(io);
    stream_type client(io);

    SECTION("happy path") {
        io.restart();
        auto guard = boost::asio::make_work_guard(io);
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};
        bool is_completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
           [&](boost::system::error_code ec, std::size_t bytes_written) {
               is_completed = true;
               std::cout << "bytes_written " << bytes_written << std::endl;
               INFO("error: " << ec.message());
               CHECK_FALSE(ec);
           }
        );

        const static std::string static_str = "0123456"; // static embedded assets such as js or images etc..
        ostream.write(std::string("ABC"));         // goes to buffered stream
        ostream.disable_buffering();
        ostream.write(static_str);                 // pumping started
        ostream.write(std::string("ABC"));         // write while pumping
        ostream.write(std::string("DEFG"));        // write while pumping

        guard.reset();
        threads.join_all();

        CHECK(!is_completed);
        std::cout << "output: " << client.str() << std::endl;

        io.restart();
        ostream.finish();
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread(std::bind(&boost::asio::io_context::run, std::ref(io)));
        }
        threads.join_all();

        CHECK(is_completed);
        std::cout << "output: " << client.str() << std::endl;
    }

    SECTION("composite stream - write after finish should not crash") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        int callback_count = 0;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
           [&](boost::system::error_code ec, std::size_t) {
               callback_count++;
               CHECK_FALSE(ec);
           }
        );

        ostream.write(std::string("ABC"));
        ostream.finish();

        ostream.write(std::string("SHOULD_NOT_APPEAR"));

        mio.join();

        CHECK(callback_count == 1);
        CHECK(client.str().find("SHOULD_NOT_APPEAR") == std::string::npos);
    }

    SECTION("composite stream - multiple finish calls are idempotent") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        int callback_count = 0;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
           [&](boost::system::error_code ec, std::size_t) {
               callback_count++;
               CHECK_FALSE(ec);
           }
        );

        ostream.write(std::string("ABC"));
        ostream.finish();
        ostream.finish(); // Second finish should not cause issues
        ostream.finish(); // Third finish

        mio.join();

        CHECK(callback_count == 1); // Only one completion callback
    }

    SECTION("composite stream - empty writes") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
           [&](boost::system::error_code ec, std::size_t bytes) {
                completed = true;
                CHECK_FALSE(ec);
                CHECK(bytes > 0);
           }
        );

        // Empty string write
        ostream.write(std::string(""));
        ostream.write(udho::utils::string_view(""));
        ostream.disable_buffering();
        ostream.write(std::string("")); // Empty in queued mode
        ostream.finish();

        mio.join();

        CHECK(completed);
        // Should still have headers
        CHECK(client.str().find("HTTP/1.1 200 OK") != std::string::npos);
    }

    SECTION("composite stream - large writes that exceed typical buffer") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        const std::size_t large_size = 1024 * 1024; // 1MB
        std::string large_data(large_size, 'X');

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
            [&](boost::system::error_code ec, std::size_t bytes) {
                completed = true;
                CHECK_FALSE(ec);
                CHECK(bytes > large_size);
            }
        );

        ostream.disable_buffering();
        ostream.write(large_data);
        ostream.finish();

        mio.join();

        CHECK(completed);
        CHECK(client.str().size() > large_size);
    }

    SECTION("composite stream - concurrent writes from multiple threads") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        std::atomic<int> write_count{0};
        std::atomic<int> callback_count{0};
        const int total_writes = 100;

        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
            [&](boost::system::error_code ec, std::size_t) {
                callback_count++;
                CHECK_FALSE(ec);
            }
        );

        ostream.disable_buffering();

        // Multiple threads writing concurrently
        boost::thread_group writers;
        for (int i = 0; i < 4; ++i) {
            writers.create_thread([&]() {
                for (int j = 0; j < total_writes/4; ++j) {
                    ostream.write(std::string("ThreadWrite"));
                    write_count++;
                }
            });
        }

        writers.join_all();
        ostream.finish();

        mio.join();

        CHECK(write_count == total_writes);
        CHECK(callback_count == 1);
    }

    SECTION("composite stream - rapid disable_buffering calls") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
            [&](boost::system::error_code ec, std::size_t) {
                completed = true;
                CHECK_FALSE(ec);
            }
        );

        // Call disable_buffering multiple times (should be idempotent after first)
        ostream.disable_buffering();
        ostream.disable_buffering(); // Second call should be no-op
        ostream.disable_buffering(); // Third call

        ostream.write(std::string("Test"));
        ostream.finish();

        mio.join();

        CHECK(completed);
    }

    SECTION("composite stream with chunked encoding - buffered then queued") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::chunked};

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
        [&](boost::system::error_code ec, std::size_t bytes) {
                completed = true;
                CHECK_FALSE(ec);
                // Calculate expected bytes: headers + chunked data + terminal chunk
                CHECK(bytes > 0);
            }
        );

        // Write in buffered mode
        ostream.write(std::string("Buffered"));
        ostream.disable_buffering();

        // Write in queued mode
        ostream.write(std::string("Queued"));
        ostream.write(std::string("Data"));

        ostream.finish();

        mio.join();

        CHECK(completed);
        std::string output = client.str();
        CHECK(output.find("HTTP/1.1 200 OK") != std::string::npos);
        CHECK(output.find("0\r\n\r\n") != std::string::npos); // Terminal chunk
    }

    SECTION("composite stream with chunked encoding - only buffered") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::chunked};

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
            [&](boost::system::error_code ec, std::size_t) {
                completed = true;
                CHECK_FALSE(ec);
            }
        );

        ostream.write(std::string("All buffered"));
        ostream.finish(); // Finish without disabling buffering

        mio.join();

        CHECK(completed);
        std::string output = client.str();
        CHECK(output.find("c\r\nAll buffered\r\n") != std::string::npos); // "All buffered" is 12 chars = C in hex
        CHECK(output.find("0\r\n\r\n") != std::string::npos);
    }



    SECTION("composite stream - mixed write types (string, string_view, char*)") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
            [&](boost::system::error_code ec, std::size_t) {
                completed = true;
                CHECK_FALSE(ec);
            }
        );

        // Test all write overloads
        ostream.write(std::string("String"));                    // std::string&&

        std::string temp = "Temporary";
        ostream.write(temp);                                     // udho::utils::string_view

        const char* cstr = "CString";
        ostream.write(cstr, strlen(cstr));                      // const char*, size_t

        ostream.write(TestType{42});

        ostream.disable_buffering();
        ostream.finish();

        mio.join();

        CHECK(completed);
        std::string output = client.str();
        CHECK(output.find("String") != std::string::npos);
        CHECK(output.find("Temporary") != std::string::npos);
        CHECK(output.find("CString") != std::string::npos);
        CHECK(output.find("TestType:42") != std::string::npos);
    }

    SECTION("composite stream - sequence of many small writes") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        const int num_writes = 1000;
        std::atomic<int> writes_done{0};

        bool completed = false;
        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
           [&](boost::system::error_code ec, std::size_t) {
               completed = true;
               CHECK_FALSE(ec);
           }
        );

        ostream.disable_buffering();

        // Many small writes
        for (int i = 0; i < num_writes; ++i) {
            ostream.write(std::string("W")); // Single character
            writes_done++;
        }

        ostream.finish();

        mio.join();

        CHECK(writes_done == num_writes);
        CHECK(completed);
        std::string output = client.str();
        CHECK(output.size() > num_writes);
    }

    SECTION("composite stream - stress test with mixed operations") {
        multithreaded_io<4> mio(io);

        server.connect(client);
        udho::net::types::headers::response headers;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::chunked};

        std::atomic<int> operations_completed{0};
        const int total_operations = 50;

        udho::manifold::basic_ostream<stream_type> ostream(server, headers, enc,
            [&](boost::system::error_code ec, std::size_t) {
                operations_completed++;
                CHECK_FALSE(ec);
            }
        );

        // Mix of operations
        for (int i = 0; i < total_operations; ++i) {
            if (i == 10) ostream.disable_buffering();
            if (i == 25) {
                // Write a larger chunk
                ostream.write(std::string(1000, 'X'));
            } else {
                ostream.write(std::to_string(i));
            }

            // Occasionally call finish and check it's handled
            if (i == total_operations - 1) {
                ostream.finish();
            }
        }

        mio.join();

        // Should complete without crashing
        CHECK(operations_completed == 1); // Only one completion callback
    }
}
