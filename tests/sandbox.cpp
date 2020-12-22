#include <iostream>
#include <udho/folding/node/proxy.h>

struct A{};
struct B{};
struct C{};
struct D{};
struct E{};

int main(){
    using namespace udho::util::folding;
    
    
    typedef sanitize<A, B, C, D, E> sanitizer;

    std::cout << sanitizer::template count<A>::value << std::endl;
    std::cout << sanitizer::template count<B>::value << std::endl;
    std::cout << sanitizer::template count<C>::value << std::endl;
    std::cout << sanitizer::template count<A>::value << std::endl;
    std::cout << sanitizer::template count<E>::value << std::endl;
    
    sanitizer s;
    
    s.print(std::cout);
    
    return 0;
}

