#include <iostream>
#include <udho/logging/setup.h>
#include <udho/logging/macros.h>

using logger_type = udho::logging::setup<udho::logging::rotating_file>;

int main() {
    if(!logger_type::apply()) return 0;

    assert(logger_type::running());

    std::cout << "Consumer PID: " << logger_type::pid() << std::endl;

    using namespace udho::logging::params;
    UDHO_LOG_INFO("http", "Request received", method(boost::beast::http::verb::get), request_id("req-12345"), client(boost::asio::ip::make_address("192.168.1.100")), session_id(udho::session::id()));

    logger_type::stop();
    return 0;
}
