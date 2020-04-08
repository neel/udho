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
    std::vector<std::string>  authors;
    unsigned     year;
       
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("title",   &book::title)
                     | var("authors", &book::authors)
                     | var("year",    &book::year);
    }
};

struct student: udho::prepare<student>{
    unsigned int roll;
    std::string  first;
    std::string  last;
    std::vector<book> books_issued;
    std::map<std::string, double> marks_obtained;
    
    std::string name() const{
        return first + " " + last;
    }
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("roll",  &student::roll)
                     | var("first", &student::first)
                     | var("last",  &student::last)
                     | var("books", &student::books_issued)
                     | var("marks", &student::marks_obtained)
                     | fn("name",   &student::name);
    }    
};

// std::string world(udho::contexts::stateless ctx){
//     return "{'planet': 'Earth'}";
// }
// std::string planet(udho::contexts::stateless ctx, std::string name){
//     return "Hello "+name;
// }
int main(){
    book b1;
    b1.title = "Book1 Title";
    b1.year  = 2020;
    b1.authors.push_back("Sunanda Bose");
    b1.authors.push_back("Neel Bose");
    b1.authors.push_back("Anindita Sinha Roy");
    b1.authors.push_back("Nandini Mukherjee");
    
    student neel;
    neel.roll  = 2;
    neel.first = "Neel";
    neel.last  = "Bose";
    
    neel.books_issued.push_back(b1);
    neel.marks_obtained["chemistry"] = -10.42;
    
    udho::prepared<student> prepared(neel);
    std::cout << prepared["roll"] << std::endl;
    std::cout << prepared.at<unsigned>("roll") << std::endl;
    std::cout << prepared["first"] << std::endl;
    std::cout << prepared["name"] << std::endl;
    std::cout << prepared["books:0.year"] << std::endl;
    std::cout << prepared["books:0.authors:0"] << std::endl;
    std::cout << prepared["marks:chemistry"] << std::endl;
    
    std::cout << prepared.count("books:0.authors") << std::endl;
    
    std::vector<std::string> keys = prepared.keys("books:0.authors");
    std::copy(keys.begin(), keys.end(), std::ostream_iterator<std::string>(std::cout, ","));
    std::cout << std::endl;
    
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
