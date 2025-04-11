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

namespace udho{
namespace url{

namespace detail{
/**
 * @class routing_table
 * @brief Template class for managing URL routing with mount points and file serving capabilities
 *
 * @tparam MountPointsT Sequence of mount points (udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>)
 */
template <typename MountPointsT>
struct routing_table{

    /**
     * @brief operator overload for streaming the routing table's mount points
     * @param stream Output stream
     * @param router Routing table
     * @return Reference to the output stream
     */
    template <typename Mountpoints>
    friend std::ostream& operator<<(std::ostream& stream, const udho::url::detail::routing_table<Mountpoints>& router){
        stream << router._mountpoints;
        return stream;
    }

    /// Type alias for the mount points collection
    using mountpoints_type = MountPointsT;

    routing_table() = delete;
    routing_table(const routing_table<MountPointsT>&) = delete;
    routing_table(routing_table<MountPointsT>&&) = delete;

    /**
     * @brief Constructs a routing table with mount points
     * @param mountpoints Rvalue reference to mount points collection
     * @post Initializes internal summary that can be accessed through the @ref summary function
     */
    routing_table(mountpoints_type&& mountpoints): _mountpoints(std::move(mountpoints)) { summarize(); }

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
     * @brief Checks if a URL path exists in the routing table or filesystem
     * @tparam Ch Character type for the URL string
     * @param subject URL path to search for
     * @return true if path is found in mount points or filesystem, false otherwise
     */
    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const {
        bool found = false;
        _mountpoints.visit([&subject, &found](const auto& mointpoint){
            if(found)
                return;
            auto path = mointpoint.path();
            if(!boost::starts_with(subject, path))
                return;
            auto rest = path == "/" ? subject : subject.substr(path.size());
            found = mointpoint.find(rest);
        });
        return found;
    }

    /**
     * @brief Invokes the action associated with a URL path
     * @tparam Ch Character type for the URL string
     * @tparam Args Types of arguments to forward
     * @param subject URL path to invoke
     * @param args Arguments to forward to the action
     * @return true if action was invoked or file was served, false otherwise
     */
    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        bool found = false;
        _mountpoints.visit([&subject, &found, &args...](const auto& mointpoint){
            if(found)
                return;
            auto path = mointpoint.path();
            if(!boost::starts_with(subject, path))
                return;
            auto rest = path == "/" ? subject : subject.substr(path.size());
            found = mointpoint.invoke(rest, std::forward<Args>(args)...);
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
        mountpoints_type           _mountpoints;
        udho::url::summary::router _summary;
};

template <typename RoutingTableT = void>
struct basic_router;

template <typename MountPointsT>
struct basic_router<detail::routing_table<MountPointsT>>: private detail::routing_table<MountPointsT>{
    using routing_table    = detail::routing_table<MountPointsT>;
    using mountpoints_type = MountPointsT;

    using routing_table::operator[];
    using routing_table::summary;

    basic_router() = delete;
    basic_router(const basic_router<routing_table>&) = delete;
    basic_router(basic_router<routing_table>&&) = delete;
    basic_router(mountpoints_type&& mountpoints): routing_table(std::move(mountpoints)) {}
    basic_router(mountpoints_type&& mountpoints, udho::url::explorers::registry&& registry): routing_table(std::move(mountpoints)), _registry(std::move(registry)) {}

    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const {
        bool found = routing_table::find(subject);
        if(!found){
            return _registry.exists(subject);
        }
        return found;
    }

    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        bool invoked = routing_table::invoke(subject, std::forward<Args>(args)...);
        if(!invoked){
            if constexpr (sizeof...(args) == 1){
                invoked = _registry.serve(subject, std::forward<Args>(args)...);
            }
        }
        return invoked;
    }

    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const { return this->invoke(url, std::forward<Args>(args)...); }

    const detail::routing_table<MountPointsT>& table() const { return *this; }

private:
    udho::url::explorers::registry _registry;
};

template <>
struct basic_router<void>{
    using routing_table    = void;
    using mountpoints_type = void;

    basic_router() = delete;
    basic_router(const basic_router<routing_table>&) = delete;
    basic_router(basic_router<routing_table>&&) = delete;

    basic_router(udho::url::explorers::registry&& registry): _registry(std::move(registry)) {}

    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const { return _registry.exists(subject); }

    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        if constexpr (sizeof...(args) == 1){
            return _registry.serve(subject, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const { return this->invoke(url, std::forward<Args>(args)...); }

    const udho::url::summary::router& summary() const { return _summary; }

private:
    udho::url::explorers::registry _registry;
    udho::url::summary::router     _summary;
};

}

/**
 * @defgroup Router Routing System
 * @brief Core components for URL routing with template specialization support
 */

/**
 * @brief Primary template for URL router with mount points and optional asset store
 * @tparam MountPointsT Type sequence defining routing endpoints
 * @tparam StoreT Storage type for resources (default: void = no storage)
 * @ingroup Router
 *
 * @par Specialization Behavior:
 * - void store: Basic routing without asset management
 * - const_store: Routing with compiled-in asset resources
 */
template <typename MountPointsT = void>
struct basic_router;

/// @addtogroup Router
/// @{

/**
 * @brief Specialization for basic routing without asset storage
 * @tparam MountPointsT Mount points sequence udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>
 *
 * Inherits core routing functionality from detail::routing_table.
 * Use this version when you don't need embedded resources.
 */
template <typename MountPointsT>
struct basic_router: private detail::basic_router<detail::routing_table<MountPointsT>>{
    using detail_basic_router   = detail::basic_router<detail::routing_table<MountPointsT>>;
    using routing_table         = typename detail_basic_router::routing_table;
    using mountpoints_type      = typename detail_basic_router::mountpoints_type;

    using detail_basic_router::operator[];
    using detail_basic_router::summary;
    using detail_basic_router::find;
    using detail_basic_router::invoke;
    using detail_basic_router::operator();

    basic_router() = delete;
    basic_router(const basic_router<MountPointsT>&) = delete;
    basic_router(basic_router<MountPointsT>&&) = delete;
    basic_router(mountpoints_type&& mountpoints): detail_basic_router(std::forward<mountpoints_type>(mountpoints)) {}
    basic_router(mountpoints_type&& mountpoints, udho::url::explorers::registry&& registry): detail_basic_router(std::forward<mountpoints_type>(mountpoints), std::forward<udho::url::explorers::registry>(registry)) {}

    template <typename Mountpoints>
    friend std::ostream& operator<<(std::ostream& stream, const basic_router<Mountpoints>& router){
        const detail::routing_table<Mountpoints>& table = router.table();
        stream << table;
        return stream;
    }

};

/**
 * @brief Asset-only specialization without mount points
 *
 * Pure asset server configuration. Use when only serving assets without custom routes.
 */
template <>
struct basic_router<void>: private detail::basic_router<void>{
    using detail_basic_router   = detail::basic_router<void>;
    using routing_table         = typename detail_basic_router::routing_table;
    using mountpoints_type      = typename detail_basic_router::mountpoints_type;

    using detail_basic_router::summary;
    using detail_basic_router::find;
    using detail_basic_router::invoke;
    using detail_basic_router::operator();

    basic_router() = delete;
    basic_router(const basic_router<void>&) = delete;
    basic_router(basic_router<void>&&) = delete;
    basic_router(udho::url::explorers::registry&& registry): detail_basic_router(std::forward<udho::url::explorers::registry>(registry)) {}

    friend std::ostream& operator<<(std::ostream& stream, const basic_router<void>& router){
        return stream;
    }

};

/// @}

/**
 * @name Router Factory Functions
 * @brief Convenience functions for creating router configurations
 * @relates basic_router
 * @ingroup Router
 *
 * These functions automatically select the appropriate router specialization
 * based on input parameters.
 */
/// @{

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
template <typename MountPointsT>
basic_router<MountPointsT> router(MountPointsT&& mountpoints, udho::url::explorers::registry&& registry){
    return basic_router<MountPointsT>{std::forward<MountPointsT>(mountpoints), std::forward<udho::url::explorers::registry>(registry)};
}

template <typename MountPointsT>
basic_router<MountPointsT> router(MountPointsT&& mountpoints){
    return basic_router<MountPointsT>{std::forward<MountPointsT>(mountpoints)};
}

/**
 * @brief Create explorer router without mount points
 * @param explorer
 * @return Pure explorer based server router
 */
inline basic_router<void> router(udho::url::explorers::registry&& registry){
    return basic_router<void>{std::forward<udho::url::explorers::registry>(registry)};
}

template <typename... ExplorerT>
inline basic_router<void> router(ExplorerT&&... explorers){
    return basic_router<void>{udho::url::explorers::registry{std::move(explorers)...}};
}

template <typename MountPointsT>
basic_router<MountPointsT> router(MountPointsT&& mountpoints, const udho::view::resources::asset::const_store& assets){
    using router_type = basic_router<MountPointsT>;

    return  router_type{
                std::forward<MountPointsT>(mountpoints),
                udho::url::explorers::registry{
                    udho::url::explorers::assets{"assets", assets}
                }
            };
}

inline basic_router<void> router(const udho::view::resources::asset::const_store& assets){
    using router_type = basic_router<void>;

    return  router_type{
                udho::url::explorers::registry{
                    udho::url::explorers::assets{"assets", assets}
                }
            };
}

inline basic_router<void> router(const udho::view::resources::asset::const_store& assets, const std::filesystem::path& docroot){
    using router_type = basic_router<void>;

    return  router_type{
        udho::url::explorers::registry{
            udho::url::explorers::files{"docroot", docroot},
            udho::url::explorers::assets{"assets", assets}
        }
    };
}

/// @}

}
}

#endif // ROUTER_H
