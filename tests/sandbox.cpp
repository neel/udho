#include <iostream>
#include <udho/hazo/seq/proxy.h>
#include <udho/hazo/seq.h>
#include <udho/hazo/map.h>
#include <udho/hazo/map/hana.h>

#include <boost/hana.hpp>
using namespace boost;

HAZO_ELEMENT(first_name, std::string)
HAZO_ELEMENT(last_name, std::string)
HAZO_ELEMENT(age, std::size_t)

int main(){
    using namespace udho::util::hazo;
    
    typedef seq_v<int, std::string, seq_v<double, int>, double, int> seq_v_type;
    
    seq_v_type seq1(1, "Neel", 3.14, 4, 12.45, 5);
    
    std::cout << seq1.value<int, 2>() << std::endl;
    
    return 0;
}

