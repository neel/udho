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


#ifndef UDHO_ACTIVITIES_DB_PRETTY_PRINTER_H
#define UDHO_ACTIVITIES_DB_PRETTY_PRINTER_H

#include <map>
#include <string>
#include <boost/algorithm/string/replace.hpp>
#include <udho/db/pretty/detail.h>
#include <udho/db/pretty/fwd.h>

namespace udho{
namespace db{
namespace pretty{
    
namespace detail{
    
    /**
     * substitute helper for pretty type
     */
    struct substitution{       
        /**
         * replace any occurence of demangle<T>() with replacement string which defaults to the pretty name for type T
         */
        template <typename T>
        void substitute(const std::string& replacement = type<T>::name()){
            _dictionary.insert(std::make_pair(detail::demangle<T>(), replacement));
        }
        
        /**
         * replace any occurence of demangle<T>() with replacement string which defaults to the pretty name for type T
         */
        template <typename T>
        void substitute(const T&&, const std::string& replacement = type<T>::name()){
            _dictionary.insert(std::make_pair(detail::demangle<T>(), replacement));
        }
        
        /**
         * replace any occurence of demangle<T>... with pretty name of type T
         */
        template <typename... T>
        void substitute_all(){
            [[maybe_unused]] int x[] = {(substitute<T>(), 0)...};
        }
        
        protected:
            inline void apply(std::string& demangled) const {
                for(const auto& pair: _dictionary){
                    boost::replace_all(demangled, pair.first, pair.second);
                }
            }
        private:
            std::map<std::string, std::string, std::greater<std::string>> _dictionary;
    };
}

struct printer: detail::substitution{
    inline std::string sanitize(const std::string& input) const{
        std::string sanitized = input;
        detail::substitution::apply(sanitized);
        return sanitized;
    }
    inline std::string operator()(const std::string& input) const{
        return sanitize(input);
    }
};
    
}
}
}


#endif // UDHO_ACTIVITIES_DB_PRETTY_PRINTER_H
