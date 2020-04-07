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
        
    auto index = neel.index();
    
    udho::detail::association_value_extractor<unsigned> extractor;
    udho::detail::association_value_extractor<std::string> str_extractor;
    udho::detail::association_lexical_extractor<std::string> lex_str_extractor;
    udho::detail::association_lexical_extractor<unsigned> lex_uint_extractor;
    
    auto visitor = udho::detail::visit(index);
    
    visitor.find(extractor, "roll");
    std::cout << extractor.value() << std::endl;
    extractor.clear();
    
    visitor.find(str_extractor, "last");
    std::cout << str_extractor.value() << std::endl;
    str_extractor.clear();
    
    visitor.find(extractor, "book.year");
    std::cout << extractor.value() << std::endl;
    extractor.clear();
    
    visitor.find(lex_str_extractor, "book.year");
    std::cout << lex_str_extractor.value() << std::endl;
    lex_str_extractor.clear();
    
    visitor.find(lex_uint_extractor, "book.year");
    std::cout << lex_uint_extractor.value()+2 << std::endl;
    lex_uint_extractor.clear();
    
//     std::cout << neel["roll"].as<unsigned>() << std::endl;
//     std::cout << neel["first"].as<std::string>() << std::endl;
//     std::cout << neel["last"].as<std::string>() << std::endl;
//     std::cout << neel["name"].as<std::string>() << std::endl;
//     std::cout << neel["book"]["year"].as<unsigned>() << std::endl;
    
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
