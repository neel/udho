#include <udho/hazo.h>
#include <udho/hazo/map/operations.h>
#include <udho/hazo/map/basic.h>
#include <udho/hazo/operations/flatten.h>

HAZO_ELEMENT_HANA(first_name, std::string)
HAZO_ELEMENT_HANA(last_name, std::string)
HAZO_ELEMENT_HANA(age, std::size_t)

using namespace udho::util::hazo;

int main(){
    typedef basic_map_d<> empty_map;
    typedef basic_map_d<first_name, last_name, age> person_map;
    
    // operations::first_of<basic_map_d, basic_map_d<basic_map_d<age, last_name>, first_name>>::type::xyz();
    
//     udho::util::hazo::operations::append<udho::util::hazo::basic_map_d<>, first_name>::type::xyz();
    
    typedef typename udho::util::hazo::operations::basic_flatten<
        udho::util::hazo::basic_map_d, 
        udho::util::hazo::basic_map_d<>, 
        first_name, last_name, age
    >::rest initial_type;
    
    initial_type::xyz();
    
    return 0;
}
