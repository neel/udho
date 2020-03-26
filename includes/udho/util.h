#ifndef UDHO_UTIL_H
#define UDHO_UTIL_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/beast/http/verb.hpp>

namespace udho{

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
