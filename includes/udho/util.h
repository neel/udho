#ifndef UTIL_H
#define UTIL_H

#include <string>

namespace udho{
namespace util{
    inline unsigned char to_hex(unsigned char x);
    std::string urlencode(const std::string& s);
    inline unsigned char from_hex(unsigned char ch);
    std::string urldecode(const std::string& str);
}
}

#endif // UTIL_H
