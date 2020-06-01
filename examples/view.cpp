#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <iostream>
#include <cmath>

namespace data{
    
struct planet: udho::prepare<planet>{
    std::string name;
    long double radius;
    long double mass;
    
    double escape_velocity() const{
        const static long double G = 6.67L * std::pow(10L, -11L);
        return std::sqrt((2 * G * mass) / radius);
    }
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("planet",  &planet::name)
                     | var("radius",  &planet::radius)
                     | fn ("escape",  &planet::escape_velocity);
    }
};

struct person: udho::prepare<person>{
    std::string name;
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("name",  &person::name);
    }
};

struct ship: udho::prepare<ship>{
    std::string name;
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("ship",  &ship::name);
    }
};

}

std::string world(udho::contexts::stateless ctx){
    return "{'planet': 'Earth'}";
}
std::string planet(udho::contexts::stateless ctx, std::string name){
    data::planet planet;
    planet.name = name;
    
    data::person person;
    person.name = "Neel Bose";
    
    data::ship ship;
    ship.name = "Alpha Traveller";
    
    return ctx.aux().render("planet.html", planet, person, ship);
}

int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto urls = udho::router() | "/world"          >> udho::get(&world).json() 
                               | "/planet/(\\w+)"  >> udho::get(&planet).html();

    server.serve(urls, 9198, WWW_PATH);

    io.run();
    return 0;
}


