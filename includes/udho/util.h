#ifndef UDHO_UTIL_H
#define UDHO_UTIL_H

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
}
}

#endif // UDHO_UTIL_H
