#ifndef UDHO_MANIFOLD_FEATURES_H
#define UDHO_MANIFOLD_FEATURES_H

#include <type_traits>
#include <udho/manifold/fwd.h>

namespace udho {
namespace manifold {

/**
 * @enum features
 * @brief Enumerates available manifold features
 *
 * Defines the types of functionality a manifold component can provide
 */
namespace feature{

    /**
     * @brief generates an unique signature for the request
     */
    struct hash{};

    /**
     * @brief tracks events associated with the request (e.g. mini logging)
     */
    struct track{};

    /**
     * @brief decides whether to accept or reject this request
     */
    struct filter{};

    /**
     * @brief rate control, reject requests when exceeds server capacity
     */
    struct throttle{};

    /**
     * @brief provides global and local cache facility
     */
    struct cache{};

    /**
     * @brief provides facilities for generating, and verification of scalar tokens with TTL
     */
    struct token{};

    /**
     * @brief provide policy based session extraction policy connected with session storage and management system
     */
    struct session{};

    /**
     * @brief checks uploaded file satisfies constraints and if it does then copies
     */
    struct upload{};

}

template <typename... Features>
struct features{
    template <typename FeatureT>
    using has = std::disjunction<std::is_same<FeatureT, Features>...>;
};

}
}

#endif // UDHO_MANIFOLD_FEATURES_H
