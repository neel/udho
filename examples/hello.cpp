#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <iostream>

std::string hello_world(udho::contexts::stateless ctx, std::string name, int num){
    return (boost::format("Hi! %1% this is number %2%") % name % num).str(); 
}
int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto router = udho::router()
         | (udho::get(&hello_world).plain() = "^/hello/(\\w+)/(\\d+)$");

    std::string doc_root("/path/to/static/document/root");  
    server.serve(router, 9198, doc_root);

    io.run();
    return 0;
}
