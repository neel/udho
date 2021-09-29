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


#ifndef UDHO_PRETTY_DETAIL_H
#define UDHO_PRETTY_DETAIL_H

#include <string>
#include <cstdlib>
#include <cxxabi.h>
#include <typeinfo>
#include <udho/hazo/detail/has_member.h>
#include <ctti/nameof.hpp>

namespace udho{
namespace pretty{
   
namespace detail{
    GENERATE_HAS_MEMBER(pretty);
    
    template <typename T, bool IsClass = std::is_class<T>::value>
    struct is_pretty{};
    
    template <typename T>
    struct is_pretty<T, false>{
        enum { value = false };
    };
    
    template <typename T>
    struct is_pretty<T, true>{
        enum { value = has_member_pretty<T>::value };
    };
    
    template <typename T>
    std::string demangle(){
        // int error = -4;                                                          
        // std::string name = typeid(T).name();
        // char* demangled = abi::__cxa_demangle(name.c_str(), NULL, NULL, &error);
        // if(!error){
        //     name = demangled;
        //     std::free(demangled);
        // }
        // return name;
        return ctti::nameof<T>().str();
    }
}

}
}

#endif // UDHO_ACTIVITIES_DB_PRETTY_DETAIL_H
