#ifndef UDHO_UTILS_STRING_VIEW_H
#define UDHO_UTILS_STRING_VIEW_H


#if __cplusplus < 201703L
#include <boost/utility/string_view.hpp>
#else
#include <string_view>
#endif

namespace udho{
namespace utils{

#if __cplusplus < 201703L
template<typename CharT, typename Traits>
using basic_string_view = boost::basic_string_view<CharT, Traits>;
using string_view = basic_string_view<char>;
#endif

}
}

#endif // UDHO_UTILS_STRING_VIEW_H
