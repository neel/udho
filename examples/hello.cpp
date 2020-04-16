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

enum class sy_token_type{
    id,
    op,
    fnc,
    real,
    parenthesis_open,
    parenthesis_close,
    comma
};

enum class sy_token_associativity{
    none,
    left,
    right
};

struct sy_op_property{
    std::size_t _precedence;
    sy_token_associativity _associativity;
    
    sy_op_property(): _precedence(0){}
    sy_op_property(std::size_t precedence, sy_token_associativity associativity = sy_token_associativity::left){
        _precedence = precedence;
        _associativity = associativity;
    }
};

struct sy_rules{
    std::map<std::string, sy_op_property> _operators;
    std::set<std::string> _functions;
    
    sy_rules(){
        _operators.insert(std::make_pair("&&", sy_op_property(1)));
        _operators.insert(std::make_pair("||", sy_op_property(1)));
        _operators.insert(std::make_pair("<",  sy_op_property(2)));
        _operators.insert(std::make_pair(">",  sy_op_property(2)));
        _operators.insert(std::make_pair("<=", sy_op_property(2)));
        _operators.insert(std::make_pair(">=", sy_op_property(2)));
        _operators.insert(std::make_pair("==", sy_op_property(2)));
        _operators.insert(std::make_pair("+",  sy_op_property(3)));
        _operators.insert(std::make_pair("-",  sy_op_property(3)));
        _operators.insert(std::make_pair("/",  sy_op_property(4)));
        _operators.insert(std::make_pair("*",  sy_op_property(4)));
        _operators.insert(std::make_pair("*",  sy_op_property(4)));
        
        _functions.insert("count");
        _functions.insert("max");
        _functions.insert("min");
    }
    bool is_op(const std::string& buff) const{
        auto oit = _operators.find(buff);
        if(oit != _operators.end()){
            return true;
        }
        return false;
    };
    bool is_fnc(const std::string& buff) const{
        auto oit = _functions.find(buff);
        if(oit != _functions.end()){
            return true;
        }
        return false;
    };
    bool is_comma(const std::string& buff) const{
        return buff == ",";
    }
    bool is_parenthesis_open(const std::string& buff) const{
        return buff == "(";
    }
    bool is_parenthesis_close(const std::string& buff) const{
        return buff == ")";
    }
    bool is_parenthesis(const std::string& buff) const{
        return is_parenthesis_open(buff) || is_parenthesis_close(buff);
    }
    std::size_t precedence(const std::string& buff) const{
        if(is_op(buff)){
            auto oit = _operators.find(buff);
            if(oit != _operators.end()){
                return oit->second._precedence;
            }
        }
        return 0;
    }
    sy_token_associativity associativity(const std::string& buff) const{
        if(is_op(buff)){
            auto oit = _operators.find(buff);
            if(oit != _operators.end()){
                return oit->second._associativity;
            }
        }
        return sy_token_associativity::none;
    }
};

struct sy_token{    
    std::string _buff;
    sy_token_type  _type;
    const sy_rules& _rules;
    
    sy_token(const sy_rules& rules, const std::string& buff): _rules(rules), _buff(buff){        
        _type = detect();
    }
    sy_token_type detect(){
        if(_rules.is_parenthesis_open(_buff)){
            return sy_token_type::parenthesis_open;
        }
        if(_rules.is_parenthesis_close(_buff)){
            return sy_token_type::parenthesis_close;
        }
        if(_rules.is_op(_buff)){
            return sy_token_type::op;
        }
        if(_rules.is_fnc(_buff)){
            return sy_token_type::fnc;
        }
        double res;
        if(boost::conversion::try_lexical_convert<double>(_buff, res)){
            return sy_token_type::real;
        }
        if(_rules.is_comma(_buff)){
            return sy_token_type::comma;
        }
        return sy_token_type::id;
    }
    sy_token_type type() const{
        return _type;
    }
    bool is_real() const{
        return type() == sy_token_type::real;
    }
    bool is_id() const{
        return type() == sy_token_type::id;
    }
    bool is_fnc() const{
        return type() == sy_token_type::fnc;
    }
    bool is_op() const{
        return type() == sy_token_type::op;
    }
    bool is_parenthesis_open() const{
        return _type == sy_token_type::parenthesis_open;
    }
    bool is_parenthesis_close() const{
        return _type == sy_token_type::parenthesis_close;
    }
    bool is_parenthesis() const{
        return _rules.is_parenthesis(_buff);
    }
    bool is_comma() const{
        return _rules.is_comma(_buff);
    }
    std::size_t precedence() const{
        if(is_op()){
            return _rules.precedence(_buff);
        }
        return 0;
    }
    sy_token_associativity associativity() const{
        return _rules.associativity(_buff);
    }
    double real() const{
        double res;
        if(boost::conversion::try_lexical_convert<double>(_buff, res)){
            return res;
        }
        return 0.0f;
    }
    std::string value() const{
        return _buff;
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
    
    neel.books_issued.push_back(b1);
    neel.books_issued.push_back(b2);
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
    
    
    std::string xml_template = R"(
            <div class="foo">
                <span class="name">
                    <udho:text name="name" />
                </span>
                <udho:block>
                    <article class="thesis">
                        <udho:var name="thesis" value="books:0" />
                        <label class="year">
                            <udho:text name="thesis.year" />
                        </label>
                    </article>
                </udho:block>
                <div class="publications">
                    <udho:for value="book" key="id" in="books">
                        <div class="title">
                            <udho:text name="book.title" />
                        </div>
                    </udho:for>
                </div>
            </div>
    )";
    
    udho::view::parse_xml(table, xml_template);
    
    std::string str = "count(books) >= 0";
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep(" \t\n", ",+-*/%()&|<>=", boost::drop_empty_tokens);
    tokenizer tokens(str, sep);
    std::vector<std::string> raw_tokens, processed_tokens;
    for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter){
        std::string ctoken = *tok_iter;
        tokenizer::iterator next = tok_iter;
        ++next;
        if(next != tokens.end()){
            std::string ntoken = *next;
            if(ctoken == "<" && ntoken == "="){
                processed_tokens.push_back("<=");
                ++tok_iter;
            }else if(ctoken == ">" && ntoken == "="){
                processed_tokens.push_back(">=");
                ++tok_iter;
            }else if(ctoken == "=" && ntoken == "="){
                processed_tokens.push_back("==");
                ++tok_iter;
            }else if(ctoken == "&" && ntoken == "&"){
                processed_tokens.push_back("&&");
                ++tok_iter;
            }else if(ctoken == "|" && ntoken == "|"){
                processed_tokens.push_back("||");
                ++tok_iter;
            }else{
                processed_tokens.push_back(ctoken);
            }
        }else{
            processed_tokens.push_back(ctoken);
        }
    }
    
    std::vector<std::string> output;
    std::stack<std::string> stack;

    sy_rules rules;
    std::size_t fargc = 0;
    for (std::vector<std::string>::iterator tok_iter = processed_tokens.begin(); tok_iter != processed_tokens.end(); ++tok_iter){
        sy_token token(rules, *tok_iter);
        if(token.is_real()){
            output.push_back(token.value());
        }else if(token.is_id()){
            output.push_back(token.value());
        }else if(token.is_fnc()){
            stack.push(token.value());
        }else if(token.is_comma()){
            ++fargc;
        }else if(token.is_parenthesis_open()){
            stack.push(token.value());
        }else if(token.is_parenthesis_close()){
            while(!stack.empty() && !rules.is_parenthesis_open(stack.top())){
                output.push_back(stack.top());
                stack.pop();
            }
            if(!stack.empty()){
                stack.pop();
            }
            if(!stack.empty() && rules.is_fnc(stack.top())){
                output.push_back(stack.top()+"@"+boost::lexical_cast<std::string>(fargc+1));
                fargc = 0;
                stack.pop();
            }
        }else if(token.is_op()){
            while(!stack.empty() 
                && (
                    (rules.is_op(stack.top()) && rules.precedence(stack.top()) >= token.precedence())
                )
            ){
                output.push_back(stack.top());
                stack.pop();
            }
            stack.push(token.value());
        }
    }
    
    std::vector<std::string> rest;
    while(!stack.empty()){
        rest.push_back(stack.top());
        stack.pop();
    }
    
    for(auto it = rest.begin(); it != rest.end(); ++it){
        output.push_back(*it);
    }
    
    for(std::string x: output){
        std::cout << x << " ";
    }
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


