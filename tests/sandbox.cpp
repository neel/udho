#include <iostream>
#include <udho/hazo/seq/proxy.h>
#include <udho/hazo/seq.h>
#include <udho/hazo/map.h>
#include <udho/hazo/map/hana.h>

#include <boost/hana.hpp>
using namespace boost;

DEFINE_ELEMENT_SIMPLE(first_name, std::string)
DEFINE_ELEMENT_SIMPLE(last_name, std::string)
DEFINE_ELEMENT_SIMPLE(age, std::size_t)

int main(){
    using namespace udho::util::hazo;
    
    typedef seq_v<int, std::string, double, int> seq_v_type;
    seq_v_type vec(42, "Hello", 3.14, 84);
    seq_proxy_v<int, int> proxy(vec);
    
    proxy.data<1>() = 93;
    
    std::cout << vec.data<0>() << std::endl;
    std::cout << vec.data<3>() << std::endl;
    std::cout << proxy.data<0>() << std::endl;
    std::cout << proxy.data<1>() << std::endl;
    
    
    typedef map_v<first_name, last_name, age, first_name> map_v_type;
    map_v_type map("Neel", "Basu", 32, "Sunanda");
    
    std::cout << map[first_name::val] << std::endl;
    
    auto accessors_d = hana::accessors<map_v_type::hana_tag>();
    hana::for_each(accessors_d, [&map](const auto& k) mutable{
        std::cout << ": " << hana::second(k)(map) << ", ";
    });
    
    return 0;
}

