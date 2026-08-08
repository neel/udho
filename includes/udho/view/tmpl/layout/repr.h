#ifndef UDHO_VIEW_TMPL_LAYOUT_REPR_H
#define UDHO_VIEW_TMPL_LAYOUT_REPR_H

#include <type_traits>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

template <typename Data>
struct repr;

template<typename T, typename = void>
struct has_repr: std::false_type {};

template<typename T>
struct has_repr<T, std::void_t<decltype(sizeof(repr<T>))>>: std::true_type {};

template<typename T>
inline constexpr bool has_repr_v = has_repr<T>::value;

}
}
}
}

#endif // UDHO_VIEW_TMPL_LAYOUT_REPR_H
