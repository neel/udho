#include <iostream>
#include <udho/folding.h>
#include <boost/hana.hpp>
#include <sstream>
#include <cassert>

DEFINE_ELEMENT(first_name, std::string)
DEFINE_ELEMENT(last_name, std::string)
DEFINE_ELEMENT(age, std::size_t)

using namespace boost;
using namespace boost::hana::literals;
using namespace udho::util::folding;

int main(){
    seq_v<int, std::string, double, int> vec(42, "Hello", 3.14, 84);
    std::cout << vec << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.data<int>() << std::endl;
    std::cout << vec.data<std::string>() << std::endl;
    std::cout << vec.data<double>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.data<0>() << std::endl;
    std::cout << vec.data<1>() << std::endl;
    std::cout << vec.data<2>() << std::endl;
    std::cout << vec.data<3>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.set<0>(43) << std::endl;
    std::cout << vec.set<1>("World") << std::endl;
    std::cout << vec.set<2>(6.28) << std::endl;
    std::cout << vec.set<3>(42) << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << vec.data<0>() << std::endl;
    std::cout << vec.data<1>() << std::endl;
    std::cout << vec.data<2>() << std::endl;
    std::cout << vec.data<3>() << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << "Comparable: "<< hana::Comparable<seq_v<int, std::string, double, int>>::value << std::endl;
    std::cout << std::boolalpha << (make_seq_v(42, 34.5, "World") == make_seq_v(42, 34.5, "Hello")) << std::endl;
    assert((make_seq_v(1, 2, 3) == make_seq_v(1, 2, 3)));
    assert((make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3, 4)));
    assert((make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3.0)));
    assert((make_seq_v(1, 2, 3) != make_seq_v(1, 2, 3.14)));
    assert((make_seq_v(12, 2, 3) != make_seq_v(1, 2, 3)));
    std::cout << "-----" << std::endl;
    std::cout << "Foldable: "<< hana::Foldable<seq_v<int, std::string, double, int>>::value << std::endl;
    BOOST_HANA_CONSTEXPR_LAMBDA auto add = [](auto x, auto y, auto z) {
        return x + y + z;
    };
    auto tpl = make_seq_v(1, 2, 3);
    std::cout << tpl.unpack(add) << std::endl;
    BOOST_HANA_CONSTEXPR_CHECK(hana::unpack(make_seq_v(1, 2, 3), add) == 6);
    std::cout << "-----" << std::endl;
    std::cout << "Iterable: "<< hana::Iterable<seq_v<int, std::string, double, int>>::value << std::endl;
    std::cout << hana::at(vec, hana::size_t<0>{}) << std::endl;
    std::cout << hana::at(vec, hana::size_t<1>{}) << std::endl;
    std::cout << "-----" << std::endl;
    hana::for_each(make_seq_v(1, "Hello", 3.14), [](const auto& x){
        std::cout << x << ' ';
    });
    std::cout << std::endl;
    auto to_string = [](auto x) {
        std::ostringstream ss;
        ss << x;
        return ss.str();
    };
    BOOST_HANA_RUNTIME_CHECK(hana::transform(make_seq_v(1, '2', "345", std::string{"67"}), to_string) == make_seq_v("1", "2", "345", "67"));
    BOOST_HANA_RUNTIME_CHECK(hana::fill(make_seq_v(1, '2', 3.3, nullptr), 'x') == make_seq_v('x', 'x', 'x', 'x'), "");
    BOOST_HANA_CONSTEXPR_LAMBDA auto negate = [](auto x) {
        return -x;
    };
    BOOST_HANA_CONSTEXPR_CHECK(hana::adjust(make_seq_v(1, 4, 9, 2, 3, 4), 4, negate) == make_seq_v(1, -4, 9, 2, 3, -4));
    std::cout << "----" << std::endl;
    typedef map_v<first_name, last_name, age> map_type;
    map_type m1(first_name("Neel"), last_name("Basu"), age(32));

    std::cout << m1.data("first_name"_s) << std::endl;
    std::cout << m1.data("last_name"_s) << std::endl;
    std::cout << m1.data("age"_s) << std::endl;
    std::cout << m1.value("first_name"_s) << std::endl;
    std::cout << m1.value("last_name"_s) << std::endl;
    std::cout << m1.value("age"_s) << std::endl;
    std::cout << m1.data<0>() << std::endl;
    std::cout << m1.data<1>() << std::endl;
    std::cout << m1.data<2>() << std::endl;
    std::cout << m1.value<0>() << std::endl;
    std::cout << m1.value<1>() << std::endl;
    std::cout << m1.value<2>() << std::endl;
    std::cout << m1.data<first_name>() << std::endl;
    std::cout << m1.data<last_name>() << std::endl;
    std::cout << m1.data<age>()<< std::endl;
    std::cout << m1.value<first_name>() << std::endl;
    std::cout << m1.value<last_name>() << std::endl;
    std::cout << m1.value<age>() << std::endl;
    std::cout << m1["first_name"_s] << std::endl;
    std::cout << m1["last_name"_s] << std::endl;
    std::cout << m1["age"_s] << std::endl;
    std::cout << m1.element(first_name::val) << std::endl;
    std::cout << m1.element(last_name::val) << std::endl;
    std::cout << m1[first_name::val] << std::endl;
    std::cout << m1[last_name::val] << std::endl;
    std::cout << m1[age::val] << std::endl;
    std::cout << m1 << std::endl;
    map_type m2;
    m2 = m1;
    
    const map_type& mc = m1;
    std::cout << mc[first_name::val] << std::endl;
    std::cout << mc[last_name::val] << std::endl;
    std::cout << mc[age::val] << std::endl;
    
    std::cout << "hana::Struct<map_type>::value " << hana::Struct<map_type>::value << std::endl;
    std::cout << "hana::Searchable<map_type>::value " << hana::Searchable<map_type>::value << std::endl;
    auto accessors = hana::accessors<map_type::hana_tag>();
    hana::for_each(accessors, [&m1](const auto& k){
        std::cout << hana::first(k).c_str() << ": " << hana::second(k)(m1) << std::endl;
    });

//     std::cout << (hana::find(m1, "last_name"_s) == hana::just(first_name("Basu"))) << std::endl;
//     std::cout << (hana::find(m1, "last_name"_s) == hana::just(std::string("Basu"))) << std::endl;
    
    first_name d_first;
    last_name d_last;
    age d_age;
    m1 >> d_first >> d_last >> d_age;
    std::string v_first, v_last;
    std::size_t v_age;
    m1 >> v_first >> v_last >> v_age;
    
    map_v<first_name, last_name> m3(m1);
    std::cout << m3[first_name::val] << " " << m3[last_name::val] << std::endl;
    
    return 0;
}
