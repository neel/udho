#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::folding)"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <udho/folding.h>
#include <boost/lexical_cast.hpp>
#include <boost/hana.hpp>
#include <boost/format.hpp>

DEFINE_ELEMENT(first_name, std::string)
DEFINE_ELEMENT(last_name, std::string)
DEFINE_ELEMENT(age, std::size_t)

using namespace boost;
using namespace boost::hana::literals;
using namespace udho::util::folding;

BOOST_AUTO_TEST_SUITE(folding)

BOOST_AUTO_TEST_CASE(element){
    first_name f("Neel");
    last_name l("Basu");
    age a(32);
    
    BOOST_CHECK(f == f);
    BOOST_CHECK(l == l);
    BOOST_CHECK(a == a);
    BOOST_CHECK(f == "Neel");
    BOOST_CHECK(l == "Basu");
    BOOST_CHECK(a == 32);
}

BOOST_AUTO_TEST_CASE(seq_by_data_data){
    seq_d<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    
    BOOST_CHECK(vec.data<int>() == 42);
    BOOST_CHECK(vec.data<std::string>() == "Hello");
    BOOST_CHECK(vec.data<double>() == 3.14);

    BOOST_CHECK(vec.data<0>() == 42);
    BOOST_CHECK(vec.data<1>() == "Hello");
    BOOST_CHECK(vec.data<2>() == 3.14);
    BOOST_CHECK(vec.data<3>() == 84);

    BOOST_CHECK(vec.set<0>(43));
    BOOST_CHECK(vec.set<1>("World"));
    BOOST_CHECK(vec.set<2>(6.28));
    BOOST_CHECK(vec.set<3>(42));

    BOOST_CHECK(vec.data<0>() == 43);
    BOOST_CHECK(vec.data<1>() == "World");
    BOOST_CHECK(vec.data<2>() == 6.28);
    BOOST_CHECK(vec.data<3>() == 42);
}

BOOST_AUTO_TEST_CASE(seq_by_data_value){
    seq_d<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    
    BOOST_CHECK(vec.value<int>() == 42);
    BOOST_CHECK(vec.value<std::string>() == "Hello");
    BOOST_CHECK(vec.value<double>() == 3.14);
    BOOST_CHECK((vec.value<int, 1>() == 84));

    BOOST_CHECK(vec.value<0>() == 42);
    BOOST_CHECK(vec.value<1>() == "Hello");
    BOOST_CHECK(vec.value<2>() == 3.14);
    BOOST_CHECK(vec.value<3>() == 84);

    BOOST_CHECK(vec.set<0>(43));
    BOOST_CHECK(vec.set<1>("World"));
    BOOST_CHECK(vec.set<2>(6.28));
    BOOST_CHECK(vec.set<3>(42));

    BOOST_CHECK(vec.value<0>() == 43);
    BOOST_CHECK(vec.value<1>() == "World");
    BOOST_CHECK(vec.value<2>() == 6.28);
    BOOST_CHECK(vec.value<3>() == 42);
}

BOOST_AUTO_TEST_CASE(seq_by_value_value){
    seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    
    BOOST_CHECK(vec.value<int>() == 42);
    BOOST_CHECK(vec.value<std::string>() == "Hello");
    BOOST_CHECK(vec.value<double>() == 3.14);

    BOOST_CHECK(vec.value<0>() == 42);
    BOOST_CHECK(vec.value<1>() == "Hello");
    BOOST_CHECK(vec.value<2>() == 3.14);
    BOOST_CHECK(vec.value<3>() == 84);

    BOOST_CHECK(vec.set<0>(43));
    BOOST_CHECK(vec.set<1>("World"));
    BOOST_CHECK(vec.set<2>(6.28));
    BOOST_CHECK(vec.set<3>(42));

    BOOST_CHECK(vec.value<0>() == 43);
    BOOST_CHECK(vec.value<1>() == "World");
    BOOST_CHECK(vec.value<2>() == 6.28);
    BOOST_CHECK(vec.value<3>() == 42);
}

BOOST_AUTO_TEST_CASE(seq_by_value_data){
    seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    
    BOOST_CHECK(vec.data<int>() == 42);
    BOOST_CHECK(vec.data<std::string>() == "Hello");
    BOOST_CHECK(vec.data<double>() == 3.14);

    BOOST_CHECK(vec.data<0>() == 42);
    BOOST_CHECK(vec.data<1>() == "Hello");
    BOOST_CHECK(vec.data<2>() == 3.14);
    BOOST_CHECK(vec.data<3>() == 84);

    BOOST_CHECK(vec.set<0>(43));
    BOOST_CHECK(vec.set<1>("World"));
    BOOST_CHECK(vec.set<2>(6.28));
    BOOST_CHECK(vec.set<3>(42));

    BOOST_CHECK(vec.data<0>() == 43);
    BOOST_CHECK(vec.data<1>() == "World");
    BOOST_CHECK(vec.data<2>() == 6.28);
    BOOST_CHECK(vec.data<3>() == 42);
}

BOOST_AUTO_TEST_CASE(seq_io){
    seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    BOOST_CHECK(boost::lexical_cast<std::string>(vec) == "(42, Hello, 3.14, 84)");
}

BOOST_AUTO_TEST_CASE(seq_hana){
    typedef seq_v<int, std::string, double, int> seq_v_type;
    
    BOOST_CHECK((hana::Comparable<seq_v_type>::value));
    BOOST_CHECK((hana::Foldable<seq_v_type>::value));
    BOOST_CHECK((hana::Iterable<seq_v_type>::value));
    
    BOOST_CHECK(make_seq_v(42, 34.5, "World") != make_seq_v(42, 34.5, "Hello"));
    BOOST_CHECK(make_seq_v(1, 2, 3) == make_seq_v(1, 2, 3));
    BOOST_CHECK(make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3, 4));
    BOOST_CHECK(make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3.0));
    BOOST_CHECK(make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3.14));
    BOOST_CHECK(make_seq_v(12, 2, 3) != make_seq_v(1, 2, 3));
    
    BOOST_HANA_CONSTEXPR_LAMBDA auto add = [](auto x, auto y, auto z) {
        return x + y + z;
    };
    auto tpl = make_seq_v(1, 2, 3);
    BOOST_CHECK(tpl.unpack(add) == 6);
    BOOST_CHECK(hana::unpack(tpl, add) == 6);
    
    seq_v_type vec_v(42, "Hello", 3.14, 84);
    BOOST_CHECK(hana::at(vec_v, hana::size_t<0>{}) == 42);
    BOOST_CHECK(hana::at(vec_v, hana::size_t<1>{}) == "Hello");
    
    auto to_string = [](auto x) {
        std::ostringstream ss;
        ss << x;
        return ss.str();
    };
    BOOST_CHECK(hana::transform(make_seq_v(1, '2', "345", std::string{"67"}), to_string) == make_seq_v("1", "2", "345", "67"));
    BOOST_HANA_RUNTIME_CHECK(hana::fill(make_seq_v(1, '2', 3.3, nullptr), 'x') == make_seq_v('x', 'x', 'x', 'x'), "");
    BOOST_HANA_CONSTEXPR_LAMBDA auto negate = [](auto x) {
        return -x;
    };
    BOOST_CHECK(hana::adjust(make_seq_v(1, 4, 9, 2, 3, 4), 4, negate) == make_seq_v(1, -4, 9, 2, 3, -4));
}

BOOST_AUTO_TEST_CASE(map_value_by_value){
    typedef map_v<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1.value("first_name"_s) == "Neel");
    BOOST_CHECK(m1.value("last_name"_s) == "Basu");
    BOOST_CHECK(m1.value("age"_s) == 32);
    BOOST_CHECK(m1.value<0>() == "Neel");
    BOOST_CHECK(m1.value<1>() == "Basu");
    BOOST_CHECK(m1.value<2>() == 32);
    BOOST_CHECK(m1.value<first_name>() == "Neel");
    BOOST_CHECK(m1.value<last_name>() == "Basu");
    BOOST_CHECK(m1.value<age>() == 32);
}

BOOST_AUTO_TEST_CASE(map_value_by_data){
    typedef map_v<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1.data("first_name"_s) == first_name("Neel"));
    BOOST_CHECK(m1.data("last_name"_s) == last_name("Basu"));
    BOOST_CHECK(m1.data("age"_s) == age(32));
    BOOST_CHECK(m1.data<0>() == first_name("Neel"));
    BOOST_CHECK(m1.data<1>() == last_name("Basu"));
    BOOST_CHECK(m1.data<2>() == age(32));
    BOOST_CHECK(m1.data<first_name>() == first_name("Neel"));
    BOOST_CHECK(m1.data<last_name>() == last_name("Basu"));
    BOOST_CHECK(m1.data<age>() == age(32));
    
    BOOST_CHECK(m1.data("first_name"_s) == "Neel");
    BOOST_CHECK(m1.data("last_name"_s) == "Basu");
    BOOST_CHECK(m1.data("age"_s) == 32);
    BOOST_CHECK(m1.data<0>() == "Neel");
    BOOST_CHECK(m1.data<1>() == "Basu");
    BOOST_CHECK(m1.data<2>() == 32);
    BOOST_CHECK(m1.data<first_name>() == "Neel");
    BOOST_CHECK(m1.data<last_name>() == "Basu");
    BOOST_CHECK(m1.data<age>() == 32);
}

BOOST_AUTO_TEST_CASE(map_data_by_value){
    typedef map_d<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1.value("first_name"_s) == "Neel");
    BOOST_CHECK(m1.value("last_name"_s) == "Basu");
    BOOST_CHECK(m1.value("age"_s) == 32);
    BOOST_CHECK(m1.value<0>() == "Neel");
    BOOST_CHECK(m1.value<1>() == "Basu");
    BOOST_CHECK(m1.value<2>() == 32);
    BOOST_CHECK(m1.value<first_name>() == "Neel");
    BOOST_CHECK(m1.value<last_name>() == "Basu");
    BOOST_CHECK(m1.value<age>() == 32);
}

BOOST_AUTO_TEST_CASE(map_data_by_data){
    typedef map_d<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1.data("first_name"_s) == first_name("Neel"));
    BOOST_CHECK(m1.data("last_name"_s) == last_name("Basu"));
    BOOST_CHECK(m1.data("age"_s) == age(32));
    BOOST_CHECK(m1.data<0>() == first_name("Neel"));
    BOOST_CHECK(m1.data<1>() == last_name("Basu"));
    BOOST_CHECK(m1.data<2>() == age(32));
    BOOST_CHECK(m1.data<first_name>() == first_name("Neel"));
    BOOST_CHECK(m1.data<last_name>() == last_name("Basu"));
    BOOST_CHECK(m1.data<age>() == age(32));
    
    BOOST_CHECK(m1.data("first_name"_s) == "Neel");
    BOOST_CHECK(m1.data("last_name"_s) == "Basu");
    BOOST_CHECK(m1.data("age"_s) == 32);
    BOOST_CHECK(m1.data<0>() == "Neel");
    BOOST_CHECK(m1.data<1>() == "Basu");
    BOOST_CHECK(m1.data<2>() == 32);
    BOOST_CHECK(m1.data<first_name>() == "Neel");
    BOOST_CHECK(m1.data<last_name>() == "Basu");
    BOOST_CHECK(m1.data<age>() == 32);
}

BOOST_AUTO_TEST_CASE(map_value_by_element){
    typedef map_v<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1.element(first_name::val) == first_name("Neel"));
    BOOST_CHECK(m1.element(last_name::val) == last_name("Basu"));
    BOOST_CHECK(m1.element(age::val) == age(32));
    BOOST_CHECK(m1.element(first_name::val) == "Neel");
    BOOST_CHECK(m1.element(last_name::val) == "Basu");
    BOOST_CHECK(m1.element(age::val) == 32);
}

BOOST_AUTO_TEST_CASE(map_data_by_element){
    typedef map_d<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1.element(first_name::val) == first_name("Neel"));
    BOOST_CHECK(m1.element(last_name::val) == last_name("Basu"));
    BOOST_CHECK(m1.element(age::val) == age(32));
    BOOST_CHECK(m1.element(first_name::val) == "Neel");
    BOOST_CHECK(m1.element(last_name::val) == "Basu");
    BOOST_CHECK(m1.element(age::val) == 32);
}

BOOST_AUTO_TEST_CASE(map_value_by_subscript){
    typedef map_v<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1["first_name"_s] == "Neel");
    BOOST_CHECK(m1["last_name"_s] == "Basu");
    BOOST_CHECK(m1["age"_s] == 32);
}

BOOST_AUTO_TEST_CASE(map_data_by_subscript){
    typedef map_d<first_name, last_name, age> map_type;
    
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK(m1["first_name"_s] == first_name("Neel"));
    BOOST_CHECK(m1["last_name"_s] == last_name("Basu"));
    BOOST_CHECK(m1["age"_s] == age(32));
    BOOST_CHECK(m1["first_name"_s] == "Neel");
    BOOST_CHECK(m1["last_name"_s] == "Basu");
    BOOST_CHECK(m1["age"_s] == 32);
}

BOOST_AUTO_TEST_CASE(map_hana){
    typedef map_d<first_name, last_name, age> map_d_type;
    typedef map_v<first_name, last_name, age> map_v_type;
    
    map_d_type md1(first_name("Neel"), last_name("Basu"), age(32));
    map_v_type mv1(first_name("Neel"), last_name("Basu"), age(32));
    
    BOOST_CHECK((hana::Comparable<map_d_type>::value));
    BOOST_CHECK((hana::Foldable<map_d_type>::value));
    BOOST_CHECK((hana::Struct<map_d_type>::value));
    BOOST_CHECK((hana::Searchable<map_d_type>::value));
    BOOST_CHECK((hana::Comparable<map_v_type>::value));
    BOOST_CHECK((hana::Foldable<map_v_type>::value));
    BOOST_CHECK((hana::Struct<map_v_type>::value));
    BOOST_CHECK((hana::Searchable<map_v_type>::value));
    
    {
        std::ostringstream ss_v;
        auto accessors_v = hana::accessors<map_v_type::hana_tag>();
        hana::for_each(accessors_v, [&mv1, &ss_v](const auto& k) mutable{
            ss_v << hana::first(k).c_str() << ": " << hana::second(k)(mv1) << ", ";
        });
        BOOST_CHECK(ss_v.str() == "first_name: Neel, last_name: Basu, age: 32, ");
        BOOST_CHECK(hana::find(mv1, "last_name"_s) == hana::just("Basu"));
    }
    
    {
        std::ostringstream ss_d;
        auto accessors_d = hana::accessors<map_d_type::hana_tag>();
        hana::for_each(accessors_d, [&md1, &ss_d](const auto& k) mutable{
            ss_d << hana::first(k).c_str() << ": " << hana::second(k)(md1) << ", ";
        });
        BOOST_CHECK(ss_d.str() == (boost::format("first_name: %1%, last_name: %2%, age: %3%, ") % first_name("Neel") % last_name("Basu") % age(32)).str());
        BOOST_CHECK(hana::find(md1, "last_name"_s) == hana::just(last_name("Basu")));
    }
}

BOOST_AUTO_TEST_SUITE_END()
