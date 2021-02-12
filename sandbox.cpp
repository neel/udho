#include <udho/hazo.h>
#include <udho/hazo/map/operations.h>
#include <udho/hazo/map/basic.h>
#include <udho/hazo/operations/flatten.h>
#include <ctti/nameof.hpp>
#include <iostream>

struct T1{};
struct T2{};
struct T3{};
struct T4{};
struct T5{};
struct T6{};
struct T7{};
struct T8{};
struct T9{};
struct T10{};
struct T11{};
struct T12{};
struct T13{};

using namespace udho::util::hazo;

int main(){
    typedef basic_map_d<basic_map_d<basic_map_d<basic_map_d<T1, T2>, T3>, T4>, T5, T6, T7, basic_map_d<T8, T9, T10>, T11, basic_map_d<T12, T13>> map_type;
    
    std::cout << ctti::nameof<map_type>() << std::endl;
    std::cout << std::endl;
    std::cout << ctti::nameof<typename operations::flatten<basic_map_d, map_type>::type>() << std::endl;
    
    return 0;
}
