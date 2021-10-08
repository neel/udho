#include "udho/hazo/node/fwd.h"
#include <type_traits>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/node/node.h>

#include <string>

namespace h = udho::hazo;

namespace plain{
    using n1_t = h::node<int>;
    using n2_t = h::node<int, h::node<double>>;
    using n3_t = h::node<int, h::node<std::string, h::node<double>>>;

    namespace meta{
        using n1_t = h::meta_node<int>;
        using n2_t = h::meta_node<int, h::meta_node<double>>;
        using n3_t = h::meta_node<int, h::meta_node<std::string, h::meta_node<double>>>;
    }
}

struct no_arg{
    int _v;

    inline no_arg(): _v(42){}
};
struct wrap_int{
    int _v;

    inline wrap_int() = default;
    inline explicit wrap_int(const wrap_int&) = default;
    inline explicit wrap_int(int v): _v(v){}
};
struct wrap_str{
    std::string _v;

    inline wrap_str() = default;
    inline explicit wrap_str(const wrap_str&) = default;
    inline explicit wrap_str(const char*& v): _v(v){}
    inline explicit wrap_str(const std::string& v): _v(v){}
};
struct value_str{
    typedef std::string value_type;

    std::string _v;

    inline value_str() = default;
    inline explicit value_str(const value_str&) = default;
    inline explicit value_str(const char*& v): _v(v){}
    inline explicit value_str(const std::string& v): _v(v){}
    inline const std::string& value() const { return _v; }
};

namespace complex{
    using n1_t = h::node<no_arg>;
    using n2_t = h::node<no_arg, h::node<wrap_int>>;
    using n3_t = h::node<no_arg, h::node<wrap_int, h::node<wrap_str>>>;
    using n4_t = h::node<no_arg, h::node<wrap_int, h::node<wrap_str, h::node<value_str>>>>;

    namespace meta{
        using n1_t = h::meta_node<no_arg>;
        using n2_t = h::meta_node<no_arg, h::meta_node<wrap_int>>;
        using n3_t = h::meta_node<no_arg, h::meta_node<wrap_int, h::meta_node<wrap_str>>>;
        using n4_t = h::meta_node<no_arg, h::meta_node<wrap_int, h::meta_node<wrap_str, h::meta_node<value_str>>>>;
    }
}

TEST_CASE( "depth", "hazo::node" ) {
    REQUIRE(plain::n1_t::depth   == 0);
    REQUIRE(plain::n2_t::depth   == 1);
    REQUIRE(plain::n3_t::depth   == 2);
    REQUIRE(complex::n1_t::depth == 0);
    REQUIRE(complex::n2_t::depth == 1);
    REQUIRE(complex::n3_t::depth == 2);
    REQUIRE(complex::n4_t::depth == 3);
}

TEST_CASE( "types", "hazo::meta_node" ) {
    REQUIRE(std::is_void_v<plain::meta::n1_t::tail_type>);
    REQUIRE(!std::is_void_v<plain::meta::n2_t::tail_type>);
    REQUIRE(std::is_void_v<plain::meta::n2_t::tail_type::tail_type>);

    REQUIRE(std::is_void_v<complex::meta::n1_t::tail_type>);
    REQUIRE(!std::is_void_v<complex::meta::n2_t::tail_type>);
    REQUIRE(std::is_void_v<complex::meta::n2_t::tail_type::tail_type>);
    REQUIRE(std::is_void_v<complex::meta::n3_t::tail_type::tail_type::tail_type>);
    REQUIRE(std::is_void_v<complex::meta::n4_t::tail_type::tail_type::tail_type::tail_type>);

    SECTION( "tail_type" ){
        REQUIRE(std::is_same_v<plain::meta::n2_t::tail_type, h::meta_node<double>>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::tail_type, h::meta_node<std::string, h::meta_node<double>>>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::tail_type::tail_type, h::meta_node<double>>);
        REQUIRE(std::is_void_v<plain::meta::n3_t::tail_type::tail_type::tail_type>);

        REQUIRE(std::is_same_v<complex::meta::n2_t::tail_type, h::meta_node<wrap_int>>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::tail_type, h::meta_node<wrap_int, h::meta_node<wrap_str>>>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::tail_type::tail_type, h::meta_node<wrap_str>>);
        REQUIRE(std::is_void_v<complex::meta::n3_t::tail_type::tail_type::tail_type>);
    }

    SECTION( "tail_at" ){
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::tail_at<0>, plain::meta::n1_t::tail_type>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::tail_at<0>, plain::meta::n2_t::tail_type>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::tail_at<0>, plain::meta::n3_t::tail_type>);

        REQUIRE(std::is_same_v<complex::meta::n1_t::types::tail_at<0>, complex::meta::n1_t::tail_type>);
        REQUIRE(std::is_same_v<complex::meta::n2_t::types::tail_at<0>, complex::meta::n2_t::tail_type>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::types::tail_at<0>, complex::meta::n3_t::tail_type>);

        REQUIRE(std::is_same_v<plain::meta::n1_t::types::tail_at<0>, void>);
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<plain::meta::n2_t::types::tail_at<0>, h::meta_node<double>>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<plain::meta::n3_t::types::tail_at<0>, h::meta_node<std::string, h::meta_node<double>>>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::tail_at<1>, h::meta_node<double>>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::tail_at<3>, void>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<complex::meta::n1_t::types::tail_at<0>, void>);
        REQUIRE(std::is_same_v<complex::meta::n1_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<complex::meta::n1_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<complex::meta::n2_t::types::tail_at<0>, h::meta_node<wrap_int>>);
        REQUIRE(std::is_same_v<complex::meta::n2_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<complex::meta::n2_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<complex::meta::n2_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<complex::meta::n3_t::types::tail_at<0>, h::meta_node<wrap_int, h::meta_node<wrap_str>>>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::types::tail_at<1>, h::meta_node<wrap_str>>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::types::tail_at<3>, void>);
        REQUIRE(std::is_same_v<complex::meta::n3_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<complex::meta::n4_t::types::tail_at<2>, h::meta_node<value_str>>);
        REQUIRE(std::is_same_v<complex::meta::n4_t::types::tail_at<3>, void>);
        REQUIRE(std::is_same_v<complex::meta::n4_t::types::tail_at<99>, void>);
    }

    SECTION( "capsule_at" ){
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::capsule_at<0>, h::capsule<int>>);

        REQUIRE(std::is_same_v<plain::meta::n2_t::types::capsule_at<0>, h::capsule<int>>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::capsule_at<1>, h::capsule<double>>);

        REQUIRE(std::is_same_v<plain::meta::n3_t::types::capsule_at<0>, h::capsule<int>>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::capsule_at<1>, h::capsule<std::string>>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::capsule_at<2>, h::capsule<double>>);
    }

    SECTION( "data_at" ){
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::data_at<0>, int>);

        REQUIRE(std::is_same_v<plain::meta::n2_t::types::data_at<0>, int>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::data_at<1>, double>);
        
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::data_at<0>, int>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::data_at<1>, std::string>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::data_at<2>, double>);
    }

    SECTION( "value_at" ){
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::value_at<0>, int>);

        REQUIRE(std::is_same_v<plain::meta::n2_t::types::value_at<0>, int>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::value_at<1>, double>);
        
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::value_at<0>, int>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::value_at<1>, std::string>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::value_at<2>, double>);
    }

    SECTION( "data_of" ){
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::data_of<int>, int>);

        REQUIRE(std::is_same_v<plain::meta::n2_t::types::data_of<int>, int>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::data_of<double>, double>);
        
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::data_of<int>, int>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::data_of<std::string>, std::string>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::data_of<double>, double>);
    }

    SECTION( "value_of" ){
        REQUIRE(std::is_same_v<plain::meta::n1_t::types::value_of<int>, int>);

        REQUIRE(std::is_same_v<plain::meta::n2_t::types::value_of<int>, int>);
        REQUIRE(std::is_same_v<plain::meta::n2_t::types::value_of<double>, double>);
        
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::value_of<int>, int>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::value_of<std::string>, std::string>);
        REQUIRE(std::is_same_v<plain::meta::n3_t::types::value_of<double>, double>);
    }

    SECTION( "exists" ){
        REQUIRE(plain::meta::n1_t::types::exists<int>::value);
        REQUIRE(!plain::meta::n1_t::types::exists<double>::value);
        REQUIRE(plain::meta::n2_t::types::exists<int>::value);
        REQUIRE(plain::meta::n2_t::types::exists<double>::value);
        REQUIRE(!plain::meta::n2_t::types::exists<std::string>::value);
        REQUIRE(plain::meta::n3_t::types::exists<int>::value);
        REQUIRE(plain::meta::n3_t::types::exists<std::string>::value);
        REQUIRE(plain::meta::n3_t::types::exists<double>::value);
    }
}

TEST_CASE( "construction", "hazo::node" ) {

    SECTION( "not construction with more values" ){
        REQUIRE(!std::is_constructible_v<plain::n1_t, int, int>);
        REQUIRE(!std::is_constructible_v<plain::n2_t, int, double, double>);
        REQUIRE(!std::is_constructible_v<plain::n3_t, int, std::string, double, double>);
    }

    SECTION( "construction with all values" ){
        REQUIRE(std::is_constructible_v<plain::n1_t, int>);
        REQUIRE(std::is_constructible_v<plain::n2_t, int, double>);
        REQUIRE(std::is_constructible_v<plain::n3_t, int, std::string, double>);
        REQUIRE(std::is_constructible_v<plain::n2_t, double, int>);
        REQUIRE(!std::is_constructible_v<plain::n3_t, int, double, std::string>);
    }

    SECTION( "construction with few values" ){
        REQUIRE(std::is_constructible_v<plain::n1_t, int>);
        REQUIRE(std::is_constructible_v<plain::n2_t, int>);
        REQUIRE(std::is_constructible_v<plain::n2_t, double>);
        REQUIRE(std::is_constructible_v<plain::n3_t, int>);
        REQUIRE(std::is_constructible_v<plain::n3_t, double>);

        REQUIRE(!std::is_constructible_v<plain::n3_t, int, double>);
        REQUIRE(!std::is_constructible_v<plain::n3_t, int, int>);
        REQUIRE(!std::is_constructible_v<plain::n3_t, double, int>);
        REQUIRE(!std::is_constructible_v<plain::n3_t, double, double>);
        REQUIRE(std::is_constructible_v<plain::n3_t, int, std::string>);
        REQUIRE(std::is_constructible_v<plain::n3_t, double, std::string>);
    }

    SECTION( "construction with no values" ){
        REQUIRE(std::is_default_constructible_v<plain::n1_t>);
        REQUIRE(std::is_default_constructible_v<plain::n2_t>);
        REQUIRE(std::is_default_constructible_v<plain::n3_t>);
    }

    SECTION( "construction using complex types with all values" ){
        REQUIRE(std::is_constructible_v<h::capsule<int>, int>);
        REQUIRE(std::is_constructible_v<h::capsule<int>, double>);
        REQUIRE(std::is_constructible_v<h::capsule<wrap_int>, wrap_int>);
        REQUIRE(std::is_constructible_v<h::capsule<wrap_int>, int>);
        REQUIRE(std::is_constructible_v<h::capsule<wrap_int>, double>);
        REQUIRE(!std::is_constructible_v<h::capsule<value_str>, double>);
        REQUIRE(!std::is_constructible_v<h::capsule<value_str>, int>);

        REQUIRE(std::is_same_v<h::capsule<value_str>::value_type, std::string>);
        REQUIRE(std::is_same_v<decltype(std::declval<const h::capsule<value_str>&>().value()), decltype(std::declval<const value_str&>().value())>);

        REQUIRE(std::is_constructible_v<h::node<int>, int>);
        REQUIRE(std::is_constructible_v<h::node<wrap_int>, wrap_int>);
        REQUIRE(std::is_constructible_v<h::node<wrap_int>, int>);
        REQUIRE(std::is_constructible_v<h::node<int, h::node<wrap_int>>, int>);
        REQUIRE(std::is_constructible_v<h::node<int, h::node<wrap_int>>, int, wrap_int>);
    }
}