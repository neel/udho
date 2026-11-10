#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/net/channel.h>
#include <udho/utils/misc.h>

namespace misc = udho::utils::misc;

TEST_CASE("socket stream termination is deferred", "[net][stream-termination]") {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket stream(io);

    bool completed = false;
    misc::detail::stream_termination<boost::asio::ip::tcp::socket>::async_apply(stream,
        [&](boost::system::error_code) {
            completed = true;
        }
    );

    CHECK_FALSE(completed);
    io.run();
    CHECK(completed);
    CHECK_FALSE(stream.is_open());
}

TEST_CASE("Beast test stream termination is deferred", "[net][stream-termination]") {
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);

    bool completed = false;
    misc::detail::stream_termination<boost::beast::test::stream>::async_apply(stream,
        [&](boost::system::error_code error) {
            CHECK_FALSE(error);
            completed = true;
        }
    );

    CHECK_FALSE(completed);
    io.run();
    CHECK(completed);
}

TEST_CASE("SSL stream termination closes its next layer", "[net][stream-termination][ssl]") {
    boost::asio::io_context io;
    boost::asio::ssl::context context(boost::asio::ssl::context::tls_client);
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream(io, context);

    (void)misc::native_handle(stream);
    CHECK(misc::detail::stream_available<decltype(stream)>::apply(stream) == 0);

    bool completed = false;
    misc::detail::stream_termination<decltype(stream)>::async_apply(stream,
        [&](boost::system::error_code) {
            completed = true;
        }
    );

    CHECK_FALSE(completed);
    io.run();
    CHECK(completed);
    CHECK_FALSE(stream.next_layer().is_open());
}

TEST_CASE("HTTPS channel reports handshake failure with an unopened socket", "[net][channel][ssl]") {
    boost::asio::io_context io;
    boost::asio::ip::tcp::endpoint endpoint;
    udho::net::channel<udho::net::transports::https> channel(std::move(endpoint));
    udho::net::transports::https::socket_type socket(io);

    bool completed = false;
    channel.async_open(std::move(socket),
        [&](boost::system::error_code error, udho::net::transports::https::stream_type stream) {
            CHECK(error);
            channel.abort(stream);
            completed = true;
        }
    );

    CHECK_FALSE(completed);
    io.run();
    CHECK(completed);
}
