#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>

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
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
    router.listen(io, 9198, doc_root);
    
    udho::util::print_summary(router);
        
    io.run();
    
    return 0;
}
