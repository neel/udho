#include "udho/hazo/node/encapsulate.h"
#include "udho/hazo/node/fwd.h"
#include <type_traits>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo/node/node.h>
#include <udho/hazo/node/meta.h>
#include <string>

namespace h = udho::hazo;

namespace pod{
    using n1_t = h::node<int>;
    using n2_t = h::node<int, double>;
    using n3_t = h::node<int, std::string, double>;

    namespace meta{
        using n1_t = h::meta<int>;
        using n2_t = h::meta<int, double>;
        using n3_t = h::meta<int, std::string, double>;
    }
}

struct no_arg_key{};
struct no_arg{
    int _v;

    static constexpr no_arg_key key() { return no_arg_key{}; }

    inline no_arg(): _v(42){}
};

struct wrap_int_key{};
struct wrap_int_index{};
struct wrap_int{
    typedef wrap_int_index index_type;

    int _v;

    static constexpr wrap_int_key key() { return wrap_int_key{}; }

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
};

namespace nod{
    using n1_t = h::node<no_arg>;
    using n2_t = h::node<no_arg, wrap_int>;
    using n3_t = h::node<no_arg, wrap_int, wrap_str>;
    using n4_t = h::node<no_arg, wrap_int, wrap_str, value_str>;

    namespace meta{
        using n1_t = h::meta_node<no_arg>;
        using n2_t = h::meta_node<no_arg, h::meta_node<wrap_int>>;
        using n3_t = h::meta_node<no_arg, h::meta_node<wrap_int, h::meta_node<wrap_str>>>;
        using n4_t = h::meta_node<no_arg, h::meta_node<wrap_int, h::meta_node<wrap_str, h::meta_node<value_str>>>>;
    }
}

TEST_CASE( "node depth", "[hazo]" ) {
    REQUIRE(pod::n1_t::depth   == 0);
    REQUIRE(pod::n2_t::depth   == 1);
    REQUIRE(pod::n3_t::depth   == 2);
    REQUIRE(nod::n1_t::depth == 0);
    REQUIRE(nod::n2_t::depth == 1);
    REQUIRE(nod::n3_t::depth == 2);
    REQUIRE(nod::n4_t::depth == 3);
}

TEST_CASE( "capsule traits (key, value, index)", "[hazo]" ) {
    REQUIRE(std::is_same_v<h::capsule<no_arg>::key_type,  no_arg_key>);
    REQUIRE(std::is_same_v<h::capsule<wrap_int>::key_type, wrap_int_key>);
    REQUIRE(std::is_same_v<h::capsule<wrap_str>::key_type, void>);
    REQUIRE(std::is_same_v<h::capsule<value_str>::key_type, value_str_key>);

    REQUIRE(std::is_same_v<h::capsule<no_arg>::value_type,  no_arg>);
    REQUIRE(std::is_same_v<h::capsule<wrap_int>::value_type, wrap_int>);
    REQUIRE(std::is_same_v<h::capsule<wrap_str>::value_type, wrap_str>);
    REQUIRE(std::is_same_v<h::capsule<value_str>::value_type, std::string>);

    REQUIRE(std::is_same_v<h::capsule<no_arg>::index_type,  no_arg>);
    REQUIRE(std::is_same_v<h::capsule<wrap_int>::index_type, wrap_int_index>);
    REQUIRE(std::is_same_v<h::capsule<wrap_str>::index_type, wrap_str>);
    REQUIRE(std::is_same_v<h::capsule<value_str>::index_type, value_str>);
}

TEST_CASE( "node type assists", "[hazo]" ) {
    REQUIRE(std::is_void_v<pod::meta::n1_t::tail_type>);
    REQUIRE(!std::is_void_v<pod::meta::n2_t::tail_type>);
    REQUIRE(std::is_void_v<pod::meta::n2_t::tail_type::tail_type>);

    REQUIRE(std::is_void_v<nod::meta::n1_t::tail_type>);
    REQUIRE(!std::is_void_v<nod::meta::n2_t::tail_type>);
    REQUIRE(std::is_void_v<nod::meta::n2_t::tail_type::tail_type>);
    REQUIRE(std::is_void_v<nod::meta::n3_t::tail_type::tail_type::tail_type>);
    REQUIRE(std::is_void_v<nod::meta::n4_t::tail_type::tail_type::tail_type::tail_type>);

    SECTION( "tail_type" ){
        REQUIRE(std::is_same_v<pod::meta::n2_t::tail_type, h::meta_node<double>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::tail_type, h::meta_node<std::string, h::meta_node<double>>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::tail_type::tail_type, h::meta_node<double>>);
        REQUIRE(std::is_void_v<pod::meta::n3_t::tail_type::tail_type::tail_type>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::tail_type, h::meta_node<wrap_int>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::tail_type, h::meta_node<wrap_int, h::meta_node<wrap_str>>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::tail_type::tail_type, h::meta_node<wrap_str>>);
        REQUIRE(std::is_void_v<nod::meta::n3_t::tail_type::tail_type::tail_type>);
    }

    SECTION( "tail_at" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::tail_at<0>, pod::meta::n1_t::tail_type>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::tail_at<0>, pod::meta::n2_t::tail_type>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::tail_at<0>, pod::meta::n3_t::tail_type>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::tail_at<0>, nod::meta::n1_t::tail_type>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::tail_at<0>, nod::meta::n2_t::tail_type>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::tail_at<0>, nod::meta::n3_t::tail_type>);

        REQUIRE(std::is_same_v<pod::meta::n1_t::types::tail_at<0>, void>);
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::tail_at<0>, h::meta_node<double>>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<pod::meta::n3_t::types::tail_at<0>, h::meta_node<std::string, h::meta_node<double>>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::tail_at<1>, h::meta_node<double>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::tail_at<3>, void>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::tail_at<0>, void>);
        REQUIRE(std::is_same_v<nod::meta::n1_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<nod::meta::n1_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::tail_at<0>, h::meta_node<wrap_int>>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::tail_at<1>, void>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::tail_at<0>, h::meta_node<wrap_int, h::meta_node<wrap_str>>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::tail_at<1>, h::meta_node<wrap_str>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::tail_at<2>, void>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::tail_at<3>, void>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::tail_at<99>, void>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::tail_at<2>, h::meta_node<value_str>>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::tail_at<3>, void>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::tail_at<99>, void>);
    }

    SECTION( "exists" ){
        REQUIRE(pod::meta::n1_t::types::exists<int>::value);
        REQUIRE(!pod::meta::n1_t::types::exists<double>::value);
        REQUIRE(pod::meta::n2_t::types::exists<int>::value);
        REQUIRE(pod::meta::n2_t::types::exists<double>::value);
        REQUIRE(!pod::meta::n2_t::types::exists<std::string>::value);
        REQUIRE(pod::meta::n3_t::types::exists<int>::value);
        REQUIRE(pod::meta::n3_t::types::exists<std::string>::value);
        REQUIRE(pod::meta::n3_t::types::exists<double>::value);

        REQUIRE(!nod::meta::n1_t::types::exists<std::string>::value);
        REQUIRE(nod::meta::n1_t::types::exists<no_arg>::value);
        REQUIRE(nod::meta::n2_t::types::exists<no_arg>::value);
        REQUIRE(nod::meta::n2_t::types::exists<wrap_int_index>::value);
        REQUIRE(!nod::meta::n2_t::types::exists<wrap_int>::value);
        REQUIRE(!nod::meta::n3_t::types::exists<wrap_int>::value);
        REQUIRE(nod::meta::n3_t::types::exists<wrap_int_index>::value);
        REQUIRE(!nod::meta::n4_t::types::exists<wrap_int>::value);
        REQUIRE(nod::meta::n4_t::types::exists<wrap_int_index>::value);
    }

    SECTION( "capsule_at" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::capsule_at<0>, h::capsule<int>>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::capsule_at<0>, h::capsule<int>>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::capsule_at<1>, h::capsule<double>>);

        REQUIRE(std::is_same_v<pod::meta::n3_t::types::capsule_at<0>, h::capsule<int>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::capsule_at<1>, h::capsule<std::string>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::capsule_at<2>, h::capsule<double>>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::capsule_at<0>, h::capsule<no_arg>>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::capsule_at<0>, h::capsule<no_arg>>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::capsule_at<1>, h::capsule<wrap_int>>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_at<0>, h::capsule<no_arg>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_at<1>, h::capsule<wrap_int>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_at<2>, h::capsule<wrap_str>>);
    }

    SECTION( "data_at" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::data_at<0>, int>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::data_at<0>, int>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::data_at<1>, double>);
        
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::data_at<0>, int>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::data_at<1>, std::string>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::data_at<2>, double>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::data_at<0>, no_arg>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::data_at<0>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::data_at<1>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_at<0>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_at<1>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_at<2>, wrap_str>);
    }

    SECTION( "value_at" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::value_at<0>, int>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::value_at<0>, int>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::value_at<1>, double>);
        
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::value_at<0>, int>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::value_at<1>, std::string>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::value_at<2>, double>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::value_at<0>, no_arg>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::value_at<0>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::value_at<1>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_at<0>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_at<1>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_at<2>, wrap_str>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_at<0>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_at<1>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_at<2>, wrap_str>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_at<3>, std::string>);
    }

    SECTION( "capsule_of" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::capsule_of<int>, h::capsule<int>>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::capsule_of<int>, h::capsule<int>>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::capsule_of<double>, h::capsule<double>>);
        
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::capsule_of<int>, h::capsule<int>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::capsule_of<std::string>, h::capsule<std::string>>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::capsule_of<double>, h::capsule<double>>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::capsule_of<no_arg>, h::capsule<no_arg>>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::capsule_of<no_arg>, h::capsule<no_arg>>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::capsule_of<wrap_int_index>, h::capsule<wrap_int>>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::capsule_of<wrap_int>, void>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_of<no_arg>, h::capsule<no_arg>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_of<wrap_int_index>, h::capsule<wrap_int>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_of<wrap_str>, h::capsule<wrap_str>>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::capsule_of<wrap_int>, void>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::capsule_of<no_arg>, h::capsule<no_arg>>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::capsule_of<wrap_int_index>, h::capsule<wrap_int>>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::capsule_of<wrap_str>, h::capsule<wrap_str>>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::capsule_of<value_str>, h::capsule<value_str>>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::capsule_of<wrap_int>, void>);
    }

    SECTION( "data_of" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::data_of<int>, int>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::data_of<int>, int>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::data_of<double>, double>);
        
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::data_of<int>, int>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::data_of<std::string>, std::string>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::data_of<double>, double>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::data_of<no_arg>, no_arg>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::data_of<no_arg>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::data_of<wrap_int_index>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_of<no_arg>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_of<wrap_int_index>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_of<wrap_str>, wrap_str>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_of<no_arg>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_of<wrap_int_index>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_of<wrap_str>, wrap_str>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_of<value_str>, value_str>);
    }

    SECTION( "value_of" ){
        REQUIRE(std::is_same_v<pod::meta::n1_t::types::value_of<int>, int>);

        REQUIRE(std::is_same_v<pod::meta::n2_t::types::value_of<int>, int>);
        REQUIRE(std::is_same_v<pod::meta::n2_t::types::value_of<double>, double>);
        
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::value_of<int>, int>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::value_of<std::string>, std::string>);
        REQUIRE(std::is_same_v<pod::meta::n3_t::types::value_of<double>, double>);

        REQUIRE(std::is_same_v<nod::meta::n1_t::types::value_of<no_arg>, no_arg>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::value_of<no_arg>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::value_of<wrap_int_index>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_of<no_arg>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_of<wrap_int_index>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_of<wrap_str>, wrap_str>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_of<no_arg>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_of<wrap_int_index>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_of<wrap_str>, wrap_str>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_of<value_str>, std::string>);
    }

    SECTION( "data_for" ){
        REQUIRE(std::is_same_v<nod::meta::n1_t::types::data_for<no_arg_key>, no_arg>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::data_for<no_arg_key>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::data_for<wrap_int_key>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_for<no_arg_key>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::data_for<wrap_int_key>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_for<no_arg_key>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_for<wrap_int_key>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::data_for<value_str_key>, value_str>);
    }

    SECTION( "value_for" ){
        REQUIRE(std::is_same_v<nod::meta::n1_t::types::value_for<no_arg_key>, no_arg>);

        REQUIRE(std::is_same_v<nod::meta::n2_t::types::value_for<no_arg_key>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n2_t::types::value_for<wrap_int_key>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_for<no_arg_key>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n3_t::types::value_for<wrap_int_key>, wrap_int>);

        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_for<no_arg_key>, no_arg>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_for<wrap_int_key>, wrap_int>);
        REQUIRE(std::is_same_v<nod::meta::n4_t::types::value_for<value_str_key>, std::string>);
    }

}

TEST_CASE( "node construction", "[hazo]" ) {

    SECTION( "not construction with more values" ){
        REQUIRE(!std::is_constructible_v<pod::n1_t, int, int>);
        REQUIRE(!std::is_constructible_v<pod::n2_t, int, double, double>);
        REQUIRE(!std::is_constructible_v<pod::n3_t, int, std::string, double, double>);
    }

    SECTION( "construction with all values" ){
        REQUIRE(std::is_constructible_v<pod::n1_t, int>);
        REQUIRE(std::is_constructible_v<pod::n2_t, int, double>);
        REQUIRE(std::is_constructible_v<pod::n3_t, int, std::string, double>);
        REQUIRE(std::is_constructible_v<pod::n2_t, double, int>);
        REQUIRE(!std::is_constructible_v<pod::n3_t, int, double, std::string>);
    }

    SECTION( "construction with few values" ){
        REQUIRE(std::is_constructible_v<pod::n1_t, int>);
        REQUIRE(std::is_constructible_v<pod::n2_t, int>);
        REQUIRE(std::is_constructible_v<pod::n2_t, double>);
        REQUIRE(std::is_constructible_v<pod::n3_t, int>);
        REQUIRE(std::is_constructible_v<pod::n3_t, double>);

        REQUIRE(!std::is_constructible_v<pod::n3_t, int, double>);
        REQUIRE(!std::is_constructible_v<pod::n3_t, int, int>);
        REQUIRE(!std::is_constructible_v<pod::n3_t, double, int>);
        REQUIRE(!std::is_constructible_v<pod::n3_t, double, double>);
        REQUIRE(std::is_constructible_v<pod::n3_t, int, std::string>);
        REQUIRE(std::is_constructible_v<pod::n3_t, double, std::string>);
    }

    SECTION( "construction with no values" ){
        REQUIRE(std::is_default_constructible_v<pod::n1_t>);
        REQUIRE(std::is_default_constructible_v<pod::n2_t>);
        REQUIRE(std::is_default_constructible_v<pod::n3_t>);
    }

    SECTION( "construction using nod types with all values" ){
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
        REQUIRE(std::is_constructible_v<h::node<int, wrap_int>, int>);
        REQUIRE(std::is_constructible_v<h::node<int, wrap_int>, int, wrap_int>);
    }
}