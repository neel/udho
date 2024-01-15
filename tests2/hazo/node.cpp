#define CATCH_CONFIG_MAIN
#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
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

struct wrap_int_key{};
struct wrap_int_index{};
struct wrap_int{
    typedef wrap_int_index index_type;

    int _v;

    static constexpr wrap_int_key key() { return wrap_int_key{}; }

    inline wrap_int() = default;
    inline explicit wrap_int(const wrap_int&) = default;
    inline explicit wrap_int(int v): _v(v){}
    inline bool operator==(const wrap_int& other) const { return _v == other._v; }
    inline bool operator!=(const wrap_int& other) const { return !operator==(other); }
};
struct wrap_str{
    std::string _v;

    inline wrap_str() = default;
    inline explicit wrap_str(const wrap_str&) = default;
    inline explicit wrap_str(const char*& v): _v(v){}
    inline explicit wrap_str(const std::string& v): _v(v){}
    inline bool operator==(const wrap_str& other) const { return _v == other._v; }
    inline bool operator!=(const wrap_str& other) const { return !operator==(other); }
};
struct value_str_key{};
struct value_str{
    typedef std::string value_type;

    std::string _v;

    static constexpr value_str_key key() { return value_str_key{}; }

    inline value_str() = default;
    inline explicit value_str(const value_str&) = default;
    inline explicit value_str(const char*& v): _v(v){}
    inline explicit value_str(const std::string& v): _v(v){}
    inline const std::string& value() const { return _v; }
    inline std::string& value() { return _v; }
    inline bool operator==(const value_str& other) const { return _v == other._v; }
    inline bool operator!=(const value_str& other) const { return !operator==(other); }
};

TEST_CASE("node", "[hazo]") {

    SECTION( "data can be retrieved and modified by position" ) {
        h::node<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"));

        CHECK(chain.data<0>()  == 42);
        CHECK(chain.data<1>()  == "Fourty Two");
        CHECK(chain.data<2>()  == "Neel");
        CHECK(chain.data<3>()  == "Basu");
        CHECK(chain.data<4>()  == age(32));
        CHECK(chain.data<5>()  == 4.2);
        CHECK(chain.data<6>()  == '!');
        CHECK(chain.data<7>()  == "Fourty Two");
        CHECK(chain.data<8>()  == name("Neel Basu"));
        CHECK(chain.data<9>()  == "India");
        CHECK(chain.data<10>() == 24);
        CHECK(chain.data<11>() == wrap_int(10));
        CHECK(chain.data<12>() == wrap_str("Hello World"));
        CHECK(chain.data<13>() == value_str("Hi"));

        chain.data<0>()  = 24;
        chain.data<1>()  = "Twenty Four";
        chain.data<2>()  = first_name("Sunanda");
        chain.data<3>()  = last_name("Bose");
        chain.data<4>()  = age(33);
        chain.data<5>()  = 2.4;
        chain.data<6>()  = '?';
        chain.data<7>()  = "Twenty Four";
        chain.data<8>()  = name("Sunanda Bose");
        chain.data<9>()  = country("India");
        chain.data<10>() = 42;
        chain.data<11>() = wrap_int(20);
        chain.data<12>() = wrap_str("Hello");
        chain.data<13>() = value_str("Hi!");

        CHECK(chain.data<0>()  == 24);
        CHECK(chain.data<1>()  == "Twenty Four");
        CHECK(chain.data<2>()  == "Sunanda");
        CHECK(chain.data<3>()  == "Bose");
        CHECK(chain.data<4>()  == 33);
        CHECK(chain.data<5>()  == 2.4);
        CHECK(chain.data<6>()  == '?');
        CHECK(chain.data<7>()  == "Twenty Four");
        CHECK(chain.data<8>()  == name("Sunanda Bose"));
        CHECK(chain.data<9>()  == "India");
        CHECK(chain.data<10>() == 42);
        CHECK(chain.data<11>() == wrap_int(20));
        CHECK(chain.data<12>() == wrap_str("Hello"));
        CHECK(chain.data<13>() == value_str("Hi!"));
    }

    SECTION( "data can be retrieved and modified by type and position" ) {
        h::node<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str, wrap_int> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"), wrap_int(64));

        CHECK(chain.data<int, 0>()            == 42);
        CHECK(chain.data<std::string, 0>()    == "Fourty Two");
        CHECK(chain.data<first_name, 0>()     == "Neel");
        CHECK(chain.data<last_name, 0>()      == "Basu");
        CHECK(chain.data<age, 0>()            == age(32));
        CHECK(chain.data<double, 0>()         == 4.2);
        CHECK(chain.data<char, 0>()           == '!');
        CHECK(chain.data<std::string, 1>()    == "Fourty Two");
        CHECK(chain.data<name, 0>()           == name("Neel Basu"));
        CHECK(chain.data<country, 0>()        == "India");
        CHECK(chain.data<int, 1>()            == 24);
        CHECK(chain.data<wrap_int_index, 0>() == wrap_int(10));
        CHECK(chain.data<wrap_str>()          == wrap_str("Hello World"));
        CHECK(chain.data<value_str>()         == value_str("Hi"));
        CHECK(chain.data<wrap_int_index, 1>() == wrap_int(64));

        chain.data<int, 0>()                    = 24;
        chain.data<std::string, 0>()            = "Twenty Four";
        chain.data<first_name, 0>()             = first_name("Sunanda");
        chain.data<last_name, 0>()              = last_name("Bose");
        chain.data<age, 0>()                    = age(33);
        chain.data<double, 0>()                 = 2.4;
        chain.data<char, 0>()                   = '?';
        chain.data<std::string, 1>()            = "Twenty Four";
        chain.data<name, 0>()                   = name("Sunanda Bose");
        chain.data<country, 0>()                = country("India");
        chain.data<int, 1>()                    = 42;
        chain.data<wrap_int_index, 0>()         = wrap_int(20);
        chain.data<wrap_str>()                  = wrap_str("Hello");
        chain.data<value_str>()                 = value_str("Hi!");
        chain.data<wrap_int_index, 1>()         = wrap_int(46);

        CHECK(chain.data<int, 0>()            == 24);
        CHECK(chain.data<std::string, 0>()    == "Twenty Four");
        CHECK(chain.data<first_name, 0>()     == "Sunanda");
        CHECK(chain.data<last_name, 0>()      == "Bose");
        CHECK(chain.data<age, 0>()            == 33);
        CHECK(chain.data<double, 0>()         == 2.4);
        CHECK(chain.data<char, 0>()           == '?');
        CHECK(chain.data<std::string, 1>()    == "Twenty Four");
        CHECK(chain.data<name, 0>()           == name("Sunanda Bose"));
        CHECK(chain.data<country, 0>()        == "India");
        CHECK(chain.data<int, 1>()            == 42);
        CHECK(chain.data<wrap_int_index, 0>() == wrap_int(20));
        CHECK(chain.data<wrap_str>()          == wrap_str("Hello"));
        CHECK(chain.data<value_str>()         == value_str("Hi!"));
        CHECK(chain.data<wrap_int_index, 1>() == wrap_int(46));
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
            CHECK(std::is_same_v<decltype(node1.element(first_name::val)), first_name&>);
            CHECK(std::is_same_v<decltype(node1.element(last_name::val)), last_name&>);
            CHECK(std::is_same_v<decltype(node1.element(age::val)), age&>);
            CHECK(std::is_same_v<decltype(node1.element(name::val)), name&>);
            CHECK(std::is_same_v<decltype(node1.element(country::val)), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(node1.element(first_name::val))>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1.element(last_name::val))>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1.element(age::val))>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1.element(name::val))>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1.element(country::val))>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(node1.element(first_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.element(last_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.element(age::val))>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.element(name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.element(country::val))>, std::string>);

            CHECK(node1.element(first_name::val) == f);
            CHECK(node1.element(last_name::val) == l);
            CHECK(node1.element(age::val) == a);
            CHECK(node1.element(name::val) == n);
            CHECK(node1.element(country::val) == c);

            CHECK(node1.element(first_name::val) == "Neel");
            CHECK(node1.element(last_name::val) == "Basu");
            CHECK(node1.element(age::val) == 32);
            CHECK(node1.element(name::val) == "Neel Basu");
            CHECK(node1.element(country::val) == "India");

            CHECK(std::is_same_v<decltype(node2.element(first_name::val)), first_name&>);
            CHECK(std::is_same_v<decltype(node2.element(last_name::val)), last_name&>);
            CHECK(std::is_same_v<decltype(node2.element(age::val)), age&>);
            CHECK(std::is_same_v<decltype(node2.element(name::val)), name&>);
            CHECK(std::is_same_v<decltype(node2.element(country::val)), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(node2.element(first_name::val))>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2.element(last_name::val))>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2.element(age::val))>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2.element(name::val))>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2.element(country::val))>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(node2.element(first_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2.element(last_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2.element(age::val))>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2.element(name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2.element(country::val))>, std::string>);

            CHECK(node2.element(first_name::val) == f);
            CHECK(node2.element(last_name::val) == l);
            CHECK(node2.element(age::val) == a);
            CHECK(node2.element(name::val) == n);
            CHECK(node2.element(country::val) == c);

            CHECK(node2.element(first_name::val) == "Neel");
            CHECK(node2.element(last_name::val) == "Basu");
            CHECK(node2.element(age::val) == 32);
            CHECK(node2.element(name::val) == "Neel Basu");
            CHECK(node2.element(country::val) == "India");
        }

        THEN( "elements can be retrieved through operator[] using element handle" ) {
            CHECK(std::is_same_v<decltype(node1[first_name::val]), first_name&>);
            CHECK(std::is_same_v<decltype(node1[last_name::val]), last_name&>);
            CHECK(std::is_same_v<decltype(node1[age::val]), age&>);
            CHECK(std::is_same_v<decltype(node1[name::val]), name&>);
            CHECK(std::is_same_v<decltype(node1[country::val]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(node1[first_name::val])>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1[last_name::val])>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1[age::val])>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1[name::val])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1[country::val])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(node1[first_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1[last_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1[age::val])>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1[name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1[country::val])>, std::string>);

            CHECK(node1[first_name::val] == f);
            CHECK(node1[last_name::val] == l);
            CHECK(node1[age::val] == a);
            CHECK(node1[name::val] == n);
            CHECK(node1[country::val] == c);

            CHECK(node1[first_name::val] == "Neel");
            CHECK(node1[last_name::val] == "Basu");
            CHECK(node1[age::val] == 32);
            CHECK(node1[name::val] == "Neel Basu");
            CHECK(node1[country::val] == "India");

            CHECK(std::is_same_v<decltype(node2[first_name::val]), first_name&>);
            CHECK(std::is_same_v<decltype(node2[last_name::val]), last_name&>);
            CHECK(std::is_same_v<decltype(node2[age::val]), age&>);
            CHECK(std::is_same_v<decltype(node2[name::val]), name&>);
            CHECK(std::is_same_v<decltype(node2[country::val]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(node2[first_name::val])>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2[last_name::val])>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2[age::val])>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2[name::val])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2[country::val])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(node2[first_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2[last_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2[age::val])>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2[name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2[country::val])>, std::string>);

            CHECK(node2[first_name::val] == f);
            CHECK(node2[last_name::val] == l);
            CHECK(node2[age::val] == a);
            CHECK(node2[name::val] == n);
            CHECK(node2[country::val] == c);

            CHECK(node2[first_name::val] == "Neel");
            CHECK(node2[last_name::val] == "Basu");
            CHECK(node2[age::val] == 32);
            CHECK(node2[name::val] == "Neel Basu");
            CHECK(node2[country::val] == "India");
        }

        THEN( "elements can be retrieved through operator[] using key" ) {
            using namespace boost::hana::literals;
            CHECK(std::is_same_v<decltype(node1["name"_s]), name&>);
            CHECK(std::is_same_v<decltype(node1["country"_s]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(node1["name"_s])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node1["country"_s])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(node1["name"_s])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1["country"_s])>, std::string>);

            CHECK(node1["name"_s] == n);
            CHECK(node1["country"_s] == c);

            CHECK(node1["name"_s] == "Neel Basu");
            CHECK(node1["country"_s] == "India");

            CHECK(std::is_same_v<decltype(node2["name"_s]), name&>);
            CHECK(std::is_same_v<decltype(node2["country"_s]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(node2["name"_s])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(node2["country"_s])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(node2["name"_s])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node2["country"_s])>, std::string>);

            CHECK(node2["name"_s] == n);
            CHECK(node2["country"_s] == c);

            CHECK(node2["name"_s] == "Neel Basu");
            CHECK(node2["country"_s] == "India");
        }

        THEN( "data and value methods work in the expected way" ) {
            CHECK(node1.data<0>() == 42);
            CHECK(node1.data<1>() == f);
            CHECK(node1.data<1>() == "Neel");
            CHECK(node1.data<6>() == n);
            CHECK(node1.data<6>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(node1.data<0>()), int&>);
            CHECK(std::is_same_v<decltype(node1.data<1>()), first_name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.data<1>())>, std::string>);
            CHECK(std::is_same_v<decltype(node1.data<6>()), name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.data<6>())>, std::string>);

            CHECK(node1.value<0>() == 42);
            CHECK(node1.value<1>() == "Neel");
            CHECK(node1.value<6>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(node1.value<0>()), int&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.value<1>())>, first_name>);
            CHECK(std::is_same_v<decltype(node1.value<1>()), std::string&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.value<6>())>, name>);
            CHECK(std::is_same_v<decltype(node1.value<6>()), std::string&>);

            CHECK(node1.data<int>() == 42);
            CHECK(node1.data<first_name>() == f);
            CHECK(node1.data<first_name>() == "Neel");
            CHECK(node1.data<name>() == n);
            CHECK(node1.data<name>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(node1.data<int>()), int&>);
            CHECK(std::is_same_v<decltype(node1.data<first_name>()), first_name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.data<first_name>())>, std::string>);
            CHECK(std::is_same_v<decltype(node1.data<name>()), name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.data<name>())>, std::string>);

            CHECK(node1.value<int>() == 42);
            CHECK(node1.value<first_name>() == "Neel");
            CHECK(node1.value<name>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(node1.value<int>()), int&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.value<first_name>())>, first_name>);
            CHECK(std::is_same_v<decltype(node1.value<first_name>()), std::string&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(node1.value<name>())>, name>);
            CHECK(std::is_same_v<decltype(node1.value<name>()), std::string&>);
        }
    }
}
