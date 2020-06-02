#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/contexts.h>
#include <iostream>
#include <udho/configuration.h>

std::string world(udho::contexts::stateless ctx){
    return "{'planet': 'Earth'}";
}
std::string planet(udho::contexts::stateless ctx, std::string name){
    return "Hello "+name;
}

int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto urls = udho::router() | "/world"          >> udho::get(&world).json() 
                               | "/planet/(\\w+)"  >> udho::get(&planet).plain();

    server.serve(urls, 9198);

    io.run();
    return 0;
}


