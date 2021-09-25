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


#ifndef UDHO_ACTIVITIES_DB_PRETTY_TYPE_H
#define UDHO_ACTIVITIES_DB_PRETTY_TYPE_H

#include <set>
#include <string>
#include <udho/db/pretty/detail.h>
#include <udho/db/pretty/fwd.h>
#include <udho/db/pretty/printer.h>

namespace udho{
namespace db{
namespace pretty{
   
/**
 * If type T is either not a class or is a class but does not have a static method named pretty that returns std::string
 */
template <typename T>
struct type<T, false>{
    static std::string name(const printer& p = printer()){
        return p(detail::demangle<T>());
    }
};

template <typename T>
std::string demangle(const printer& p = printer()){
    return p(detail::demangle<T>());
}

/**
 * If type T is a class and has a static method named pretty that return std::string
 */
template <typename T>
struct type<T, true>{
    static std::string name(const printer& p = printer()){
        return p(T::pretty());
    }
};

/**
 * returns a pretty demangled name for the type provided
 */
template <typename T>
std::string name(const printer& p = printer()){
    return type<T>::name(p);
}

/**
 * returns an indented pretty demangled name for the type provided
 */
template <typename T>
std::string indent(const printer& p = printer()){
    std::string in = name<T>(p);
    std::string out;
    std::string::const_iterator begin = in.cbegin();
    auto inserter = std::back_inserter(out);
    std::string local;
    std::size_t level = 0;
    bool expanding = true;
    std::size_t collapsing_since = 0;
    
    std::set<std::string> inline_types;
    inline_types.insert("boost::hana::string");
    inline_types.insert("udho::db::pg::constants::string");
    inline_types.insert("udho::db::pg::column");
    inline_types.insert("udho::db::pg::max");
    inline_types.insert("udho::db::pg::min");
    inline_types.insert("udho::db::pg::count");
    inline_types.insert("udho::db::pg::avg");
    inline_types.insert("udho::db::pg::sum");
    inline_types.insert("udho::db::pg::from");
    inline_types.insert("::limit");
   
    auto indent = [&]() mutable{
        if(expanding){
            *(inserter++) = '\n';
            for(std::size_t j = 0; j != level; ++j) *(inserter++) = '\t';
        }
    };
   
    for(std::string::const_iterator i = begin; i != in.cend(); ++i){
        if(*i == ' ' && *(i-1) != '\'') continue;
        if(*i == '<'){
            ++level;
            *(inserter++) = *i;
            indent();
        }else if(*i == ','){
            *(inserter++) = *i;
            indent();
        }else if(*i == '>'){
            --level;
            indent();
            *(inserter++) = *i;
            if(!expanding){
                expanding = level <= collapsing_since;
            }
        }else{
            *(inserter++) = *i;
        }
       
        if((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i >= '0' && *i <= '9') || *i == '_' || *i == ':'){
            if(expanding){
                local += *i;
                if(inline_types.count(local) > 0){
                    collapsing_since = level;
                    expanding = false;
                }
            }
        }else{
            local = "";
        }
    }
    return out;
}

}
}
}

#endif // UDHO_ACTIVITIES_DB_PRETTY_TYPE_H
