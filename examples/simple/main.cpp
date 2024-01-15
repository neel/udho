#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <udho/url/url.h>
#include <boost/asio/io_context.hpp>
#include "manifest.h"

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using http_listener   = udho::net::listener<http_connection>;

int main(int, char**){
    simple::manifest manifest;
    auto router = manifest.router();

    std::cout << router << std::endl;

    boost::asio::io_service io;
    auto server = udho::net::server<http_listener>(io, router, 9000);
    server.run();

    io.run();

    return 0;
}
