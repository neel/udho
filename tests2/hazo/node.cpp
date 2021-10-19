#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/node/node.h>
#include <string>

namespace h = udho::hazo;

TEST_CASE("node", "[hazo]") {

    SECTION( "data can be retrieved and modified by position" ) {
        h::node<int, std::string, double, char, std::string, int> chain(42, "Fourty Two", 4.2, '!', "Twenty Four", 24);

        REQUIRE(chain.data<0>() == 42);
        REQUIRE(chain.data<1>() == "Fourty Two");
        REQUIRE(chain.data<2>() == 4.2);
        REQUIRE(chain.data<3>() == '!');
        REQUIRE(chain.data<4>() == "Twenty Four");
        REQUIRE(chain.data<5>() == 24);

        chain.data<0>() = 24;
        chain.data<1>() = "Twenty Four";
        chain.data<2>() = 2.4;
        chain.data<3>() = '?';
        chain.data<4>() = "Fourty Two";
        chain.data<5>() = 42;

        REQUIRE(chain.data<0>() == 24);
        REQUIRE(chain.data<1>() == "Twenty Four");
        REQUIRE(chain.data<2>() == 2.4);
        REQUIRE(chain.data<3>() == '?');
        REQUIRE(chain.data<4>() == "Fourty Two");
        REQUIRE(chain.data<5>() == 42);
    }

    SECTION( "data can be retrieved and modified by type and position" ) {
        h::node<int, std::string, double, char, std::string, int> chain(42, "Fourty Two", 4.2, '!', "Twenty Four", 24);

        REQUIRE(chain.data<int, 0>() == 42);
        REQUIRE(chain.data<std::string, 0>() == "Fourty Two");
        REQUIRE(chain.data<double, 0>() == 4.2);
        REQUIRE(chain.data<char, 0>() == '!');
        REQUIRE(chain.data<std::string, 1>() == "Twenty Four");
        REQUIRE(chain.data<int, 1>() == 24);

        chain.data<int, 0>() = 24;
        chain.data<std::string, 0>() = "Twenty Four";
        chain.data<double, 0>() = 2.4;
        chain.data<char, 0>() = '?';
        chain.data<std::string, 1>() = "Fourty Two";
        chain.data<int, 1>() = 42;

        REQUIRE(chain.data<int, 0>() == 24);
        REQUIRE(chain.data<std::string, 0>() == "Twenty Four");
        REQUIRE(chain.data<double, 0>() == 2.4);
        REQUIRE(chain.data<char, 0>() == '?');
        REQUIRE(chain.data<std::string, 1>() == "Fourty Two");
        REQUIRE(chain.data<int, 1>() == 42);
    }
}