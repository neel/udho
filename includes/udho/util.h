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
//     boost::beast::string_view mime_type(boost::beast::string_view path);
    std::string path_cat(boost::beast::string_view base, boost::beast::string_view path);
}

struct module_info{
    boost::beast::http::verb _method;
    std::string _pattern;
    std::string _compositor;
    const void* _fptr;
    
    std::vector<module_info> _children;
};

template<class ForwardIt1, class ForwardIt2>
constexpr ForwardIt1 _search(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last){
    for (; ; ++first) {
        ForwardIt1 it = first;
        for (ForwardIt2 s_it = s_first; ; ++it, ++s_it) {
            if (s_it == s_last) {
                return first;
            }
            if (it == last) {
                return last;
            }
            if (!(*it == *s_it)) {
                break;
            }
        }
    }
}
    
template <typename Iterator>
struct bounded_str{
    typedef bounded_str<Iterator> self_type;
    typedef std::pair<Iterator, Iterator> pair_type;
    
    pair_type _pair;
    
    bounded_str(Iterator begin, Iterator end): _pair(begin, end){}
    bounded_str(const self_type& other): _pair(other._pair){}
    self_type& operator=(const self_type& other){_pair = other._pair; return *this;}
    Iterator begin() const{return _pair.first;}
    Iterator end() const{return _pair.second;}
    bool invalid() const{return begin() == end();}
    bool valid() const{return !invalid();}
    template <typename Itarator>
    void copy(Itarator it) const{
        std::copy(begin(), end(), it);
    }
    template <typename ContainerT>
    ContainerT copied() const{
        ContainerT out;
        copy(std::back_inserter(out));
        return out;
    }
    std::size_t size() const{
        return std::distance(begin(), end());
    }
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
    inline std::string charToHex(char c){
        std::string result;
        char first, second;

        first = (c & 0xF0) / 16;
        first += first > 9 ? 'A' - 10 : '0';
        second = c & 0x0F;
        second += second > 9 ? 'A' - 10 : '0';

        result.append(1, first);
        result.append(1, second);

        return result;
    }

    inline char hexToChar(char first, char second){
        int digit;

        digit = (first >= 'A' ? ((first & 0xDF) - 'A') + 10 : (first - '0'));
        digit *= 16;
        digit += (second >= 'A' ? ((second & 0xDF) - 'A') + 10 : (second - '0'));
        return static_cast<char>(digit);
    }
    
    template <typename CharT>
    std::basic_string<CharT> urlencode(const std::basic_string<CharT>& src){
        typedef std::basic_ostringstream<CharT> stream_type;
        typedef std::basic_string<CharT> string_type;
        typedef typename string_type::const_iterator iterator;
        
        string_type result;
        iterator iter;
        
        for(iter = src.begin(); iter != src.end(); ++iter) {
            switch(*iter) {
                case ' ':
                    result.append(1, '+');
                    break;
                // alnum
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
                case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
                case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
                case 'V': case 'W': case 'X': case 'Y': case 'Z':
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
                case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
                case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
                case 'v': case 'w': case 'x': case 'y': case 'z':
                case '0': case '1': case '2': case '3': case '4': case '5': case '6':
                case '7': case '8': case '9':
                // mark
                case '-': case '_': case '.': case '!': case '~': case '*': case '\'': 
                case '(': case ')':
                    result.append(1, *iter);
                    break;
                // escape
                default:
                    result.append(1, '%');
                    result.append(charToHex(*iter));
                    break;
            }
        }
        
        return result;
    }
    template <typename CharT>
    std::basic_string<CharT> urldecode(const std::basic_string<CharT>& src){
        typedef CharT character_type;
        typedef std::basic_ostringstream<CharT> stream_type;
        typedef std::basic_string<CharT> string_type;
        typedef typename string_type::const_iterator iterator;
        typedef typename string_type::size_type size_type;
        
        string_type result;
        iterator iter;
        char c;

        for(iter = src.begin(); iter != src.end(); ++iter) {
            switch(*iter) {
                case '+':
                    result.append(1, ' ');
                    break;
                case '%':
                    // Don't assume well-formed input
                    if(std::distance(iter, src.end()) >= 2 && std::isxdigit(*(iter + 1)) && std::isxdigit(*(iter + 2))) {
                        c = *++iter;
                        result.append(1, hexToChar(c, *++iter));
                    }
                    // Just pass the % through untouched
                    else {
                        result.append(1, '%');
                    }
                    break;
                default:
                    result.append(1, *iter);
                    break;
            }
        }

        return result;
    }

}
}

#endif // UDHO_UTIL_H
