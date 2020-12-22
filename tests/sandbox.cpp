#include <iostream>
#include <udho/folding/node/proxy.h>
#include <udho/folding/seq.h>

struct A{};
struct B{};
struct C{};
struct D{};
struct E{};

int main(){
    using namespace udho::util::folding;
    
    typedef seq_v<int, std::string, double, int> seq_v_type;
    seq_v_type vec(42, "Hello", 3.14, 84);
    seq_v_type::node_type& n = vec; 
    
    typedef sanitize<int, double, int> sanitizer;

//     std::cout << sanitizer::template count<A>::value << std::endl;
//     std::cout << sanitizer::template count<B>::value << std::endl;
//     std::cout << sanitizer::template count<C>::value << std::endl;
//     std::cout << sanitizer::template count<D>::value << std::endl;
//     std::cout << sanitizer::template count<E>::value << std::endl;
    
    sanitizer s(n);
    
    s.print(std::cout);
    
    return 0;
}

