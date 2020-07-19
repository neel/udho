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
#include <boost/algorithm/string/predicate.hpp>
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
    
namespace parsing{

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
                    std::string value = t.value();
                    boost::split(parts, value, boost::is_any_of("@"));
                    std::size_t args = boost::lexical_cast<std::size_t>(parts[1]);
                    std::string name = parts[0];
                    T result;
                    token key_token = stack.top(); stack.pop();
                    if(name == "count"){
                        result = static_cast<T>(count(key_token));
                    }else if(name == "not"){
                        try{
                            result = !fetch<T>(key_token);
                        }catch(const std::out_of_range&){
                            result = T(0);
                        }
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

struct xml{
    typedef boost::function<bool (pugi::xml_node, pugi::xml_node)>                      tag_directive;
    typedef boost::function<bool (pugi::xml_node, pugi::xml_node, pugi::xml_attribute)> attr_directive;
    typedef std::map<std::string, tag_directive>    tag_directive_map;
    typedef std::map<std::string, attr_directive>   attr_directive_map;
    
    pugi::xml_document  _source;
    pugi::xml_document  _transformed;
    tag_directive_map   _directives_tag;
    attr_directive_map  _directives_attr;
    
    inline void open(const std::string& contents){
        _source.load_string(contents.c_str());
        _transformed.append_child("udho:transformed");
    }
    inline void clear(){
        _source = pugi::xml_document();
        _transformed = pugi::xml_document();
    }
    inline void parse(pugi::xml_node node, pugi::xml_node target){
        std::string name = node.name();
        bool processor_matched = false;
        for(auto& p: _directives_tag){
            if(boost::starts_with(name, p.first)){
                p.second(node, target);
                processor_matched = true;
                break;
            }
        }
        if(!processor_matched){
            bool keep = true;
            for(pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end();){
                pugi::xml_attribute attr = *it++;
                std::string name = attr.name();
                for(auto& p: _directives_attr){
                    if(boost::starts_with(name, p.first)){
                        keep = keep && p.second(node, target, attr);
                        break;
                    }
                }
                if(!keep){
                    break;
                }
            }
            if(keep){
                step_in(node, target);
            }
        }
    }
    inline void step_in(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_node child = target.append_child(node.name());
        for(const pugi::xml_attribute& attr: node.attributes()){
            child.append_copy(attr);
        }
        target = child;
        travarse(node, target);
    }
    inline void travarse(pugi::xml_node node, pugi::xml_node target){
        for(pugi::xml_node& child: node.children()){
            if(child.type() == pugi::node_pcdata){
                target.append_copy(child);
            }else{
                parse(child, target);
            }
        }
    }
    inline void parse(){
        parse(_source.document_element(), _transformed.document_element());
    }
    inline std::string output(){
        std::stringstream stream;
        pugi::xml_node root = _transformed.document_element();
        for(pugi::xml_node child: root){
            child.print(stream, "\t", pugi::format_indent | pugi::format_no_empty_element_tags);
        }
        return stream.str();
    }
    template <typename DirectiveT>
    inline xml& add_directive_tag(DirectiveT directive){
        _directives_tag.insert(std::make_pair(directive.prefix(), directive));
        return *this;
    }
    template <typename DirectiveT>
    inline xml& add_directive_attr(DirectiveT directive){
        _directives_attr.insert(std::make_pair(directive.prefix(), directive));
        return *this;
    }
};

namespace tags{

/**
 * udho::block
 */
template <typename ScopeT>
struct block{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    block(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        _table.down();
        _ctrl.travarse(node, target);
        _table.up();
        return true;
    }
    std::string prefix() const{
        return "udho:block";
    }
};

/**
 * udho::var
 */
template <typename ScopeT>
struct var{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    var(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_attribute name = node.find_attribute([](const pugi::xml_attribute& attr){
            return attr.name() == std::string("name");
        });
        pugi::xml_attribute value = node.find_attribute([](const pugi::xml_attribute& attr){
            return attr.name() == std::string("value");
        });
        _table.add(name.as_string(), value.as_string());
        return true;
    }
    std::string prefix() const{
        return "udho:var";
    }
};
    
/**
 * udho::for
 */
template <typename ScopeT>
struct loop{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    loop(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
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
            pugi::xml_node cloned = node.parent().append_copy(node);
            _ctrl.travarse(cloned, target);
            node.parent().remove_child(cloned);
            _table.up();
        }
        return true;
    }
    std::string prefix() const{
        return "udho:for";
    }
};

/**
 * udho::if
 */
template <typename ScopeT>
struct condition{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    condition(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_attribute cond = node.find_attribute([](const pugi::xml_attribute& attr){
            return attr.name() == std::string("test");
        });
        std::string condition = cond.as_string();
        bool truth = _evaluator.template evaluate<int>(condition);
        if(truth){
            _ctrl.travarse(node, target);
        }
        return true;
    }
    std::string prefix() const{
        return "udho:if";
    }
};

/**
 * udho::text
 */
template <typename ScopeT>
struct text{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    text(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_attribute name = node.find_attribute([](const pugi::xml_attribute& attr){
            return attr.name() == std::string("name");
        });
        std::string var = name.as_string();
        std::string value = _table.eval(var);
        target.append_child(pugi::node_pcdata).text().set(value.c_str());
        return true;
    }
    std::string prefix() const{
        return "udho:text";
    }
};

/**
 * udho::html
 */
template <typename ScopeT>
struct html{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    html(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_attribute name = node.find_attribute([](const pugi::xml_attribute& attr){
            return attr.name() == std::string("name");
        });
        std::string var = name.as_string();
        std::string value = _table.eval(var);
        pugi::xml_document doc;
        if (!doc.load_buffer(value.c_str(), value.length()))
            return false;
        for (pugi::xml_node child = doc.first_child(); child; child = child.next_sibling())
            target.append_copy(child);
        return true;
    }
    std::string prefix() const{
        return "udho:html";
    }
};

/**
 * udho::eval
 */
template <typename ScopeT>
struct eval{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    eval(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        pugi::xml_attribute name = node.find_attribute([](const pugi::xml_attribute& attr){
            return attr.name() == std::string("name");
        });
        std::string var = name.as_string();
        std::string value = boost::lexical_cast<std::string>(_evaluator.template evaluate<int>(var));
        target.append_child(pugi::node_pcdata).text().set(value.c_str());
        return true;
    }
    std::string prefix() const{
        return "udho:eval";
    }
};

/**
 * udho::template
 */
template <typename ScopeT>
struct skip{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    skip(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node target){
        _ctrl.step_in(node, target);
        return true;
    }
    std::string prefix() const{
        return "udho:view";
    }
};

}

namespace attrs{
    
/**
 * works like the udho:if tag but can be used in attribute 
 */
template <typename ScopeT>
struct condition{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    condition(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node /*target*/, pugi::xml_attribute attr){
        std::string expr = attr.value();
        
        bool truth = _evaluator.template evaluate<int>(expr);
        if(truth){
            node.remove_attribute(attr);
            return true;
        }else{
            return false;
        }
    }
    std::string prefix() const{
        return "udho:if";
    }
};
    
/**
 * udho::target:NAME = KEY will add an attribute named `NAME` with value retrieved by resolving `KEY` through scope table. 
 * It can have a conditional form too where the condition `expr` is first evaluated and if expr is true then the attribute is added, otherwise not
 * \code
 * <div class="book" udho:target:title="book.title"></div>
 * <div class="book" udho:target:title="expr ? book.title"></div>
 * \endcode
 * is transformed to
 * \code
 * <div class="book" title="Some title"></div>
 * <div class="book" title="Some title"></div>
 * \endcode
 */
template <typename ScopeT>
struct target{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    target(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node /*target*/, pugi::xml_attribute attr){
        std::string name = attr.name();
        std::size_t indx = name.find(prefix());
        std::string rest = name.substr(indx+prefix().size()+1);
        std::string expr = attr.value();
        
        bool truth = true;
        if(expr.find("?") != std::string::npos){
            std::vector<std::string> parts;
            boost::split(parts, expr, boost::is_any_of("?"));
            std::string condition = boost::algorithm::trim_copy(parts[0]);
            expr = parts[1];
            truth = _evaluator.template evaluate<int>(condition);
        }
        if(truth){
            std::string resolved = _table.eval(boost::algorithm::trim_copy(expr));
            node.append_attribute(rest.c_str()) = resolved.c_str();
        }
        node.remove_attribute(attr);
        return true;
    }
    std::string prefix() const{
        return "udho:target";
    }
};

/**
 * udho::eval:NAME = EXPR will add an attribute named `NAME` with value retrieved by evaluating the expression `EXPR`. 
 * It can have a conditional form too where the condition `expr` is first evaluated and if expr is true then the attribute is added, otherwise not
 * \code
 * <input type="text" udho:eval:value="count(books)"></input>
 * <input type="text" udho:eval:title="expr ? count(books)"></input>
 * \endcode
 * is transformed to
 * \code
 * <input type="text" value="2"></input>
 * <input type="text" value="2"></input>
 * \endcode
 */
template <typename ScopeT>
struct eval{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    eval(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node /*target*/, pugi::xml_attribute attr){
        std::string name = attr.name();
        std::size_t indx = name.find(prefix());
        std::string rest = name.substr(indx+prefix().size()+1);
        std::string expr = attr.value();
        
        bool truth = true;
        if(expr.find("?") != std::string::npos){
            std::vector<std::string> parts;
            boost::split(parts, expr, boost::is_any_of("?"));
            std::string condition = boost::algorithm::trim_copy(parts[0]);
            expr = parts[1];
            truth = _evaluator.template evaluate<int>(condition);
        }
        if(truth){
            std::string resolved = boost::lexical_cast<std::string>(_evaluator.template evaluate<int>(boost::algorithm::trim_copy(expr)));
            attr.set_name(rest.c_str());
            attr.set_value(resolved.c_str());
        }else{
            node.remove_attribute(attr);
        }
        return true;
    }
    std::string prefix() const{
        return "udho:eval";
    }
};

/**
 * udho::add-class:NAME = EXPR will add a class named `NAME` if the expression `EXPR` is true. 
 * \code
 * <div udho:add-class:visible="count(books) > 1"></div>
 * \endcode
 * is transformed to
 * \code
 * <div class="visible"></div>
 * \endcode
 */
template <typename ScopeT>
struct add_class{
    typedef ScopeT table_type;
    typedef parsing::expression<ScopeT> evaluator_type;
    
    parsing::xml&   _ctrl;
    table_type&    _table;
    evaluator_type _evaluator;
    
    add_class(parsing::xml& ctrl, table_type& table): _ctrl(ctrl), _table(table), _evaluator(table){}
    bool operator()(pugi::xml_node node, pugi::xml_node /*target*/, pugi::xml_attribute attr){
        std::string name = attr.name();
        std::size_t indx = name.find(prefix());
        std::string rest = (indx+prefix().size() != name.size()) ? name.substr(indx+prefix().size()+1) : std::string();
        std::string expr = attr.value();
        
        bool truth = rest.empty() ? true : _evaluator.template evaluate<int>(expr);
        if(truth){
            pugi::xml_attribute class_attr = node.attribute("class");
            std::string classname = rest;
            if(classname.empty()){
                classname = _table.eval(boost::algorithm::trim_copy(expr));
            }
            if(class_attr.empty()){
                class_attr.set_name("class");
                class_attr.set_value(classname.c_str());
                node.append_copy(class_attr);
            }else{
                std::string existing_classes = class_attr.value();
                existing_classes += " "+classname;
                class_attr.set_value(existing_classes.c_str());
            }
        }
        node.remove_attribute(attr);
        return true;
    }
    std::string prefix() const{
        return "udho:add-class";
    }
};
    
}

}

template <typename ScopeT>
parsing::expression<ScopeT> expression(ScopeT& scope){
    return parsing::expression<ScopeT>(scope);
}

template <typename ScopeT>
struct parser{
    typedef ScopeT                                  table_type;
    typedef parsing::expression<table_type>         evaluator_type;
    typedef parsing::xml                            xml_parser_type;
    typedef parsing::tags::block<table_type>        block_directive_type;
    typedef parsing::tags::var<table_type>          var_directive_type;
    typedef parsing::tags::loop<table_type>         loop_directive_type;
    typedef parsing::tags::condition<table_type>    condition_directive_type;
    typedef parsing::tags::text<table_type>         text_directive_type;
    typedef parsing::tags::html<table_type>         html_directive_type;
    typedef parsing::tags::eval<table_type>         eval_directive_type;
    typedef parsing::tags::skip<table_type>         skip_directive_type;
    typedef parsing::attrs::target<table_type>      target_directive_type;
    typedef parsing::attrs::eval<table_type>        eval_attr_directive_type;
    typedef parsing::attrs::add_class<table_type>   add_class_directive_type;
    typedef parsing::attrs::condition<table_type>   condition_attr_directive_type;
    
    table_type&              _table;
    evaluator_type           _evaluator;
    xml_parser_type          _xml_parser;
    block_directive_type     _block;
    var_directive_type       _var;
    loop_directive_type      _loop;
    condition_directive_type _condition;
    text_directive_type      _text;
    html_directive_type      _html;
    eval_directive_type      _eval;
    skip_directive_type      _skip;
    target_directive_type    _target;
    eval_attr_directive_type _eval_attr;
    add_class_directive_type _add_class;
    condition_attr_directive_type _condition_attr;
    
    parser(table_type& scope): _table(scope), _evaluator(scope), 
    _block(_xml_parser, scope), _var(_xml_parser, scope), _loop(_xml_parser, scope), _condition(_xml_parser, scope), _text(_xml_parser, scope), _html(_xml_parser, scope), _eval(_xml_parser, scope),_skip(_xml_parser, scope),
    _target(_xml_parser, scope), _eval_attr(_xml_parser, scope), _add_class(_xml_parser, scope), _condition_attr(_xml_parser, scope)
    {
        prepare();
    }
    void prepare(){
        _xml_parser.add_directive_tag(_block)
            .add_directive_tag(_var)
            .add_directive_tag(_loop)
            .add_directive_tag(_condition)
            .add_directive_tag(_text)
            .add_directive_tag(_html)
            .add_directive_tag(_eval)
            .add_directive_tag(_skip);
        _xml_parser.add_directive_attr(_target)
            .add_directive_attr(_eval_attr)
            .add_directive_attr(_add_class)
            .add_directive_attr(_condition_attr);
    }
    std::string process(const std::string& contents){
        _xml_parser.open(contents);
        _xml_parser.parse();
        return _xml_parser.output();
    }
};

/**
 * returns a parser interface using which an xml based view can be parsed
 * ScopeT must be a lookup table which can be obtained by using udho::scope
 */
template <typename ScopeT>
parser<ScopeT> processor(ScopeT& scope){
    return parser<ScopeT>(scope);
}

}


}

#endif // UDHO_PARSER_H
