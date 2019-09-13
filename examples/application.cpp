#include <string>
#include <functional>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <udho/application.h>

// #include <iostream>
// #include "udho/util.h"
// #include <boost/regex.hpp>
// #include <boost/regex/icu.hpp>

std::string hello(udho::request_type req){
    return "Hello World";
}

std::string data(udho::request_type req){
    return "{id: 2, name: 'udho'}";
}

int add(udho::request_type req, int a, int b){
    return a + b;
}

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;

    auto router = udho::router()
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::app<udho::my_app>() = "^/my_app");
    router.listen(io, 9198, doc_root);
          
    io.run();
    
//     std::string subject = "/apps/উধো/বুধো";
//     std::string pattern = "^/apps";
//     std::string subject_decoded = udho::util::urldecode(subject);
//     boost::smatch match;
//     bool result = boost::u32regex_search(subject_decoded, match, boost::make_u32regex(pattern));
//     std::cout << result << std::endl;
//     std::cout << match.length() << std::endl;
//     std::cout << subject.substr(match.length()) << std::endl;
    
    return 0;
}

