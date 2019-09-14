#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
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
    inline unsigned char to_hex(unsigned char x);
    std::string urlencode(const std::string& s);
    inline unsigned char from_hex(unsigned char ch);
    std::string urldecode(const std::string& str);
    void dump_module_info(const std::vector<udho::module_info>& infos);
    template <typename T>
    void print_summary(T& router){
        std::vector<udho::module_info> summary;
        router.summary(summary);
        dump_module_info(summary);
    }
}
}

#endif // UTIL_H
