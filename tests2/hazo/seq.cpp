#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/hana.h>
#include <udho/hazo/seq.h>
#include <string>

namespace h = udho::hazo;

HAZO_ELEMENT(first_name, std::string);
HAZO_ELEMENT(last_name, std::string);
HAZO_ELEMENT(age, std::size_t);
HAZO_ELEMENT_HANA(name, std::string);
HAZO_ELEMENT_HANA(country, std::string);

TEST_CASE("sequence", "[hazo]") {
    typedef h::seq_d<int, std::string, double, first_name, last_name, int, name, country> seq_d_type;
    REQUIRE(h::make_seq_d(4.2, 85) == h::make_seq_d(4.2, 85));
    REQUIRE(h::make_seq_d(4.2, 85) != h::make_seq_d(4.2, 85, "Hello World"));

    SECTION("one or more types can be excluded") {
        REQUIRE(seq_d_type::exclude<int, std::string>(4.2, "Neel", "Basu", 85, "Neel Basu", "India") == h::make_seq_d(4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India")));
        REQUIRE(seq_d_type::exclude<int, int>("Hello", 4.2, "Neel", "Basu", "Neel Basu", "India") == h::make_seq_d("Hello", 4.2, first_name("Neel"), last_name("Basu"), name("Neel Basu"), country("India")));
        REQUIRE(seq_d_type::exclude<int>("Hello", 4.2, "Neel", "Basu", 85, "Neel Basu", "India") == h::make_seq_d("Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India")));
        REQUIRE(seq_d_type::exclude<first_name, last_name>(42, "Hello", 4.2, 85, "Neel Basu", "India") == h::make_seq_d(42, "Hello", 4.2, 85, name("Neel Basu"), country("India")));
    }
}