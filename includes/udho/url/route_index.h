#ifndef UDHO_URL_ROUTE_INDEX_H
#define UDHO_URL_ROUTE_INDEX_H

#include <string>

namespace udho{
namespace url{


/**
 * @addtogroup DoxyG_url_router
 * @{
 */

namespace detail{

/**
 * @brief Identifies a resolved router target.
 *
 * A route index describes how a request target was resolved by a router. It
 * may identify:
 *
 * - no route;
 * - an action within a mount point; or
 * - a resource provided by an explorer registry.
 *
 * Action routes store both a mount-point index and an action index. Registry
 * routes store the request target but do not use the numeric indices.
 */
class route_index{
public:
    /**
     * @brief Describes the kind of route represented by a route index.
     */
    enum class type{
        none, action, registry
    };
private:
    type        _type;
    int         _mountpoint;
    int         _action;
    std::string _target;

public:
    /**
     * @brief Constructs an action route index.
     *
     * @param target Request target associated with the route.
     * @param mountpoint Index of the resolved mount point.
     * @param action Index of the resolved action within the mount point.
     *
     * @post type() returns type::action.
     * @post mountpoint() returns @p mountpoint.
     * @post action() returns @p action.
     */
    inline route_index(const std::string& target, int mountpoint, int action): _type(type::action), _target(target), _mountpoint(mountpoint), _action(action) {}
    /**
     * @brief Constructs a route index without action indices.
     *
     * This constructor is used for unresolved routes and registry routes.
     * Both the mount-point and action indices are initialized to `-1`.
     *
     * @param target Request target associated with the route.
     * @param type Route type. Defaults to type::none.
     *
     * @post mountpoint() returns `-1`.
     * @post action() returns `-1`.
     */
    inline route_index(const std::string& target, route_index::type type = route_index::type::none): _type(type), _target(target), _mountpoint(-1), _action(-1) {}

    /**
     * @brief Copy-constructs a route index.
     */
    route_index(const route_index&) = default;

    /**
     * @brief Constructs a route index by copying another index and replacing
     *        its route type.
     *
     * The target, mount-point index, and action index are copied from
     * @p other.
     *
     * @param type Route type assigned to the new index.
     * @param other Route index whose remaining state is copied.
     */
    inline route_index(route_index::type type, const route_index& other): _type(type), _mountpoint(other._mountpoint), _action(other._action), _target(other._target) {}

    /**
     * @brief Constructs a route index by moving another index and replacing
     *        its route type.
     *
     * The mount-point and action indices are copied, while the request target
     * is moved from @p other.
     *
     * @param type Route type assigned to the new index.
     * @param other Route index whose state is transferred.
     */
    inline route_index(route_index::type type, route_index&& other): _type(type), _mountpoint(other._mountpoint), _action(other._action), _target(std::move(other._target)) {}

    /**
     * @brief Returns the route type.
     *
     * @return type::none, type::action, or type::registry according to how the
     *         request target was resolved.
     */
    inline route_index::type type() const { return _type; }

    /**
     * @brief Tests whether the route index identifies a usable route.
     *
     * A registry route is valid when its target is not empty. An action route
     * is valid when both its mount-point and action indices are non-negative.
     * An index of type type::none is always invalid.
     *
     * @return `true` when the index identifies a valid action or registry
     *         route; otherwise `false`.
     */
    inline bool valid() const {
        return _type != type::none && (
            (_type == type::registry && !_target.empty())               ||
            (_type == type::action   && _mountpoint >= 0 && _action >= 0)
        );
    }

    /**
     * @brief Returns the mount-point index.
     *
     * The value is meaningful for action routes. Routes constructed without
     * action indices store `-1`.
     *
     * @return Index of the resolved mount point, or `-1` when no mount-point
     *         index is stored.
     */
    inline int mountpoint() const { return _mountpoint; }

    /**
     * @brief Returns the action index.
     *
     * The value is meaningful for action routes. Routes constructed without
     * action indices store `-1`.
     *
     * @return Index of the resolved action, or `-1` when no action index is
     *         stored.
     */
    inline int action() const { return _action; }

    /**
     * @brief Returns the request target associated with the route.
     *
     * @return A copy of the stored request target.
     */
    inline const std::string target() const { return _target; }
};


}

/// @}

}
}

#endif // UDHO_URL_ROUTE_INDEX_H
