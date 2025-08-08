#ifndef UDHO_UTILS_TRAITS_H
#define UDHO_UTILS_TRAITS_H

#include <utility>
#include <ostream>

namespace udho{
namespace utils{
namespace traits{

template<typename S, typename T>
class is_streamable{
    template<typename SS, typename TT>
    static auto test(int) -> decltype( std::declval<SS&>() << std::declval<TT>(), std::true_type() );
    template<typename, typename>
    static auto test(...) -> std::false_type;
  public:
    static const bool value = decltype(test<S,T>(0))::value;
};

template<typename S, typename T>
using is_streamable_t = is_streamable<S, T>;

template<typename S, typename T>
constexpr bool is_streamable_v = is_streamable_t<S, T>::value;

template <typename T>
using is_ostreamable = is_streamable<std::ostream, T>;

template<typename T>
using is_ostreamable_t = is_ostreamable<T>;

template<typename T>
constexpr bool is_ostreamable_v = is_ostreamable_t<T>::value;

template <typename...>
struct accumulate : std::integral_constant<std::size_t, 0> {};

template <typename T, typename... Ts>
struct accumulate<T, Ts...> : std::integral_constant<std::size_t,  T::value + accumulate<Ts...>::value> {};

}
}
}

#endif // UDHO_UTILS_TRAITS_H
