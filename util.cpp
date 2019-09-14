#include "udho/util.h"
#include <sstream>
#include <iostream>
#include <boost/format.hpp>

unsigned char udho::util::to_hex(unsigned char x){
    return x + (x > 9 ? ('A'-10) : '0');
}

std::string udho::util::urlencode(const std::string& s){
    std::ostringstream os;

    for(std::string::const_iterator ci = s.begin(); ci != s.end(); ++ci){
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

unsigned char udho::util::from_hex(unsigned char ch){
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

std::string udho::util::urldecode(const std::string& str){
    std::string result;
    std::string::size_type i;
    for (i = 0; i < str.size(); ++i){
        if(str[i] == '+'){
            result += ' ';
        }else if (str[i] == '%' && str.size() > i+2){
            const unsigned char ch1 = from_hex(str[i+1]);
            const unsigned char ch2 = from_hex(str[i+2]);
            const unsigned char ch = (ch1 << 4) | ch2;
            result += ch;
            i += 2;
        }else{
            result += str[i];
        }
    }
    return result;
}


void udho::util::dump_module_info(const std::vector<udho::module_info>& infos){
    static int indent = 0;
    for(auto i = infos.rbegin(); i != infos.rend(); ++i){
        auto info = *i;
        std::string method_str;
        
        if(info._compositor == "UNSPECIFIED"){
            continue;
        }
        
        for(int j = 0; j < indent; ++j){
            std::cout << "\t";
        }
        std::cout << boost::format("%1% %2% -> %3% (%4%)") % info._method % info._pattern % info._fptr % info._compositor << std::endl;
        if(info._children.size() > 0){
            ++indent;
            dump_module_info(info._children);
            --indent;
        }
    }
}

