/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_PARSER_H
#define UDHO_PARSER_H

#include <vector>
#include <iterator>
#include <iostream>
#include <udho/util.h>
#include <udho/scope.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <set>
#include <map>
#include <stack>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>

namespace udho{
    
namespace view{
    
namespace parser{

struct token{
    typedef std::map<std::string, std::size_t> operator_map_type;
    typedef std::map<std::string, int>         function_map_type;
    enum class type{
        op, function, call, comma, parenthesis_open, parenthesis_close, real, id
    };
    enum class category{
        none, arithmetic, logical, junction
    };
    inline static token create(const std::string& buff){
        static operator_map_type opmap = operator_map();
        static function_map_type fnmap = function_map();
        
        double res;
        bool is_real = boost::conversion::try_lexical_convert<double>(buff, res);
        if(is_real){
            return token(type::real, buff);
        }else if(buff == ","){
            return token(type::comma, buff);
        }else if(buff == "("){
            return token(type::parenthesis_open, buff);
        }else if(buff == ")"){
            return token(type::parenthesis_close, buff);
        }else if(opmap.count(buff)){
            return token(type::op, buff);
        }else if(fnmap.count(buff)){
            return token(type::function, buff);
        }else if(buff.find('@') != std::string::npos){
            return token(type::call, buff);
        }else{
            return token(type::id, buff);
        }
    }
    inline bool is(const type& t) const{
        return _type == t;
    }
    inline bool is(const category& c) const{
        if(_type != type::op){
            return c == category::none;
        }
        static const std::set<std::string> arithmatic_operators = {"+", "-", "/", "*"};
        static const std::set<std::string> logical_operators    = {"<", ">", "<=", ">=", "==", "=", "!="};
        static const std::set<std::string> junction_operators   = {"&", "|", "&&", "||"};
        if(arithmatic_operators.count(value())){
            return c == category::arithmetic;
        }else if(logical_operators.count(value())){
            return c == category::logical;
        }else if(junction_operators.count(value())){
            return c == category::junction;
        }
        return false;
    }
    template <typename T>
    static T arithmatic(const token& op_token, const T& l, const T& r){
        T res;
        static const std::map<std::string, boost::function<T (const T&, const T&)>> action_map_arithmetic = {
            {"+", std::plus<T>()},
            {"-", std::minus<T>()},
            {"/", std::divides<T>()},
            {"*", std::multiplies<T>()}
        };
        if(op_token.is(category::arithmetic)){
            if(action_map_arithmetic.count(op_token.value())){
                const auto& action = action_map_arithmetic.at(op_token.value());
                res = action(l, r);
            }
        }
        return res;
    }
    template <typename T>
    static bool logical(const token& op_token, const T& l, const T& r){
        T res;
        static const std::map<std::string, boost::function<T (const T&, const T&)>> action_map_logical = {
            {"<",  std::less<T>()},
            {">",  std::greater<T>()},
            {"<=", std::less_equal<T>()},
            {">=", std::greater_equal<T>()},
            {"==", std::equal_to<T>()},
            {"=",  std::equal_to<T>()},
            {"!=", std::not_equal_to<T>()},
        };
        if(op_token.is(category::logical)){
            if(action_map_logical.count(op_token.value())){
                const auto& action = action_map_logical.at(op_token.value());
                res = action(l, r);
            }
        }
        return res;
    }
    inline int precedence() const{
        static operator_map_type opmap = operator_map();
        
        if(_type == type::op){
            return opmap[_value];
        }
        return 0;
    }
    inline std::string value() const{
        return _value;
    }
  private:
    type         _type;
    std::string _value;
    
    inline token(type t, const std::string& v): _type(t), _value(v){}
    inline static operator_map_type operator_map(){
        return {
            {"&&", 1}, {"||", 1}, {"&", 1}, {"|", 1},
            {"<=", 2}, {">=", 2}, {"<", 2}, {">", 2}, {"==", 2}, {"=", 2}, {"!=", 2},
            {"+",  3}, {"-",  3},
            {"*",  4}, {"/",  4}
        };
    }
    inline static function_map_type function_map(){
        return {
            {"count", 1},
            {"not",   1},
            {"max",  -1},
            {"min",  -1},
            {"avg",  -1},
        };
    }
};

template <typename InputIt, typename OutputIt>
OutputIt tokenize(InputIt begin, InputIt end, OutputIt output){
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep(" \t\n", ",+-*/%()&|<>=", boost::drop_empty_tokens);
    tokenizer tokens(begin, end, sep);
    for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter){
        std::string ctoken = *tok_iter;
        tokenizer::iterator next = tok_iter;
        ++next;
        if(next != tokens.end()){
            std::string ntoken = *next;
            if(ctoken == "<" && ntoken == "="){
                *output++ = token::create("<=");
                tok_iter++;
            }else if(ctoken == ">" && ntoken == "="){
                *output++ = token::create(">=");
                tok_iter++;
            }else if(ctoken == "=" && ntoken == "="){
                *output++ = token::create("==");
                tok_iter++;
            }else if(ctoken == "&" && ntoken == "&"){
                *output++ = token::create("&&");
                tok_iter++;
            }else if(ctoken == "|" && ntoken == "|"){
                *output++ = token::create("||");
                tok_iter++;
            }else{
                *output++ = token::create(ctoken);
            }
        }else{
            *output++ = token::create(ctoken);
        }
    }
    return output;
}


template <typename ScopeT>
struct expression{
    typedef ScopeT scope_type;
    
    scope_type& _table;
    
    explicit expression(scope_type& table): _table(table){}
    
    std::vector<token> rpn(const std::string& infix){
        typedef std::vector<token> container_type;
        typedef std::stack<token>  stack_type;

        container_type output;
        stack_type     stack;
        container_type tokens;
        tokenize(infix.cbegin(), infix.cend(), std::back_inserter(tokens));
        std::size_t fargc = 0;
        for (container_type::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter){
            token t = *tok_iter;
            if(t.is(token::type::real)){
                output.push_back(t);
            }else if(t.is(token::type::id)){
                output.push_back(t);
            }else if(t.is(token::type::function)){
                stack.push(t);
            }else if(t.is(token::type::comma)){
                ++fargc;
            }else if(t.is(token::type::parenthesis_open)){
                stack.push(t);
            }else if(t.is(token::type::parenthesis_close)){
                while(!stack.empty() && !stack.top().is(token::type::parenthesis_open)){
                    output.push_back(stack.top());
                    stack.pop();
                }
                if(!stack.empty()){
                    stack.pop();
                }
                if(!stack.empty() && stack.top().is(token::type::function)){
                    std::string func_name = stack.top().value();
                    std::string call_name = func_name+"@"+boost::lexical_cast<std::string>(fargc+1);
                    output.push_back(token::create(call_name));
                    fargc = 0;
                    stack.pop();
                }
            }else if(t.is(token::type::op)){
                while(!stack.empty() && (stack.top().is(token::type::op) && stack.top().precedence() >= t.precedence())){
                    output.push_back(stack.top());
                    stack.pop();
                }
                stack.push(t);
            }
        }
        std::vector<token> rest;
        while(!stack.empty()){
            rest.push_back(stack.top());
            stack.pop();
        }
        for(auto it = rest.begin(); it != rest.end(); ++it){
            output.push_back(*it);
        }
        return output;
    }
    template <typename T>
    T fetch(const token& t){
        T value;
        if(t.is(token::type::real)){
            boost::conversion::try_lexical_convert<T>(t.value(), value);
        }else if(t.is(token::type::id)){
            value = _table.template parse<T>(t.value());
        }
        return value;
    }
    std::size_t count(const token& t) const{
        if(t.is(token::type::id)){
            return _table.count(t.value());
        }
        return 0;
    }
    template <typename T>
    T evaluate(const std::string& infix){
        std::vector<token> tokens = rpn(infix);
        std::stack<token> stack;
        
        for(const token& t: tokens){
            if(!t.is(token::type::op) && !t.is(token::type::call)){
                stack.push(t);
            }else{
                if(t.is(token::type::op)){
                    token rhs  = stack.top(); stack.pop();
                    token lhs  = stack.top(); stack.pop();
                    T r, l, result;
                    if(t.is(token::category::arithmetic)){
                        l = fetch<T>(lhs);
                        r = fetch<T>(rhs);
                        result  = token::arithmatic(t, l, r);
                    }else if(t.is(token::category::logical)){
                        l = fetch<T>(lhs);
                        r = fetch<T>(rhs);
                        result  = token::logical(t, l, r);
                    }else if(t.is(token::category::junction)){
                        l = fetch<T>(lhs);
                        if(t.value().front() == '&'){ // & or &&
                            if(!l){
                                result = static_cast<T>(false);
                            }else{
                                r = fetch<T>(rhs);
                                result = l && r;
                            }
                        }else if(t.value().front() == '|'){ // | or ||
                            if(l){
                                result = static_cast<T>(true);
                            }else{
                                r = fetch<T>(rhs);
                                result = l || r;
                            }
                        }
                    }
                    stack.push(token::create(boost::lexical_cast<std::string>(result)));
                }else if(t.is(token::type::call)){
                    std::vector<std::string> parts;
                    boost::split(parts, t.value(), boost::is_any_of("@"));
                    std::size_t args = boost::lexical_cast<std::size_t>(parts[1]);
                    std::string name = parts[0];
                    T result;
                    token key_token = stack.top(); stack.pop();
                    if(name == "count"){
                        result = static_cast<T>(count(key_token));
                    }else if(name == "not"){
                        result = !fetch<T>(key_token);
                    }
                    stack.push(token::create(boost::lexical_cast<std::string>(result)));
                }
            }
        }
        token last = stack.top();
        stack.pop();
        return fetch<T>(last);
    }
};
    
}

template <typename ScopeT>
parser::expression<ScopeT> arithmatic(ScopeT& scope){
    return parser::expression<ScopeT>(scope);
}
    
      
template <typename ScopeT>
struct xml_parser{
    typedef ScopeT& lookup_table_type;
    typedef parser::expression<ScopeT> expression_evaluator_type;

    lookup_table_type&  _table;
    pugi::xml_document  _source;
    pugi::xml_document  _transformed;
    expression_evaluator_type _expression_evaluator;
    
    xml_parser(lookup_table_type& table): _table(table), _expression_evaluator(table){}
    void open(const std::string& contents){
        _source.load_string(contents.c_str());
        _transformed.append_child("udho:transformed");
    }    
    void parse(pugi::xml_node node, pugi::xml_node head){
        pugi::xml_node target = head;
        std::string name = node.name();
        std::vector<std::string> parts;        
        boost::split(parts, name, boost::is_any_of(":"));
        if(parts.front() == "udho" && parts.size() > 1){
            if(parts[1] == "block"){
                _table.down();
                travarse(node, target);
                _table.up();
            }else if(parts[1] == "for"){
                pugi::xml_attribute in = node.find_attribute([](const pugi::xml_attribute& attr){
                    return attr.name() == std::string("in");
                });
                pugi::xml_attribute value = node.find_attribute([](const pugi::xml_attribute& attr){
                    return attr.name() == std::string("value");
                });
                std::string collection = in.as_string();
                std::vector<std::string> keys = _table.keys(collection);
                for(const std::string& key: keys){
                    _table.down();
                    std::string ref = collection + ":" +key;
                    _table.add(value.as_string(), ref);
                    travarse(node, target);
                    _table.up();
                }
            }else if(parts[1] == "if"){
                pugi::xml_attribute cond = node.find_attribute([](const pugi::xml_attribute& attr){
                    return attr.name() == std::string("test");
                });
                std::string condition = cond.as_string();
                bool truth = _expression_evaluator.template evaluate<int>(condition);
                // std::cout << "udho:if (" << condition << ") : " << truth << std::endl;
                if(truth){
                    travarse(node, target);
                }
            }else if(parts[1] == "var"){
                pugi::xml_attribute name = node.find_attribute([](const pugi::xml_attribute& attr){
                    return attr.name() == std::string("name");
                });
                pugi::xml_attribute value = node.find_attribute([](const pugi::xml_attribute& attr){
                    return attr.name() == std::string("value");
                });
                _table.add(name.as_string(), value.as_string());
            }else if(parts[1] == "text"){
                pugi::xml_attribute name = node.find_attribute([](const pugi::xml_attribute& attr){
                    return attr.name() == std::string("name");
                });
                std::string var = name.as_string();
                std::string value = _table.eval(var);
                target.append_child(pugi::node_pcdata).text().set(value.c_str());
            }else if(parts[1] == "template"){
                step_in(node, target);
            }
        }else{
            std::vector<pugi::xml_attribute> removed;
            for(pugi::xml_attribute attr: node.attributes()){
                std::string name = attr.name();
                std::size_t index = name.find(':');
                if(index != std::string::npos){
                    std::size_t next = name.find(':', index+1);
                    if(next != std::string::npos){
                        std::string sub = name.substr(index+1, next-index-1);
                        std::string rest = name.substr(next+1);
                        
                        // std::cout << "sub  " << sub  << std::endl;
                        // std::cout << "rest " << rest << std::endl;
                        
                        if(sub == "target"){                                // <div class="book" udho:target:title="book.title"></div> -> <div class="book" title="Some title"></div>
                            std::string target_attr_name  = rest;
                            std::string target_attr_value = attr.value();
                            bool truth = false;
                            if(target_attr_value.find("?") != std::string::npos){
                                std::vector<std::string> condition_parts;
                                boost::split(condition_parts, target_attr_value, boost::is_any_of("?"));
                                std::string value_condition = condition_parts[0];
                                target_attr_value = condition_parts[1];
                                truth = _expression_evaluator.template evaluate<int>(value_condition);
                            }else{
                                truth = true;
                            }
                            if(truth){
                                std::string resolved_value    = _table.eval(boost::algorithm::trim_copy(target_attr_value));
                                attr.set_name(target_attr_name.c_str());
                                attr.set_value(resolved_value.c_str());
                            }else{
                                removed.push_back(attr);
                            }
                        }else if(sub == "eval"){                                // <div class="book" udho:target:title="book.title"></div> -> <div class="book" title="Some title"></div>
                            std::string target_attr_name  = rest;
                            std::string target_attr_value = attr.value();
                            bool truth = false;
                            if(target_attr_value.find("?") != std::string::npos){
                                std::vector<std::string> condition_parts;
                                boost::split(condition_parts, target_attr_value, boost::is_any_of("?"));
                                std::string value_condition = condition_parts[0];
                                target_attr_value = condition_parts[1];
                                truth = _expression_evaluator.template evaluate<int>(value_condition);
                            }else{
                                truth = true;
                            }
                            if(truth){
                                std::string resolved_value    = boost::lexical_cast<std::string>(_expression_evaluator.template evaluate<int>(target_attr_value));
                                attr.set_name(target_attr_name.c_str());
                                attr.set_value(resolved_value.c_str());
                            }else{
                                removed.push_back(attr);
                            }
                        }else if(sub == "add-class"){                      // <div class="message" udho:add-class:warning="not(field.valid)"></div> -> <div class="message warning"></div>
                            std::string target_attr_name  = rest;
                            std::string target_attr_value = attr.value();
                            bool truth = _expression_evaluator.template evaluate<int>(target_attr_value);
                            if(truth){
                                pugi::xml_attribute class_attr = node.attribute("class");
                                if(class_attr.empty()){
                                    class_attr.set_name("class");
                                    class_attr.set_value(target_attr_name.c_str());
                                    node.append_copy(class_attr);
                                }else{
                                    std::string existing_classes = class_attr.value();
                                    existing_classes += " "+target_attr_name;
                                    class_attr.set_value(existing_classes.c_str());
                                }
                            }
                            removed.push_back(attr);
                        }
                    }
                }
//                 boost::split(parts, name, boost::is_any_of(":"));
//                 if(parts[0] == "udho"){
//                     
//                 }
            }
            for(pugi::xml_attribute attr: removed){
                node.remove_attribute(attr);
            }
            step_in(node, target);
        }
    }
    void step_in(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_node child = target.append_child(node.name());
        for(const pugi::xml_attribute& attr: node.attributes()){
            child.append_copy(attr);
        }
        target = child;
        travarse(node, target);
    }
    void travarse(pugi::xml_node node, pugi::xml_node target){
        for(pugi::xml_node& child: node.children()){
            if(child.type() == pugi::node_pcdata){
                target.append_copy(child);
            }else{
                parse(child, target);
            }
        }
    }
    void parse(){
        parse(_source.document_element(), _transformed.document_element());
    }
    std::string output(){
        std::stringstream stream;
        pugi::xml_node root = _transformed.document_element();
        for(pugi::xml_node child: root){
            child.print(stream, "\t", pugi::format_indent | pugi::format_no_empty_element_tags);
        }
        return stream.str();
    }
};

template <typename ScopeT>
xml_parser<ScopeT> parse_xml(ScopeT& table, const std::string& contents){
    xml_parser<ScopeT> prsr(table);
    prsr.open(contents);
    prsr.parse();
    return prsr;
}

}


}

#endif // UDHO_PARSER_H
