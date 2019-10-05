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

#include "util.h"
#include <string>
#include <functional>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/function.hpp>
#include <udho/router.h>

namespace udho{

namespace internal{
    
    template <typename R, typename First, typename... Rest>
    struct reducer{
        typedef First object_type;
        typedef boost::function<R (Rest...)> function_type;
        typedef boost::function<R (First, Rest...)> actual_function_type;
        
        object_type _that;
        actual_function_type _actual;
        
        reducer(object_type that, actual_function_type actual): _that(that), _actual(actual){}
        R operator()(const Rest&... rest){
            return _actual(_that, rest...);
        }
    };

    template <typename T>
    struct bind_first;

    template <typename R, typename... Args>
    struct bind_first<boost::function<R (Args...)>>{
        typedef reducer<R, Args...> reducer_type;
        typedef boost::function<R (Args...)> base_function_type;
        typedef typename reducer_type::object_type object_type;
        typedef typename reducer_type::function_type reduced_function_type;
        
        object_type _that;
        reducer_type _reducer;
        
        bind_first(object_type that, base_function_type base): _reducer(that, base){}
        reduced_function_type reduced(){
            reduced_function_type reduced_function = _reducer;
            return reduced_function;
        }
    };

    template <typename T>
    struct reduced_;

    template <typename R, typename C, typename... Args>
    struct reduced_<R (C::* ) (Args...)>{
        typedef R (C::* actual_function_type) (Args...);
        typedef boost::function<R (C*, Args...)> base_function_type;
        typedef bind_first<base_function_type> binder_type;
        typedef typename binder_type::object_type object_type;
        typedef typename binder_type::reduced_function_type reduced_function_type;
        
        base_function_type _function;
        
        reduced_(actual_function_type function): _function(function){}
        reduced_function_type reduced(C* that){
            binder_type binder(that, _function);
            return binder.reduced();
        }
    };

    template <typename T>
    typename reduced_<T>::reduced_function_type reduced(T function, typename reduced_<T>::object_type that){
        reduced_<T> reduced_function(function);
        return reduced_function.reduced(that);
    }
}
    
/**
 * @brief all applications derive from this class
 * @details
 * applications are a class having a set of methods bound with urls
 * @code
 * struct my_app: public udho::application<my_app>{
 *      my_app();
 *      int add(udho::request_type req, int a, int b);
 *      int mul(udho::request_type req, int a, int b);
 *      template <typename RouterT>
 *      auto route(RouterT& router){
 *          return router | (get(&my_app::add).plain() = "^/add/(\\d+)/(\\d+)$")
 *                        | (get(&my_app::mul).plain() = "^/mul/(\\d+)/(\\d+)$");
 *      }
 * };
 * @endcode
 * @example examples/application.cpp
 * @tparam DerivedT The derived application class
 */
template <typename DerivedT>
class application{
  std::string _name;
  public:
    application(const std::string& name): _name(name){}
  public:
    std::string name() const{return _name;}
  public:
    /**
     * add a get method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto get(F f){
        return udho::get(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a post method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto post(F f){
        return udho::post(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a head method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto head(F f){
        return udho::head(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a put method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto put(F f){
        return udho::put(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a delete method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto del(F f){
        return udho::del(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
};

template <typename AppT>
struct app_{
    typedef app_<AppT> self_type;
    
    std::string _path;
    AppT        _app;
    
    self_type& operator=(const std::string& path){
        _path = path;
        return *this;
    }
    template <typename ReqT, typename Lambda>
    http::status serve(const ReqT& req, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        auto router = udho::router();
        std::cout << "app serve " << subject << std::endl;
        return _app.route(router).serve(req, request_method, subject, send);
    }
    void summary(std::vector<module_info>& stack){
        auto router = udho::router();
        module_info info;
        info._pattern = _path;
        info._fptr = &_app;
        info._compositor = "APPLICATION";
        info._method = boost::beast::http::verb::unknown;
        _app.route(router).summary(info._children);
        stack.push_back(info);
    }
};

template <typename U, typename V>
struct overload_group<U, app_<V>>{
    typedef overload_group<U, app_<V>>  self_type;
    typedef U                           parent_type;
    typedef app_<V>                     overload_type;
    
    parent_type   _parent;
    overload_type _overload;
    
    overload_group(const parent_type& parent, const overload_type& overload): _parent(parent), _overload(overload){}
    template <typename ReqT, typename Lambda>
    http::status serve(const ReqT& req, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        std::string subject_decoded = udho::util::urldecode(subject);
        boost::smatch match;
        bool result = boost::u32regex_search(subject_decoded, match, boost::make_u32regex(_overload._path));
        std::cout << "app match " << result << std::endl;
        if(result){
            std::string rest = result ? subject.substr(match.length()) : subject;
            return _overload.serve(req, request_method, rest, send);
        }else{
            return _parent.template serve<ReqT, Lambda>(req, request_method, subject, send);
        }
    }
    
    void summary(std::vector<module_info>& stack){
        _overload.summary(stack);
        _parent.summary(stack);
    }
    self_type& listen(boost::asio::io_service& io, int port=9198, std::string doc_root=""){
        typedef udho::listener<self_type> listener_type;
        std::make_shared<listener_type>(*this, io, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port), std::make_shared<std::string>(doc_root))->run();
        return *this;
    }
};

template <typename AppT>
app_<AppT> app(){
    return app_<AppT>();
}

}

#endif // APPLICATION_H
