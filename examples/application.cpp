#include <string>
#include <functional>
#include <udho/router.h>
#include <udho/server.h>
#include <udho/context.h>
#include <boost/asio.hpp>
#include <udho/application.h>

std::string hello(udho::contexts::stateless ctx){
    return "Hello World";
}

std::string data(udho::contexts::stateless ctx){
    return "{id: 2, name: 'udho'}";
}

int add(udho::contexts::stateless ctx, int a, int b){
    return a + b;
}

struct my_app: public udho::application<my_app>{
    typedef udho::application<my_app> base;
    
    struct state{
        int _value;
    };
    
    my_app();
    int add(udho::contexts::stateful<my_app::state> ctx, int a, int b);
    int mul(udho::contexts::stateful<my_app::state> ctx, int a, int b);
    
    template <typename RouterT>
    auto route(RouterT& router){
        return router | (get(&my_app::add).plain() = "^/add/(\\d+)/(\\d+)$")
                      | (get(&my_app::mul).plain() = "^/mul/(\\d+)/(\\d+)$");
    }
};

my_app::my_app(): base("my_app"){}
int my_app::add(udho::contexts::stateful<my_app::state> ctx, int a, int b){
    return a + b;
}
int my_app::mul(udho::contexts::stateful<my_app::state> ctx, int a, int b){
    return a * b;
}

int main(){
    std::string doc_root("/tmp/www"); // path to static content
    boost::asio::io_service io;

    udho::servers::ostreamed::stateful<my_app::state> server(io, std::cout);
    
    auto router = udho::router()
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::app<my_app>()       = "^/my_app");
                  
    udho::util::print_summary(router);
    
    server.serve(router, 9198, doc_root);
    
    io.run();
    
    return 0;
}

