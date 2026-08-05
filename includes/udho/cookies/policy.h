#ifndef UDHO_COOKIES_POLICY_H
#define UDHO_COOKIES_POLICY_H

namespace udho{

namespace cookies{

/** @addtogroup DoxyG_cookies
 *  @{
 */

/**
 * @enum policy
 * @brief Defines SameSite cookie policies
 *
 * Determines how cookies are sent with cross-site requests
 */
enum class policy{
    undefined,
    none,   ///< Cookies will be sent in all contexts
    strict, ///< Cookies only sent in first-party context
    lax     ///< Cookies sent with top-level navigation
};

/** @} */

}

}

#endif // UDHO_COOKIES_POLICY_H
