#ifndef UDHO_UTILS_STRING_VIEW_H
#define UDHO_UTILS_STRING_VIEW_H

#if __cplusplus < 201703L
#include <boost/utility/string_view.hpp>
#else
#include <string_view>
#endif

namespace udho {
namespace utils {

#if __cplusplus < 201703L
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = boost::basic_string_view<CharT, Traits>;
#else
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = std::basic_string_view<CharT, Traits>;
#endif

using string_view = basic_string_view<char>;

}
}

#endif // UDHO_UTILS_STRING_VIEW_H
