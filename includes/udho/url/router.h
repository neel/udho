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

namespace udho{
namespace url{

namespace detail{

/**
 * mounts points
 * @tparam MountPointsT udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>
 */
template <typename MountPointsT>
struct routing_table{
    template <typename Mountpoints>
    friend std::ostream& operator<<(std::ostream& stream, const udho::url::detail::routing_table<Mountpoints>& router){
        stream << router._mountpoints;
        return stream;
    }

    using mountpoints_type = MountPointsT;

    routing_table() = delete;
    routing_table(const routing_table<MountPointsT>&) = delete;
    routing_table(routing_table<MountPointsT>&&) = delete;

    routing_table(mountpoints_type&& mountpoints): _mountpoints(std::move(mountpoints)) {
        summarize();
    }

    template <typename XStrT>
    auto& operator[](XStrT&& xstr) { return _mountpoints[std::move(xstr)]; }

    template <typename XStrT>
    const auto& operator[](XStrT&& xstr) const { return _mountpoints[std::move(xstr)]; }

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
            return serve_file(subject, std::forward<Args>(args)...);
        }
        return found;
    }

    template <typename... Args>
    bool operator()(const std::string& url, Args&&... args) const {
        return this->invoke(url, std::forward<Args>(args)...);
    }

    const udho::url::summary::router& summary() const { return _summary; }

    void docroot(const std::filesystem::path& path) {
        _docroot = path;
    }
    const std::filesystem::path& docroot() const {
        return _docroot;
    }

    private:
        template <typename Ch>
        bool find_file(const std::basic_string<Ch>& subject) const {
            std::filesystem::path normalized_path = normalize(subject);
            if(normalized_path.empty()){
                return false;
            }
            return std::filesystem::exists(normalized_path) && std::filesystem::is_regular_file(normalized_path);
        }

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

        template <typename Ch>
        bool serve_file(const std::basic_string<Ch>& subject, udho::net::stream& stream) const {
            std::filesystem::path normalized_path = normalize(subject);
            if(normalized_path.empty()){
                return false;
            }
            if(!std::filesystem::exists(normalized_path)){
                return false;
            }

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

        inline std::string mime_type(const std::filesystem::path& path) const {
            magic_t magic = magic_open(MAGIC_MIME_TYPE);
            magic_load(magic, nullptr);
            const char* mime_type = magic_file(magic, path.c_str());
            std::string result = mime_type ? mime_type : "application/octet-stream";
            magic_close(magic);
            return result;
        }
    private:
        void summarize(){
            _mountpoints.visit([this](const auto& m){
                _summary.add(m);
            });
        }
    private:
        mountpoints_type           _mountpoints;
        udho::url::summary::router _summary;
        std::filesystem::path      _docroot;
};

}

template <typename MountPointsT, typename StoreT = void>
struct basic_router;

/**
 * mounts points
 * @tparam MountPointsT udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>
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
        // TODO print the assets
        return stream;
    }

    using mountpoints_type = MountPointsT;

    basic_router() = delete;
    basic_router(const basic_router<MountPointsT>&) = delete;
    basic_router(basic_router<MountPointsT>&&) = delete;

    basic_router(mountpoints_type&& mountpoints): routing_table(std::move(mountpoints)) {}
};

/**
 * mounts points
 * @tparam MountPointsT udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>
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
        stream << table;
        // TODO print the assets
        return stream;
    }

    using mountpoints_type = MountPointsT;

    basic_router() = delete;
    basic_router(const basic_router<MountPointsT>&) = delete;
    basic_router(basic_router<MountPointsT>&&) = delete;

    basic_router(mountpoints_type&& mountpoints, const udho::view::resources::asset::const_store& assets): routing_table(std::move(mountpoints)), _assets(assets) {}

    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const {
        bool found = routing_table::find(subject);
        if(!found){
            return _assets.find(subject);
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

template <>
struct basic_router<void, udho::view::resources::asset::const_store>{

    friend std::ostream& operator<<(std::ostream& stream, const basic_router<void, udho::view::resources::asset::const_store>& router){
        // TODO print the assets
        return stream;
    }


    basic_router() = delete;
    basic_router(const basic_router<void, udho::view::resources::asset::const_store>&) = delete;
    basic_router(basic_router<void, udho::view::resources::asset::const_store>&&) = delete;

    basic_router(const udho::view::resources::asset::const_store& assets): _assets(assets) {}

    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const {
        return _assets.find(subject);
    }

    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const {
        return serve_asset(subject, std::forward<Args>(args)...);
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

/**
 * @brief create router from a set of mountpoints
 *
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
basic_router<MountPointsT, void> router(MountPointsT&& mountpoints){
    return basic_router<MountPointsT, void>{std::move(mountpoints)};
}

template <typename MountPointsT>
basic_router<MountPointsT, udho::view::resources::asset::const_store> router(MountPointsT&& mountpoints, const udho::view::resources::asset::const_store& assets){
    return basic_router<MountPointsT, udho::view::resources::asset::const_store>{std::move(mountpoints), assets};
}

template <typename MountPointsT>
basic_router<MountPointsT, udho::view::resources::asset::const_store> router(const udho::view::resources::asset::const_store& assets, MountPointsT&& mountpoints){
    return basic_router<MountPointsT, udho::view::resources::asset::const_store>{std::move(mountpoints), assets};
}

inline basic_router<void, udho::view::resources::asset::const_store> router(const udho::view::resources::asset::const_store& assets){
    return basic_router<void, udho::view::resources::asset::const_store>{assets};
}

}
}

#endif // ROUTER_H
