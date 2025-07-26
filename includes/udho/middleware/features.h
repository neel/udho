#ifndef UDHO_MIDDLEWARE_FEATURES_H
#define UDHO_MIDDLEWARE_FEATURES_H

namespace udho {
namespace middleware {

/**
 * @enum features
 * @brief Enumerates available middleware features
 *
 * Defines the types of functionality a middleware component can provide
 */
namespace features{

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


}
}

#endif // UDHO_MIDDLEWARE_FEATURES_H
