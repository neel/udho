// SPDX-FileCopyrightText: 2024 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_ROUTER_H
#define UDHO_URL_ROUTER_H

#include <udho/hazo/seq/seq.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <udho/url/summary.h>
#include <udho/view/resources/asset/store.h>
#include <iostream>
#include <magic.h>
#include <udho/view/resources/asset/io.h>
#include <udho/url/explorers.h>
#include <udho/pages/system.h>
#include <udho/url/io_fwd.h>

namespace udho{
namespace url{

/**
 * @addtogroup DoxyG_url_router
 * @{
 */


namespace detail{

template <typename MountpointsTable>
struct routing_table;



/**
 * @class routing_table
 * @brief Internal routing table that dispatches requests across mount points.
 *
 * Stores an ordered mount-points table, locates actions by HTTP method and
 * target, invokes matched actions, and builds a summary of the configured
 * routes.
 *
 * @tparam Mountpoints Types of the mount points stored in the table.
 *
 */
template <typename... Mountpoints>
struct routing_table<udho::url::mountpoints_table<Mountpoints...>>{

    /**
     * @brief operator overload for streaming the routing table's mount points
     * @param stream Output stream
     * @param router Routing table
     * @return Reference to the output stream
     */
    template <typename... XMountpoints>
    friend std::ostream& udho::url::operator<<(std::ostream& stream, const udho::url::detail::routing_table<udho::url::mountpoints_table<XMountpoints...>>& router);

    /// Type alias for the mount points collection
    using mountpoints_table_type    = udho::url::mountpoints_table<Mountpoints...>;
    using routing_table_type        = routing_table<udho::url::mountpoints_table<Mountpoints...>>;

    routing_table() = delete;
    routing_table(const routing_table_type&) = delete;
    routing_table(routing_table_type&&) = default;

    /**
     * @brief Constructs a routing table with mount points
     * @param mountpoints Rvalue reference to mount points collection
     * @post Initializes internal summary that can be accessed through the @ref summary function
     */
    routing_table(mountpoints_table_type&& mountpoints): _mountpoints(std::move(mountpoints)) { summarize(); }

    /**
     * @brief Subscript operator for accessing mount points
     * @tparam XStrT Type of the mount point key
     * @param xstr Key to access in mount points
     * @return Reference to the associated mount point
     */
    template <typename XStrT>
    auto& operator[](XStrT&& xstr) { return _mountpoints[std::move(xstr)]; }

    /**
     * @brief Const subscript operator for accessing mount points
     * @tparam XStrT Type of the mount point key (deduced)
     * @param xstr Key to access in mount points
     * @return Const reference to the associated mount point
     */
    template <typename XStrT>
    const auto& operator[](XStrT&& xstr) const { return _mountpoints[std::move(xstr)]; }

    /**
     * @brief Locates the action matching an HTTP method and request target.
     *
     * Visits mount points in the order set during compile time, removes the matching mount path from
     * the target, and asks the mount point to locate an action for the remaining path.
     *
     * @tparam Ch Character type of the request target.
     * @param method HTTP method to match.
     * @param subject Request target to resolve.
     * @return A valid route index when a match is found; otherwise an invalid route index retaining @p subject.
     *
     */
    template <typename Ch>
    route_index index_of(boost::beast::http::verb method, const std::basic_string<Ch>& subject) const {
        int mountpoint_index = -1;
        int action_index = -1;

        _mountpoints.visit_at([method, &subject, &mountpoint_index, &action_index](const auto& mountpoint, std::size_t depth){
            if(mountpoint_index >= 0) return;
            auto path = mountpoint.path();
            if(!boost::starts_with(subject, path))
                return;
            auto rest = path == "/" ? subject : subject.substr(path.size());
            int action_idx = mountpoint.index_of(method, rest);
            if(action_idx >= 0){
                mountpoint_index = depth;
                action_index = action_idx;
            }
        });
        if(mountpoint_index < 0) {
            return route_index{subject};
        } else {
            std::size_t total_depth = _mountpoints.length();
            return route_index{subject, static_cast<int>(total_depth) - mountpoint_index, action_index};
        }
    }

    /**
     * @brief Invokes the action identified by a route index.
     *
     * @tparam Args Types of the arguments forwarded to the action.
     * @param index Valid route index containing mount-point and action indices.
     * @param args Arguments forwarded to the matched action.
     * @return `true` when the indexed mount point was found and invoked; otherwise `false`.
     * @pre @p index is valid.
     *
     */
    template <typename... Args>
    bool invoke_at(const route_index& index, Args&&... args) const {
        assert(index.valid());

        bool found = false;
        std::size_t total_depth = _mountpoints.length();
        _mountpoints.visit_at([total_depth, &index, &found, &args...](const auto& mountpoint, std::size_t depth){
            if(found)  return;
            std::size_t expected_depth = (total_depth - depth);
            found = (expected_depth == index.mountpoint());
            if(found){
                mountpoint.invoke_at(index.action(), index.target(), std::forward<Args>(args)...);
            }
        });
        return found;
    }

    /**
     * @brief Applies the indexed action's configuration to a configuration superset.
     *
     * @tparam ConfigSupersetT Configuration type capable of receiving the action-specific options.
     * @param index Route index identifying the action whose options are used.
     * @param config Configuration object to update.
     * @return boolean success
     *
     */
    template <typename ConfigSupersetT>
    bool reconfigure_for(const route_index& index, ConfigSupersetT& config) const {
        if(!index.valid()) return false;

        bool found = false;
        std::size_t total_depth = _mountpoints.length();
        _mountpoints.visit_at([total_depth, &index, &found, &config](const auto& mountpoint, std::size_t depth) mutable {
            if(found)  return;
            std::size_t expected_depth = (total_depth - depth);
            found = (expected_depth == index.mountpoint());
            if(found){
                mountpoint.reconfigure_for(index.action(), config);
            }
        });
        return found;
    }

    /**
     * @brief Tests whether a mounted action matches a method and request target.
     *
     * @tparam Ch Character type of the request target.
     * @param method HTTP method to match.
     * @param subject Request target to search for.
     * @return `true` when one of the mount points contains a matching action;
     *         otherwise `false`.
     *
     */
    template <typename Ch>
    bool find(boost::beast::http::verb method, const std::basic_string<Ch>& subject) const {
        bool found = false;
        _mountpoints.visit([method, &subject, &found](const auto& mointpoint){
            if(found)
                return;
            auto path = mointpoint.path();
            if(!boost::starts_with(subject, path))
                return;
            auto rest = path == "/" ? subject : subject.substr(path.size());
            found = mointpoint.find(method, rest);
        });
        return found;
    }

    /**
     * @brief Invokes the mounted action matching a method and request target.
     *
     * @tparam Ch Character type of the request target.
     * @tparam Args Types of the arguments forwarded to the action.
     * @param method HTTP method to match.
     * @param subject Request target to dispatch.
     * @param args Arguments forwarded to the matched action.
     * @return `true` when a matching action was invoked; otherwise `false`.
     *
     */
    template <typename Ch, typename... Args>
    bool invoke(boost::beast::http::verb method, const std::basic_string<Ch>& subject, Args&&... args) const {
        bool found = false;
        _mountpoints.visit([method, &subject, &found, &args...](const auto& mointpoint){
            if(found)
                return;
            auto path = mointpoint.path();
            if(!boost::starts_with(subject, path))
                return;
            auto rest = path == "/" ? subject : subject.substr(path.size());
            found = mointpoint.invoke(method, rest, std::forward<Args>(args)...);
        });
        return found;
    }

    /**
     * @brief Function call operator that delegates to invoke()
     * @param url URL path to process
     * @param args Arguments to forward to the action
     * @return bool indicating if request was handled
     */
    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const { return this->invoke(url, std::forward<Args>(args)...); }

    /**
     * @brief Gets the routing summary
     * @return Const reference to the summary object
     */
    const udho::url::summary::router& summary() const { return _summary; }

    /**
     * @brief Writes the routing table's mount points to an output stream.
     * @param stream Output stream that receives the serialized mount points.
     * @return Reference to @p stream.
     */
    std::ostream& print(std::ostream& stream) const {
        stream << _mountpoints;
        return stream;
    }

    private:

        /**
         * @brief Builds summary information by visiting all mount points
         * @post Populates the _summary member with mount point information
         */
        void summarize(){
            _mountpoints.visit([this](const auto& m){
                _summary.add(m);
            });
        }

    private:
        mountpoints_table_type           _mountpoints;
        udho::url::summary::router _summary;
};

/**
 * @class routing_table
 * @brief Internal routing table that dispatches requests across mount points.
 *
 * Specialization for router with a single mount point
 *
 * @tparam Mountpoints Types of the mount points stored in the table.
 *
 */
template <typename StrT, typename ActionsT>
struct routing_table<udho::url::mount_point<StrT, ActionsT>>{

    /**
     * @brief operator overload for streaming the routing table's mount points
     * @param stream Output stream
     * @param router Routing table
     * @return Reference to the output stream
     */
    template <typename Mountpoints>
    friend std::ostream& udho::url::operator<<(std::ostream& stream, const udho::url::detail::routing_table<Mountpoints>& router);

    /// Type alias for the mount points collection
    using mountpoint_type = udho::url::mount_point<StrT, ActionsT>;

    routing_table() = delete;
    routing_table(const routing_table<mountpoint_type>&) = delete;
    routing_table(routing_table<mountpoint_type>&&) = default;

    /**
     * @brief Constructs a routing table with mount points
     * @param mountpoints Rvalue reference to mount points collection
     * @post Initializes internal summary that can be accessed through the @ref summary function
     */
    routing_table(mountpoint_type&& mountpoints): _mountpoint(std::move(mountpoints)) { summarize(); }

    /**
     * @brief Subscript operator for accessing mount points
     * @tparam XStrT Type of the mount point key
     * @param xstr Key to access in mount points
     * @return Reference to the associated mount point
     */
    template <typename XStrT>
    auto& operator[](XStrT&& xstr) { return _mountpoint[std::move(xstr)]; }

    /**
     * @brief Const subscript operator for accessing mount points
     * @tparam XStrT Type of the mount point key (deduced)
     * @param xstr Key to access in mount points
     * @return Const reference to the associated mount point
     */
    template <typename XStrT>
    const auto& operator[](XStrT&& xstr) const { return _mountpoint[std::move(xstr)]; }

    /**
     * @brief Locates the action matching an HTTP method and request target.
     *
     * @param method HTTP method to match.
     * @param subject Request target to resolve.
     * @return A valid action route index when the mount point contains a matching action; otherwise an invalid route index.
     *
     */
    route_index index_of(boost::beast::http::verb method, const std::string& subject) const {
        auto path = _mountpoint.path();
        if(!boost::starts_with(subject, path))
            return route_index(subject, -1, -1);

        auto rest = path == "/" ? subject : subject.substr(path.size());
        int action_idx = _mountpoint.index_of(method, rest);
        if(action_idx >= 0){
            return route_index(subject, 0, action_idx);
        }

        return route_index(subject, -1, -1);
    }

    /**
     * @brief Invokes the action identified by a route index.
     *
     * @tparam Args Types of the arguments forwarded to the action.
     * @param index Route index identifying the action in the sole mount point.
     * @param args Arguments forwarded to the indexed action.
     * @return `true` when @p index refers to this mount point; otherwise `false`.
     * @pre The mount-point and action indices in @p index are non-negative.
     *
     */
    template <typename... Args>
    bool invoke_at(const route_index& index, Args&&... args) const {
        int mountpoint_index = index.mountpoint();
        int action_index     = index.action();

        assert(mountpoint_index > -1);
        assert(action_index > -1);

        bool found = (0 == mountpoint_index);
        if(found){
            _mountpoint.invoke_at(action_index, std::forward<Args>(args)...);
        }

        return found;
    }

    /**
     * @brief Applies the indexed action's configuration to a configuration superset.
     *
     * @tparam ConfigSupersetT Configuration type capable of receiving the action-specific options.
     * @param index Route index identifying the action whose options are used.
     * @param config Configuration object to update.
     * @return bool
     * @pre The mount-point and action indices in @p index are non-negative.
     *
     */
    template <typename ConfigSupersetT>
    bool reconfigure_for(const route_index& index, ConfigSupersetT& config) const {
        int mountpoint_index = index.mountpoint();
        int action_index     = index.action();

        assert(mountpoint_index > -1);
        assert(action_index > -1);

        bool found = (0 == mountpoint_index);
        if(found){
            _mountpoint.reconfigure_for(index.action(), config);
        }

        return found;
    }

    /**
     * @brief Tests whether the mount point contains a matching action.
     *
     * @param method HTTP method to match.
     * @param subject Request target to search for.
     * @return `true` when the mount point contains a matching action; otherwise `false`.
     *
     */
    bool find(boost::beast::http::verb method, const std::string& subject) const {
        auto path = _mountpoint.path();
        if(!boost::starts_with(subject, path))
            return false;
        auto rest = path == "/" ? subject : subject.substr(path.size());
        return _mountpoint.find(method, rest);
    }

    /**
     * @brief Invokes the action associated with a URL path
     * @tparam Ch Character type for the URL string
     * @tparam Args Types of arguments to forward
     * @param subject URL path to invoke
     * @param args Arguments to forward to the action
     * @return true if action was invoked or file was served, false otherwise
     */
    template <typename... Args>
    bool invoke(boost::beast::http::verb method, const std::string& subject, Args&&... args) const {
        auto path = _mountpoint.path();
        if(!boost::starts_with(subject, path))
            return false;
        auto rest = path == "/" ? subject : subject.substr(path.size());
        return _mountpoint.invoke(method, rest, std::forward<Args>(args)...);
    }

    /**
     * @brief Function call operator that delegates to invoke()
     * @param url URL path to process
     * @param args Arguments to forward to the action
     * @return bool indicating if request was handled
     */
    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const { return this->invoke(url, std::forward<Args>(args)...); }

    /**
     * @brief Gets the routing summary
     * @return Const reference to the summary object
     */
    const udho::url::summary::router& summary() const { return _summary; }

    /**
     * @brief Writes the configured mount point to an output stream.
     *
     * @param stream Destination stream.
     * @return Reference to @p stream.
     *
     */
    std::ostream& print(std::ostream& stream) const {
        stream << _mountpoint;
        return stream;
    }


private:

    /**
     * @brief Builds summary information by visiting all mount points
     * @post Populates the _summary member with mount point information
     */
    void summarize(){
        _summary.add(_mountpoint);
    }

private:
    mountpoint_type           _mountpoint;
    udho::url::summary::router _summary;
};

template <typename RoutingTableT = void>
struct basic_router;

/**
 * @brief Internal router implementation combining mounted actions and explorers.
 *
 * This specialization dispatches requests to actions in a mount-points table.
 * When no mounted action matches, an optional explorer registry can provide
 * files, directories, compiled assets, or other resources.
 *
 * @tparam Mountpoints Types of the mount points stored in the routing table.
 *
 * @internal
 */
template <typename... Mountpoints>
struct basic_router<detail::routing_table<udho::url::mountpoints_table<Mountpoints...>>>: private detail::routing_table<udho::url::mountpoints_table<Mountpoints...>>{
    /**
     * @brief Type of the mount-points table managed by this router.
     */
    using mountpoints_type = udho::url::mountpoints_table<Mountpoints...>;
    /**
     * @brief Underlying action-routing table type.
     */
    using routing_table    = detail::routing_table<mountpoints_type>;

    /**
     * @brief Exposes lookup by compile-time mount-point key.
     * @see routing_table::operator[]
     */
    using routing_table::operator[];
    /**
     * @brief Exposes the routing summary generated from the mount points.
     * @see routing_table::summary
     */
    using routing_table::summary;
    /**
     * @brief Exposes action-route lookup from the underlying routing table.
     * @see routing_table::index_of
     */
    using routing_table::index_of;
    /**
     * @brief Exposes invocation of a mounted action by route index.
     * @see routing_table::invoke_at
     */
    using routing_table::invoke_at;
    /**
     * @brief Exposes action-specific configuration application.
     * @see routing_table::reconfigure_for
     */
    using routing_table::reconfigure_for;

    basic_router() = delete;
    basic_router(const basic_router<routing_table>&) = delete;
    basic_router(basic_router<routing_table>&&) = default;

    /**
     * @brief Constructs a router from a mount-points table.
     *
     * The resulting router dispatches mounted actions and uses an empty
     * explorer registry.
     *
     * @param mountpoints Mount-points table moved into the routing table.
     */
    basic_router(mountpoints_type&& mountpoints): routing_table(std::move(mountpoints)) {}

    /**
     * @brief Constructs a router from mount points and an explorer registry.
     *
     * Mounted actions are checked before the explorer registry.
     *
     * @param mountpoints Mount-points table moved into the routing table.
     * @param registry Explorer registry moved into the router.
     */
    basic_router(mountpoints_type&& mountpoints, udho::url::explorers::registry&& registry): routing_table(std::move(mountpoints)), _registry(std::move(registry)) {}

    /**
     * @brief Resolves a request to a mounted action or explorer resource.
     *
     * The underlying routing table is searched first. When no mounted action
     * matches and @p method is GET, the explorer registry is checked for a file
     * or directory matching @p subject.
     *
     * @tparam Ch Character type of the request target.
     *
     * @param method HTTP method to resolve.
     * @param subject Request target to resolve.
     */
    template <typename Ch>
    route_index index_of(boost::beast::http::verb method, const std::basic_string<Ch>& subject) const {
        route_index index = routing_table::index_of(method, subject);
        if(!index.valid() && method == boost::beast::http::verb::get) {
            bool is_file = _registry.exists(subject);
            bool is_dir  = _registry.is_subset(subject);

            if(is_file || is_dir) {
                return route_index(route_index::type::registry, index);
            }
        }
        return index;
    }

    /**
     * @brief Invokes the handler identified by a route index.
     *
     * Action indices are delegated to the underlying routing table. Registry
     * indices are delegated to invoke_registry().
     *
     * @tparam Args Types of the arguments forwarded to the selected handler.
     *
     * @param index Valid action or registry route index.
     * @param args Arguments forwarded to the selected handler.
     *
     * @return `true` when the indexed action or registry resource was handled;
     *         otherwise `false`.
     *
     * @pre @p index is valid.
     */
    template <typename... Args>
    bool invoke_at(const route_index& index, Args&&... args) const {
        assert(index.valid());

        if(index.type() == route_index::type::action) {
            return routing_table::invoke_at(index, std::forward<Args>(args)...);
        } else if(index.type() == route_index::type::registry) {
            return invoke_registry(index, std::forward<Args>(args)...);
        }
        return false;
    }

    /**
     * @brief Tests whether a request matches an action or registry resource.
     *
     * The routing table is searched first. The registry is checked only when
     * no action matches and the HTTP method is GET.
     *
     * @tparam Ch Character type of the request target.
     *
     * @param method HTTP method to search for.
     * @param subject Request target to search for.
     *
     * @return `true` when a mounted action or registry file matches;
     *         otherwise `false`.
     */
    template <typename Ch>
    bool find(boost::beast::http::verb method, const std::basic_string<Ch>& subject) const {
        bool found = routing_table::find(method, subject);
        if(!found && method == boost::beast::http::verb::get){
            return _registry.exists(subject);
        }
        return found;
    }

    /**
     * @brief Invokes a mounted action or serves an explorer resource.
     *
     * Mounted action dispatch is attempted first. If the action routing table
     * does not handle the request and exactly one invocation argument is
     * supplied, the explorer registry is asked to serve @p subject.
     *
     * @tparam Ch Character type of the request target.
     * @tparam Args Types of the forwarded invocation arguments.
     *
     * @param subject Request target to dispatch.
     * @param args Arguments forwarded to the action or explorer registry.
     *
     * @return `true` when an action, file, or directory was handled;
     *         otherwise `false`.
     */
    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        bool invoked = routing_table::invoke(subject, std::forward<Args>(args)...);
        if(!invoked){
            if constexpr (sizeof...(args) == 1){
                udho::url::explorers::registry::status status = _registry.serve(subject, std::forward<Args>(args)...);
                invoked = (status == udho::url::explorers::registry::status::file || status == udho::url::explorers::registry::status::directory);
            }
        }
        return invoked;
    }

    /**
     * @brief Serves an explorer resource identified by a route index.
     *
     * @tparam ContextT Context type accepted by the explorer registry.
     * @tparam Args Types of additional ignored arguments.
     *
     * @param index Registry route index whose target is served.
     * @param context Context passed to the matching explorer.
     *
     * @return `true` when the registry served a file or directory;
     *         otherwise `false`.
     */
    template <typename ContextT, typename... Args>
    bool invoke_registry(const route_index& index, ContextT& context, Args&&...) const {
        udho::url::explorers::registry::status status = _registry.serve(index.target(), context);
        return (status == udho::url::explorers::registry::status::file || status == udho::url::explorers::registry::status::directory);
    }

    /**
     * @brief Dispatches a URL through invoke().
     *
     * @tparam Args Types of the arguments forwarded to invoke().
     *
     * @param url Request target to dispatch.
     * @param args Arguments forwarded to invoke().
     *
     * @return `true` when the request was handled; otherwise `false`.
     */
    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const { return this->invoke(url, std::forward<Args>(args)...); }

    /**
     * @brief Returns the underlying mounted-action routing table.
     *
     * @return Constant reference to the routing-table base subobject.
     */
    const detail::routing_table<mountpoints_type>& table() const { return *this; }

private:
    udho::url::explorers::registry _registry;
};

/**
 * @brief Internal explorer-only router implementation.
 *
 * This specialization contains no mount points and resolves requests solely
 * through an explorer registry.
 *
 *
 * @internal
 */
template <>
struct basic_router<void>{
    /**
     * @brief Indicates that no action-routing table is present.
     */
    using routing_table    = void;
    /**
     * @brief Indicates that no mount-points table is present.
     */
    using mountpoints_type = void;

    basic_router() = delete;
    basic_router(const basic_router<routing_table>&) = delete;
    basic_router(basic_router<routing_table>&&) = default;

    /**
     * @brief Constructs an explorer-only router.
     * @param registry Explorer registry moved into the router.
     */
    basic_router(udho::url::explorers::registry&& registry): _registry(std::move(registry)) {}

    /**
     * @brief Invokes a registry resource identified by a route index.
     *
     * @tparam Args Types of the arguments forwarded to the registry handler.
     *
     * @param index Route index to invoke.
     * @param args Arguments forwarded to invoke_registry().
     *
     * @return `true` when @p index identifies a registry resource that was served; otherwise `false`.
     */
    template <typename... Args>
    bool invoke_at(const route_index& index, Args&&... args) const {
        if(index.type() == route_index::type::registry) {
            return invoke_registry(index, std::forward<Args>(args)...);
        }
        return false;
    }

    /**
     * @brief Reports that explorer resources have no action configuration.
     *
     * @tparam SupersetT Configuration superset type.
     *
     * @param index Unused route index.
     * @param superset Unused configuration object.
     *
     * @return Always `false`.
     */
    template <typename SupersetT>
    bool reconfigure_for(const route_index& index, SupersetT& superset) const { return false; }

    /**
     * @brief Resolves a GET request to a registry file or directory.
     *
     * Non-GET methods are not resolved by an explorer-only router.
     *
     * @param method HTTP method to resolve.
     * @param subject Request target to resolve.
     *
     * @return A registry route index when a file or directory exists;
     *         otherwise an index of type route_index::type::none.
     */
    route_index index_of(boost::beast::http::verb method, const std::string& subject) const {
        if(method == boost::beast::http::verb::get) {
            bool is_file = _registry.exists(subject);
            bool is_dir  = _registry.is_subset(subject);

            if(is_file || is_dir) {
                return route_index(subject, route_index::type::registry);
            }
        }
        return route_index{subject, route_index::type::none};
    }

    /**
     * @brief Tests whether the registry contains a target.
     *
     * @param subject Request target to search for.
     *
     * @return `true` when the registry contains the target; otherwise `false`.
     */
    bool find(const std::string& subject) const { return _registry.exists(subject); }

    /**
     * @brief Serves a target through the explorer registry.
     *
     * Registry serving is attempted only when exactly one invocation argument
     * is supplied.
     *
     * @tparam Ch Character type of the request target.
     * @tparam Args Types of the forwarded registry arguments.
     *
     * @param subject Request target to serve.
     * @param args Arguments forwarded to the explorer registry.
     *
     * @return `true` when a file or directory was served; otherwise `false`.
     */
    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        if constexpr (sizeof...(args) == 1){
            udho::url::explorers::registry::status status = _registry.serve(subject, std::forward<Args>(args)...);
            return (status == udho::url::explorers::registry::status::file || status == udho::url::explorers::registry::status::directory);
        }
        return false;
    }

    /**
     * @brief Serves a registry resource identified by a route index.
     *
     * @tparam ContextT Context type accepted by the explorer registry.
     * @tparam Args Types of additional ignored arguments.
     *
     * @param index Registry route index whose target is served.
     * @param context Context passed to the matching explorer.
     *
     * @return `true` when the registry served a file or directory;
     *         otherwise `false`.
     */
    template <typename ContextT, typename... Args>
    bool invoke_registry(const route_index& index, ContextT& context, Args&&...) const {
        udho::url::explorers::registry::status status = _registry.serve(index.target(), context);
        return (status == udho::url::explorers::registry::status::file || status == udho::url::explorers::registry::status::directory);
    }

    /**
     * @brief Dispatches a URL through invoke().
     *
     * @tparam Args Types of the arguments forwarded to invoke().
     *
     * @param url Request target to dispatch.
     * @param args Arguments forwarded to invoke().
     *
     * @return `true` when the registry handled the request; otherwise `false`.
     */
    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const { return this->invoke(url, std::forward<Args>(args)...); }

    /**
     * @brief Returns the router summary.
     *
     * Explorer-only routers currently expose a default-constructed summary.
     *
     * @return Constant reference to the router summary.
     */
    const udho::url::summary::router& summary() const { return _summary; }

private:
    udho::url::explorers::registry _registry;
    udho::url::summary::router     _summary;
};

}


/**
 * @brief basic_router forward declaration
 *
 * `basic_router` provides the interface for managing and serving resources through url router and through explorer registry
 *
 * | Specialization | Purpose |
 * | --- | --- |
 * | `basic_router<mountpoints_table<Mountpoints...>>` | Dispatches requests to mounted actions and optionally serves resources through an explorer registry. |
 * | `basic_router<void>` | Serves resources exclusively through an explorer registry and contains no mounted actions. |
 *
 * @tparam MountPointsT Mount-points table type, or `void` for an explorer-only router.
 *
 *
 */

template <typename MountPointsT = void>
struct basic_router;

/**
 * @brief Router containing an ordered collection of mount points.
 *
 * This specialization dispatches requests to actions stored in a
 * mountpoints_table. It may also contain an explorer registry that serves
 * files, directories, compiled assets, or other resources when no mounted
 * action handles a request.
 *
 * Mounted actions take precedence over explorer resources.
 *
 * The router is move-only and must be constructed with a mount-points table.
 *
 * @tparam Mountpoints Types of the mount points stored in the router.
 */
template <typename... Mountpoints>
struct basic_router<udho::url::mountpoints_table<Mountpoints...>>: private detail::basic_router<detail::routing_table<udho::url::mountpoints_table<Mountpoints...>>>{
    using detail_basic_router   = detail::basic_router<detail::routing_table<udho::url::mountpoints_table<Mountpoints...>>>;

    /**
     * @brief Underlying mounted-action routing-table type.
     */
    using routing_table         = typename detail_basic_router::routing_table;
    /**
     * @brief Type of the mount-points table stored by this router.
     */
    using mountpoints_type      = typename detail_basic_router::mountpoints_type;

    /**
     * @brief Retrieves a mount point by its compile-time key.
     *
     * Returns reference to the matching mount point.
     *
     * @tparam XStrT Compile-time key type.
     * @param xstr Compile-time key identifying the mount point.
     * @return Reference to the mount point associated with @p xstr.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::operator[]
     */
    using detail_basic_router::operator[];

    /**
     * @brief Returns a summary of the configured mounted routes.
     *
     * The summary is built from the router's mount points and describes the
     * routes and actions available through the routing table.
     *
     * @return Constant reference to the router summary.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::summary
     */
    using detail_basic_router::summary;

    /**
     * @brief Tests whether a request can be handled by the router.
     *
     * The mounted routing table is searched first. When no mounted action
     * matches and the request method is GET, the explorer registry is checked
     * for a matching resource.
     *
     * @tparam Ch Character type of the request target.
     * @param method HTTP method to match.
     * @param subject Request target to search for.
     * @return `true` when a mounted action or explorer resource can handle the
     *         request; otherwise `false`.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::find
     */
    using detail_basic_router::find;

    /**
     * @brief Resolves a request to a route index.
     *
     * Mounted actions are searched first. If no mounted action matches a GET
     * request, the explorer registry is checked for a file or directory.
     *
     * @tparam Ch Character type of the request target.
     * @param method HTTP method to resolve.
     * @param subject Request target to resolve.
     * @return An action route index when a mounted action matches, a registry
     *         route index when an explorer resource exists, or an invalid
     *         route index when no handler is available.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::index_of
     */
    using detail_basic_router::index_of;

    /**
     * @brief Invokes the handler identified by a route index.
     *
     * Action indices are dispatched through the mounted routing table.
     * Registry indices are served through the explorer registry.
     *
     * @tparam Args Types of the arguments forwarded to the selected handler.
     * @param index Valid action or registry route index.
     * @param args Arguments forwarded to the action or explorer.
     * @return `true` when the indexed handler processes the request;
     *         otherwise `false`.
     *
     * @pre @p index is valid.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::invoke_at
     */
    using detail_basic_router::invoke_at;

    /**
     * @brief Serves an explorer resource identified by a route index.
     *
     * The target stored in @p index is passed to the explorer registry together
     * with the supplied context. The operation succeeds when an explorer serves
     * either a file or a directory.
     *
     * @tparam ContextT Context type accepted by the explorer registry.
     * @tparam Args Types of additional ignored arguments.
     * @param index Registry route index whose target is served.
     * @param context Context passed to the matching explorer.
     * @param args Additional arguments accepted for interface compatibility.
     * @return `true` when a file or directory is served; otherwise `false`.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::invoke_registry
     */
    using detail_basic_router::invoke_registry;

    /**
     * @brief Applies an action's configuration to a configuration superset.
     *
     * The mount point and action identified by @p index are located, and the
     * selected action's configuration options are applied to @p config.
     *
     * @tparam ConfigSupersetT Configuration type capable of receiving the
     *         action-specific options.
     * @param index Route index identifying the configured action.
     * @param config Configuration object to update.
     * @return A nonzero value when the indexed mount point is found and its
     *         action configuration is applied; otherwise zero.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::reconfigure_for
     */
    using detail_basic_router::reconfigure_for;

    /**
     * @brief Dispatches a request target to a mounted action or explorer.
     *
     * Mounted action dispatch is attempted first. If no action handles the
     * target, the explorer registry may serve the target when the invocation
     * arguments are compatible with registry serving.
     *
     * @tparam Ch Character type of the request target.
     * @tparam Args Types of the arguments forwarded to the selected handler.
     * @param subject Request target to dispatch.
     * @param args Arguments forwarded to the action or explorer.
     * @return `true` when an action, file, or directory handles the request;
     *         otherwise `false`.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::invoke
     */
    using detail_basic_router::invoke;

    /**
     * @brief Returns the underlying mounted-action routing table.
     *
     * This provides read-only access to the routing table used to store,
     * summarize, locate, and invoke mounted actions.
     *
     * @return Constant reference to the underlying routing table.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::table
     */
    using detail_basic_router::table;

    /**
     * @brief Dispatches a request using function-call syntax.
     *
     * Calling a router object is equivalent to calling invoke() with the same
     * URL and arguments.
     *
     * @tparam Args Types of the arguments forwarded to the selected handler.
     * @param url Request target to dispatch.
     * @param args Arguments forwarded to invoke().
     * @return `true` when the request is handled; otherwise `false`.
     *
     * @see detail::basic_router<detail::routing_table<mountpoints_type>>::operator()
     */
    using detail_basic_router::operator();


    basic_router() = delete;
    basic_router(const basic_router<udho::url::mountpoints_table<Mountpoints...>>&) = delete;
    basic_router(basic_router<udho::url::mountpoints_table<Mountpoints...>>&&) = default;

    /**
     * @brief Constructs a router from a mount-points table.
     *
     * The resulting router dispatches mounted actions and contains the default
     * explorer registry.
     *
     * @param mountpoints Mount-points table moved into the router.
     */
    basic_router(mountpoints_type&& mountpoints): detail_basic_router(std::forward<mountpoints_type>(mountpoints)) {}

    /**
     * @brief Constructs a router from mount points and an explorer registry.
     *
     * Mounted actions are checked before resources from the supplied registry.
     *
     * @param mountpoints Mount-points table moved into the router.
     * @param registry Explorer registry moved into the router.
     */
    basic_router(mountpoints_type&& mountpoints, udho::url::explorers::registry&& registry): detail_basic_router(std::forward<mountpoints_type>(mountpoints), std::forward<udho::url::explorers::registry>(registry)) {}

    /**
     * @brief Writes the router's mounted routing table to an output stream.
     *
     * @tparam XMountpointsTable Mount-points table type stored by the router.
     * @param stream Destination output stream.
     * @param router Router whose routing table is written.
     * @return Reference to @p stream.
     */
    template <typename XMountpointsTable>
    friend std::ostream& operator<<(std::ostream& stream, const basic_router<XMountpointsTable>& router);
};

/**
 * @brief Explorer-only router without mounted actions.
 *
 * This specialization resolves and serves resources entirely through an
 * explorer registry. It can serve files, directories, compiled assets, and
 * other resource types supported by registered explorers.
 *
 * Only GET requests can be resolved to registry route indices.
 *
 * The router is move-only and must be constructed with an explorer registry.
 */
template <>
struct basic_router<void>: private detail::basic_router<void>{
    using detail_basic_router   = detail::basic_router<void>;
    using routing_table         = typename detail_basic_router::routing_table;
    using mountpoints_type      = typename detail_basic_router::mountpoints_type;

    /**
     * @brief Returns the router summary.
     *
     * Explorer-only routers contain no mounted actions, so the returned summary
     * contains no mount-point route entries.
     *
     * @return Constant reference to the router summary.
     *
     * @see detail::basic_router<void>::summary
     */
    using detail_basic_router::summary;

    /**
     * @brief Tests whether the explorer registry contains a target.
     *
     * @param subject Request target to search for.
     * @return `true` when a registered explorer contains the target;
     *         otherwise `false`.
     *
     * @see detail::basic_router<void>::find
     */
    using detail_basic_router::find;

    /**
     * @brief Resolves a GET request to a registry route index.
     *
     * The explorer registry is checked for a file or directory matching the
     * supplied request target. Non-GET requests are not resolved.
     *
     * @param method HTTP method to resolve.
     * @param subject Request target to resolve.
     * @return A registry route index when the target exists, or an index of
     *         type route_index::type::none when it does not.
     *
     * @see detail::basic_router<void>::index_of
     */
    using detail_basic_router::index_of;

    /**
     * @brief Invokes a registry resource identified by a route index.
     *
     * The operation delegates registry indices to invoke_registry(). Other
     * route-index types are not handled.
     *
     * @tparam Args Types of the arguments forwarded to the explorer.
     * @param index Route index to invoke.
     * @param args Arguments forwarded to the registry handler.
     * @return `true` when @p index identifies a resource that is served;
     *         otherwise `false`.
     *
     * @see detail::basic_router<void>::invoke_at
     */
    using detail_basic_router::invoke_at;

    /**
     * @brief Serves a registry resource identified by a route index.
     *
     * The target stored in @p index is passed to the explorer registry together
     * with the supplied context.
     *
     * @tparam ContextT Context type accepted by the explorer registry.
     * @tparam Args Types of additional ignored arguments.
     * @param index Registry route index whose target is served.
     * @param context Context passed to the matching explorer.
     * @param args Additional arguments accepted for interface compatibility.
     * @return `true` when a file or directory is served; otherwise `false`.
     *
     * @see detail::basic_router<void>::invoke_registry
     */
    using detail_basic_router::invoke_registry;

    /**
     * @brief Provides the router reconfiguration interface.
     *
     * Explorer-only routers contain no actions and therefore have no
     * action-specific configuration to apply.
     *
     * @tparam SupersetT Configuration superset type.
     * @param index Route index supplied by the caller.
     * @param superset Configuration object supplied by the caller.
     * @return Always `false`.
     *
     * @see detail::basic_router<void>::reconfigure_for
     */
    using detail_basic_router::reconfigure_for;

    /**
     * @brief Serves a request target through the explorer registry.
     *
     * Registry serving is attempted when the invocation contains exactly one
     * argument compatible with the registered explorers.
     *
     * @tparam Ch Character type of the request target.
     * @tparam Args Types of the arguments forwarded to the explorer registry.
     * @param subject Request target to serve.
     * @param args Arguments forwarded to the matching explorer.
     * @return `true` when a file or directory is served; otherwise `false`.
     *
     * @see detail::basic_router<void>::invoke
     */
    using detail_basic_router::invoke;

    /**
     * @brief Dispatches a request using function-call syntax.
     *
     * Calling the router object is equivalent to calling invoke() with the same
     * URL and arguments.
     *
     * @tparam Args Types of the arguments forwarded to the explorer registry.
     * @param url Request target to serve.
     * @param args Arguments forwarded to invoke().
     * @return `true` when the registry handles the request; otherwise `false`.
     *
     * @see detail::basic_router<void>::operator()
     */
    using detail_basic_router::operator();

    basic_router() = delete;
    basic_router(const basic_router<void>&) = delete;
    basic_router(basic_router<void>&&) = default;

    /**
     * @brief Constructs an explorer-only router.
     *
     * @param registry Explorer registry moved into the router.
     */
    basic_router(udho::url::explorers::registry&& registry): detail_basic_router(std::forward<udho::url::explorers::registry>(registry)) {}

    /**
     * @brief Writes an explorer-only router to an output stream.
     *
     * @param stream Destination output stream.
     * @param router Explorer-only router to write.
     * @return Reference to @p stream.
     */
    friend std::ostream& operator<<(std::ostream& stream, const basic_router<void>& router);
};



/**
 * @name Router Factory Functions
 * @brief Convenience functions for creating router configurations
 * @relates basic_router
 *
 *
 * These functions automatically select the appropriate router specialization
 * based on input parameters.
 */


/**
 * @brief Create basic router from a set of mountpoints without asset store
 * @tparam MountPointsT Deduced mount points type
 * @param mountpoints Routing configuration
 * @return Router without asset support
 *
 * @par Example:
 * @code
 * void f0(udho::net::stream context){
 *   context << "Hello f0";
 *   context.finish();
 * }
 *
 * int f1(udho::net::stream context, int a, const std::string& b, const double& c){
 *   context << "Hello f1 ";
 *   context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
 *   context.finish();
 *   return a+b.size()+c;
 * }
 *
 * void chunk3(udho::net::stream context){
 *   context << "Chunk 3 (Final)";
 *   context.finish();
 * }
 *
 * void chunk2(udho::net::stream context){
 *   context << "chunk 2";
 *   context.flush(std::bind(&chunk3, context));
 * }
 *
 * void chunk(udho::net::stream context){
 *   context.encoding(udho::net::types::transfer::encoding::chunked);
 *   context << "Chunk 1";
 *   context.flush(std::bind(&chunk2, context));
 * }
 *
 * struct X{
 *     void f0(udho::net::context<udho::view::data::bridges::lua> context){
 *         ..
 *         context << "Hello X::f0";
 *         context << context.route("f0").name();
 *         context.finish();
 *         std::cout << context.route("f0").name() << std::endl;
 *     }
 *
 *     int f1(udho::net::stream context, int a, const std::string& b, const double& c){
 *         context << "Hello X::f1 ";
 *         context << udho::url::format("a: {}, b: {}, c: {}", a, b, c);
 *         context.finish();
 *         return a+b.size()+c;
 *     }
 * };
 *
 * auto router = udho::url::router(
 *      udho::url::root(
 *           udho::url::slot("f0"_h,  &f0)         << udho::url::home  (udho::url::verb::get)
 *         | udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed (udho::url::verb::get, "/x/f0", "/x/f0")
 *         | udho::url::slot("chunked"_h,  &chunk) << udho::url::fixed (udho::url::verb::get, "/chunk")
 *     )
 *   | udho::url::mount("b"_h, "/b",
 *         udho::url::slot("f1"_h,  &f1)         << udho::url::regx  (udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)", "/f1/{}/{}/{}")
 *       | udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx  (udho::url::verb::get, "/x/f1/(\\d+)/(\\w+)/(\\d+\\.\\d)", "/x/f1/{}/{}/{}")
 *   )
 * );
 * @endcode
 */
template <typename... Mountpoints>
basic_router<udho::url::mountpoints_table<Mountpoints...>> router(udho::url::mountpoints_table<Mountpoints...>&& mountpoints, udho::url::explorers::registry&& registry){
    return basic_router<udho::url::mountpoints_table<Mountpoints...>>{std::forward<udho::url::mountpoints_table<Mountpoints...>>(mountpoints), std::forward<udho::url::explorers::registry>(registry)};
}

/**
 * @brief Creates a router from a mount-points table.
 *
 * The returned router dispatches requests to the actions contained in the
 * supplied mount points.
 *
 * @tparam Mountpoints Types of the mount points stored in the table.
 *
 * @param mountpoints Mount-points table moved into the router.
 *
 * @return A mounted-action router containing @p mountpoints.
 *
 * @see basic_router<udho::url::mountpoints_table<Mountpoints...>>
 */
template <typename... Mountpoints>
basic_router<udho::url::mountpoints_table<Mountpoints...>> router(udho::url::mountpoints_table<Mountpoints...>&& mountpoints){
    return basic_router<udho::url::mountpoints_table<Mountpoints...>>{std::forward<udho::url::mountpoints_table<Mountpoints...>>(mountpoints)};
}

/**
 * @brief Creates an explorer-only router from an explorer registry.
 *
 * The returned router contains no mounted actions. Requests are resolved and
 * served exclusively by the explorers stored in @p registry.
 *
 * @param registry Explorer registry moved into the router.
 *
 * @return An explorer-only router containing @p registry.
 *
 * @see basic_router<void>
 */
inline basic_router<void> router(udho::url::explorers::registry&& registry){
    return basic_router<void>{std::forward<udho::url::explorers::registry>(registry)};
}

/**
 * @brief Creates an explorer-only router from one or more explorer objects.
 *
 * The explorer objects are moved into a newly constructed explorer registry,
 * which is then moved into the returned router.
 *
 * This overload participates in overload resolution only when every supplied
 * type derives from explorers::abstract_explorer after reference and
 * cv-qualifiers are removed.
 *
 * @tparam ExplorerT Types of the explorer objects.
 *
 * @param explorers Explorer objects moved into the router's registry.
 *
 * @return An explorer-only router containing the supplied explorers.
 *
 * @see basic_router<void>
 * @see udho::url::explorers::abstract_explorer
 * @see udho::url::explorers::registry
 */
template <typename... ExplorerT, std::enable_if_t< std::conjunction_v< std::is_base_of<udho::url::explorers::abstract_explorer,  std::decay_t<ExplorerT>>...>,  void >* = nullptr >
inline basic_router<void> router(ExplorerT&&... explorers){
    return basic_router<void>{udho::url::explorers::registry{std::move(explorers)...}};
}

/**
 * @brief Creates a mounted-action router with compiled asset resources.
 *
 * The returned router dispatches mounted actions and contains an asset
 * explorer named `"assets"` backed by @p assets.
 *
 * @tparam Mountpoints Types of the mount points stored in the table.
 *
 * @param mountpoints Mount-points table moved into the router.
 * @param assets Compiled asset store referenced by the asset explorer.
 *
 * @return A router containing the supplied mount points and an asset explorer.
 *
 * @note The asset store is referenced by the router's explorer. It must remain
 *       valid for as long as the router may serve its resources.
 *
 * @see basic_router<udho::url::mountpoints_table<Mountpoints...>>
 * @see udho::url::explorers::assets
 */
template <typename... Mountpoints>
basic_router<udho::url::mountpoints_table<Mountpoints...>> router(udho::url::mountpoints_table<Mountpoints...>&& mountpoints, const udho::view::resources::asset::const_store& assets){
    using router_type = basic_router<udho::url::mountpoints_table<Mountpoints...>>;

    return  router_type{
                std::forward<udho::url::mountpoints_table<Mountpoints...>>(mountpoints),
                udho::url::explorers::registry{
                    udho::url::explorers::assets{"assets", assets}
                }
            };
}

/**
 * @brief Creates a mounted-action router with asset and filesystem resources.
 *
 * The returned router dispatches mounted actions and contains:
 *
 * - a file explorer named `"docroot"` rooted at @p docroot; and
 * - an asset explorer named `"assets"` backed by @p assets.
 *
 * @tparam Mountpoints Types of the mount points stored in the table.
 *
 * @param mountpoints Mount-points table moved into the router.
 * @param assets Compiled asset store referenced by the asset explorer.
 * @param docroot Filesystem directory exposed by the file explorer.
 *
 * @return A router containing the supplied mount points, file explorer, and
 *         asset explorer.
 *
 * @note The asset store is referenced by the router's explorer. It must remain
 *       valid for as long as the router may serve its resources.
 *
 * @see basic_router<udho::url::mountpoints_table<Mountpoints...>>
 * @see udho::url::explorers::files
 * @see udho::url::explorers::assets
 */
template <typename... Mountpoints>
basic_router<udho::url::mountpoints_table<Mountpoints...>> router(udho::url::mountpoints_table<Mountpoints...>&& mountpoints, const udho::view::resources::asset::const_store& assets, const std::filesystem::path& docroot){
    using router_type = basic_router<udho::url::mountpoints_table<Mountpoints...>>;

    return  router_type{
        std::forward<udho::url::mountpoints_table<Mountpoints...>>(mountpoints),
        udho::url::explorers::registry{
            udho::url::explorers::files{"docroot", docroot},
            udho::url::explorers::assets{"assets", assets}
        }
    };
}

/**
 * @brief Creates an explorer-only router for compiled asset resources.
 *
 * The returned router contains an asset explorer named `"assets"` backed by
 * @p assets and contains no mounted actions.
 *
 * @param assets Compiled asset store referenced by the asset explorer.
 *
 * @return An explorer-only router serving resources from @p assets.
 *
 * @note The asset store is referenced by the router's explorer. It must remain
 *       valid for as long as the router may serve its resources.
 *
 * @see basic_router<void>
 * @see udho::url::explorers::assets
 */
inline basic_router<void> router(const udho::view::resources::asset::const_store& assets){
    using router_type = basic_router<void>;

    return  router_type{
                udho::url::explorers::registry{
                    udho::url::explorers::assets{"assets", assets}
                }
            };
}

/**
 * @brief Creates an explorer-only router for asset and filesystem resources.
 *
 * The returned router contains:
 *
 * - a file explorer named `"docroot"` rooted at @p docroot; and
 * - an asset explorer named `"assets"` backed by @p assets.
 *
 * The router contains no mounted actions.
 *
 * @param assets Compiled asset store referenced by the asset explorer.
 * @param docroot Filesystem directory exposed by the file explorer.
 *
 * @return An explorer-only router containing file and asset explorers.
 *
 * @note The asset store is referenced by the router's explorer. It must remain
 *       valid for as long as the router may serve its resources.
 *
 * @see basic_router<void>
 * @see udho::url::explorers::files
 * @see udho::url::explorers::assets
 */
inline basic_router<void> router(const udho::view::resources::asset::const_store& assets, const std::filesystem::path& docroot){
    using router_type = basic_router<void>;

    return  router_type{
        udho::url::explorers::registry{
            udho::url::explorers::files{"docroot", docroot},
            udho::url::explorers::assets{"assets", assets}
        }
    };
}

/**
 * @brief Creates a router from a single mount point.
 *
 * The mount point is moved into a one-element mountpoints_table before the
 * mounted-action router is constructed.
 *
 * @tparam StrT Compile-time name type of the mount point.
 * @tparam Actions Action collection type stored by the mount point.
 *
 * @param mountpoint Mount point moved into the router.
 *
 * @return A mounted-action router containing @p mountpoint.
 *
 * @see basic_router<udho::url::mountpoints_table<udho::url::mount_point<StrT, Actions>>>
 * @see udho::url::mountpoints_table
 */
template <typename StrT, typename Actions>
basic_router<udho::url::mountpoints_table<udho::url::mount_point<StrT, Actions>>> router(udho::url::mount_point<StrT, Actions>&& mountpoint) {
    return router(udho::url::mountpoints_table(std::move(mountpoint)));
}

/**
 * @brief Creates a router from one mount point and an explorer registry.
 *
 * The mount point is moved into a one-element mountpoints_table. The supplied
 * explorer registry provides fallback resource handling when no mounted action
 * resolves a GET request.
 *
 * @tparam StrT Compile-time name type of the mount point.
 * @tparam Actions Action collection type stored by the mount point.
 *
 * @param mountpoint Mount point moved into the router.
 * @param registry Explorer registry moved into the router.
 *
 * @return A mounted-action router containing @p mountpoint and @p registry.
 *
 * @see basic_router<udho::url::mountpoints_table<udho::url::mount_point<StrT, Actions>>>
 * @see udho::url::explorers::registry
 */
template <typename StrT, typename Actions>
basic_router<udho::url::mountpoints_table<udho::url::mount_point<StrT, Actions>>> router(udho::url::mount_point<StrT, Actions>&& mountpoint, udho::url::explorers::registry&& registry) {
    return router(udho::url::mountpoints_table(std::move(mountpoint)), std::forward<udho::url::explorers::registry>(registry));
}

/**
 * @brief Creates a router from one mount point and compiled asset resources.
 *
 * The mount point is moved into a one-element mountpoints_table. The returned
 * router also contains an asset explorer named `"assets"` backed by @p assets.
 *
 * @tparam StrT Compile-time name type of the mount point.
 * @tparam Actions Action collection type stored by the mount point.
 *
 * @param mountpoint Mount point moved into the router.
 * @param assets Compiled asset store referenced by the asset explorer.
 *
 * @return A mounted-action router containing @p mountpoint and an asset
 *         explorer.
 *
 * @note The asset store is referenced by the router's explorer. It must remain
 *       valid for as long as the router may serve its resources.
 *
 * @see basic_router<udho::url::mountpoints_table<
 *          udho::url::mount_point<StrT, Actions>>>
 * @see udho::url::explorers::assets
 */
template <typename StrT, typename Actions>
basic_router<udho::url::mountpoints_table<udho::url::mount_point<StrT, Actions>>> router(udho::url::mount_point<StrT, Actions>&& mountpoint, const udho::view::resources::asset::const_store& assets) {
    return router(udho::url::mountpoints_table(std::move(mountpoint)), assets);
}

/**
 * @brief Creates a router from one mount point, assets, and a document root.
 *
 * The mount point is moved into a one-element mountpoints_table. The returned
 * router also contains:
 *
 * - a file explorer named `"docroot"` rooted at @p docroot; and
 * - an asset explorer named `"assets"` backed by @p assets.
 *
 * @tparam StrT Compile-time name type of the mount point.
 * @tparam Actions Action collection type stored by the mount point.
 *
 * @param mountpoint Mount point moved into the router.
 * @param assets Compiled asset store referenced by the asset explorer.
 * @param docroot Filesystem directory exposed by the file explorer.
 *
 * @return A mounted-action router containing the mount point, file explorer,
 *         and asset explorer.
 *
 * @note The asset store is referenced by the router's explorer. It must remain
 *       valid for as long as the router may serve its resources.
 *
 * @see basic_router<udho::url::mountpoints_table<
 *          udho::url::mount_point<StrT, Actions>>>
 * @see udho::url::explorers::files
 * @see udho::url::explorers::assets
 */
template <typename StrT, typename Actions>
basic_router<udho::url::mountpoints_table<udho::url::mount_point<StrT, Actions>>> router(udho::url::mount_point<StrT, Actions>&& mountpoint, const udho::view::resources::asset::const_store& assets, const std::filesystem::path& docroot) {
    return router(udho::url::mountpoints_table(std::move(mountpoint)), assets, docroot);
}


template <typename T>
struct is_router: std::false_type{};

/**
 * @brief Compile-time trait specialization for basic_router types.
 *
 * Evaluates to `std::true_type` for every specialization of basic_router,
 * including routers backed by a mount-points table and the explorer-only
 * `basic_router<void>` specialization.
 *
 * @tparam Table Routing-table template argument of the router. This is
 *         typically a mountpoints_table specialization or `void`.
 *
 * @par Example
 * @code
 * using explorer_router = udho::url::basic_router<void>;
 *
 * static_assert(udho::url::is_router<explorer_router>::value);
 * @endcode
 *
 *
 *
 * @see basic_router
 * @see basic_router<void>
 */
template <typename Table>
struct is_router<basic_router<Table>>: std::true_type{};

/// @}

}
}

#endif // ROUTER_H
