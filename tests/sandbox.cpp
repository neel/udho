#include <iostream>
#include <udho/folding/seq/proxy.h>
#include <udho/folding/seq.h>

#include <boost/hana.hpp>

struct A{};
struct B{};
struct C{};
struct D{};
struct E{};

int main(){
    using namespace udho::util::folding;
    
    typedef seq_v<int, std::string, double, int> seq_v_type;
    seq_v_type vec(42, "Hello", 3.14, 84);
    seq_proxy_v<int, int> proxy(vec);
    
//     std::cout << sanitizer::template count<A>::value << std::endl;
//     std::cout << sanitizer::template count<B>::value << std::endl;
//     std::cout << sanitizer::template count<C>::value << std::endl;
//     std::cout << sanitizer::template count<D>::value << std::endl;
//     std::cout << sanitizer::template count<E>::value << std::endl;
    
    proxy.data<1>() = 93;
    
    std::cout << vec.data<0>() << std::endl;
    std::cout << vec.data<3>() << std::endl;
    std::cout << proxy.data<0>() << std::endl;
    std::cout << proxy.data<1>() << std::endl;
    return 0;
}

