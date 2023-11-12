#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/map/element.h>
#include <udho/hazo/map/hana.h>
#include <udho/hazo/seq.h>
#include <udho/hazo/seq/hana.h>
#include <boost/hana/string.hpp>
#include <string>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

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

TEST_CASE("sequence common node functionalities", "[hazo]") {

    SECTION( "data can be retrieved and modified by position" ) {
        h::seq_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"));

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

    SECTION( "value can be retrieved and modified by position" ) {
        h::seq_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"));

        CHECK(chain.value<0>()  == 42);
        CHECK(chain.value<1>()  == "Fourty Two");
        CHECK(chain.value<2>()  == "Neel");
        CHECK(chain.value<3>()  == "Basu");
        CHECK(chain.value<4>()  == 32);
        CHECK(chain.value<5>()  == 4.2);
        CHECK(chain.value<6>()  == '!');
        CHECK(chain.value<7>()  == "Fourty Two");
        CHECK(chain.value<8>()  == "Neel Basu");
        CHECK(chain.value<9>()  == "India");
        CHECK(chain.value<10>() == 24);
        CHECK(chain.value<11>() == wrap_int(10));
        CHECK(chain.value<12>() == wrap_str("Hello World"));
        CHECK(chain.value<13>() == "Hi");

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

        CHECK(chain.value<0>()  == 24);
        CHECK(chain.value<1>()  == "Twenty Four");
        CHECK(chain.value<2>()  == "Sunanda");
        CHECK(chain.value<3>()  == "Bose");
        CHECK(chain.value<4>()  == 33);
        CHECK(chain.value<5>()  == 2.4);
        CHECK(chain.value<6>()  == '?');
        CHECK(chain.value<7>()  == "Twenty Four");
        CHECK(chain.value<8>()  == "Sunanda Bose");
        CHECK(chain.value<9>()  == "India");
        CHECK(chain.value<10>() == 42);
        CHECK(chain.value<11>() == wrap_int(20));
        CHECK(chain.value<12>() == wrap_str("Hello"));
        CHECK(chain.value<13>() == "Hi!");
    }

    SECTION( "data can be retrieved and modified by type and position" ) {
        h::seq_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str, wrap_int> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"), wrap_int(64));

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

    SECTION( "value can be retrieved and modified by type and position" ) {
        h::seq_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str, wrap_int> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"), wrap_int(64));

        CHECK(chain.value<int, 0>()            == 42);
        CHECK(chain.value<std::string, 0>()    == "Fourty Two");
        CHECK(chain.value<first_name, 0>()     == "Neel");
        CHECK(chain.value<last_name, 0>()      == "Basu");
        CHECK(chain.value<age, 0>()            == 32);
        CHECK(chain.value<double, 0>()         == 4.2);
        CHECK(chain.value<char, 0>()           == '!');
        CHECK(chain.value<std::string, 1>()    == "Fourty Two");
        CHECK(chain.value<name, 0>()           == "Neel Basu");
        CHECK(chain.value<country, 0>()        == "India");
        CHECK(chain.value<int, 1>()            == 24);
        CHECK(chain.value<wrap_int_index, 0>() == wrap_int(10));
        CHECK(chain.value<wrap_str>()          == wrap_str("Hello World"));
        CHECK(chain.value<value_str>()         == "Hi");
        CHECK(chain.value<wrap_int_index, 1>() == wrap_int(64));

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

        CHECK(chain.value<int, 0>()            == 24);
        CHECK(chain.value<std::string, 0>()    == "Twenty Four");
        CHECK(chain.value<first_name, 0>()     == "Sunanda");
        CHECK(chain.value<last_name, 0>()      == "Bose");
        CHECK(chain.value<age, 0>()            == 33);
        CHECK(chain.value<double, 0>()         == 2.4);
        CHECK(chain.value<char, 0>()           == '?');
        CHECK(chain.value<std::string, 1>()    == "Twenty Four");
        CHECK(chain.value<name, 0>()           == "Sunanda Bose");
        CHECK(chain.value<country, 0>()        == "India");
        CHECK(chain.value<int, 1>()            == 42);
        CHECK(chain.value<wrap_int_index, 0>() == wrap_int(20));
        CHECK(chain.value<wrap_str>()          == wrap_str("Hello"));
        CHECK(chain.value<value_str>()         == "Hi!");
        CHECK(chain.value<wrap_int_index, 1>() == wrap_int(46));
    }

    GIVEN("a seq_d constructed with elements as well as pod types") {
        first_name f("Neel");
        last_name l("Basu");
        age a(32);
        name n("Neel Basu");
        country c("India");

        h::seq_d<int, first_name, last_name, age, std::string, double, name, country, char> seq_d1(42, "Neel", "Basu", 32, "Hello World", 3.14, "Neel Basu", "India", '!');
        h::seq_d<int, first_name, last_name, age, std::string, double, name, country, char> seq_d2(42, f, l, a, "Hello World", 3.14, n, c, '!');

        THEN( "elements can be retrieved through the element method" ) {
            CHECK(std::is_same_v<decltype(seq_d1.element(first_name::val)), first_name&>);
            CHECK(std::is_same_v<decltype(seq_d1.element(last_name::val)), last_name&>);
            CHECK(std::is_same_v<decltype(seq_d1.element(age::val)), age&>);
            CHECK(std::is_same_v<decltype(seq_d1.element(name::val)), name&>);
            CHECK(std::is_same_v<decltype(seq_d1.element(country::val)), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1.element(first_name::val))>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1.element(last_name::val))>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1.element(age::val))>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1.element(name::val))>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1.element(country::val))>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.element(first_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.element(last_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.element(age::val))>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.element(name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.element(country::val))>, std::string>);

            CHECK(seq_d1.element(first_name::val) == f);
            CHECK(seq_d1.element(last_name::val) == l);
            CHECK(seq_d1.element(age::val) == a);
            CHECK(seq_d1.element(name::val) == n);
            CHECK(seq_d1.element(country::val) == c);

            CHECK(seq_d1.element(first_name::val) == "Neel");
            CHECK(seq_d1.element(last_name::val) == "Basu");
            CHECK(seq_d1.element(age::val) == 32);
            CHECK(seq_d1.element(name::val) == "Neel Basu");
            CHECK(seq_d1.element(country::val) == "India");

            CHECK(std::is_same_v<decltype(seq_d2.element(first_name::val)), first_name&>);
            CHECK(std::is_same_v<decltype(seq_d2.element(last_name::val)), last_name&>);
            CHECK(std::is_same_v<decltype(seq_d2.element(age::val)), age&>);
            CHECK(std::is_same_v<decltype(seq_d2.element(name::val)), name&>);
            CHECK(std::is_same_v<decltype(seq_d2.element(country::val)), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2.element(first_name::val))>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2.element(last_name::val))>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2.element(age::val))>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2.element(name::val))>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2.element(country::val))>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2.element(first_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2.element(last_name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2.element(age::val))>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2.element(name::val))>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2.element(country::val))>, std::string>);

            CHECK(seq_d2.element(first_name::val) == f);
            CHECK(seq_d2.element(last_name::val) == l);
            CHECK(seq_d2.element(age::val) == a);
            CHECK(seq_d2.element(name::val) == n);
            CHECK(seq_d2.element(country::val) == c);

            CHECK(seq_d2.element(first_name::val) == "Neel");
            CHECK(seq_d2.element(last_name::val) == "Basu");
            CHECK(seq_d2.element(age::val) == 32);
            CHECK(seq_d2.element(name::val) == "Neel Basu");
            CHECK(seq_d2.element(country::val) == "India");
        }

        THEN( "elements can be retrieved through operator[] using element handle" ) {
            CHECK(std::is_same_v<decltype(seq_d1[first_name::val]), first_name&>);
            CHECK(std::is_same_v<decltype(seq_d1[last_name::val]), last_name&>);
            CHECK(std::is_same_v<decltype(seq_d1[age::val]), age&>);
            CHECK(std::is_same_v<decltype(seq_d1[name::val]), name&>);
            CHECK(std::is_same_v<decltype(seq_d1[country::val]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1[first_name::val])>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1[last_name::val])>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1[age::val])>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1[name::val])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1[country::val])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1[first_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1[last_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1[age::val])>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1[name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1[country::val])>, std::string>);

            CHECK(seq_d1[first_name::val] == f);
            CHECK(seq_d1[last_name::val] == l);
            CHECK(seq_d1[age::val] == a);
            CHECK(seq_d1[name::val] == n);
            CHECK(seq_d1[country::val] == c);

            CHECK(seq_d1[first_name::val] == "Neel");
            CHECK(seq_d1[last_name::val] == "Basu");
            CHECK(seq_d1[age::val] == 32);
            CHECK(seq_d1[name::val] == "Neel Basu");
            CHECK(seq_d1[country::val] == "India");

            CHECK(std::is_same_v<decltype(seq_d2[first_name::val]), first_name&>);
            CHECK(std::is_same_v<decltype(seq_d2[last_name::val]), last_name&>);
            CHECK(std::is_same_v<decltype(seq_d2[age::val]), age&>);
            CHECK(std::is_same_v<decltype(seq_d2[name::val]), name&>);
            CHECK(std::is_same_v<decltype(seq_d2[country::val]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2[first_name::val])>, first_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2[last_name::val])>, last_name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2[age::val])>, age>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2[name::val])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2[country::val])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2[first_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2[last_name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2[age::val])>, std::size_t>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2[name::val])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2[country::val])>, std::string>);

            CHECK(seq_d2[first_name::val] == f);
            CHECK(seq_d2[last_name::val] == l);
            CHECK(seq_d2[age::val] == a);
            CHECK(seq_d2[name::val] == n);
            CHECK(seq_d2[country::val] == c);

            CHECK(seq_d2[first_name::val] == "Neel");
            CHECK(seq_d2[last_name::val] == "Basu");
            CHECK(seq_d2[age::val] == 32);
            CHECK(seq_d2[name::val] == "Neel Basu");
            CHECK(seq_d2[country::val] == "India");
        }

        THEN( "elements can be retrieved through operator[] using key" ) {
            using namespace boost::hana::literals;
            CHECK(std::is_same_v<decltype(seq_d1["name"_s]), name&>);
            CHECK(std::is_same_v<decltype(seq_d1["country"_s]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1["name"_s])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d1["country"_s])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1["name"_s])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1["country"_s])>, std::string>);

            CHECK(seq_d1["name"_s] == n);
            CHECK(seq_d1["country"_s] == c);

            CHECK(seq_d1["name"_s] == "Neel Basu");
            CHECK(seq_d1["country"_s] == "India");

            CHECK(std::is_same_v<decltype(seq_d2["name"_s]), name&>);
            CHECK(std::is_same_v<decltype(seq_d2["country"_s]), country&>);

            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2["name"_s])>, name>);
            CHECK(std::is_same_v<std::decay_t<decltype(seq_d2["country"_s])>, country>);

            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2["name"_s])>, std::string>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d2["country"_s])>, std::string>);

            CHECK(seq_d2["name"_s] == n);
            CHECK(seq_d2["country"_s] == c);

            CHECK(seq_d2["name"_s] == "Neel Basu");
            CHECK(seq_d2["country"_s] == "India");
        }

        THEN( "data and value methods work in the expected way" ) {
            CHECK(seq_d1.data<0>() == 42);
            CHECK(seq_d1.data<1>() == f);
            CHECK(seq_d1.data<1>() == "Neel");
            CHECK(seq_d1.data<6>() == n);
            CHECK(seq_d1.data<6>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(seq_d1.data<0>()), int&>);
            CHECK(std::is_same_v<decltype(seq_d1.data<1>()), first_name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.data<1>())>, std::string>);
            CHECK(std::is_same_v<decltype(seq_d1.data<6>()), name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.data<6>())>, std::string>);

            CHECK(seq_d1.value<0>() == 42);
            CHECK(seq_d1.value<1>() == "Neel");
            CHECK(seq_d1.value<6>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(seq_d1.value<0>()), int&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.value<1>())>, first_name>);
            CHECK(std::is_same_v<decltype(seq_d1.value<1>()), std::string&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.value<6>())>, name>);
            CHECK(std::is_same_v<decltype(seq_d1.value<6>()), std::string&>);

            CHECK(seq_d1.data<int>() == 42);
            CHECK(seq_d1.data<first_name>() == f);
            CHECK(seq_d1.data<first_name>() == "Neel");
            CHECK(seq_d1.data<name>() == n);
            CHECK(seq_d1.data<name>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(seq_d1.data<int>()), int&>);
            CHECK(std::is_same_v<decltype(seq_d1.data<first_name>()), first_name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.data<first_name>())>, std::string>);
            CHECK(std::is_same_v<decltype(seq_d1.data<name>()), name&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.data<name>())>, std::string>);

            CHECK(seq_d1.value<int>() == 42);
            CHECK(seq_d1.value<first_name>() == "Neel");
            CHECK(seq_d1.value<name>() == "Neel Basu");

            CHECK(std::is_same_v<decltype(seq_d1.value<int>()), int&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.value<first_name>())>, first_name>);
            CHECK(std::is_same_v<decltype(seq_d1.value<first_name>()), std::string&>);
            CHECK(!std::is_same_v<std::decay_t<decltype(seq_d1.value<name>())>, name>);
            CHECK(std::is_same_v<decltype(seq_d1.value<name>()), std::string&>);
        }
    }
}

TEST_CASE("sequence specific functionalities", "[hazo]") {
    typedef h::seq_d<int, std::string, double, first_name, last_name, int, name, country> seq_d_type;
    CHECK(h::make_seq_d(4.2, 85) == h::make_seq_d(4.2, 85));
    CHECK(h::make_seq_d(4.2, 85) != h::make_seq_d(4.2, 85, "Hello World"));

    SECTION("one or more types can be excluded") {
        CHECK(seq_d_type::exclude<int, std::string>(4.2, "Neel", "Basu", 85, "Neel Basu", "India") == h::make_seq_d(4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India")));
        CHECK(seq_d_type::exclude<int, int>("Hello", 4.2, "Neel", "Basu", "Neel Basu", "India") == h::make_seq_d("Hello", 4.2, first_name("Neel"), last_name("Basu"), name("Neel Basu"), country("India")));
        CHECK(seq_d_type::exclude<int>("Hello", 4.2, "Neel", "Basu", 85, "Neel Basu", "India") == h::make_seq_d("Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India")));
        CHECK(seq_d_type::exclude<first_name, last_name>(42, "Hello", 4.2, 85, "Neel Basu", "India") == h::make_seq_d(42, "Hello", 4.2, 85, name("Neel Basu"), country("India")));

        CHECK(std::is_same<seq_d_type::exclude_if<std::is_pod>, h::seq_d<std::string, first_name, last_name, name, country>>::value);
    }

    SECTION("one or more types can be included") {
        CHECK(seq_d_type::extend<int, std::string>(42, "Hello", 4.2, "Neel", "Basu", 85, "Neel Basu", "India", 10, "Hi") == h::make_seq_d(42, "Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India"), 10, "Hi"));
    }

    SECTION("contains") {
        typedef h::seq_d<int, std::string, double, first_name, last_name, int, name, country, wrap_int> sequence_type;

        CHECK(sequence_type::contains<int>::value);
        CHECK(sequence_type::contains<std::string>::value);
        CHECK(sequence_type::contains<double>::value);
        CHECK(sequence_type::contains<first_name>::value);
        CHECK(sequence_type::contains<last_name>::value);
        CHECK(sequence_type::contains<name>::value);
        CHECK(sequence_type::contains<country>::value);
        CHECK(sequence_type::contains<wrap_int_index>::value);
        CHECK(!sequence_type::contains<wrap_int>::value);
        CHECK(!sequence_type::contains<value_str>::value);
    }

    SECTION("has") {
        typedef h::seq_d<int, std::string, double, first_name, last_name, int, name, country, wrap_int> sequence_type;
        sequence_type xs;
        using namespace boost::hana::literals;
        CHECK(Has(xs, "name"_s));
        CHECK(Has(xs, "country"_s));
        CHECK(Has(xs, first_name::val));
        CHECK(Has(xs, last_name::val));
        CHECK(!Has(xs, age::val));
        CHECK(!Has(xs, "first_name]"_s));
        CHECK(!Has(xs, "first_name]"));
    }

    SECTION("monoid") {
        typedef h::seq_v<unsigned, double> seq_type1;
        typedef h::seq_v<seq_type1, int> seq_type2;
        typedef h::seq_v<seq_type2, seq_type2> seq_type3;
        typedef h::seq_v<seq_type3> seq_type4;
        
        CHECK((std::is_same<seq_type4::types::data_at<0>, unsigned>::value));
        CHECK((std::is_same<seq_type4::types::data_at<1>, double>::value));
        CHECK((std::is_same<seq_type4::types::data_at<2>, int>::value));
        CHECK((std::is_same<seq_type4::types::data_at<3>, unsigned>::value));
        CHECK((std::is_same<seq_type4::types::data_at<4>, double>::value));
        CHECK((std::is_same<seq_type4::types::data_at<5>, int>::value));
        
        double res = 0.0f;
        seq_type4 seq4(42, 2.4f, 24, 84, 4.8f, 48);
        
        CHECK(seq4.data<0>() == 42);
        CHECK(seq4.data<1>() == 2.4f);
        CHECK(seq4.data<2>() == 24);
        CHECK(seq4.data<3>() == 84);
        CHECK(seq4.data<4>() == 4.8f);
        CHECK(seq4.data<5>() == 48);
        
        seq4.visit([&res](auto val){
            res += (double)val;
        });
        CHECK(unsigned(res*10) == 2052);
        
        double out = seq4.accumulate([](auto val, double out = 0){
            out += (double)val;
            return out;
        });
        CHECK(unsigned(out*10) == 2052);
    }

    SECTION("concat") {
        typedef h::seq_d<int, double> seq1_type;
        typedef h::seq_d<first_name> seq2_type;

        seq1_type seq1{42, 4.2};
        seq2_type seq2{"Neel Basu"};

        auto seq3 = seq1.concat(seq2);
        std::cout << seq3 << std::endl;

        // decltype(seq3)::x;
    }
}

TEST_CASE("sequence hana functionalities", "[hazo]") {
    typedef h::seq_d<int, std::string, double, first_name, last_name, int, name, country> seq_d_type;
    typedef h::seq_v<int, std::string, double, int> seq_v_type;
    namespace hana = boost::hana;

    CHECK((hana::Comparable<seq_v_type>::value));
    CHECK((hana::Foldable<seq_v_type>::value));
    CHECK((hana::Iterable<seq_v_type>::value));
    CHECK(hana::size(h::make_seq_v(42, 34.5, "World")) == hana::size_c<3>());

    CHECK((hana::Comparable<seq_d_type>::value));
    CHECK((hana::Foldable<seq_d_type>::value));
    CHECK((hana::Iterable<seq_d_type>::value));
    CHECK((hana::size(seq_d_type(42, "Hello", 4.2, first_name("Neel"), last_name("Basu"), 85, name("Neel Basu"), country("India"))) == hana::size_c<8>()));

    auto add = [](auto x, auto y, auto z) {
        return x + y + z;
    };
    auto tpl = h::make_seq_v(1, 2, 3);
    CHECK(tpl.unpack(add) == 6);
    CHECK(hana::unpack(tpl, add) == 6);

    seq_v_type vec_v(42, "Hello", 3.14, 84);
    CHECK(hana::at(vec_v, hana::size_t<0>{}) == 42);
    CHECK(hana::at(vec_v, hana::size_t<1>{}) == "Hello");
    
    auto to_string = [](auto x) {
        std::ostringstream ss;
        ss << x;
        return ss.str();
    };
    CHECK(hana::transform(h::make_seq_v(1, '2', "345", std::string{"67"}), to_string) == h::make_seq_v("1", "2", "345", "67"));
    auto negate = [](auto x) {
        return -x;
    };
    CHECK(hana::adjust(h::make_seq_v(1, 4, 9, 2, 3, 4), 4, negate) == h::make_seq_v(1, -4, 9, 2, 3, -4));
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-comparison"
    BOOST_HANA_RUNTIME_CHECK(hana::fill(h::make_seq_v(1, '2', 3.3, nullptr), 'x') == h::make_seq_v('x', 'x', 'x', 'x'), "");
    #pragma GCC diagnostic pop

}

TEST_CASE("sequence proxy functionalities", "[hazo]") {
    
}
