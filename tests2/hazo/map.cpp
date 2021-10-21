#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/hana.h>
#include <udho/hazo/map.h>
#include <udho/hazo/map/hana.h>
#include <boost/hana/string.hpp>
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

template <typename ContainerT, typename KeyT>
bool Has(const ContainerT&, const KeyT&){
    return ContainerT::template has<KeyT>::value;
}

TEST_CASE("map common node functionalities", "[hazo]") {

    SECTION( "data can be retrieved and modified by position" ) {
        h::map_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"));

        REQUIRE(chain.data<0>()  == 42);
        REQUIRE(chain.data<1>()  == "Fourty Two");
        REQUIRE(chain.data<2>()  == "Neel");
        REQUIRE(chain.data<3>()  == "Basu");
        REQUIRE(chain.data<4>()  == age(32));
        REQUIRE(chain.data<5>()  == 4.2);
        REQUIRE(chain.data<6>()  == '!');
        REQUIRE(chain.data<7>()  == "Fourty Two");
        REQUIRE(chain.data<8>()  == name("Neel Basu"));
        REQUIRE(chain.data<9>()  == "India");
        REQUIRE(chain.data<10>() == 24);
        REQUIRE(chain.data<11>() == wrap_int(10));
        REQUIRE(chain.data<12>() == wrap_str("Hello World"));
        REQUIRE(chain.data<13>() == value_str("Hi"));

        chain.data<0>()  = 24;
        chain.data<1>()  = "Twenty Four";
        chain.data<2>()  = first_name("Sunanda");
        chain.data<3>()  = "Bose";
        chain.data<4>()  = 33;
        chain.data<5>()  = 2.4;
        chain.data<6>()  = '?';
        chain.data<7>()  = "Twenty Four";
        chain.data<8>()  = name("Sunanda Bose");
        chain.data<9>()  = "India";
        chain.data<10>() = 42;
        chain.data<11>() = wrap_int(20);
        chain.data<12>() = wrap_str("Hello");
        chain.data<13>() = value_str("Hi!");

        REQUIRE(chain.data<0>()  == 24);
        REQUIRE(chain.data<1>()  == "Twenty Four");
        REQUIRE(chain.data<2>()  == "Sunanda");
        REQUIRE(chain.data<3>()  == "Bose");
        REQUIRE(chain.data<4>()  == 33);
        REQUIRE(chain.data<5>()  == 2.4);
        REQUIRE(chain.data<6>()  == '?');
        REQUIRE(chain.data<7>()  == "Twenty Four");
        REQUIRE(chain.data<8>()  == name("Sunanda Bose"));
        REQUIRE(chain.data<9>()  == "India");
        REQUIRE(chain.data<10>() == 42);
        REQUIRE(chain.data<11>() == wrap_int(20));
        REQUIRE(chain.data<12>() == wrap_str("Hello"));
        REQUIRE(chain.data<13>() == value_str("Hi!"));
    }

    SECTION( "value can be retrieved and modified by position" ) {
        h::map_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"));

        REQUIRE(chain.value<0>()  == 42);
        REQUIRE(chain.value<1>()  == "Fourty Two");
        REQUIRE(chain.value<2>()  == "Neel");
        REQUIRE(chain.value<3>()  == "Basu");
        REQUIRE(chain.value<4>()  == 32);
        REQUIRE(chain.value<5>()  == 4.2);
        REQUIRE(chain.value<6>()  == '!');
        REQUIRE(chain.value<7>()  == "Fourty Two");
        REQUIRE(chain.value<8>()  == "Neel Basu");
        REQUIRE(chain.value<9>()  == "India");
        REQUIRE(chain.value<10>() == 24);
        REQUIRE(chain.value<11>() == wrap_int(10));
        REQUIRE(chain.value<12>() == wrap_str("Hello World"));
        REQUIRE(chain.value<13>() == "Hi");

        chain.value<0>()  = 24;
        chain.value<1>()  = "Twenty Four";
        chain.value<2>()  = "Sunanda";
        chain.value<3>()  = "Bose";
        chain.value<4>()  = 33;
        chain.value<5>()  = 2.4;
        chain.value<6>()  = '?';
        chain.value<7>()  = "Twenty Four";
        chain.value<8>()  = "Sunanda Bose";
        chain.value<9>()  = "India";
        chain.value<10>() = 42;
        chain.value<11>() = wrap_int(20);
        chain.value<12>() = wrap_str("Hello");
        chain.value<13>() = "Hi!";

        REQUIRE(chain.value<0>()  == 24);
        REQUIRE(chain.value<1>()  == "Twenty Four");
        REQUIRE(chain.value<2>()  == "Sunanda");
        REQUIRE(chain.value<3>()  == "Bose");
        REQUIRE(chain.value<4>()  == 33);
        REQUIRE(chain.value<5>()  == 2.4);
        REQUIRE(chain.value<6>()  == '?');
        REQUIRE(chain.value<7>()  == "Twenty Four");
        REQUIRE(chain.value<8>()  == "Sunanda Bose");
        REQUIRE(chain.value<9>()  == "India");
        REQUIRE(chain.value<10>() == 42);
        REQUIRE(chain.value<11>() == wrap_int(20));
        REQUIRE(chain.value<12>() == wrap_str("Hello"));
        REQUIRE(chain.value<13>() == "Hi!");
    }

    SECTION( "data can be retrieved and modified by type and position" ) {
        h::map_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str, wrap_int> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"), wrap_int(64));

        REQUIRE(chain.data<int, 0>()            == 42);
        REQUIRE(chain.data<std::string, 0>()    == "Fourty Two");
        REQUIRE(chain.data<first_name, 0>()     == "Neel");
        REQUIRE(chain.data<last_name, 0>()      == "Basu");
        REQUIRE(chain.data<age, 0>()            == age(32));
        REQUIRE(chain.data<double, 0>()         == 4.2);
        REQUIRE(chain.data<char, 0>()           == '!');
        REQUIRE(chain.data<std::string, 1>()    == "Fourty Two");
        REQUIRE(chain.data<name, 0>()           == name("Neel Basu"));
        REQUIRE(chain.data<country, 0>()        == "India");
        REQUIRE(chain.data<int, 1>()            == 24);
        REQUIRE(chain.data<wrap_int_index, 0>() == wrap_int(10));
        REQUIRE(chain.data<wrap_str>()          == wrap_str("Hello World"));
        REQUIRE(chain.data<value_str>()         == value_str("Hi"));
        REQUIRE(chain.data<wrap_int_index, 1>() == wrap_int(64));

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

        REQUIRE(chain.data<int, 0>()            == 24);
        REQUIRE(chain.data<std::string, 0>()    == "Twenty Four");
        REQUIRE(chain.data<first_name, 0>()     == "Sunanda");
        REQUIRE(chain.data<last_name, 0>()      == "Bose");
        REQUIRE(chain.data<age, 0>()            == 33);
        REQUIRE(chain.data<double, 0>()         == 2.4);
        REQUIRE(chain.data<char, 0>()           == '?');
        REQUIRE(chain.data<std::string, 1>()    == "Twenty Four");
        REQUIRE(chain.data<name, 0>()           == name("Sunanda Bose"));
        REQUIRE(chain.data<country, 0>()        == "India");
        REQUIRE(chain.data<int, 1>()            == 42);
        REQUIRE(chain.data<wrap_int_index, 0>() == wrap_int(20));
        REQUIRE(chain.data<wrap_str>()          == wrap_str("Hello"));
        REQUIRE(chain.data<value_str>()         == value_str("Hi!"));
        REQUIRE(chain.data<wrap_int_index, 1>() == wrap_int(46));
    }

    SECTION( "value can be retrieved and modified by type and position" ) {
        h::map_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str, wrap_int> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"), wrap_int(64));

        REQUIRE(chain.value<int, 0>()            == 42);
        REQUIRE(chain.value<std::string, 0>()    == "Fourty Two");
        REQUIRE(chain.value<first_name, 0>()     == "Neel");
        REQUIRE(chain.value<last_name, 0>()      == "Basu");
        REQUIRE(chain.value<age, 0>()            == 32);
        REQUIRE(chain.value<double, 0>()         == 4.2);
        REQUIRE(chain.value<char, 0>()           == '!');
        REQUIRE(chain.value<std::string, 1>()    == "Fourty Two");
        REQUIRE(chain.value<name, 0>()           == "Neel Basu");
        REQUIRE(chain.value<country, 0>()        == "India");
        REQUIRE(chain.value<int, 1>()            == 24);
        REQUIRE(chain.value<wrap_int_index, 0>() == wrap_int(10));
        REQUIRE(chain.value<wrap_str>()          == wrap_str("Hello World"));
        REQUIRE(chain.value<value_str>()         == "Hi");
        REQUIRE(chain.value<wrap_int_index, 1>() == wrap_int(64));

        chain.value<int, 0>()                    = 24;
        chain.value<std::string, 0>()            = "Twenty Four";
        chain.value<first_name, 0>()             = "Sunanda";
        chain.value<last_name, 0>()              = "Bose";
        chain.value<age, 0>()                    = 33;
        chain.value<double, 0>()                 = 2.4;
        chain.value<char, 0>()                   = '?';
        chain.value<std::string, 1>()            = "Twenty Four";
        chain.value<name, 0>()                   = "Sunanda Bose";
        chain.value<country, 0>()                = "India";
        chain.value<int, 1>()                    = 42;
        chain.value<wrap_int_index, 0>()         = wrap_int(20);
        chain.value<wrap_str>()                  = wrap_str("Hello");
        chain.value<value_str>()                 = "Hi!";
        chain.value<wrap_int_index, 1>()         = wrap_int(46);

        REQUIRE(chain.value<int, 0>()            == 24);
        REQUIRE(chain.value<std::string, 0>()    == "Twenty Four");
        REQUIRE(chain.value<first_name, 0>()     == "Sunanda");
        REQUIRE(chain.value<last_name, 0>()      == "Bose");
        REQUIRE(chain.value<age, 0>()            == 33);
        REQUIRE(chain.value<double, 0>()         == 2.4);
        REQUIRE(chain.value<char, 0>()           == '?');
        REQUIRE(chain.value<std::string, 1>()    == "Twenty Four");
        REQUIRE(chain.value<name, 0>()           == "Sunanda Bose");
        REQUIRE(chain.value<country, 0>()        == "India");
        REQUIRE(chain.value<int, 1>()            == 42);
        REQUIRE(chain.value<wrap_int_index, 0>() == wrap_int(20));
        REQUIRE(chain.value<wrap_str>()          == wrap_str("Hello"));
        REQUIRE(chain.value<value_str>()         == "Hi!");
        REQUIRE(chain.value<wrap_int_index, 1>() == wrap_int(46));
    }

    GIVEN("a map_d constructed with elements as well as pod types") {
        first_name f("Neel");
        last_name l("Basu");
        age a(32);
        name n("Neel Basu");
        country c("India");

        h::map_d<int, first_name, last_name, age, std::string, double, name, country, char> map_d1(42, "Neel", "Basu", 32, "Hello World", 3.14, "Neel Basu", "India", '!');
        h::map_d<int, first_name, last_name, age, std::string, double, name, country, char> map_d2(42, f, l, a, "Hello World", 3.14, n, c, '!');

        THEN( "elements can be retrieved through the element method" ) {
            REQUIRE(std::is_same_v<decltype(map_d1.element(first_name::val)), first_name&>);
            REQUIRE(std::is_same_v<decltype(map_d1.element(last_name::val)), last_name&>);
            REQUIRE(std::is_same_v<decltype(map_d1.element(age::val)), age&>);
            REQUIRE(std::is_same_v<decltype(map_d1.element(name::val)), name&>);
            REQUIRE(std::is_same_v<decltype(map_d1.element(country::val)), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1.element(first_name::val))>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1.element(last_name::val))>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1.element(age::val))>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1.element(name::val))>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1.element(country::val))>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.element(first_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.element(last_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.element(age::val))>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.element(name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.element(country::val))>, std::string>);

            REQUIRE(map_d1.element(first_name::val) == f);
            REQUIRE(map_d1.element(last_name::val) == l);
            REQUIRE(map_d1.element(age::val) == a);
            REQUIRE(map_d1.element(name::val) == n);
            REQUIRE(map_d1.element(country::val) == c);

            REQUIRE(map_d1.element(first_name::val) == "Neel");
            REQUIRE(map_d1.element(last_name::val) == "Basu");
            REQUIRE(map_d1.element(age::val) == 32);
            REQUIRE(map_d1.element(name::val) == "Neel Basu");
            REQUIRE(map_d1.element(country::val) == "India");

            REQUIRE(std::is_same_v<decltype(map_d2.element(first_name::val)), first_name&>);
            REQUIRE(std::is_same_v<decltype(map_d2.element(last_name::val)), last_name&>);
            REQUIRE(std::is_same_v<decltype(map_d2.element(age::val)), age&>);
            REQUIRE(std::is_same_v<decltype(map_d2.element(name::val)), name&>);
            REQUIRE(std::is_same_v<decltype(map_d2.element(country::val)), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2.element(first_name::val))>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2.element(last_name::val))>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2.element(age::val))>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2.element(name::val))>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2.element(country::val))>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2.element(first_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2.element(last_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2.element(age::val))>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2.element(name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2.element(country::val))>, std::string>);

            REQUIRE(map_d2.element(first_name::val) == f);
            REQUIRE(map_d2.element(last_name::val) == l);
            REQUIRE(map_d2.element(age::val) == a);
            REQUIRE(map_d2.element(name::val) == n);
            REQUIRE(map_d2.element(country::val) == c);

            REQUIRE(map_d2.element(first_name::val) == "Neel");
            REQUIRE(map_d2.element(last_name::val) == "Basu");
            REQUIRE(map_d2.element(age::val) == 32);
            REQUIRE(map_d2.element(name::val) == "Neel Basu");
            REQUIRE(map_d2.element(country::val) == "India");
        }

        THEN( "elements can be retrieved through operator[] using element handle" ) {
            REQUIRE(std::is_same_v<decltype(map_d1[first_name::val]), first_name&>);
            REQUIRE(std::is_same_v<decltype(map_d1[last_name::val]), last_name&>);
            REQUIRE(std::is_same_v<decltype(map_d1[age::val]), age&>);
            REQUIRE(std::is_same_v<decltype(map_d1[name::val]), name&>);
            REQUIRE(std::is_same_v<decltype(map_d1[country::val]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1[first_name::val])>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1[last_name::val])>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1[age::val])>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1[name::val])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1[country::val])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1[first_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1[last_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1[age::val])>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1[name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1[country::val])>, std::string>);

            REQUIRE(map_d1[first_name::val] == f);
            REQUIRE(map_d1[last_name::val] == l);
            REQUIRE(map_d1[age::val] == a);
            REQUIRE(map_d1[name::val] == n);
            REQUIRE(map_d1[country::val] == c);

            REQUIRE(map_d1[first_name::val] == "Neel");
            REQUIRE(map_d1[last_name::val] == "Basu");
            REQUIRE(map_d1[age::val] == 32);
            REQUIRE(map_d1[name::val] == "Neel Basu");
            REQUIRE(map_d1[country::val] == "India");

            REQUIRE(std::is_same_v<decltype(map_d2[first_name::val]), first_name&>);
            REQUIRE(std::is_same_v<decltype(map_d2[last_name::val]), last_name&>);
            REQUIRE(std::is_same_v<decltype(map_d2[age::val]), age&>);
            REQUIRE(std::is_same_v<decltype(map_d2[name::val]), name&>);
            REQUIRE(std::is_same_v<decltype(map_d2[country::val]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2[first_name::val])>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2[last_name::val])>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2[age::val])>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2[name::val])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2[country::val])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2[first_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2[last_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2[age::val])>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2[name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2[country::val])>, std::string>);

            REQUIRE(map_d2[first_name::val] == f);
            REQUIRE(map_d2[last_name::val] == l);
            REQUIRE(map_d2[age::val] == a);
            REQUIRE(map_d2[name::val] == n);
            REQUIRE(map_d2[country::val] == c);

            REQUIRE(map_d2[first_name::val] == "Neel");
            REQUIRE(map_d2[last_name::val] == "Basu");
            REQUIRE(map_d2[age::val] == 32);
            REQUIRE(map_d2[name::val] == "Neel Basu");
            REQUIRE(map_d2[country::val] == "India");
        }

        THEN( "elements can be retrieved through operator[] using key" ) {
            using namespace boost::hana::literals;
            REQUIRE(std::is_same_v<decltype(map_d1["name"_s]), name&>);
            REQUIRE(std::is_same_v<decltype(map_d1["country"_s]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1["name"_s])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d1["country"_s])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1["name"_s])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1["country"_s])>, std::string>);

            REQUIRE(map_d1["name"_s] == n);
            REQUIRE(map_d1["country"_s] == c);

            REQUIRE(map_d1["name"_s] == "Neel Basu");
            REQUIRE(map_d1["country"_s] == "India");

            REQUIRE(std::is_same_v<decltype(map_d2["name"_s]), name&>);
            REQUIRE(std::is_same_v<decltype(map_d2["country"_s]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2["name"_s])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(map_d2["country"_s])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2["name"_s])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d2["country"_s])>, std::string>);

            REQUIRE(map_d2["name"_s] == n);
            REQUIRE(map_d2["country"_s] == c);

            REQUIRE(map_d2["name"_s] == "Neel Basu");
            REQUIRE(map_d2["country"_s] == "India");
        }

        THEN( "data and value methods work in the expected way" ) {
            REQUIRE(map_d1.data<0>() == 42);
            REQUIRE(map_d1.data<1>() == f);
            REQUIRE(map_d1.data<1>() == "Neel");
            REQUIRE(map_d1.data<6>() == n);
            REQUIRE(map_d1.data<6>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(map_d1.data<0>()), int&>);
            REQUIRE(std::is_same_v<decltype(map_d1.data<1>()), first_name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.data<1>())>, std::string>);
            REQUIRE(std::is_same_v<decltype(map_d1.data<6>()), name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.data<6>())>, std::string>);

            REQUIRE(map_d1.value<0>() == 42);
            REQUIRE(map_d1.value<1>() == "Neel");
            REQUIRE(map_d1.value<6>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(map_d1.value<0>()), int&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.value<1>())>, first_name>);
            REQUIRE(std::is_same_v<decltype(map_d1.value<1>()), std::string&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.value<6>())>, name>);
            REQUIRE(std::is_same_v<decltype(map_d1.value<6>()), std::string&>);

            REQUIRE(map_d1.data<int>() == 42);
            REQUIRE(map_d1.data<first_name>() == f);
            REQUIRE(map_d1.data<first_name>() == "Neel");
            REQUIRE(map_d1.data<name>() == n);
            REQUIRE(map_d1.data<name>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(map_d1.data<int>()), int&>);
            REQUIRE(std::is_same_v<decltype(map_d1.data<first_name>()), first_name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.data<first_name>())>, std::string>);
            REQUIRE(std::is_same_v<decltype(map_d1.data<name>()), name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.data<name>())>, std::string>);

            REQUIRE(map_d1.value<int>() == 42);
            REQUIRE(map_d1.value<first_name>() == "Neel");
            REQUIRE(map_d1.value<name>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(map_d1.value<int>()), int&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.value<first_name>())>, first_name>);
            REQUIRE(std::is_same_v<decltype(map_d1.value<first_name>()), std::string&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(map_d1.value<name>())>, name>);
            REQUIRE(std::is_same_v<decltype(map_d1.value<name>()), std::string&>);
        }
    }
}

TEST_CASE("map specific functionalities", "[hazo]") {
    typedef h::map_d<int, std::string, double, first_name, last_name, int, name, country> map_d_type;
    REQUIRE(h::make_map_d(4.2, 85) == h::make_map_d(4.2, 85));
    REQUIRE(h::make_map_d(4.2, 85) != h::make_map_d(4.2, 85, "Hello World"));

    SECTION("one or more types can be excluded") {
        REQUIRE(map_d_type::exclude<int, std::string>(4.2, "Neel", "Basu", 85, "Neel Basu", "India") == h::make_map_d(4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India")));
        REQUIRE(map_d_type::exclude<int, int>("Hello", 4.2, "Neel", "Basu", "Neel Basu", "India") == h::make_map_d("Hello", 4.2, first_name("Neel"), last_name("Basu"), name("Neel Basu"), country("India")));
        REQUIRE(map_d_type::exclude<int>("Hello", 4.2, "Neel", "Basu", 85, "Neel Basu", "India") == h::make_map_d("Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India")));
        REQUIRE(map_d_type::exclude<first_name, last_name>(42, "Hello", 4.2, 85, "Neel Basu", "India") == h::make_map_d(42, "Hello", 4.2, 85, name("Neel Basu"), country("India")));

        REQUIRE(std::is_same<map_d_type::exclude_if<std::is_pod>, h::map_d<std::string, first_name, last_name, name, country>>::value);
    }

    SECTION("one or more types can be included") {
        REQUIRE(map_d_type::extend<int, std::string>(42, "Hello", 4.2, "Neel", "Basu", 85, "Neel Basu", "India", 10, "Hi") == h::make_map_d(42, "Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India"), 10, "Hi"));
    }

    SECTION("contains") {
        typedef h::map_d<int, std::string, double, first_name, last_name, int, name, country, wrap_int> map_type;

        REQUIRE(map_type::contains<int>::value);
        REQUIRE(map_type::contains<std::string>::value);
        REQUIRE(map_type::contains<double>::value);
        REQUIRE(map_type::contains<first_name>::value);
        REQUIRE(map_type::contains<last_name>::value);
        REQUIRE(map_type::contains<name>::value);
        REQUIRE(map_type::contains<country>::value);
        REQUIRE(map_type::contains<wrap_int_index>::value);
        REQUIRE(!map_type::contains<wrap_int>::value);
        REQUIRE(!map_type::contains<value_str>::value);
    }

    SECTION("has") {
        typedef h::map_d<int, std::string, double, first_name, last_name, int, name, country, wrap_int> map_type;
        map_type xs;
        using namespace boost::hana::literals;
        REQUIRE(Has(xs, "name"_s));
        REQUIRE(Has(xs, "country"_s));
        REQUIRE(Has(xs, first_name::val));
        REQUIRE(Has(xs, last_name::val));
        REQUIRE(!Has(xs, age::val));
        REQUIRE(!Has(xs, "first_name]"_s));
        REQUIRE(!Has(xs, "first_name]"));
    }

    SECTION("monoid") {
        typedef h::map_v<unsigned, double> map_type1;
        typedef h::map_v<map_type1, int> map_type2;
        typedef h::map_v<map_type2, map_type2> map_type3;
        typedef h::map_v<map_type3> map_type4;
        
        REQUIRE((std::is_same<map_type4::types::data_at<0>, unsigned>::value));
        REQUIRE((std::is_same<map_type4::types::data_at<1>, double>::value));
        REQUIRE((std::is_same<map_type4::types::data_at<2>, int>::value));
        REQUIRE((std::is_same<map_type4::types::data_at<3>, unsigned>::value));
        REQUIRE((std::is_same<map_type4::types::data_at<4>, double>::value));
        REQUIRE((std::is_same<map_type4::types::data_at<5>, int>::value));
        
        double res = 0.0f;
        map_type4 map4(42, 2.4f, 24, 84, 4.8f, 48);
        
        REQUIRE(map4.data<0>() == 42);
        REQUIRE(map4.data<1>() == 2.4f);
        REQUIRE(map4.data<2>() == 24);
        REQUIRE(map4.data<3>() == 84);
        REQUIRE(map4.data<4>() == 4.8f);
        REQUIRE(map4.data<5>() == 48);
        
        map4.visit([&res](auto val){
            res += (double)val;
        });
        REQUIRE(unsigned(res*10) == 2052);
        
        double out = map4.accumulate([](auto val, double out = 0){
            out += (double)val;
            return out;
        });
        REQUIRE(unsigned(out*10) == 2052);
    }
}

TEST_CASE("map hana functionalities", "[hazo]") {
    typedef h::map_d<int, std::string, double, first_name, last_name, int, name, country> map_d_type;
    typedef h::map_v<int, std::string, double, int> map_v_type;
    namespace hana = boost::hana;

    REQUIRE(hana::Comparable<map_v_type>::value);
    REQUIRE(hana::Foldable<map_v_type>::value);
    REQUIRE(hana::Struct<map_v_type>::value);
    REQUIRE(hana::Searchable<map_v_type>::value);
    REQUIRE(hana::size(h::make_map_v(42, 34.5, "World")) == hana::size_c<3>);

    REQUIRE(hana::Comparable<map_d_type>::value);
    REQUIRE(hana::Foldable<map_d_type>::value);
    REQUIRE(hana::Struct<map_d_type>::value);
    REQUIRE(hana::Searchable<map_d_type>::value);
    REQUIRE((hana::size(map_d_type(42, "Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India"))) == hana::size_c<8>));

    auto add = [](auto x, auto y, auto z) {
        return x + y + z;
    };
    auto tpl = h::make_map_v(1, 2, 3);
    REQUIRE(tpl.unpack(add) == 6);
    REQUIRE(hana::unpack(tpl, add) == 6);

    // map_v_type vec_v(42, "Hello", 3.14, 84);
    // REQUIRE(hana::at(vec_v, hana::size_t<0>{}) == 42);
    // REQUIRE(hana::at(vec_v, hana::size_t<1>{}) == "Hello");
    
    // auto to_string = [](auto x) {
    //     std::ostringstream ss;
    //     ss << x;
    //     return ss.str();
    // };
    // REQUIRE(hana::transform(h::make_map_v(1, '2', "345", std::string{"67"}), to_string) == h::make_map_v("1", "2", "345", "67"));
    // auto negate = [](auto x) {
    //     return -x;
    // };
    // REQUIRE(hana::adjust(h::make_map_v(1, 4, 9, 2, 3, 4), 4, negate) == h::make_map_v(1, -4, 9, 2, 3, -4));
    // BOOST_HANA_RUNTIME_CHECK(hana::fill(h::make_map_v(1, '2', 3.3, nullptr), 'x') == h::make_map_v('x', 'x', 'x', 'x'), "");
}

TEST_CASE("map proxy functionalities", "[hazo]") {
    
}