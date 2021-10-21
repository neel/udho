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

TEST_CASE("sequence common node functionalities", "[hazo]") {

    SECTION( "data can be retrieved and modified by position" ) {
        h::seq_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"));

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

    SECTION( "data can be retrieved and modified by type and position" ) {
        h::seq_d<int, std::string, first_name, last_name, age, double, char, std::string, name, country, int, wrap_int, wrap_str, value_str, wrap_int> chain(42, "Fourty Two", "Neel", "Basu", age(32), 4.2, '!', "Fourty Two", name("Neel Basu"), "India", 24, wrap_int(10), wrap_str("Hello World"), value_str("Hi"), wrap_int(64));

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

    GIVEN("a seq_d constructed with elements as well as pod types") {
        first_name f("Neel");
        last_name l("Basu");
        age a(32);
        name n("Neel Basu");
        country c("India");

        h::seq_d<int, first_name, last_name, age, std::string, double, name, country, char> seq_d1(42, "Neel", "Basu", 32, "Hello World", 3.14, "Neel Basu", "India", '!');
        h::seq_d<int, first_name, last_name, age, std::string, double, name, country, char> seq_d2(42, f, l, a, "Hello World", 3.14, n, c, '!');

        THEN( "elements can be retrieved through the element method" ) {
            REQUIRE(std::is_same_v<decltype(seq_d1.element(first_name::val)), first_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1.element(last_name::val)), last_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1.element(age::val)), age&>);
            REQUIRE(std::is_same_v<decltype(seq_d1.element(name::val)), name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1.element(country::val)), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1.element(first_name::val))>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1.element(last_name::val))>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1.element(age::val))>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1.element(name::val))>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1.element(country::val))>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.element(first_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.element(last_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.element(age::val))>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.element(name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.element(country::val))>, std::string>);

            REQUIRE(seq_d1.element(first_name::val) == f);
            REQUIRE(seq_d1.element(last_name::val) == l);
            REQUIRE(seq_d1.element(age::val) == a);
            REQUIRE(seq_d1.element(name::val) == n);
            REQUIRE(seq_d1.element(country::val) == c);

            REQUIRE(seq_d1.element(first_name::val) == "Neel");
            REQUIRE(seq_d1.element(last_name::val) == "Basu");
            REQUIRE(seq_d1.element(age::val) == 32);
            REQUIRE(seq_d1.element(name::val) == "Neel Basu");
            REQUIRE(seq_d1.element(country::val) == "India");

            REQUIRE(std::is_same_v<decltype(seq_d2.element(first_name::val)), first_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2.element(last_name::val)), last_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2.element(age::val)), age&>);
            REQUIRE(std::is_same_v<decltype(seq_d2.element(name::val)), name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2.element(country::val)), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2.element(first_name::val))>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2.element(last_name::val))>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2.element(age::val))>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2.element(name::val))>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2.element(country::val))>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2.element(first_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2.element(last_name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2.element(age::val))>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2.element(name::val))>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2.element(country::val))>, std::string>);

            REQUIRE(seq_d2.element(first_name::val) == f);
            REQUIRE(seq_d2.element(last_name::val) == l);
            REQUIRE(seq_d2.element(age::val) == a);
            REQUIRE(seq_d2.element(name::val) == n);
            REQUIRE(seq_d2.element(country::val) == c);

            REQUIRE(seq_d2.element(first_name::val) == "Neel");
            REQUIRE(seq_d2.element(last_name::val) == "Basu");
            REQUIRE(seq_d2.element(age::val) == 32);
            REQUIRE(seq_d2.element(name::val) == "Neel Basu");
            REQUIRE(seq_d2.element(country::val) == "India");
        }

        THEN( "elements can be retrieved through operator[] using element handle" ) {
            REQUIRE(std::is_same_v<decltype(seq_d1[first_name::val]), first_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1[last_name::val]), last_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1[age::val]), age&>);
            REQUIRE(std::is_same_v<decltype(seq_d1[name::val]), name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1[country::val]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1[first_name::val])>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1[last_name::val])>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1[age::val])>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1[name::val])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1[country::val])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1[first_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1[last_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1[age::val])>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1[name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1[country::val])>, std::string>);

            REQUIRE(seq_d1[first_name::val] == f);
            REQUIRE(seq_d1[last_name::val] == l);
            REQUIRE(seq_d1[age::val] == a);
            REQUIRE(seq_d1[name::val] == n);
            REQUIRE(seq_d1[country::val] == c);

            REQUIRE(seq_d1[first_name::val] == "Neel");
            REQUIRE(seq_d1[last_name::val] == "Basu");
            REQUIRE(seq_d1[age::val] == 32);
            REQUIRE(seq_d1[name::val] == "Neel Basu");
            REQUIRE(seq_d1[country::val] == "India");

            REQUIRE(std::is_same_v<decltype(seq_d2[first_name::val]), first_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2[last_name::val]), last_name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2[age::val]), age&>);
            REQUIRE(std::is_same_v<decltype(seq_d2[name::val]), name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2[country::val]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2[first_name::val])>, first_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2[last_name::val])>, last_name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2[age::val])>, age>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2[name::val])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2[country::val])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2[first_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2[last_name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2[age::val])>, std::size_t>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2[name::val])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2[country::val])>, std::string>);

            REQUIRE(seq_d2[first_name::val] == f);
            REQUIRE(seq_d2[last_name::val] == l);
            REQUIRE(seq_d2[age::val] == a);
            REQUIRE(seq_d2[name::val] == n);
            REQUIRE(seq_d2[country::val] == c);

            REQUIRE(seq_d2[first_name::val] == "Neel");
            REQUIRE(seq_d2[last_name::val] == "Basu");
            REQUIRE(seq_d2[age::val] == 32);
            REQUIRE(seq_d2[name::val] == "Neel Basu");
            REQUIRE(seq_d2[country::val] == "India");
        }

        THEN( "elements can be retrieved through operator[] using key" ) {
            using namespace boost::hana::literals;
            REQUIRE(std::is_same_v<decltype(seq_d1["name"_s]), name&>);
            REQUIRE(std::is_same_v<decltype(seq_d1["country"_s]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1["name"_s])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d1["country"_s])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1["name"_s])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1["country"_s])>, std::string>);

            REQUIRE(seq_d1["name"_s] == n);
            REQUIRE(seq_d1["country"_s] == c);

            REQUIRE(seq_d1["name"_s] == "Neel Basu");
            REQUIRE(seq_d1["country"_s] == "India");

            REQUIRE(std::is_same_v<decltype(seq_d2["name"_s]), name&>);
            REQUIRE(std::is_same_v<decltype(seq_d2["country"_s]), country&>);

            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2["name"_s])>, name>);
            REQUIRE(std::is_same_v<std::decay_t<decltype(seq_d2["country"_s])>, country>);

            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2["name"_s])>, std::string>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d2["country"_s])>, std::string>);

            REQUIRE(seq_d2["name"_s] == n);
            REQUIRE(seq_d2["country"_s] == c);

            REQUIRE(seq_d2["name"_s] == "Neel Basu");
            REQUIRE(seq_d2["country"_s] == "India");
        }

        THEN( "data and value methods work in the expected way" ) {
            REQUIRE(seq_d1.data<0>() == 42);
            REQUIRE(seq_d1.data<1>() == f);
            REQUIRE(seq_d1.data<1>() == "Neel");
            REQUIRE(seq_d1.data<6>() == n);
            REQUIRE(seq_d1.data<6>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(seq_d1.data<0>()), int&>);
            REQUIRE(std::is_same_v<decltype(seq_d1.data<1>()), first_name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.data<1>())>, std::string>);
            REQUIRE(std::is_same_v<decltype(seq_d1.data<6>()), name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.data<6>())>, std::string>);

            REQUIRE(seq_d1.value<0>() == 42);
            REQUIRE(seq_d1.value<1>() == "Neel");
            REQUIRE(seq_d1.value<6>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(seq_d1.value<0>()), int&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.value<1>())>, first_name>);
            REQUIRE(std::is_same_v<decltype(seq_d1.value<1>()), std::string&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.value<6>())>, name>);
            REQUIRE(std::is_same_v<decltype(seq_d1.value<6>()), std::string&>);

            REQUIRE(seq_d1.data<int>() == 42);
            REQUIRE(seq_d1.data<first_name>() == f);
            REQUIRE(seq_d1.data<first_name>() == "Neel");
            REQUIRE(seq_d1.data<name>() == n);
            REQUIRE(seq_d1.data<name>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(seq_d1.data<int>()), int&>);
            REQUIRE(std::is_same_v<decltype(seq_d1.data<first_name>()), first_name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.data<first_name>())>, std::string>);
            REQUIRE(std::is_same_v<decltype(seq_d1.data<name>()), name&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.data<name>())>, std::string>);

            REQUIRE(seq_d1.value<int>() == 42);
            REQUIRE(seq_d1.value<first_name>() == "Neel");
            REQUIRE(seq_d1.value<name>() == "Neel Basu");

            REQUIRE(std::is_same_v<decltype(seq_d1.value<int>()), int&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.value<first_name>())>, first_name>);
            REQUIRE(std::is_same_v<decltype(seq_d1.value<first_name>()), std::string&>);
            REQUIRE(!std::is_same_v<std::decay_t<decltype(seq_d1.value<name>())>, name>);
            REQUIRE(std::is_same_v<decltype(seq_d1.value<name>()), std::string&>);
        }
    }
}

TEST_CASE("sequence specific functionalities", "[hazo]") {
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

TEST_CASE("sequence hana functionalities", "[hazo]") {
    
}

TEST_CASE("sequence proxy functionalities", "[hazo]") {
    
}