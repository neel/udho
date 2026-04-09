#include <iostream>
#include <udho/logging/setup.h>
#include <udho/logging/macros.h>

int main() {
    auto logger = udho::logging::start<udho::logging::rotating_file>();

    assert(logger.running());

    std::cout << "Consumer PID: " << logger.pid() << std::endl;

    using namespace udho::logging::params;
    UDHO_LOG_INFO("http", "Request received", method(boost::beast::http::verb::get), request_id("req-12345"), client(boost::asio::ip::make_address("192.168.1.100")), session_id(udho::session::id()));
    return 0;
}
