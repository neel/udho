#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/contexts.h>
#include <iostream>
#include <udho/configuration.h>
#include <udho/forms.h>

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> datetime_type;

int main(){
    using namespace udho::forms::constraints;
    
    std::string query = "&k1=v1&k2=6&k3=5&k4=2011-02-18T23:12:34";

    udho::forms::form<udho::forms::drivers::urlencoded_> form;
    form.parse(query.begin(), query.end());
    std::cout << "k1: " << form.field<std::string>("k1") << std::endl;
    std::cout << "k2: " << form.field<std::size_t>("k2") << std::endl;
    std::cout << "k3: " << form.field<double>("k3") << std::endl;
    datetime_type dt = form.field<datetime_type>("k4", "%Y-%m-%dT%H:%M:%S");
    {
        auto tt = std::chrono::system_clock::to_time_t(dt);
        std::cout << "k4: " << std::ctime(&tt) << std::endl;
    }
      
    auto k1 = udho::forms::required<std::string>("k1")
                .absent("k1 Missing")
                .unparsable("k1 not parsable")
                .constrain<gte>(3, "k1.size < 3 not allowed")
                .constrain<lte>(4, "k1.size > 4 not allowed");
                    
    auto k2 = udho::forms::optional<unsigned>("k2", 42)                
                .absent("k2 Missing")
                .unparsable("k2 not parsable")
                .constrain<gte>(1, "k2 < 1 not allowed")
                .constrain<lte>(10, "k2 > 10 not allowed")
                .constrain<in>(2, 3, 4);
                
    k2.constrain<3>().error("k2 must be in [2, 3, 4]");
                
    auto k3 = udho::forms::required<double>("k3")
                .absent("k3 Missing")
                .unparsable("k3 not parsable")
                .constrain<gte>(3.0, "k3 < 3.0 not allowed")
                .constrain<lte>(4.0, "k3 > 4.0 not allowed");
                
    auto k4 = udho::forms::required<datetime_type>("k4", "%Y-%m-%dT%H:%M:%S")
                .absent("k4 Missing")
                .unparsable("k4 not parsable");
                
    std::cout << "k1.depth " << k1.depth << std::endl;
    std::cout << "k2.depth " << k2.depth << std::endl;
                           
    udho::forms::accumulated acc;
    form >> k1 >> k2 >> k3 >> k4;
    acc << k1 << k2 << k3 << k4;
                
    auto validated = udho::forms::validate(form);
    validated >> k1 >> k2 >> k3 >> k4;
    
    udho::prepared<udho::forms::accumulated> prepared(validated);
    
    std::cout << "accumulated.valid() " << validated.valid() << std::endl;
    std::cout << "valid " << prepared["valid"] << std::endl;
    std::cout << "fields:k1.valid " << prepared["fields:k1.valid"] << std::endl;
    std::cout << "fields:k1.value " << prepared["fields:k1.value"] << std::endl;
    std::cout << "fields:k1.error " << prepared["fields:k1.error"] << std::endl;
    std::cout << "fields:k2.valid " << prepared["fields:k2.valid"] << std::endl;
    std::cout << "fields:k2.value " << prepared["fields:k2.value"] << std::endl;
    std::cout << "fields:k2.error " << prepared["fields:k2.error"] << std::endl;
    std::cout << "fields:k3.valid " << prepared["fields:k3.valid"] << std::endl;
    std::cout << "fields:k3.value " << prepared["fields:k3.value"] << std::endl;
    std::cout << "fields:k3.error " << prepared["fields:k3.error"] << std::endl;
    std::cout << "fields:k4.valid " << prepared["fields:k4.valid"] << std::endl;
    std::cout << "fields:k4.value " << prepared["fields:k4.value"] << std::endl;
    std::cout << "fields:k4.error " << prepared["fields:k4.error"] << std::endl;
    
    std::cout << "----------k1-----------------" << std::endl;
    std::cout << std::boolalpha << !!k1 << std::endl;
    std::cout << *k1 << std::endl;
    std::cout << k1.message() << std::endl;
    std::cout << "----------k2-----------------" << std::endl;
    std::cout << std::boolalpha << !!k2 << std::endl;
    std::cout << *k2 << std::endl;
    std::cout << k2.message() << std::endl;
    std::cout << "----------k3-----------------" << std::endl;
    std::cout << std::boolalpha << !!k3 << std::endl;
    std::cout << *k3 << std::endl;
    std::cout << k3.message() << std::endl;
    std::cout << "----------k4-----------------" << std::endl;
    std::cout << std::boolalpha << !!k4 << std::endl;
    {
        auto tt = std::chrono::system_clock::to_time_t(*k4);
        std::cout << std::ctime(&tt) << std::endl;
    }
    std::cout << k4.message() << std::endl;
    std::cout << "-----------------------------" << std::endl;
    
    std::cout << "done" << std::endl;
    
    return 0;
}


