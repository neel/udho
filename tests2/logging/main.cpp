#include <iostream>
#include <udho/logging/setup.h>
#include <udho/logging/macros.h>

using logger_type = udho::logging::setup<udho::logging::rotating_file>;

int main() {
    if(!logger_type::apply()) return 0;

    using namespace udho::logging::params;
    UDHO_LOG_INFO("http", "Request received", request_id("req-12345"), client_ip("192.168.1.100"));

    logger_type::stop();
    return 0;
}
