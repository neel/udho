#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/hana.h>
#include <udho/hazo/node.h>
#include <string>

namespace h = udho::hazo;

HAZO_ELEMENT(first_name, std::string);
HAZO_ELEMENT(last_name, std::string);
HAZO_ELEMENT(age, std::size_t);
HAZO_ELEMENT_HANA(name, std::string);
HAZO_ELEMENT_HANA(country, std::string);

TEST_CASE("element", "[hazo]") {
    first_name f("Neel");
    last_name l("Basu");
    age a(32);
    name n("Neel Basu");
    country c("India");
    
    REQUIRE(f == f);
    REQUIRE(l == l);
    REQUIRE(a == a);
    REQUIRE(f == "Neel");
    REQUIRE(l == "Basu");
    REQUIRE(a == 32);
    REQUIRE(n == "Neel Basu");
    REQUIRE(c == "India");

    GIVEN("a node constructed with elements as well as pod types") {
        h::node<int, first_name, last_name, age, std::string, double, name, country, char> node(42, "Neel", "Basu", 32, "Hello World", 3.14, "Neel Basu", "India", '!');

        THEN( "elements can be retrieved through the element method" ) {
            REQUIRE(node.element(first_name::val) == "Neel");
            REQUIRE(node.element(last_name::val) == "Basu");
            REQUIRE(node.element(age::val) == 32);
            REQUIRE(node.element(name::val) == "Neel Basu");
            REQUIRE(node.element(country::val) == "India");
        }
    }
}