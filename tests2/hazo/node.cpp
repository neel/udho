#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/node/node.h>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/hana.h>
#include <string>

namespace h = udho::hazo;

HAZO_ELEMENT(first_name, std::string);
HAZO_ELEMENT(last_name, std::string);
HAZO_ELEMENT(age, std::size_t);
HAZO_ELEMENT_HANA(name, std::string);
HAZO_ELEMENT_HANA(country, std::string);

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

    GIVEN("a node constructed with elements as well as pod types") {
        first_name f("Neel");
        last_name l("Basu");
        age a(32);
        name n("Neel Basu");
        country c("India");

        h::node<int, first_name, last_name, age, std::string, double, name, country, char> node1(42, "Neel", "Basu", 32, "Hello World", 3.14, "Neel Basu", "India", '!');
        h::node<int, first_name, last_name, age, std::string, double, name, country, char> node2(42, f, l, a, "Hello World", 3.14, n, c, '!');

        THEN( "elements can be retrieved through the element method" ) {
            REQUIRE(std::is_same_v<decltype(node1.element(first_name::val)), first_name&>);
            REQUIRE(std::is_same_v<decltype(node1.element(last_name::val)), last_name&>);
            REQUIRE(std::is_same_v<decltype(node1.element(age::val)), age&>);
            REQUIRE(std::is_same_v<decltype(node1.element(name::val)), name&>);
            REQUIRE(std::is_same_v<decltype(node1.element(country::val)), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(node1.element(first_name::val))>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1.element(last_name::val))>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1.element(age::val))>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1.element(name::val))>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1.element(country::val))>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.element(first_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.element(last_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.element(age::val))>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.element(name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.element(country::val))>, std::string>);

            REQUIRE(node1.element(first_name::val) == f);
            REQUIRE(node1.element(last_name::val) == l);
            REQUIRE(node1.element(age::val) == a);
            REQUIRE(node1.element(name::val) == n);
            REQUIRE(node1.element(country::val) == c);

            REQUIRE(node1.element(first_name::val) == "Neel");
            REQUIRE(node1.element(last_name::val) == "Basu");
            REQUIRE(node1.element(age::val) == 32);
            REQUIRE(node1.element(name::val) == "Neel Basu");
            REQUIRE(node1.element(country::val) == "India");

            REQUIRE(std::is_same_v<decltype(node2.element(first_name::val)), first_name&>);
            REQUIRE(std::is_same_v<decltype(node2.element(last_name::val)), last_name&>);
            REQUIRE(std::is_same_v<decltype(node2.element(age::val)), age&>);
            REQUIRE(std::is_same_v<decltype(node2.element(name::val)), name&>);
            REQUIRE(std::is_same_v<decltype(node2.element(country::val)), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(node2.element(first_name::val))>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2.element(last_name::val))>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2.element(age::val))>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2.element(name::val))>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2.element(country::val))>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2.element(first_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2.element(last_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2.element(age::val))>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2.element(name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2.element(country::val))>, std::string>);

            REQUIRE(node2.element(first_name::val) == f);
            REQUIRE(node2.element(last_name::val) == l);
            REQUIRE(node2.element(age::val) == a);
            REQUIRE(node2.element(name::val) == n);
            REQUIRE(node2.element(country::val) == c);

            REQUIRE(node2.element(first_name::val) == "Neel");
            REQUIRE(node2.element(last_name::val) == "Basu");
            REQUIRE(node2.element(age::val) == 32);
            REQUIRE(node2.element(name::val) == "Neel Basu");
            REQUIRE(node2.element(country::val) == "India");
        }

        THEN( "elements can be retrieved through operator[] using element handle" ) {
            REQUIRE(std::is_same_v<decltype(node1[first_name::val]), first_name&>);
            REQUIRE(std::is_same_v<decltype(node1[last_name::val]), last_name&>);
            REQUIRE(std::is_same_v<decltype(node1[age::val]), age&>);
            REQUIRE(std::is_same_v<decltype(node1[name::val]), name&>);
            REQUIRE(std::is_same_v<decltype(node1[country::val]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(node1[first_name::val])>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1[last_name::val])>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1[age::val])>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1[name::val])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1[country::val])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1[first_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1[last_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1[age::val])>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1[name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1[country::val])>, std::string>);

            REQUIRE(node1[first_name::val] == f);
            REQUIRE(node1[last_name::val] == l);
            REQUIRE(node1[age::val] == a);
            REQUIRE(node1[name::val] == n);
            REQUIRE(node1[country::val] == c);

            REQUIRE(node1[first_name::val] == "Neel");
            REQUIRE(node1[last_name::val] == "Basu");
            REQUIRE(node1[age::val] == 32);
            REQUIRE(node1[name::val] == "Neel Basu");
            REQUIRE(node1[country::val] == "India");

            REQUIRE(std::is_same_v<decltype(node2[first_name::val]), first_name&>);
            REQUIRE(std::is_same_v<decltype(node2[last_name::val]), last_name&>);
            REQUIRE(std::is_same_v<decltype(node2[age::val]), age&>);
            REQUIRE(std::is_same_v<decltype(node2[name::val]), name&>);
            REQUIRE(std::is_same_v<decltype(node2[country::val]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(node2[first_name::val])>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2[last_name::val])>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2[age::val])>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2[name::val])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2[country::val])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2[first_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2[last_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2[age::val])>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2[name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2[country::val])>, std::string>);

            REQUIRE(node2[first_name::val] == f);
            REQUIRE(node2[last_name::val] == l);
            REQUIRE(node2[age::val] == a);
            REQUIRE(node2[name::val] == n);
            REQUIRE(node2[country::val] == c);

            REQUIRE(node2[first_name::val] == "Neel");
            REQUIRE(node2[last_name::val] == "Basu");
            REQUIRE(node2[age::val] == 32);
            REQUIRE(node2[name::val] == "Neel Basu");
            REQUIRE(node2[country::val] == "India");
        }

        THEN( "elements can be retrieved through operator[] using key" ) {
            using namespace boost::hana::literals;
            REQUIRE(std::is_same_v<decltype(node1["name"_s]), name&>);
            REQUIRE(std::is_same_v<decltype(node1["country"_s]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(node1["name"_s])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node1["country"_s])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1["name"_s])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1["country"_s])>, std::string>);

            REQUIRE(node1["name"_s] == n);
            REQUIRE(node1["country"_s] == c);

            REQUIRE(node1["name"_s] == "Neel Basu");
            REQUIRE(node1["country"_s] == "India");

            REQUIRE(std::is_same_v<decltype(node2["name"_s]), name&>);
            REQUIRE(std::is_same_v<decltype(node2["country"_s]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(node2["name"_s])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(node2["country"_s])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2["name"_s])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node2["country"_s])>, std::string>);

            REQUIRE(node2["name"_s] == n);
            REQUIRE(node2["country"_s] == c);

            REQUIRE(node2["name"_s] == "Neel Basu");
            REQUIRE(node2["country"_s] == "India");
        }

        THEN( "data and value methods work in the expected way" ) {
            REQUIRE(node1.data<0>() == 42);
            REQUIRE(node1.data<1>() == f);
            REQUIRE(node1.data<1>() == "Neel");
            REQUIRE(node1.data<6>() == n);
            REQUIRE(node1.data<6>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(node1.data<0>()), int&>);
            REQUIRE(std::is_same_v<decltype(node1.data<1>()), first_name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.data<1>())>, std::string>);
            REQUIRE(std::is_same_v<decltype(node1.data<6>()), name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.data<6>())>, std::string>);

            REQUIRE(node1.value<0>() == 42);
            REQUIRE(node1.value<1>() == "Neel");
            REQUIRE(node1.value<6>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(node1.value<0>()), int&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.value<1>())>, first_name>);
            REQUIRE(std::is_same_v<decltype(node1.value<1>()), std::string&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.value<6>())>, name>);
            REQUIRE(std::is_same_v<decltype(node1.value<6>()), std::string&>);

            REQUIRE(node1.data<int>() == 42);
            REQUIRE(node1.data<first_name>() == f);
            REQUIRE(node1.data<first_name>() == "Neel");
            REQUIRE(node1.data<name>() == n);
            REQUIRE(node1.data<name>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(node1.data<int>()), int&>);
            REQUIRE(std::is_same_v<decltype(node1.data<first_name>()), first_name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.data<first_name>())>, std::string>);
            REQUIRE(std::is_same_v<decltype(node1.data<name>()), name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.data<name>())>, std::string>);

            REQUIRE(node1.value<int>() == 42);
            REQUIRE(node1.value<first_name>() == "Neel");
            REQUIRE(node1.value<name>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(node1.value<int>()), int&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.value<first_name>())>, first_name>);
            REQUIRE(std::is_same_v<decltype(node1.value<first_name>()), std::string&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(node1.value<name>())>, name>);
            REQUIRE(std::is_same_v<decltype(node1.value<name>()), std::string&>);
        }
    }
}