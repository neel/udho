/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_VIEW_RESOURCES_RESOURCE_H
#define UDHO_VIEW_RESOURCES_RESOURCE_H

#include <magic.h>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/filesystem/path.hpp>
#include <udho/url/detail/format.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <udho/view/resources/fwd.h>
#include <udho/net/ostream.h>

namespace udho{
namespace view{
namespace resources{

/**
 * @addtogroup DoxyG_view_resources
 * @{
 */

namespace tmpl{
    /**
     * @brief a view template written in a foreign language
     */
    struct resource{
        using buffer_type   = std::vector<char>;
        using iterator_type = buffer_type::const_iterator;
        using value_type    = char;
        using size_type     = buffer_type::size_type;

        /**
         * @brief Returns the name of the resource.
         * @return Name of the resource as a std::string.
         */
        inline std::string name() const { return _name; }
        /**
         * @brief Gets an iterator to the beginning of the resource content.
         * @return An iterator to the start.
         */
        inline iterator_type begin() const { return _buffer.begin(); }
        /**
         * @brief Gets an iterator to the end of the resource content.
         * @return An iterator to the start.
         */
        inline iterator_type end()   const { return _buffer.end();   }
        /**
         * @brief byte size of the resource content.
         * @return An iterator to the start.
         */
        inline size_type     size()  const { return _buffer.size();  }

        /**
         * @brief construct a template from an on memory string
         * @tparam Iterator iterator type
         * @param name name of the view
         * @param begin begin iterator
         * @param end end iterator
         */
        template <typename Iterator>
        resource(const std::string& name, Iterator begin, Iterator end): _name(name) {
            using value_type = typename std::iterator_traits<Iterator>::value_type;

            auto size = std::distance(begin, end);
            _buffer.reserve(size * sizeof(value_type));

            for (auto it = begin; it != end; ++it) {
                const char* data = reinterpret_cast<const char*>(&*it);
                _buffer.insert(_buffer.end(), data, data + sizeof(value_type));
            }
        }

        /**
         * @brief construct a template from an on disk file
         * @param name name of the view
         * @param path path to the view
         */
        inline resource(const std::string& name, const boost::filesystem::path& path): _name(name) {
            std::ifstream file(path.string(), std::ios::binary | std::ios::ate);
            if (!file) {
                throw std::runtime_error("Failed to open file: " + path.string());
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            _buffer.resize(size);
            if (!file.read(_buffer.data(), size)) {
                throw std::runtime_error("Failed to read file: " + path.string());
            }

            if (file.gcount() != size) {
                throw std::runtime_error("File read size mismatch for: " + path.string());
            }
        }

        private:
            std::string _name;
            buffer_type _buffer;
    };

    /**
     * @brief Associates a view template resource written in a foreign language
     *        with the bridge used to execute it.
     * @tparam BridgeT Bridge type responsible for compiling or executing the
     *                 view template written in a foreign language.
     */
    template <typename BridgeT>
    struct bridged{
        using buffer_type   = tmpl::resource::buffer_type;
        using iterator_type = tmpl::resource::iterator_type;
        using value_type    = tmpl::resource::value_type;
        using size_type     = tmpl::resource::size_type;
        using bridge_type   = BridgeT;
        /**
         * @brief construct a template from an on memory string
         * @tparam Iterator iterator type
         * @param name name of the view
         * @param begin begin iterator
         * @param end end iterator
         */
        template <typename Iterator>
        inline bridged(const std::string& name, Iterator begin, Iterator end): _res(name, begin, end) {}
        /**
         * @brief construct a template from an on disk file
         * @param name name of the view
         * @param path path to the view
         */
        inline bridged(const std::string& name, const boost::filesystem::path& path): _res(name, path) {}

        /// @brief Returns the underlying view template resource.
        const tmpl::resource& resource() const { return _res;}

        /// @brief Returns the underlying mutable view template resource.
        tmpl::resource& resource() { return _res;}
        private:
            tmpl::resource _res;

    };
}

namespace asset{
    /**
     * @brief Storage policy selected by resource source and ownership.
     * @tparam Source Source descriptor such as memory, disk, or remote.
     * @tparam Owned Whether the storage owns its underlying data.
     */
    template <typename Source, bool Owned>
    struct storage;

    template <asset::type AssetType, typename Source, bool Owned>
    struct common_resource;

    /**
     * @brief Owned in-memory asset storage.
     *
     * Copies the supplied range into an internal buffer.
     *
     * @tparam Iterator Iterator type used to supply the source range.
     */
    template <typename Iterator>
    struct storage<asset::source::memory<Iterator>, true>{
        using source = asset::source::memory<Iterator>;
        static constexpr bool owned           = true;

        template <asset::type AssetType, typename Source, bool Owned>
        friend struct common_resource;

        using iterator_type = Iterator;
        using value_type    = typename std::iterator_traits<Iterator>::value_type;
        using size_type     = std::size_t;
        using buffer_type   = std::vector<value_type>;

        /// @brief Returns the begin iterator.
        iterator_type begin() const { return _buffer.begin(); }

        /// @brief Returns the end iterator.
        iterator_type end()   const { return _buffer.end();   }

        /// @brief Returns the number of stored elements.
        size_type     size()  const { return _buffer.size();  }


        private:
            /**
             * @brief Writes the owned buffer to an output stream.
             * @tparam OstreamT Output-stream type.
             * @param stream Destination stream.
             * @return Number of elements written.
             */
            template <typename OstreamT>
            std::size_t write(OstreamT& stream) const {
                if (!_buffer.empty()) {
                    assert(_buffer.back() != '\0' && "Null in owned storage!");
                    stream.write((const char*)_buffer.data(), _buffer.size());
                }
                return _buffer.size();
            }

        private:
            /**
             * @brief Copies a source range into owned storage.
             * @param begin Iterator to the first source element.
             * @param end End iterator of the source range.
             */
            storage(Iterator begin, Iterator end) {
                if (begin == end) return;

                if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::value_type, char>) {
                    if (*(std::prev(end)) == '\0') {
                        --end;
                    }
                }
                _buffer.assign(begin, end);
            }
            /**
             * @brief Detects the MIME type of the stored data.
             * @return Detected MIME type, or `application/octet-stream` when
             *         detection fails.
             */
            std::string mime() const {
                magic_t magic = magic_open(MAGIC_MIME_TYPE);
                if (magic_load(magic, nullptr) != 0) {
                    std::cerr << "Failed to load magic database: " << magic_error(magic) << std::endl;
                    magic_close(magic);
                    return "application/octet-stream"; // Default MIME type if error
                }

                const char* mime_type = magic_buffer(magic, _buffer.data(), _buffer.size());
                std::string result = mime_type ? mime_type : "application/octet-stream";
                magic_close(magic);
                return result;
            }

        private:
            buffer_type _buffer;
    };

    /**
     * @brief Non-owning in-memory asset storage.
     *
     * Retains the supplied iterators rather than copying their contents. The
     * referenced range must remain valid for the lifetime of the resource.
     *
     * @tparam Iterator Iterator type used to reference the source range.
     */
    template <typename Iterator>
    struct storage<asset::source::memory<Iterator>, false>{
        using source = asset::source::memory<Iterator>;
        static constexpr bool owned           = false;

        template <asset::type AssetType, typename Source, bool Owned>
        friend struct common_resource;

        using iterator_type = Iterator;
        using value_type    = typename std::iterator_traits<Iterator>::value_type;
        using size_type     = std::size_t;

        /// @brief Returns the begin iterator.
        iterator_type begin() const { return _begin; }

        /// @brief Returns the end iterator.
        iterator_type end()   const { return _end;   }

        /// @brief Returns the number of referenced elements.
        size_type     size()  const { return _size;  }

        private:
            /**
             * @brief Writes the referenced range to the output stream.
             * @param stream Destination stream.
             * @return Number of elements written.
             */
            std::size_t write(udho::net::ostream_view& stream) const {
                if (_size > 0) {
                    assert(*(_end - 1) != '\0' && "Null in non-owned storage!");
                    stream.write(reinterpret_cast<const char*>(&(*_begin)), _size);
                }
                return _size;
            }

        private:
            /**
             * @brief Constructs non-owning storage over an iterator range.
             * @param begin Begin iterator of the source range.
             * @param end End iterator of the source range.
             */
            storage(iterator_type begin, iterator_type end): _begin(begin), _end(adjust_end(begin, end)), _size(std::distance(_begin, _end)) { }

            /**
             * @brief Adjusts the end iterator used for the stored range.
             * @param begin Begin iterator of the source range.
             * @param end Original end iterator.
             * @return Adjusted end iterator.
             */
            static iterator_type adjust_end(iterator_type begin, iterator_type end) {
                if constexpr (std::is_same_v<value_type, char>) {
                    if (begin != end && *(std::prev(end)) == '\0') {
                        return std::prev(end);
                    }
                }
                return end;
            }

            /**
             * @brief Detects the MIME type of the referenced data.
             * @return Detected MIME type, or `application/octet-stream` when
             *         detection fails.
             */
            std::string mime() const {
                magic_t magic = magic_open(MAGIC_MIME_TYPE);
                if (!magic) {
                    std::cerr << "Failed to initialize magic" << std::endl;
                    return "application/octet-stream";
                }
                if (magic_load(magic, nullptr) != 0) {
                    std::cerr << "Failed to load magic database: " << magic_error(magic) << std::endl;
                    magic_close(magic);
                    return "application/octet-stream";
                }

                std::string result;
                if constexpr (std::is_pointer_v<iterator_type>) {
                    const char* data = reinterpret_cast<const char*>(_begin);
                    const char* mime_type = magic_buffer(magic, data, _size);
                    result = mime_type ? mime_type : "application/octet-stream";
                } else {
                    std::vector<unsigned char> buffer(_begin, _end);
                    const char* mime_type = magic_buffer(magic, buffer.data(), buffer.size());
                    result = mime_type ? mime_type : "application/octet-stream";
                }

                magic_close(magic);
                return result;
            }
        private:
            iterator_type _begin;
            iterator_type _end;
            size_type     _size;
    };

    /**
     * @brief Owned storage loaded from a file.
     *
     * Reads the complete file into an internal buffer during construction.
     *
     * @tparam Path Filesystem path type.
     */
    template <typename Path>
    struct storage<asset::source::disk<Path>, true>{
        using source = asset::source::disk<Path>;
        static constexpr bool owned           = true;

        template <asset::type AssetType, typename Source, bool Owned>
        friend struct common_resource;

        using path_type     = Path;
        using buffer_type   = std::vector<char>;
        using iterator_type = buffer_type::const_iterator;
        using value_type    = char;
        using size_type     = buffer_type::size_type;

        /// @brief Returns the source path.
        const boost::filesystem::path& path() const { return _path; }

        /// @brief Returns the begin iterator.
        iterator_type begin() const { return _buffer.begin(); }

        /// @brief Returns the end iterator.
        iterator_type end()   const { return _buffer.end();   }

        /// @brief Returns the number of loaded bytes.
        size_type     size()  const { return _buffer.size();  }

        private:
            /**
             * @brief Writes the loaded file contents to an output stream.
             * @tparam OstreamT Output-stream type.
             * @param stream Destination stream.
             * @return Number of bytes written.
             */
            template <typename OstreamT>
            std::size_t write(OstreamT& stream) const {
                if (!_buffer.empty()) {
                    stream.write(_buffer.data(), _buffer.size());
                }
                return _buffer.size();
            }

        private:
            /**
             * @brief Loads the complete contents of a file.
             * @param path Path of the file to load.
             * @throws std::runtime_error If the file cannot be opened or read.
             */
            storage(const path_type& path): _path(path) {
                std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
                if (!file) {
                    throw std::runtime_error("Failed to open file: " + path.string());
                }

                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);

                _buffer.resize(size);
                if (!file.read(_buffer.data(), size)) {
                    throw std::runtime_error("Failed to read file: " + path.string());
                }

                if (file.gcount() != size) {
                    throw std::runtime_error("File read size mismatch for: " + path.string());
                }
            }

            /**
             * @brief Detects the MIME type of the loaded file contents.
             * @return Detected MIME type, or `application/octet-stream` when
             *         detection fails.
             */
            std::string mime() const {
                magic_t magic = magic_open(MAGIC_MIME_TYPE);
                if (magic_load(magic, nullptr) != 0) {
                    std::cerr << "Failed to load magic database: " << magic_error(magic) << std::endl;
                    magic_close(magic);
                    return "application/octet-stream"; // Default MIME type if error
                }

                const char* mime_type = magic_buffer(magic, _buffer.data(), _buffer.size());
                std::string result = mime_type ? mime_type : "application/octet-stream";
                magic_close(magic);
                return result;
            }

        private:
            path_type _path;
            std::vector<char> _buffer;
    };

    /**
     * @brief Unsupported non-owning disk-storage specialization.
     * @tparam Path Filesystem path type.
     */
    template <typename Path>
    struct storage<asset::source::disk<Path>, false>{
        static_assert("Non owned disk resource is not supported");
    };

    /**
     * @brief Non-owning remote resource storage.
     *
     * Stores a remote URL and writes an HTTP permanent-redirect response when
     * the resource is served.
     */
    template <>
    struct storage<asset::source::remote, false>{
        using source = asset::source::remote;
        static constexpr bool owned           = false;

        template <asset::type AssetType, typename Source, bool Owned>
        friend struct common_resource;

        /// @brief Returns the remote resource location.
        const std::string& location() const  { return _location; }

        private:
            /**
             * @brief Writes a permanent redirect response to the remote URL.
             * @tparam OstreamT Output-stream type.
             * @param stream Destination HTTP stream.
             * @return Size of the generated response message.
             */
            template <typename OstreamT>
            std::size_t write(OstreamT& stream) const {
                stream.status(boost::beast::http::status::moved_permanently);
                stream.set(boost::beast::http::field::location, _location);
                std::string message = "The resource has been moved permanently to " + _location;
                stream.write(message.c_str(), message.size());
                return message.size();
            }

        private:
            /**
             * @brief Constructs remote storage for a URL.
             * @param location Remote resource URL.
             */
            storage(const std::string& location): _location(location) { }

        private:
            std::string _location;
    };

    /// @brief Unsupported owned remote-storage specialization.
    template <>
    struct storage<asset::source::remote, true>{
        static_assert("Owned remore resource is not supported");
    };

    /**
     * @class abstract_resource
     * @brief Abstract base class representing a resource.
     *
     * This class provides an interface for all types of resources and is intended to be subclassed
     * with concrete implementations for specific resource types like memory, disk, or remote resources.
     */
    struct abstract_resource{
        /**
         * @brief Construct a new abstract resource object.
         * @param name The name of the resource.
         * @param type The type of the resource.
         * @param source Source from which the resource is obtained.
         * @param owned Indicates whether the resource is owned by this object.
         */
        inline abstract_resource(const std::string& name, asset::type type, asset::source::type source, bool owned): _name(name), _type(type), _source(source), _owned(owned) {}

        /**
         * @brief Get the name of the resource.
         * @return const std::string& The name of the resource.
         */
        inline const std::string& name() const { return _name; }

        /**
         * @brief Check if the resource is owned.
         * @return bool True if the resource is owned, false otherwise.
         */
        inline bool owned() const { return _owned; }

        /// @brief Returns the resource source category.
        inline asset::source::type source() const { return _source; }

        /// @brief Returns the asset type.
        inline asset::type type() const { return _type; }

        /**
         * @brief Write the resource to a given output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written to the stream.
         */
        inline virtual std::size_t write(udho::net::ostream_view& stream) const = 0;

        /**
         * @brief Writes only the resource body to an output stream.
         * @param stream Destination stream.
         * @return Number of body bytes written.
         */
        inline virtual std::size_t write_contents(udho::net::ostream_view& stream) const = 0;

        /// @brief Returns the resource MIME type, or an empty string if unset.
        inline virtual std::string mime() const { return ""; }

        /**
         * @brief Virtual destructor for abstract_resource.
         */
        inline virtual ~abstract_resource() {}

        private:
            std::string   _name;
            asset::type   _type;
            asset::source::type _source;
            bool          _owned;
    };

    using resource_ptr = std::unique_ptr<abstract_resource>;

    template <asset::type AssetType>
    struct basic_resource;

    /**
     * @brief Base policy for asset types; specializations provide additional
     *        presentation attributes.
     * @tparam AssetType Asset category.
     */
    template <asset::type AssetType>
    struct asset_policy{
        using basic_type = basic_resource<AssetType>;

        /// @brief Binds the policy to its resource.
        asset_policy(basic_type& res): _res(res) {}

        private:
            basic_type& _res;
    };

    /// @brief CSS-specific presentation policy.
    template <>
    struct asset_policy<asset::type::css>{
        using basic_type = basic_resource<asset::type::css>;

        /// @brief Constructs a CSS policy with media set to `all`.
        asset_policy(basic_type& res): _res(res), _media("all") {}

        /// @brief Returns the CSS media query value.
        const std::string& media() const { return _media; }

        /// @brief Sets the CSS media query value.
        basic_type& media(const std::string& m) { _media = m; return _res; }

        private:
            basic_type& _res;

            std::string _media;
    };

    /// @brief JavaScript-specific loading and embedding policy.
    template <>
    struct asset_policy<asset::type::js>{
        using basic_type = basic_resource<asset::type::js>;

        /// @brief Constructs a JavaScript policy with all flags disabled.
        asset_policy(basic_type& res): _res(res), _async(false), _defer(false), _module(false), _nomodule(false), _embedded(false) {}

        /// @brief Returns whether asynchronous loading is enabled.
        const bool& is_async() const { return _async; }

        /// @brief Enables or disables asynchronous loading.
        basic_type& is_async(const bool& flag) { _async = flag; return _res; }

        /// @brief Returns whether deferred loading is enabled.
        const bool& is_defer() const { return _defer; }

        /// @brief Enables or disables deferred loading.
        basic_type& is_defer(const bool& flag) { _defer = flag; return _res; }

        /// @brief Returns whether the script is an ECMAScript module.
        const bool& is_module() const { return _module; }

        /// @brief Enables or disables ECMAScript-module mode.
        basic_type& is_module(const bool& flag) { _module = flag; return _res; }

        /// @brief Returns whether the script uses the `nomodule` attribute.
        const bool& is_nomodule() const { return _nomodule; }

        /// @brief Enables or disables the `nomodule` attribute.
        basic_type& is_nomodule(const bool& flag) { _nomodule = flag; return _res; }

        /// @brief Returns the configured CORS mode.
        const std::string& cross_origin() const { return _cross_origin; }

        /// @brief Sets the CORS mode.
        basic_type& cross_origin(const std::string& v) { _cross_origin = v; return _res; }

        /// @brief Returns the configured referrer policy.
        const std::string& referrer_policy() const { return _referrer_policy; }

        /// @brief Sets the referrer policy.
        basic_type& referrer_policy(const std::string& v) { _referrer_policy = v; return _res; }

        /// @brief Returns whether the script should be embedded inline.
        bool embedded() const { return _embedded; }

        /// @brief Enables or disables inline embedding.
        basic_type& embedded(bool flag) { _embedded = flag; return _res; }

        private:
            basic_type& _res;

            bool _async;
            bool _defer;
            bool _module;
            bool _nomodule;
            bool _embedded;
            std::string _cross_origin;
            std::string _referrer_policy;
    };

    /**
     * @brief Common metadata and policy base for a typed asset resource.
     * @tparam AssetType Asset category represented by the resource.
     */
    template <asset::type AssetType>
    struct basic_resource: abstract_resource, asset_policy<AssetType>{
        using policy_type  = asset_policy<AssetType>;
        using basic_type   = basic_resource<AssetType>;

        /**
         * @brief Constructs metadata for a typed asset.
         * @param name Resource name.
         * @param source Resource source category.
         * @param owned Whether the resource owns its data.
         */
        basic_resource(const std::string& name, asset::source::type source, bool owned): abstract_resource(name, AssetType, source, owned), asset_policy<AssetType>(*this) {}


        /**
         * @brief Get the MIME type of the resource.
         * @return const std::string& The MIME type of the resource.
         */
        std::string mime() const override { return _mime; }

        /**
         * @brief Set the MIME type of the resource.
         * @param type The MIME type to set.
         * @return This resource.
         */
        basic_type& mime(const std::string& type) { _mime = type; return *this; }

        /// @brief Returns this resource as its concrete basic type.
        basic_type& self() { return *this; }

        /// @brief Returns the mutable type-specific asset policy.
        policy_type& policy() { return *this; }

        /// @brief Returns the type-specific asset policy.
        const policy_type& policy() const { return *this; }

        private:
            std::string _mime;
    };

    /**
     * @class common_resource
     * @brief Template class for basic resources handling specific types of asset sources.
     *
     * @tparam AssetType Asset category.
     * @tparam Source The source type of the resource.
     * @tparam Owned Flag indicating whether the resource is owned or not.
     */
    template <asset::type AssetType, typename Source, bool Owned>
    struct common_resource: basic_resource<AssetType>{
        using basic_type   = basic_resource<AssetType>;
        using storage_type = asset::storage<Source, Owned>;
        using self_type    = common_resource<AssetType, Source, Owned>;

        /**
         * @brief Construct a new basic resource object.
         *
         * @tparam Args Variadic template for constructor arguments.
         * @param name The name of the resource.
         * @param mime Mime type
         * @param args Arguments forwarded to the storage constructor.
         */
        template <typename... Args>
        common_resource(const std::string& name, const std::string& mime, Args&&... args): basic_resource<AssetType>(name, Source::source, Owned), _storage(std::forward<Args>(args)...) {
            if(mime.empty()){
                if (AssetType == asset::type::js) {
                    basic_type::mime("application/javascript");
                } else if (AssetType == asset::type::css) {
                    basic_type::mime("text/css");
                } else if (AssetType == asset::type::img) {
                    basic_type::mime(_storage.mime());
                } else {
                    basic_type::mime("application/octet-stream");
                }
            }
        }

        /**
         * @brief Get the storage associated with this resource.
         * @return const storage_type& A reference to the storage.
         */
        const storage_type& storage() const { return _storage; }

        /**
         * @brief Write the resource content to an output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written.
         */
        std::size_t write(udho::net::ostream_view& stream) const {
            stream.set(boost::beast::http::field::content_type, basic_type::mime());
            stream.set(boost::beast::http::field::content_length, std::to_string(_storage.size()));
            return write_contents(stream);
        }

        /**
         * @brief Writes the stored body without adding HTTP metadata.
         * @param stream Destination stream.
         * @return Number of body bytes written.
         */
        std::size_t write_contents(udho::net::ostream_view& stream) const {
            return _storage.write(stream);
        }

        private:
            storage_type _storage;
    };

    /**
     * @class common_resource
     * @brief Template class for basic resources handling url based remote resource.
     * @tparam AssetType Asset category.
     */
    template <asset::type AssetType>
    struct common_resource<AssetType, asset::source::remote, false>: basic_resource<AssetType>{
        using storage_type = asset::storage<asset::source::remote, false>;

        /**
         * @brief Construct a new basic resource object.
         *
         * @tparam Args Variadic template for constructor arguments.
         * @param name The name of the resource.
         * @param args Arguments forwarded to the storage constructor.
         */
        template <typename... Args>
        common_resource(const std::string& name, Args&&... args): basic_resource<AssetType>(name, asset::source::remote::source, false), _storage(std::forward<Args>(args)...) {}

        /// @brief Returns the remote storage descriptor.
        const storage_type& storage() const { return _storage; }

        /**
         * @brief Write the resource content to a given output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written.
         */
        std::size_t write(udho::net::ostream_view& stream) const {
            return _storage.write(stream);
        }

        /**
         * @brief Writes no local body for a remote resource.
         * @param stream Unused destination stream.
         * @return Always zero.
         */
        std::size_t write_contents(udho::net::ostream_view& stream) const {
            return 0;
        }

        private:
            storage_type _storage;
    };

    /**
     * @brief Function to create a resource object.
     *
     * This function creates a unique_ptr to an abstract_resource, managing memory resources, disk resources, or remote resources.
     *
     * @tparam Iterator Type of iterator (only valid for memory resources).
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param begin Iterator to the beginning of the resource data.
     * @param end Iterator to the end of the resource data.
     * @param mime String mime type.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<AssetType>* A raw pointer to the created resource.
     */
    template <asset::type AssetType, typename Iterator>
    inline std::unique_ptr<basic_resource<AssetType>> resource(const std::string& name, Iterator begin, Iterator end, const std::string& mime, bool owned = false) {
        if (owned) {
            return std::make_unique<common_resource<AssetType, asset::source::memory<Iterator>, true>>(name, mime, begin, end);
        } else {
            return std::make_unique<common_resource<AssetType, asset::source::memory<Iterator>, false>>(name, mime, begin, end);
        }
    }

    /**
     * @brief Function to create a resource object.
     *
     * This function creates a unique_ptr to an abstract_resource, managing memory resources, disk resources, or remote resources.
     *
     * @tparam Iterator Type of iterator (only valid for memory resources).
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param begin Iterator to the beginning of the resource data.
     * @param end Iterator to the end of the resource data.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<AssetType>* A raw pointer to the created resource.
     */
    template <asset::type AssetType, typename Iterator>
    inline std::unique_ptr<basic_resource<AssetType>> resource(const std::string& name, Iterator begin, Iterator end, bool owned = false) {
        if (owned) {
            return std::make_unique<common_resource<AssetType, asset::source::memory<Iterator>, true>>(name, "", begin, end);
        } else {
            return std::make_unique<common_resource<AssetType, asset::source::memory<Iterator>, false>>(name, "", begin, end);
        }
    }

    /**
     * @brief Overload of resource function for creating disk resources.
     *
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param path Path to the file
     * @param mime String mime type
     * @return basic_resource<AssetType>* A raw pointer to the created resource.
     */
    template <asset::type AssetType>
    inline std::unique_ptr<basic_resource<AssetType>> resource(const std::string& name, const boost::filesystem::path& path, const std::string& mime) {
        return std::make_unique<common_resource<AssetType, asset::source::disk<boost::filesystem::path>, true>>(name, mime, path);
    }

    /**
     * @brief Overload of resource function for creating disk resources.
     *
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param path Path to the file
     * @return basic_resource<AssetType>* A raw pointer to the created resource.
     */
    template <asset::type AssetType>
    inline std::unique_ptr<basic_resource<AssetType>> resource(const std::string& name, const boost::filesystem::path& path) {
        return std::make_unique<common_resource<AssetType, asset::source::disk<boost::filesystem::path>, true>>(name, "", path);
    }

    /**
     * @brief Overload of resource function for creating remote resources.
     *
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param url URL of the remote resource.
     * @return basic_resource<AssetType>* A raw pointer to the created resource.
     */
    template <asset::type AssetType>
    inline std::unique_ptr<basic_resource<AssetType>> resource(const std::string& name, const std::string& url) {
        return std::make_unique<common_resource<AssetType, asset::source::remote, false>>(name, url);
    }

    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Iterator Type of iterator.
     * @param name Name of the CSS resource.
     * @param begin Iterator to the beginning of the CSS data.
     * @param end Iterator to the end of the CSS data.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::css>* A raw pointer to the created resource.
     */
    template <typename Iterator>
    inline auto css(const std::string& name, Iterator begin, Iterator end, bool owned = false){ return resource<asset::type::css>(name, begin, end, owned); }


    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Char Type of character.
     * @param name Name of the CSS resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::css>* A raw pointer to the created resource.
     */
    template <typename Char>
    inline auto css(const std::string& name, const std::basic_string<Char>& str, bool owned = false){ return css(name, str.begin(), str.end(), owned); }

    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Char Type of character.
     * @param name Name of the CSS resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::css>* A raw pointer to the created resource.
     */
    template <typename Char>
    inline auto css(const std::string& name, const Char* str, bool owned = false){ return css(name, str, str + strlen(str), owned); }

    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Char Type of character.
     * @tparam N Number of characters.
     * @param name Name of the CSS resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::css>* A raw pointer to the created resource.
     */
    template <typename Char, std::size_t N>
    inline auto css(const std::string& name, const Char (&str)[N], bool owned = false) { return css(name, str, str + N - 1, owned); }


    /**
     * @brief Overload of css function for creating disk-based CSS resources.
     *
     * @param name Name of the CSS resource.
     * @param path Filesystem path to the CSS file.
     * @return basic_resource<asset::type::css>* A unique pointer to the created CSS resource.
     */
    inline auto css(const std::string& name, const boost::filesystem::path& path){ return resource<asset::type::css>(name, path); }
    /**
     * @brief Overload of css function for creating remote CSS resources.
     *
     * @param name Name of the CSS resource.
     * @param url URL of the remote CSS resource.
     * @return basic_resource<asset::type::css>* A raw pointer to the created resource.
     */
    inline auto css(const std::string& name, const std::string& url){ return resource<asset::type::css>(name, url); }

    /**
     * @brief Function to create a javascript resource.
     *
     * This function creates a unique_ptr to an abstract_resource for javascript assets,
     *
     * @tparam Iterator Type of iterator.
     * @param name Name of the javascript resource.
     * @param begin Iterator to the beginning of the javascript data.
     * @param end Iterator to the end of the javascript data.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::js>* A raw pointer to the created resource.
     */
    template <typename Iterator>
    inline auto js(const std::string& name, Iterator begin, Iterator end, bool owned = false){ return resource<asset::type::js>(name, begin, end, owned); }

    /**
     * @brief Function to create a javascript resource.
     *
     * This function creates a unique_ptr to an abstract_resource for javascript assets,
     *
     * @tparam Char Type of character.
     * @param name Name of the javascript resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::js>* A raw pointer to the created resource.
     */
    template <typename Char>
    inline auto js(const std::string& name, const Char* str, bool owned = false){ return js(name, str, str + strlen(str), owned); }

    /**
     * @brief Function to create a javascript resource.
     *
     * This function creates a unique_ptr to an abstract_resource for javascript assets,
     *
     * @tparam Char Type of character.
     * @tparam N Number of characters.
     * @param name Name of the javascript resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::js>* A raw pointer to the created resource.
     */
    template <typename Char, std::size_t N>
    inline auto js(const std::string& name, const Char (&str)[N], bool owned = false) { return js(name, str, str + N - 1, owned); }

    /**
     * @brief Overload of js function for creating disk-based JS resources.
     *
     * @param name Name of the JS resource.
     * @param path Filesystem path to the JS file.
     * @return basic_resource<asset::type::js>* A unique pointer to the created JS resource.
     */
    inline auto js(const std::string& name, const boost::filesystem::path& path){ return resource<asset::type::js>(name, path); }
    /**
     * @brief Overload of js function for creating remote JS resources.
     *
     * @param name Name of the JS resource.
     * @param url URL of the remote JS resource.
     * @return basic_resource<asset::type::js>* A unique pointer to the created JS resource.
     */
    inline auto js(const std::string& name, const std::string& url){ return resource<asset::type::js>(name, url); }

    /**
     * @brief Function to create a image resource.
     *
     * This function creates a unique_ptr to an abstract_resource for image assets.
     *
     * @tparam Iterator Type of iterator.
     * @param name Name of the jimage script resource.
     * @param begin Iterator to the beginning of the image data.
     * @param end Iterator to the end of the image data.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return basic_resource<asset::type::img>* A raw pointer to the created resource.
     */
    template <typename Iterator>
    inline auto img(const std::string& name, Iterator begin, Iterator end, bool owned = false){ return resource<asset::type::img>(name, begin, end, owned); }
    /**
     * @brief Overload of img function for creating disk-based image resources.
     *
     * @param name Name of the image resource.
     * @param path Filesystem path to the image file.
     * @return basic_resource<asset::type::img>* A unique pointer to the created image resource.
     */
    inline auto img(const std::string& name, const boost::filesystem::path& path){ return resource<asset::type::img>(name, path); }
    /**
     * @brief Overload of img function for creating remote image resources.
     *
     * @param name Name of the image resource.
     * @param url URL of the remote image resource.
     * @return basic_resource<asset::type::img>* A unique pointer to the created image resource.
     */
    inline auto img(const std::string& name, const std::string& url){ return resource<asset::type::img>(name, url); }

}



/** @} */

}
}
}

#endif // UDHO_VIEW_RESOURCES_RESOURCE_H
