#include <string>
#include <functional>
#include <udho/router.h>
#include <udho/server.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <udho/defs.h>
#include <udho/visitor.h>

struct simple{
    int add(int a, int b){
        return a+b;
    }
    int operator()(udho::defs::request_type req, int a, int b){
        return a+b;
    }
    std::string operator()(udho::contexts::stateless ctx){
        return "Hello World";
    }
};

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;

    simple s;
    boost::function<int (udho::defs::request_type, int, int)> add(s);
    boost::function<std::string (udho::contexts::stateless)> hello(s);

    udho::servers::ostreamed::stateless server(io, std::cout);
    
    auto router = udho::router()
        | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::get(hello).plain() = "^/hello$");
         
    router /= udho::visitors::print<udho::visitors::visitable::both, std::ostream>(std::cout);
    
    server.serve(router, 9198, doc_root);
    
    io.run();
    
    return 0;
}
