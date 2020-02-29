#include <string>
#include <functional>
#include <udho/router.h>
#include <udho/server.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>

struct simple{
    int add(int a, int b){
        return a+b;
    }
    int operator()(udho::servers::logged::context ctx, int a, int b){
        return a+b;
    }
    std::string operator()(udho::servers::logged::context ctx){
        return "Hello World";
    }
};

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;

    simple s;
    boost::function<int (udho::servers::logged::context, int, int)> add(s);
    boost::function<std::string (udho::servers::logged::context)> hello(s);

    udho::servers::logged server(io, std::cout);
    
    auto router = udho::router<>()
        | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::get(hello).plain() = "^/hello$");
         
    udho::util::print_summary(router);
    
    server.serve(router, 9198, doc_root);
    
    io.run();
    
    return 0;
}
