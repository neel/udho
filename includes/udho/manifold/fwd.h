#ifndef UDHO_MANIFOLD_FWD_H
#define UDHO_MANIFOLD_FWD_H

#include <cstdint>

namespace udho {
namespace manifold {

template <typename...>
struct composition;

template <typename...>
struct params;

template <std::size_t, typename...>
struct fabric;

template <typename...>
struct journal;

template <std::size_t, typename...>
struct evaluator;

// template <std::size_t, typename...>
// struct pipeline;

template <typename ComponentT>
struct wrapper;

namespace detail{

template <typename ComponentT, bool>
struct facet_wrapper;

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT, bool YieldsResult>
class next_evaluator_helper_internal;

}

template <typename ComponentT, typename FeatureT>
struct facet;

template <typename ComponentT>
struct config;


}
}

#endif // UDHO_MANIFOLD_FWD_H
