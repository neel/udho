#include <string>
#include <functional>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>

#include <iostream>
#include <udho/application.h>

template <typename R, typename First, typename... Rest>
struct reducer{
    typedef First object_type;
    typedef boost::function<R (Rest...)> function_type;
    typedef boost::function<R (First, Rest...)> actual_function_type;
    
    object_type _that;
    actual_function_type _actual;
    
    reducer(object_type that, actual_function_type actual): _that(that), _actual(actual){}
    R operator()(Rest... rest){
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


struct simple{
    int add(int a, int b){
        return a+b;
    }
    int operator()(udho::request_type req, int a, int b){
        return a+b;
    }
    std::string operator()(udho::request_type req){
        return "Hello World";
    }
};

int main(){
//     std::string doc_root("/home/neel/Projects/udho"); // path to static content
//     boost::asio::io_service io;
//     
    simple s;
//     boost::function<int (udho::request_type, int, int)> add(s);
//     boost::function<std::string (udho::request_type)> hello(s);
// 
//     auto router = udho::router()
//         | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
//         | (udho::get(hello).plain() = "^/hello$");
//     router.listen(io, 9198, doc_root);
    
//     udho::expose(&simple::add, &s);
    
    int (simple::* ftor) (int, int) = &simple::add;
    typedef boost::function<int (simple*, int, int)> ftype;
    ftype fnc = ftor;
    std::cout << (&s->*ftor)(2, 4) << std::endl;
    std::cout << fnc(&s, 2, 4) << std::endl;
    
    typedef boost::function<int (int, int)> ftype2;
    bind_first<ftype> binder(&s, fnc);
    ftype2 bound_function = binder.reduced();
    std::cout << bound_function(2, 4) << std::endl;
    
    std::cout << reduced(&simple::add, &s)(2, 4) << std::endl;
    auto f = reduced(&simple::add, &s);
    std::cout << f(2, 4) << std::endl;
        
//     io.run();
    
    return 0;
}
