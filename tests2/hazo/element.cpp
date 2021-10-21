#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/hana.h>
#include <udho/hazo/node.h>
#include <string>

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

    GIVEN( "a set of hazo element and hazo hana element" ) {
        THEN( "comparable with itself operator==()" ) {
            REQUIRE(f == f);
            REQUIRE(l == l);
            REQUIRE(a == a);
            REQUIRE(n == n);
            REQUIRE(c == c);
        }

        THEN( "assignable by value" ) {
            f = "Neel";
            l = "Basu";
            a = 32;
            n = "Neel Basu";
            c = "India";
        }

        THEN( "comparable with its value operator==()" ) {
            REQUIRE(f == "Neel");
            REQUIRE(l == "Basu");
            REQUIRE(a == 32);
            REQUIRE(n == "Neel Basu");
            REQUIRE(c == "India");
        }

        THEN( "key() returns appropriately" ) {
            using namespace boost::hana::literals;
            REQUIRE(f.key() == first_name::val);
            REQUIRE(l.key() == last_name::val);
            REQUIRE(a.key() == age::val);
            REQUIRE(std::is_same_v<decltype(n.key()), decltype("name"_s)>);
            REQUIRE(std::is_same_v<decltype(c.key()), decltype("country"_s)>);
        }
    }
}