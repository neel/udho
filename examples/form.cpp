#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/contexts.h>
#include <iostream>
#include <udho/configuration.h>
#include <udho/forms.h>

int main(){
    using namespace udho::forms::constraints;
    
    std::string query = "&k1=v11&k2=2&k3=2.28";
    
    udho::forms::form<udho::forms::drivers::urlencoded> form;
    form.parse(query.begin(), query.end());
    std::cout << "k1: " << form.field<std::string>("k1") << std::endl;
    std::cout << "k2: " << form.field<std::size_t>("k2") << std::endl;
    std::cout << "k3: " << form.field<double>("k3") << std::endl;
    
    auto k1 = udho::forms::required<std::string>("k1")
                .absent("k1 Missing")
                .unparsable("k1 not parsable")
                .constrain<gte>(3, "k1.size < 3 not allowed")
                .constrain<lte>(4, "k1.size > 4 not allowed");
                    
    auto k2 = udho::forms::required<unsigned>("k2");
                
    auto k3 = udho::forms::required<double>("k3")
                .absent("k3 Missing")
                .unparsable("k3 not parsable")
                .constrain<gte>(3.0, "k3 < 3.0 not allowed")
                .constrain<lte>(4.0, "k3 > 4.0 not allowed");
                
    form >> k1 >> k2 >> k3;
    
    std::cout << "----------k1-----------------" << std::endl;
    std::cout << std::boolalpha << !k1 << std::endl;
    std::cout << *k1 << std::endl;
    std::cout << k1.message() << std::endl;
    std::cout << "----------k2-----------------" << std::endl;
    std::cout << std::boolalpha << !k2 << std::endl;
    std::cout << *k2 << std::endl;
    std::cout << k2.message() << std::endl;
    std::cout << "----------k3-----------------" << std::endl;
    std::cout << std::boolalpha << !k3 << std::endl;
    std::cout << *k3 << std::endl;
    std::cout << k3.message() << std::endl;
    std::cout << "-----------------------------" << std::endl;
    
    std::cout << "done" << std::endl;
    
    return 0;
}


