#include <string>
#include <functional>
#include <udho/router.h>
#include <udho/server.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>

typedef udho::servers::ostreamed::stateful<> server_type;

struct simple{
    int add(int a, int b){
        return a+b;
    }
    int operator()(server_type::context ctx, int a, int b){
        return a+b;
    }
    std::string operator()(server_type::context ctx){
        return "Hello World";
    }
};

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;

    simple s;
    boost::function<int (server_type::context, int, int)> add(s);
    boost::function<std::string (server_type::context)> hello(s);

    server_type server(io, std::cout);
    
    auto router = udho::router<>()
        | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::get(hello).plain() = "^/hello$");
         
    udho::util::print_summary(router);
    
    server.serve(router, 9198, doc_root);
    
    io.run();
    
    return 0;
}
