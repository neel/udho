#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <string>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/common.h>
#include <type_traits>


TEST_CASE("udho network", "[net]") {
    CHECK(1 == 1);
}
