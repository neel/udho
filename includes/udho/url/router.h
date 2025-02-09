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

#include <udho/pages/system.h>

namespace udho{
namespace url{

namespace detail{

struct docroot_fs{
    docroot_fs(): _docroot(std::filesystem::current_path()){}

    /**
     * @brief Sets the document root for file serving
     * @param path Filesystem path to use as document root
     */
    void docroot(const std::filesystem::path& path) {
        _docroot = path;
    }

    /**
     * @brief Gets the current document root
     * @return Const reference to the document root path
     */
    const std::filesystem::path& docroot() const {
        return _docroot;
    }
    protected:
        /**
         * @brief Normalizes and secures a filesystem path
         * @tparam Ch Character type for the path string
         * @param subject Path to normalize
         * @return Normalized path or empty path if security check fails
         * @note Prevents directory traversal attacks by ensuring path stays within docroot
         */
        template <typename Ch>
        std::filesystem::path normalize(const std::basic_string<Ch>& subject) const {
            std::filesystem::path root = !_docroot.empty() ? _docroot : std::filesystem::current_path();
            std::string relative_subject = subject;
            if (!relative_subject.empty() && relative_subject[0] == '/') {
                relative_subject.erase(0, 1); // Remove the leading slash if present
            }

            std::filesystem::path requested_path = root / relative_subject;
            std::filesystem::path normalized_path;
            try {
                normalized_path = std::filesystem::weakly_canonical(requested_path);
                if (!boost::algorithm::starts_with(normalized_path.string(), root.string())) {
                    std::cout << "Security alert: Attempted access outside of the document root. " << normalized_path << " " << root << std::endl;
                    return std::filesystem::path{};
                }
            } catch(const std::filesystem::filesystem_error& e) {
                std::cout << "Filesystem error: " << e.what() << std::endl;
                return std::filesystem::path{};
            }
            return normalized_path;
        }

        /**
         * @brief Checks if a normalized file path exists
         * @tparam Ch Character type for the path string
         * @param subject Path to check
         * @return true if file exists and is regular, false otherwise
         */
        template <typename Ch>
        bool find_file(const std::basic_string<Ch>& subject) const {
            std::filesystem::path normalized_path = normalize(subject);
            if(normalized_path.empty()){
                return false;
            }
            return std::filesystem::exists(normalized_path);
        }

        /**
         * @brief Serves a file through the provided stream
         * @tparam Ch Character type for the path string
         * @param subject Path to serve
         * @param stream Network stream to write to
         * @return true if file was served successfully, false otherwise
         * @throws Propagates filesystem errors and libmagic exceptions
         */
        template <typename Ch>
        bool serve_file(const std::basic_string<Ch>& subject, udho::net::stream& stream) const {
            std::filesystem::path normalized_path = normalize(subject);
            if(normalized_path.empty()){
                return false;
            }
            if(!std::filesystem::exists(normalized_path)){
                return false;
            }

            return serve_file(normalized_path, stream);
        }

        /**
         * @brief Determines MIME type of a file using libmagic
         * @param path Filesystem path to analyze
         * @return MIME type as string
         * @note Requires libmagic development files during compilation
         */
        inline std::string mime_type(const std::filesystem::path& path) const {
            magic_t magic = magic_open(MAGIC_MIME_TYPE);
            magic_load(magic, nullptr);
            const char* mime_type = magic_file(magic, path.c_str());
            std::string result = mime_type ? mime_type : "application/octet-stream";
            magic_close(magic);
            return result;
        }

        /**
         * @brief serves a directory or file from docroot
         * @param ctx
         * @param target
         * @return boolean value indicating success
         */
        template <typename ContextT>
        bool serve_local(const std::string& target, ContextT ctx) const {
            std::filesystem::path normalized_path = normalize(target);
            if(normalized_path.empty()){
                return false;
            }
            if(!std::filesystem::exists(normalized_path)){
                return false;
            }

            if(std::filesystem::is_directory(normalized_path)){
                return serve_listing(normalized_path, ctx);
            } else {
                return serve_file(normalized_path, ctx);
            }
        }
    private:
        template <typename ContextT>
        bool serve_listing(const std::filesystem::path& target, ContextT ctx) const {
            if(!std::filesystem::is_directory(target)){
                return false;
            }

            namespace placeholders = udho::pages::system::layouts::placeholders;

            auto layout     = udho::pages::system::layouts::listing(ctx);
            auto header     = udho::pages::system::data::listing_header{target, docroot()};
            auto directory  = udho::pages::system::data::directory_listing{target, docroot()};
            auto footer     = udho::pages::system::data::status_info{};

            layout[placeholders::header]  = header;
            layout[placeholders::central] = directory;
            layout[placeholders::footer]  = footer;
            return true;
        }
        inline bool serve_file(const std::filesystem::path& normalized_path, udho::net::stream& stream) const {
            try {
                std::string mime = mime_type(normalized_path);
                boost::iostreams::mapped_file_source file;
                file.open(normalized_path);

                if (file.is_open()) {
                    stream.set(boost::beast::http::field::content_type, mime);
                    stream.set(boost::beast::http::field::content_length, std::to_string(file.size()));

                    stream.write(file.data(), file.size());
                    file.close();
                    stream.finish();
                    return true;
                } else {
                    std::cout << "Failed to open file: " << normalized_path << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Error serving file: " << e.what() << std::endl;
            }

            return false;
        }
    private:
        std::filesystem::path      _docroot;
};

/**
 * @class routing_table
 * @brief Template class for managing URL routing with mount points and file serving capabilities
 *
 * @tparam MountPointsT Sequence of mount points (udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>)
 */
template <typename MountPointsT>
struct routing_table: protected docroot_fs{
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

    using docroot_fs::normalize;
    using docroot_fs::docroot;

    /**
     * @brief Constructs a routing table with mount points
     * @param mountpoints Rvalue reference to mount points collection
     * @post Initializes internal summary that can be accessed through the @ref summary function
     */
    routing_table(mountpoints_type&& mountpoints): _mountpoints(std::move(mountpoints)) {
        summarize();
    }

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
        if(!found){
            return find_file(subject);
        }
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
        if(!found){
            found = serve_local(subject, std::forward<Args>(args)...);
        }
        return found;
    }

    /**
     * @brief Function call operator that delegates to invoke()
     * @param url URL path to process
     * @param args Arguments to forward to the action
     * @return bool indicating if request was handled
     */
    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const {
        return this->invoke(url, std::forward<Args>(args)...);
    }

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
template <typename MountPointsT, typename StoreT = void>
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
struct basic_router<MountPointsT, void>: private detail::routing_table<MountPointsT>{

    using routing_table = detail::routing_table<MountPointsT>;

    using routing_table::operator[];
    using routing_table::find;
    using routing_table::invoke;
    using routing_table::operator();
    using routing_table::summary;
    using routing_table::docroot;

    template <typename Mountpoints>
    friend std::ostream& operator<<(std::ostream& stream, const basic_router<Mountpoints, void>& router){
        const detail::routing_table<Mountpoints>& table = router;
        stream << table;
        return stream;
    }

    using mountpoints_type = MountPointsT;

    basic_router() = delete;
    basic_router(const basic_router<MountPointsT>&) = delete;
    basic_router(basic_router<MountPointsT>&&) = delete;

    basic_router(mountpoints_type&& mountpoints): routing_table(std::move(mountpoints)) {}
};

/**
 * @brief Specialization with compiled asset store integration
 * @tparam MountPointsT Mount points sequence udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>
 *
 * Provides routing using mountpoints with access to assets from const_store.
 * Automatically serves assets when routes don't match.
 *
 * @note If there exists a file with matching path (including directory and file name) as an asset in the docroot then that file is served as the asset.
 */
template <typename MountPointsT>
struct basic_router<MountPointsT, udho::view::resources::asset::const_store>: private detail::routing_table<MountPointsT>{

    using routing_table = detail::routing_table<MountPointsT>;

    using routing_table::operator[];
    using routing_table::summary;
    using routing_table::docroot;

    template <typename Mountpoints>
    friend std::ostream& operator<<(std::ostream& stream, const basic_router<Mountpoints, udho::view::resources::asset::const_store>& router){
        const detail::routing_table<Mountpoints>& table = router;
        stream << table << "\n";
        stream << router.assets();
        return stream;
    }

    using mountpoints_type = MountPointsT;

    basic_router() = delete;
    basic_router(const basic_router<MountPointsT>&) = delete;
    basic_router(basic_router<MountPointsT>&&) = delete;

    basic_router(mountpoints_type&& mountpoints, const udho::view::resources::asset::const_store& assets): routing_table(std::move(mountpoints)), _assets(assets) {}

    const udho::view::resources::asset::const_store& assets() const { return _assets; }

    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const {
        bool found = routing_table::find(subject);
        if(!found){
            return _assets.find(subject).valid();
        }
        return found;
    }

    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        bool invoked = routing_table::invoke(subject, std::forward<Args>(args)...);
        if(!invoked){
            return serve_asset(subject, std::forward<Args>(args)...);
        }
        return invoked;
    }

    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const {
        return this->invoke(url, std::forward<Args>(args)...);
    }

    private:
        template <typename Ch>
        bool serve_asset(const std::basic_string<Ch>& subject, udho::net::stream& stream) const {
            return _assets.serve(stream, subject);
        }
    private:
        const udho::view::resources::asset::const_store& _assets;

};

/**
 * @brief Asset-only specialization without mount points
 *
 * Pure asset server configuration. Use when only serving assets without custom routes.
 */
template <>
struct basic_router<void, udho::view::resources::asset::const_store>: private detail::docroot_fs{

    friend std::ostream& operator<<(std::ostream& stream, const basic_router<void, udho::view::resources::asset::const_store>& router){
        stream << router.assets();
        return stream;
    }


    basic_router() = delete;
    basic_router(const basic_router<void, udho::view::resources::asset::const_store>&) = delete;
    basic_router(basic_router<void, udho::view::resources::asset::const_store>&&) = delete;

    basic_router(const udho::view::resources::asset::const_store& assets): _assets(assets) {}

    const udho::view::resources::asset::const_store& assets() const { return _assets; }

    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const {
        bool found = find_file(subject);
        if(!found){
            return _assets.find(subject).valid();
        }
    }

    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        bool invoked = serve_local(subject, std::forward<Args>(args)...);
        if(!invoked){
            return serve_asset(subject, std::forward<Args>(args)...);
        }
        return invoked;
    }

    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const {
        return this->invoke(url, std::forward<Args>(args)...);
    }

    const udho::url::summary::router& summary() const { return _summary; }

    private:
        template <typename Ch>
        bool serve_asset(const std::basic_string<Ch>& subject, udho::net::stream& stream) const {
            return _assets.serve(stream, subject);
        }
    private:
        const udho::view::resources::asset::const_store& _assets;
        udho::url::summary::router _summary;

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
template <typename MountPointsT, typename std::enable_if<!std::is_same<std::decay_t<MountPointsT>, udho::view::resources::asset::const_store>::value, int>::type = 0>
basic_router<MountPointsT, void> router(MountPointsT&& mountpoints){
    return basic_router<MountPointsT, void>{std::move(mountpoints)};
}

/**
 * @brief Create router with asset store (parameter order 1)
 * @param mountpoints Routing configuration
 * @param assets asset store
 * @return Router with asset support
 */
template <typename MountPointsT>
basic_router<MountPointsT, udho::view::resources::asset::const_store> router(MountPointsT&& mountpoints, const udho::view::resources::asset::const_store& assets){
    return basic_router<MountPointsT, udho::view::resources::asset::const_store>{std::move(mountpoints), assets};
}

/**
 * @brief Create router with asset store (parameter order 2)
 * @param assets asset store
 * @param mountpoints Routing configuration
 * @return Router with asset support
 */
template <typename MountPointsT>
basic_router<MountPointsT, udho::view::resources::asset::const_store> router(const udho::view::resources::asset::const_store& assets, MountPointsT&& mountpoints){
    return basic_router<MountPointsT, udho::view::resources::asset::const_store>{std::move(mountpoints), assets};
}

/**
 * @brief Create asset-only router without mount points
 * @param assets asset store
 * @return Pure asset server router
 */
inline basic_router<void, udho::view::resources::asset::const_store> router(const udho::view::resources::asset::const_store& assets){
    return basic_router<void, udho::view::resources::asset::const_store>{assets};
}

/// @}

}
}

#endif // ROUTER_H
