#ifndef UDHO_MANIFOLD_FWD_H
#define UDHO_MANIFOLD_FWD_H


namespace udho {
namespace manifold {

template <typename...>
struct composition;

template <typename...>
struct params;

template <typename...>
struct mediator;

template <typename...>
struct journal;

template <typename...>
struct evaluator;

template <typename...>
struct pipeline;

template <typename ComponentT>
struct wrapper;

namespace detail{

template <typename ComponentT, bool>
struct facet_wrapper;

}

template <typename ComponentT, typename FeatureT>
struct facet;

template <typename ComponentT>
struct config;


}
}

#endif // UDHO_MANIFOLD_FWD_H
