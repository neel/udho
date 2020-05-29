#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::parsing)"
#include <boost/test/unit_test.hpp>
#include <set>
#include <map>
#include <stack>
#include <udho/scope.h>
#include <udho/access.h>
#include <udho/parser.h>
#include <boost/lexical_cast/try_lexical_convert.hpp>

struct book: udho::prepare<book>{
    std::string title;
    unsigned    year;
    std::vector<std::string>  authors;
       
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("title",   &book::title)
                     | var("authors", &book::authors)
                     | var("year",    &book::year);
    }
};

struct publisher: udho::prepare<publisher>{
    std::string name;
    std::string address;
    unsigned    year;
       
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("label",   &publisher::name)
                     | var("since",   &publisher::year)
                     | var("address", &publisher::address);
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

struct lazy: udho::prepare<lazy>{
    bool populated;
    mutable bool _value_called;
    
    lazy(): populated(false), _value_called(false){}
    double value() const{
        _value_called = true;
        return populated;
    }
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | var("populated",  &lazy::populated)
                     | fn ("value",      &lazy::value);
    }
};

BOOST_AUTO_TEST_SUITE(parsing)

BOOST_AUTO_TEST_CASE(expression){
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
    BOOST_CHECK(prepared["roll"] == "2");
    BOOST_CHECK(prepared.at<unsigned>("roll") == 2);
    BOOST_CHECK(prepared["first"] == "Neel");
    BOOST_CHECK(prepared["name"] == "Neel Bose");
    BOOST_CHECK(prepared["books:0.year"] == "2020");
    BOOST_CHECK(prepared["books:0.authors:0"] == "Sunanda Bose");
    BOOST_CHECK(prepared["marks:chemistry"] == "-10.42");
    BOOST_CHECK(prepared.count("books:0.authors") == 3);
    BOOST_CHECK(prepared.count("books") == 2);
    
    std::vector<std::string> keys = prepared.keys("books:0.authors");
    BOOST_CHECK(keys.size() == 3);
    BOOST_CHECK(keys[0] == "0");
    BOOST_CHECK(keys[1] == "1");
    BOOST_CHECK(keys[2] == "2");
    
    udho::lookup_table<udho::prepared<student>> table = udho::scope(prepared);
    
    table.add("papers", "books", 0);
    BOOST_CHECK(table.eval("papers:0.title") == "Book1 Title");
    table.add("thesis", "papers:0", 1);
    BOOST_CHECK(table.eval("thesis.year") == "2020");
    table.clear(1);
    BOOST_CHECK(table.eval("thesis.year") == "");
        
    auto expr = udho::view::expression(table);
    BOOST_CHECK(static_cast<unsigned>(expr.evaluate<double>("0.5 * ((books:0.year + books:1.year) / 2) + roll - 0.5")) == 1011);
    BOOST_CHECK(expr.evaluate<unsigned>("count(books)") == 2);
    BOOST_CHECK(expr.evaluate<unsigned>("not(count(books) < 3)") == false);
    BOOST_CHECK(expr.evaluate<unsigned>("not(count(books) < 1)") == true);
    BOOST_CHECK(expr.evaluate<unsigned>("not(count(books) < 1) && roll == 2") == true);
    BOOST_CHECK(expr.evaluate<unsigned>("not(count(books) < 1) && roll != 2") == false);
    
    lazy lobj;
    lobj.populated = false;
    auto plobj = udho::data(lobj);
    auto tlobj = udho::scope(plobj);
    auto elobj = udho::view::expression(tlobj);
    elobj.evaluate<double>("populated && not(value)");
    std::cout << "_value_called " << lobj._value_called << std::endl;
    
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
                <div class="publications" udho:if="count(books) > 1">
                    <udho:for value="book" key="id" in="books">
                        <div class="title" udho:target:title="book.title">
                            <udho:text name="book.title" />
                        </div>
                    </udho:for>
                </div>
                <udho:if test="not(count(books) > 1)">
                    <div class="freshers">
                        Not much publications
                    </div>
                </udho:if>
            </div>
    )TEMPLATE";
    
    udho::view::parser<udho::lookup_table<udho::prepared<student>>> processor = udho::view::processor(table);
    std::string output = processor.process(xml_template);
    std::cout << output << std::endl;
    
    publisher pub;
    pub.name    = "Some Publisher";
    pub.address = "Somewhere";
    pub.year    = 1987;
    
    auto package = (udho::data(neel) | udho::data(b1) | udho::data(pub));
    auto pkg_expr = udho::view::expression(package);
    std::cout << package["name"] << std::endl;
    std::cout << package["title"] << std::endl;
    std::cout << package["label"] << std::endl;
    std::cout << package["year"] << std::endl;
    std::cout << package["since"] << std::endl;
    std::cout << pkg_expr.evaluate<int>("since + year") << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
