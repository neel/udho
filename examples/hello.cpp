#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <iostream>

#include <udho/access.h>

struct book: udho::prepare<book>{
    std::string  title;
    std::string  author;
    unsigned     year;
       
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("title",  &book::title)
                     | var("author", &book::author)
                     | var("year",   &book::year);
    }
};

struct student: udho::prepare<student>{
    unsigned int roll;
    std::string  first;
    std::string  last;
    book         _book;
    
    std::string name() const{
        return first + " " + last;
    }
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("roll",  &student::roll)
                     | var("first", &student::first)
                     | var("last",  &student::last)
                     | fn("name",   &student::name)
                     | var("book",  &student::_book);
    }
    
};

// std::string world(udho::contexts::stateless ctx){
//     return "{'planet': 'Earth'}";
// }
// std::string planet(udho::contexts::stateless ctx, std::string name){
//     return "Hello "+name;
// }
int main(){
    student neel;
    neel.roll  = 2;
    neel.first = "Neel";
    neel.last  = "Bose";
    neel._book.year = 2020;
    
    udho::prepared<student> prepared(neel);
    std::cout << prepared["roll"] << std::endl;
    std::cout << prepared.at<unsigned>("roll") << std::endl;
    std::cout << prepared["first"] << std::endl;
    std::cout << prepared["name"] << std::endl;
    std::cout << prepared["book.year"] << std::endl;
    
//     boost::asio::io_service io;
//     udho::servers::ostreamed::stateless server(io, std::cout);
// 
//     auto urls = udho::router() | "/world"          >> udho::get(&world).json() 
//                                | "/planet/(\\w+)"  >> udho::get(&planet).plain();
// 
//     server.serve(urls, 9198, "/path/to/static/files");
// 
//     io.run();
    return 0;
}
