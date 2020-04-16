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
#include <pugixml.hpp>

namespace udho{
    
namespace view{
    
template <typename ScopeT>
struct shunting_yard{
    typedef ScopeT scope_type;
    typedef shunting_yard<ScopeT> self_type;
    
    std::map<std::string, std::size_t> _operators;
    std::set<std::string>              _functions;
    scope_type&  _table;
    
    shunting_yard(scope_type& table): _table(table){
        _operators.insert(std::make_pair("&&", 1));
        _operators.insert(std::make_pair("||", 1));
        _operators.insert(std::make_pair("&",  1));
        _operators.insert(std::make_pair("|",  1));
        _operators.insert(std::make_pair("<",  2));
        _operators.insert(std::make_pair(">",  2));
        _operators.insert(std::make_pair("<=", 2));
        _operators.insert(std::make_pair(">=", 2));
        _operators.insert(std::make_pair("==", 2));
        _operators.insert(std::make_pair("=",  2));
        _operators.insert(std::make_pair("+",  3));
        _operators.insert(std::make_pair("-",  3));
        _operators.insert(std::make_pair("/",  4));
        _operators.insert(std::make_pair("*",  4));
        
        _functions.insert("count");
        _functions.insert("not");
        _functions.insert("max");
        _functions.insert("min");
    }
    bool is_operator(const std::string& buff) const{
        auto oit = _operators.find(buff);
        if(oit != _operators.end()){
            return true;
        }
        return false;
    };
    bool is_function(const std::string& buff) const{
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
    bool is_real(const std::string& buff) const{
        double res;
        return boost::conversion::try_lexical_convert<double>(buff, res);
    }
    bool is_id(const std::string& buff) const{
        return !is_function(buff) && !is_real(buff) && !is_comma(buff) && !is_parenthesis(buff) && !is_operator(buff);
    }
    std::size_t precedence(const std::string& buff) const{
        if(is_operator(buff)){
            auto oit = _operators.find(buff);
            if(oit != _operators.end()){
                return oit->second;
            }
        }
        return 0;
    }
    void add_operator(const std::string& op, std::size_t precedence){
        _operators.insert(std::make_pair(op, precedence));
    }
    void add_function(const std::string& fnc){
        _functions.insert(fnc);
    }
    std::vector<std::string> tokenized(const std::string& infix){
        typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
        boost::char_separator<char> sep(" \t\n", ",+-*/%()&|<>=", boost::drop_empty_tokens);
        tokenizer tokens(infix, sep);
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
        return processed_tokens;
    }
    std::vector<std::string> reverse_polish(const std::string& infix){
        std::vector<std::string> output;
        std::stack<std::string> stack;
        std::vector<std::string> processed_tokens = tokenized(infix);
        std::size_t fargc = 0;
        for (std::vector<std::string>::iterator tok_iter = processed_tokens.begin(); tok_iter != processed_tokens.end(); ++tok_iter){
            std::string value = *tok_iter;
            if(self_type::is_real(value)){
                output.push_back(value);
            }else if(self_type::is_id(value)){
                output.push_back(value);
            }else if(self_type::is_function(value)){
                stack.push(value);
            }else if(self_type::is_comma(value)){
                ++fargc;
            }else if(self_type::is_parenthesis_open(value)){
                stack.push(value);
            }else if(self_type::is_parenthesis_close(value)){
                while(!stack.empty() && !self_type::is_parenthesis_open(stack.top())){
                    output.push_back(stack.top());
                    stack.pop();
                }
                if(!stack.empty()){
                    stack.pop();
                }
                if(!stack.empty() && self_type::is_function(stack.top())){
                    output.push_back(stack.top()+"@"+boost::lexical_cast<std::string>(fargc+1));
                    fargc = 0;
                    stack.pop();
                }
            }else if(self_type::is_operator(value)){
                while(!stack.empty() 
                    && (
                        (self_type::is_operator(stack.top()) && self_type::precedence(stack.top()) >= self_type::precedence(value))
                    )
                ){
                    output.push_back(stack.top());
                    stack.pop();
                }
                stack.push(value);
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
        
        return output;
    }
    template <typename T>
    T pop(std::stack<std::string>& stack){
        std::string token = stack.top();
        stack.pop();
        T value;
        bool convertible = boost::conversion::try_lexical_convert<T>(token, value);
        if(!convertible){
            value = _table.template extract<T>(token);
        }
        return value;
    }
    template<typename T>
    void push(std::stack<std::string>& stack, T value){
        stack.push(boost::lexical_cast<std::string>(value));
    }
    template <typename T>
    T evaluate(const std::string& infix){
        std::vector<std::string> rpn_tokens = reverse_polish(infix);
        std::stack<std::string> stack;
        for(const std::string& token: rpn_tokens){
            if(!is_operator(token) && token.find("@") == std::string::npos){
                // terminal
//                 T value;
//                 bool convertible = boost::conversion::try_lexical_convert<T>(token, value);
//                 if(!convertible){
//                     value = _table.template extract<T>(token);
//                 }
                stack.push(token);
            }else{
                // non terminal
                if(is_operator(token)){
                    if(token == "+"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second + first);
                    }else if(token == "-"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second - first);
                    }else if(token == "*"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second * first);
                    }else if(token == "/"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second / first);
                    }else if(token == "%"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second % first);
                    }else if(token == "<"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second < first);
                    }else if(token == ">"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second > first);
                    }else if(token == "<="){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second <= first);
                    }else if(token == ">="){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second >= first);
                    }else if(token == "==" || token == "="){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second == first);
                    }else if(token == "&&" || token == "&"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second && first);
                    }else if(token == "||" || token == "|"){
                        T first  = pop<T>(stack);
                        T second = pop<T>(stack);
                        push(stack, second || first);
                    }
                }else if(token.find("@") != std::string::npos){
                    std::vector<std::string> parts;
                    boost::split(parts, token, boost::is_any_of("@"));
                    std::size_t nargs = boost::lexical_cast<std::size_t>(parts[1]);
                    std::string function_name = parts[0];
                    if(function_name == "count"){
                        std::string key = stack.top();
                        stack.pop();
                        std::size_t count = _table.count(key);
                        push(stack, count);
                    }else if(function_name == "not"){
                        std::string key = stack.top();
                        stack.pop();
                        T value;
                        if(is_real(key)){
                            value = boost::lexical_cast<T>(key);
                        }else{
                            value = _table.template extract<T>(key);
                        }
                        push(stack, !value);
                    }
                }
            }
        }
//         std::cout << "rest" << std::endl;
//         while(!stack.empty()){
//             std::cout << stack.top() << std::endl;
//             stack.pop();
//         }
        T last = pop<T>(stack);
        return last;
    }
};

template <typename ScopeT>
shunting_yard<ScopeT> arithmatic(ScopeT& scope){
    return shunting_yard<ScopeT>(scope);
}
    
      
template <typename ScopeT>
struct xml_parser{
    typedef ScopeT& lookup_table_type;
    typedef shunting_yard<ScopeT> expression_evaluator_type;

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
                    _table.list();
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
            for(pugi::xml_attribute attr: node.attributes()){
                std::string name = attr.name();
                std::vector<std::string> parts;
                boost::split(parts, name, boost::is_any_of(":"));
                if(parts[0] == "udho"){
                    if(parts[1] == "target"){
                        std::string target_attr_name  = parts[2];
                        std::string target_attr_value = attr.value();
                        std::string resolved_value    = _table.eval(target_attr_value);
                        attr.set_name(target_attr_name.c_str());
                        attr.set_value(resolved_value.c_str());
                    }
                }
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
            child.print(stream);
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
