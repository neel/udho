#include <string>
#include <functional>
#include <udho/router.h>
#include <udho/server.h>
#include <boost/asio.hpp>
#include <udho/application.h>

typedef udho::servers::ostreamed::stateful<> server_type;

std::string hello(server_type::context ctx){
    return "Hello World";
}

std::string data(server_type::context ctx){
    return "{id: 2, name: 'udho'}";
}

int add(server_type::context ctx, int a, int b){
    return a + b;
}

struct my_app: public udho::application<my_app>{
    typedef udho::application<my_app> base;
    
    my_app();
    int add(server_type::context ctx, int a, int b);
    int mul(server_type::context ctx, int a, int b);
    
    template <typename RouterT>
    auto route(RouterT& router){
        return router | (get(&my_app::add).plain() = "^/add/(\\d+)/(\\d+)$")
                      | (get(&my_app::mul).plain() = "^/mul/(\\d+)/(\\d+)$");
    }
};

my_app::my_app(): base("my_app"){}
int my_app::add(server_type::context ctx, int a, int b){
    return a + b;
}
int my_app::mul(server_type::context ctx, int a, int b){
    return a * b;
}

int main(){
    std::string doc_root("/tmp/www"); // path to static content
    boost::asio::io_service io;

    server_type server(io, std::cout);
    
    auto router = udho::router<>()
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::app<my_app>()       = "^/my_app");
                  
    udho::util::print_summary(router);
    
    server.serve(router, 9198, doc_root);
    
    io.run();
    
    return 0;
}

