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

#ifdef WITH_PUGI
#include <pugixml.hpp>
#endif

namespace udho{

#ifdef WITH_PUGI
      
template <typename ScopeT>
struct xml_parser{
    typedef ScopeT& lookup_table_type;

    lookup_table_type&  _table;
    pugi::xml_document  _source;
    pugi::xml_document  _transformed;
    
    xml_parser(lookup_table_type& table): _table(table){}
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
        for(pugi::xml_node& child: node){
            parse(child, target);
        }
    }
    void parse(){
        parse(_source.document_element(), _transformed.document_element());
        _transformed.save(std::cout);
    }
};

template <typename ScopeT>
void parse_xml(ScopeT& table, const std::string& contents){
    xml_parser<ScopeT> prsr(table);
    prsr.open(contents);
    prsr.parse();
}

#endif

}

#endif // UDHO_PARSER_H
