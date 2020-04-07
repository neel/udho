#ifndef UDHO_UTIL_H
#define UDHO_UTIL_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/beast/http/verb.hpp>

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
        typedef R result_type;
        typedef C that_type;
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
    
    template <typename R, typename C, typename... Args>
    struct reduced_<R (C::* ) (Args...) const>{
        typedef R result_type;
        typedef C that_type;
        typedef R (C::* actual_function_type) (Args...) const;
        typedef boost::function<R (const C*, Args...)> base_function_type;
        typedef bind_first<base_function_type> binder_type;
        typedef const typename binder_type::object_type object_type;
        typedef typename binder_type::reduced_function_type reduced_function_type;
        
        base_function_type _function;
        
        reduced_(actual_function_type function): _function(function){}
        reduced_function_type reduced(const C* that){
            binder_type binder(that, _function);
            return binder.reduced();
        }
    };
    
    template <typename T>
    struct member_;
    
    template <typename R, typename C>
    struct member_<R C::*>{
        typedef R result_type;
        typedef C that_type;
        typedef R (C::* actual_function_type);
        typedef boost::function<R (const C* )> const_base_function_type;
        typedef bind_first<const_base_function_type> const_binder_type;
        typedef typename const_binder_type::object_type object_type;
        typedef typename const_binder_type::reduced_function_type const_reduced_function_type;
        
        const_base_function_type _function;
        
        member_(actual_function_type function): _function(function){}
        const_reduced_function_type reduced(const C* that){
            const_binder_type binder(that, _function);
            return binder.reduced();
        }
    };
    
//     template <typename R, typename C>
//     struct member_<R C::*>{
//         typedef C that_type;
//         typedef R (C::* actual_function_type);
//         typedef boost::function<R (C*)> base_function_type;
//         typedef bind_first<base_function_type> binder_type;
//         typedef typename binder_type::reduced_function_type reduced_function_type;
//         typedef typename binder_type::object_type object_type;
//         
//         base_function_type _function;
//         
//         member_(actual_function_type function): _function(function){}
//         reduced_function_type reduced(C* that){
//             binder_type binder(that, _function);
//             return binder.reduced();
//         }
//     };
    

    template <typename T>
    typename reduced_<T>::reduced_function_type reduced(T function, typename reduced_<T>::object_type that){
        reduced_<T> reduced_function(function);
        return reduced_function.reduced(that);
    }
    template <typename T>
    typename member_<T>::const_reduced_function_type member(T function, const typename member_<T>::that_type* that){
        member_<T> reduced_function(function);
        return reduced_function.reduced(that);
    }
}
    
namespace internal{
    boost::beast::string_view mime_type(boost::beast::string_view path);
    std::string path_cat(boost::beast::string_view base, boost::beast::string_view path);
}

struct module_info{
    boost::beast::http::verb _method;
    std::string _pattern;
    std::string _compositor;
    const void* _fptr;
    
    std::vector<module_info> _children;
};
    
namespace util{
    template <typename CharT>
    CharT to_hex(CharT x){
        return x + (x > 9 ? ('A'-10) : '0');
    }
    template <typename CharT>
    CharT from_hex(CharT ch){
        if (ch <= '9' && ch >= '0')
            ch -= '0';
        else if (ch <= 'f' && ch >= 'a')
            ch -= 'a' - 10;
        else if (ch <= 'F' && ch >= 'A')
            ch -= 'A' - 10;
        else 
            ch = 0;
        return ch;
    }
    template <typename CharT>
    std::basic_string<CharT> urlencode(const std::basic_string<CharT>& s){
        typedef std::basic_ostringstream<CharT> stream_type;
        typedef std::basic_string<CharT> string_type;
        typedef typename string_type::const_iterator iterator;
        
        stream_type os;

        for(iterator ci = s.begin(); ci != s.end(); ++ci){
            if((*ci >= 'a' && *ci <= 'z') || (*ci >= 'A' && *ci <= 'Z') || (*ci >= '0' && *ci <= '9')) { // allowed
                os << *ci;
            }else if(*ci == ' '){
                os << '+';
            }else{
                os << '%' << to_hex(*ci >> 4) << to_hex(*ci % 16);
            }
        }
        return os.str();
    }
    template <typename CharT>
    std::basic_string<CharT> urldecode(const std::basic_string<CharT>& str){
        typedef CharT character_type;
        typedef std::basic_ostringstream<CharT> stream_type;
        typedef std::basic_string<CharT> string_type;
        typedef typename string_type::const_iterator iterator;
        typedef typename string_type::size_type size_type;
        
        string_type result;
        size_type i;
        for (i = 0; i < str.size(); ++i){
            if(str[i] == '+'){
                result += ' ';
            }else if (str[i] == '%' && str.size() > i+2){
                character_type ch1 = from_hex(str[i+1]);
                character_type ch2 = from_hex(str[i+2]);
                character_type ch  = (ch1 << 4) | ch2;
                result += ch;
                i += 2;
            }else{
                result += str[i];
            }
        }
        return result;
    }
}
}

#endif // UDHO_UTIL_H
