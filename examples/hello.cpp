#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/context.h>
#include <boost/tokenizer.hpp>
#include <iostream>

#include <set>
#include <map>
#include <stack>
#include <udho/scope.h>
#include <udho/access.h>
#include <udho/parser.h>
#include <boost/lexical_cast/try_lexical_convert.hpp>

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
    std::vector<book> publications;
    std::map<std::string, double> marks_obtained;
    
    std::string name() const{
        return first + " " + last;
    }
    bool qualified() const{
        return true;
    }
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("roll",     &student::roll)
                     | var("first",    &student::first)
                     | var("last",     &student::last)
                     | var("books",    &student::publications)
                     | var("marks",    &student::marks_obtained)
                     | fn("name",      &student::name)
                     | fn("qualified", &student::qualified);
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
    b1.authors.push_back("Nandini Mukherjee");
    
    book b2;
    b2.title = "Book2 Title";
    b2.year  = 2018;
    b2.authors.push_back("Sunanda Bose");
    b2.authors.push_back("Neel Bose");
    b2.authors.push_back("Nandini Mukherjee");
    
    student neel;
    neel.roll  = 2;
    neel.first = "Neel";
    neel.last  = "Bose";
    
    neel.publications.push_back(b1);
    neel.publications.push_back(b2);
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
    
    auto table = udho::scope(prepared);
//     table.add("papers", "books", 0);
//     std::cout << table.eval("papers:0.title") << std::endl;
//     table.add("thesis", "papers:0", 1);
//     std::cout << table.eval("thesis.year") << std::endl;
//     table.clear(0);
//     std::cout << table.eval("thesis.year") << std::endl;
    
    
    std::string xml_template = R"TEMPLATE(
            <div class="foo">
                <span class="name">
                    Hi! <udho:text name="name" />
                </span>
                <udho:block>
                    <article class="thesis">
                        <udho:var name="thesis" value="books:0" />
                        <label class="year">
                            <udho:text name="thesis.year" />
                        </label>
                    </article>
                </udho:block>
                <udho:if test="count(books) > 1">
                    <div class="publications">
                        <udho:for value="book" key="id" in="books">
                            <div class="title" udho:target:title="book.title">
                                <udho:text name="book.title" />
                            </div>
                        </udho:for>
                    </div>
                </udho:if>
                <udho:if test="not(count(books) > 1)">
                    <div class="freshers">
                        Not much publications
                    </div>
                </udho:if>
            </div>
    )TEMPLATE";
    
    auto parser = udho::view::parse_xml(table, xml_template);
    std::cout << parser.output() << std::endl;
    
    auto expr = udho::view::arithmatic(table);
    std::cout << expr.evaluate<int>("not(count(books) > 1)") << std::endl;
    
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


