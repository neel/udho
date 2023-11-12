#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/hazo.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <udho/hazo/map/operations.h>
#include <udho/hazo/map/basic.h>
#include <udho/hazo/operations/flatten.h>

HAZO_ELEMENT_HANA(first_name, std::string);
HAZO_ELEMENT_HANA(last_name, std::string);
HAZO_ELEMENT_HANA(age, std::size_t);

using namespace boost;
using namespace boost::hana::literals;
using namespace udho::hazo;

TEST_CASE("hazo legacy", "[hazo]") {

    SECTION("element"){
        first_name f("Neel");
        last_name l("Basu");
        age a(32);
        
        CHECK(f == f);
        CHECK(l == l);
        CHECK(a == a);
        CHECK(f == "Neel");
        CHECK(l == "Basu");
        CHECK(a == 32);
    }

    SECTION("seq_by_data_data"){
        seq_d<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
        
        CHECK(vec.data<int>() == 42);
        CHECK(vec.data<std::string>() == "Hello");
        CHECK(vec.data<double>() == 3.14);

        CHECK(vec.data<0>() == 42);
        CHECK(vec.data<1>() == "Hello");
        CHECK(vec.data<2>() == 3.14);
        CHECK(vec.data<3>() == 84);

        CHECK(vec.set<0>(43));
        CHECK(vec.set<1>("World"));
        CHECK(vec.set<2>(6.28));
        CHECK(vec.set<3>(42));

        CHECK(vec.data<0>() == 43);
        CHECK(vec.data<1>() == "World");
        CHECK(vec.data<2>() == 6.28);
        CHECK(vec.data<3>() == 42);
    }

    SECTION("seq_by_data_value"){
        seq_d<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
        
        CHECK(vec.value<int>() == 42);
        CHECK(vec.value<std::string>() == "Hello");
        CHECK(vec.value<double>() == 3.14);
        CHECK((vec.value<int, 1>() == 84));

        CHECK(vec.value<0>() == 42);
        CHECK(vec.value<1>() == "Hello");
        CHECK(vec.value<2>() == 3.14);
        CHECK(vec.value<3>() == 84);

        CHECK(vec.set<0>(43));
        CHECK(vec.set<1>("World"));
        CHECK(vec.set<2>(6.28));
        CHECK(vec.set<3>(42));

        CHECK(vec.value<0>() == 43);
        CHECK(vec.value<1>() == "World");
        CHECK(vec.value<2>() == 6.28);
        CHECK(vec.value<3>() == 42);
    }

    SECTION("seq_by_value_value"){
        seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
        
        CHECK(vec.value<int>() == 42);
        CHECK(vec.value<std::string>() == "Hello");
        CHECK(vec.value<double>() == 3.14);

        CHECK(vec.value<0>() == 42);
        CHECK(vec.value<1>() == "Hello");
        CHECK(vec.value<2>() == 3.14);
        CHECK(vec.value<3>() == 84);

        CHECK(vec.set<0>(43));
        CHECK(vec.set<1>("World"));
        CHECK(vec.set<2>(6.28));
        CHECK(vec.set<3>(42));

        CHECK(vec.value<0>() == 43);
        CHECK(vec.value<1>() == "World");
        CHECK(vec.value<2>() == 6.28);
        CHECK(vec.value<3>() == 42);
    }

    SECTION("seq_by_value_data"){
        seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
        
        CHECK(vec.data<int>() == 42);
        CHECK(vec.data<std::string>() == "Hello");
        CHECK(vec.data<double>() == 3.14);

        CHECK(vec.data<0>() == 42);
        CHECK(vec.data<1>() == "Hello");
        CHECK(vec.data<2>() == 3.14);
        CHECK(vec.data<3>() == 84);

        CHECK(vec.set<0>(43));
        CHECK(vec.set<1>("World"));
        CHECK(vec.set<2>(6.28));
        CHECK(vec.set<3>(42));

        CHECK(vec.data<0>() == 43);
        CHECK(vec.data<1>() == "World");
        CHECK(vec.data<2>() == 6.28);
        CHECK(vec.data<3>() == 42);
    }

    SECTION("seq_io"){
        seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
        CHECK(boost::lexical_cast<std::string>(vec) == "(42, Hello, 3.14, 84)");
    }

    SECTION("seq_hana"){
        typedef seq_v<int, std::string, double, int> seq_v_type;
        
        CHECK((hana::Comparable<seq_v_type>::value));
        CHECK((hana::Foldable<seq_v_type>::value));
        CHECK((hana::Iterable<seq_v_type>::value));
        
        CHECK(hana::size(make_seq_v(42, 34.5, "World")) == hana::size_c<3>());
        
        CHECK(make_seq_v(42, 34.5, "World") != make_seq_v(42, 34.5, "Hello"));
        CHECK(make_seq_v(1, 2, 3) == make_seq_v(1, 2, 3));
        CHECK(make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3, 4));
        CHECK(make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3.0));
        CHECK(make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3.14));
        CHECK(make_seq_v(12, 2, 3) != make_seq_v(1, 2, 3));
        
        BOOST_HANA_CONSTEXPR_LAMBDA auto add = [](auto x, auto y, auto z) {
            return x + y + z;
        };
        auto tpl = make_seq_v(1, 2, 3);
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
        CHECK(hana::transform(make_seq_v(1, '2', "345", std::string{"67"}), to_string) == make_seq_v("1", "2", "345", "67"));
        BOOST_HANA_RUNTIME_CHECK(hana::fill(make_seq_v(1, '2', 3.3, nullptr), 'x') == make_seq_v('x', 'x', 'x', 'x'), "");
        BOOST_HANA_CONSTEXPR_LAMBDA auto negate = [](auto x) {
            return -x;
        };
        CHECK(hana::adjust(make_seq_v(1, 4, 9, 2, 3, 4), 4, negate) == make_seq_v(1, -4, 9, 2, 3, -4));
    }

    SECTION("map_value_by_value"){
        typedef map_v<first_name, last_name, age> map_type3;
        
        map_type3 m3(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK(m3.value("first_name"_s) == "Neel");
        CHECK(m3.value("last_name"_s) == "Basu");
        CHECK(m3.value("age"_s) == 32);
        CHECK(m3.value<0>() == "Neel");
        CHECK(m3.value<1>() == "Basu");
        CHECK(m3.value<2>() == 32);
        CHECK(m3.value<first_name>() == "Neel");
        CHECK(m3.value<last_name>() == "Basu");
        CHECK(m3.value<age>() == 32);
        
        typedef map_v<first_name, last_name> map_type2;
        map_type2 m2(first_name("Neel"), last_name("Basu"));
        CHECK(m2.value("first_name"_s) == "Neel");
        CHECK(m2.value("last_name"_s) == "Basu");
        CHECK(m2.value<0>() == "Neel");
        CHECK(m2.value<1>() == "Basu");
        CHECK(m2.value<first_name>() == "Neel");
        CHECK(m2.value<last_name>() == "Basu");
        
        typedef map_v<first_name> map_type1;
        map_type1 m1(first_name("Neel"));
        CHECK(m1.value("first_name"_s) == "Neel");
        CHECK(m1.value<0>() == "Neel");
        CHECK(m1.value<first_name>() == "Neel");
    }

    SECTION("map_value_by_data"){
        typedef map_v<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK(m1.data("first_name"_s) == first_name("Neel"));
        CHECK(m1.data("last_name"_s) == last_name("Basu"));
        CHECK(m1.data("age"_s) == age(32));
        CHECK(m1.data<0>() == first_name("Neel"));
        CHECK(m1.data<1>() == last_name("Basu"));
        CHECK(m1.data<2>() == age(32));
        CHECK(m1.data<first_name>() == first_name("Neel"));
        CHECK(m1.data<last_name>() == last_name("Basu"));
        CHECK(m1.data<age>() == age(32));
        
        CHECK(m1.data("first_name"_s) == "Neel");
        CHECK(m1.data("last_name"_s) == "Basu");
        CHECK(m1.data("age"_s) == 32);
        CHECK(m1.data<0>() == "Neel");
        CHECK(m1.data<1>() == "Basu");
        CHECK(m1.data<2>() == 32);
        CHECK(m1.data<first_name>() == "Neel");
        CHECK(m1.data<last_name>() == "Basu");
        CHECK(m1.data<age>() == 32);
    }

    SECTION("map_data_by_value"){
        typedef map_d<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK(m1.value("first_name"_s) == "Neel");
        CHECK(m1.value("last_name"_s) == "Basu");
        CHECK(m1.value("age"_s) == 32);
        CHECK(m1.value<0>() == "Neel");
        CHECK(m1.value<1>() == "Basu");
        CHECK(m1.value<2>() == 32);
        CHECK(m1.value<first_name>() == "Neel");
        CHECK(m1.value<last_name>() == "Basu");
        CHECK(m1.value<age>() == 32);
    }

    SECTION("map_data_by_data"){
        typedef map_d<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK(m1.data("first_name"_s) == first_name("Neel"));
        CHECK(m1.data("last_name"_s) == last_name("Basu"));
        CHECK(m1.data("age"_s) == age(32));
        CHECK(m1.data<0>() == first_name("Neel"));
        CHECK(m1.data<1>() == last_name("Basu"));
        CHECK(m1.data<2>() == age(32));
        CHECK(m1.data<first_name>() == first_name("Neel"));
        CHECK(m1.data<last_name>() == last_name("Basu"));
        CHECK(m1.data<age>() == age(32));
        
        CHECK(m1.data("first_name"_s) == "Neel");
        CHECK(m1.data("last_name"_s) == "Basu");
        CHECK(m1.data("age"_s) == 32);
        CHECK(m1.data<0>() == "Neel");
        CHECK(m1.data<1>() == "Basu");
        CHECK(m1.data<2>() == 32);
        CHECK(m1.data<first_name>() == "Neel");
        CHECK(m1.data<last_name>() == "Basu");
        CHECK(m1.data<age>() == 32);
    }

    SECTION("map_value_by_element"){
        typedef map_v<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK(m1.element(first_name::val) == first_name("Neel"));
        CHECK(m1.element(last_name::val) == last_name("Basu"));
        CHECK(m1.element(age::val) == age(32));
        CHECK(m1.element(first_name::val) == "Neel");
        CHECK(m1.element(last_name::val) == "Basu");
        CHECK(m1.element(age::val) == 32);
    }

    SECTION("map_data_by_element"){
        typedef map_d<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK(m1.element(first_name::val) == first_name("Neel"));
        CHECK(m1.element(last_name::val) == last_name("Basu"));
        CHECK(m1.element(age::val) == age(32));
        CHECK(m1.element(first_name::val) == "Neel");
        CHECK(m1.element(last_name::val) == "Basu");
        CHECK(m1.element(age::val) == 32);
    }

    SECTION("map_exclude"){
        typedef map_d<first_name, last_name, age> map_type3;

        CHECK((std::is_same<typename map_type3::exclude<first_name>, map_d<last_name, age>>::value));
        CHECK((std::is_same<typename map_type3::exclude<last_name>, map_d<first_name, age>>::value));
        CHECK((std::is_same<typename map_type3::exclude<age>, map_d<first_name, last_name>>::value));
        CHECK((std::is_same<typename map_type3::exclude<int>, map_d<first_name, last_name, age>>::value));
        CHECK((std::is_same<typename map_type3::exclude<first_name, age>, map_d<last_name>>::value));
        
        CHECK((std::is_same<typename map_d<map_d<first_name, last_name>, age>::exclude<first_name>, map_d<last_name, age>>::value));
        
        typedef map_d<first_name, last_name> map_type2;

        CHECK((std::is_same<typename map_type2::exclude<first_name>, map_d<last_name>>::value));
        CHECK((std::is_same<typename map_type2::exclude<last_name>, map_d<first_name>>::value));
        CHECK((std::is_same<typename map_type2::exclude<age>, map_d<first_name, last_name>>::value));
        CHECK((std::is_same<typename map_type2::exclude<int>, map_d<first_name, last_name>>::value));
        
        typedef map_d<first_name> map_type1;

        CHECK((std::is_same<typename map_type1::exclude<first_name>, map_d<void>>::value));
        CHECK((std::is_same<typename map_type1::exclude<last_name>, map_d<first_name>>::value));
        CHECK((std::is_same<typename map_type1::exclude<age>, map_d<first_name>>::value));
        CHECK((std::is_same<typename map_type1::exclude<int>, map_d<first_name>>::value));
    }

    SECTION("seq_exclude"){
        typedef seq_d<first_name, last_name, age> seq_type3;

        CHECK((std::is_same<typename seq_type3::exclude<first_name>, seq_d<last_name, age>>::value));
        CHECK((std::is_same<typename seq_type3::exclude<last_name>, seq_d<first_name, age>>::value));
        CHECK((std::is_same<typename seq_type3::exclude<age>, seq_d<first_name, last_name>>::value));
        CHECK((std::is_same<typename seq_type3::exclude<int>, seq_d<first_name, last_name, age>>::value));
        CHECK((std::is_same<typename seq_type3::exclude<first_name, age>, seq_d<last_name>>::value));
        
        typedef seq_d<first_name, last_name> seq_type2;

        CHECK((std::is_same<typename seq_type2::exclude<first_name>, seq_d<last_name>>::value));
        CHECK((std::is_same<typename seq_type2::exclude<last_name>, seq_d<first_name>>::value));
        CHECK((std::is_same<typename seq_type2::exclude<age>, seq_d<first_name, last_name>>::value));
        CHECK((std::is_same<typename seq_type2::exclude<int>, seq_d<first_name, last_name>>::value));
        
        typedef seq_d<first_name> seq_type1;

        CHECK((std::is_same<typename seq_type1::exclude<first_name>, seq_d<void>>::value));
        CHECK((std::is_same<typename seq_type1::exclude<last_name>, seq_d<first_name>>::value));
        CHECK((std::is_same<typename seq_type1::exclude<age>, seq_d<first_name>>::value));
        CHECK((std::is_same<typename seq_type1::exclude<int>, seq_d<first_name>>::value));
    }

    SECTION("map_value_by_subscript"){
        typedef map_v<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));
        std::cout << m1 << std::endl;
        
        CHECK(m1["first_name"_s] == "Neel");
        CHECK(m1["last_name"_s] == "Basu");
        CHECK(m1["age"_s] == 32);
    }

    SECTION("map_data_by_subscript"){
        typedef map_d<first_name, last_name, age> map_type;
        
        map_type m1(first_name("Neel"), last_name("Basu"), age(32));

        std::cout << m1 << std::endl;
        
        CHECK(m1["first_name"_s] == first_name("Neel"));
        CHECK(m1["last_name"_s] == last_name("Basu"));
        CHECK(m1["age"_s] == age(32));
        CHECK(m1["first_name"_s] == "Neel");
        CHECK(m1["last_name"_s] == "Basu");
        CHECK(m1["age"_s] == 32);
    }

    SECTION("map_hana"){
        typedef map_d<first_name, last_name, age> map_d_type;
        typedef map_v<first_name, last_name, age> map_v_type;
        
        map_d_type md1(first_name("Neel"), last_name("Basu"), age(32));
        map_v_type mv1(first_name("Neel"), last_name("Basu"), age(32));
        
        CHECK((hana::Comparable<map_d_type>::value));
        CHECK((hana::Foldable<map_d_type>::value));
        CHECK((hana::Struct<map_d_type>::value));
        CHECK((hana::Searchable<map_d_type>::value));
        CHECK((hana::Comparable<map_v_type>::value));
        CHECK((hana::Foldable<map_v_type>::value));
        CHECK((hana::Struct<map_v_type>::value));
        CHECK((hana::Searchable<map_v_type>::value));
        
        CHECK(hana::size(mv1) == hana::size_c<3>());
        
        {
            std::ostringstream ss_v;
            auto accessors_v = hana::accessors<map_v_type::hana_tag>();
            hana::for_each(accessors_v, [&mv1, &ss_v](const auto& k) mutable{
                ss_v << hana::first(k).c_str() << ": " << hana::second(k)(mv1) << ", ";
            });
            CHECK(ss_v.str() == "first_name: Neel, last_name: Basu, age: 32, ");
            CHECK((hana::find(mv1, "last_name"_s) == hana::just("Basu")));
        }
        
        {
            std::ostringstream ss_d;
            auto accessors_d = hana::accessors<map_d_type::hana_tag>();
            hana::for_each(accessors_d, [&md1, &ss_d](const auto& k) mutable{
                ss_d << hana::first(k).c_str() << ": " << hana::second(k)(md1) << ", ";
            });
            CHECK(ss_d.str() == (boost::format("first_name: %1%, last_name: %2%, age: %3%, ") % first_name("Neel") % last_name("Basu") % age(32)).str());
            CHECK((hana::find(md1, "last_name"_s) == hana::just(last_name("Basu"))));
        }
    }

    SECTION("seq_by_data_monoid"){
        typedef seq_v<unsigned, double> seq_type1;
        typedef seq_v<seq_type1, int> seq_type2;
        typedef seq_v<seq_type2, seq_type2> seq_type3;
        typedef seq_v<seq_type3> seq_type4;
        
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

    SECTION("seq_by_data_proxy"){
        seq_d<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
        seq_d<int, double, int>::proxy proxy(vec);
        
        CHECK(proxy.data<0>() == vec.data<0>());
        CHECK(proxy.data<1>() == vec.data<2>());
        CHECK(proxy.data<2>() == vec.data<3>());
        
        CHECK((proxy.data<int, 0>() == vec.data<0>()));
        CHECK((proxy.data<double, 0>() == vec.data<2>()));
        CHECK((proxy.data<int, 1>() == vec.data<3>()));
    }

    SECTION("map_by_data_proxy"){
        map_d<first_name, last_name, age> map(first_name("Neel"), last_name("Basu"), age(32));
        map_d<first_name, age>::proxy proxy(map);
        
        CHECK(proxy.data<0>() == map.data<0>());
        CHECK(proxy.data<1>() == map.data<2>());
        
        CHECK((proxy.data<first_name>() == map.data<0>()));
        CHECK((proxy.data<age>() == map.data<2>()));
    }

    struct T1{
        typedef char index_type;
    };
    struct T2{
        typedef double index_type;
    };
    struct T3{
        typedef std::string index_type;
    };
    struct T4{
        typedef unsigned index_type;
    };
    struct T5{};

    SECTION("seq_by_index"){
        typedef seq_d<T1, T2, T3, T4, T5> seq_type;
        
        CHECK((std::is_same<seq_type::types::data_of<char>, T1>::value));
        CHECK((std::is_same<seq_type::types::data_of<double>, T2>::value));
        CHECK((std::is_same<seq_type::types::data_of<std::string>, T3>::value));
        CHECK((std::is_same<seq_type::types::data_of<unsigned>, T4>::value));
        CHECK((std::is_same<seq_type::types::data_of<T5>, T5>::value));
    }

}
