#ifndef UDHO_MANIFOLD_FWD_H
#define UDHO_MANIFOLD_FWD_H


namespace udho {
namespace manifold {

template <typename...>
struct composition;

template <typename...>
struct params;

template <typename...>
struct delegates;

template <typename...>
struct states;

template <typename...>
struct evaluator;

template <typename...>
struct pipeline;

template <typename ComponentT>
struct wrapper;

template <typename ComponentT, bool>
struct delegate_wrapper;

template <typename ComponentT, typename FeatureT>
struct delegate;

template <typename ComponentT>
struct config;


}
}

#endif // UDHO_MANIFOLD_FWD_H
