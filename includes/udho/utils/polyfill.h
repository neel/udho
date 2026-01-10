#ifndef UDHO_UTILS_CONDITIONAL_H
#define UDHO_UTILS_CONDITIONAL_H

#include <type_traits>

namespace udho {
namespace utils {

#if __cplusplus < 201703L
template<bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <typename...>
using void_t = void;
#else
template<bool B, class T, class F>
using conditional_t = typename std::conditional_t<B, T, F>;

template <typename... Ts>
using void_t = std::void_t<Ts...>;
#endif



}
}

#endif // UDHO_UTILS_CONDITIONAL_H
