/*
 * Copyright 2019 <copyright holder> <email>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <functional>
#include <boost/function.hpp>

namespace udho{

namespace internal{
    // https://stackoverflow.com/questions/26107041/how-can-i-determine-the-return-type-of-a-c11-member-function
    template <typename T>
    struct return_type;
    template <typename R, typename C, typename... Args>
    struct return_type<R(C::*)(Args...)> { using type = R; };
    
    template <typename T>
    struct signature;
    template <typename T, typename R, typename... Args>
    struct signature<R (T::*)(Args...)>{
        using obj  = T;
        using type = boost::function<R (Args...)>;
    };
    
    
}
    
template <typename F>
void expose(F f, typename internal::signature<F>::obj* that){
//     using function_type = typename internal::signature<F>::type;
    std::bind1st(std::mem_fn(f), that);
}

/**
 * @todo write docs
 */
class application{
  std::string _name;
  public:
    application(const std::string& name);
  public:
    std::string name() const;

};

}

#endif // APPLICATION_H
