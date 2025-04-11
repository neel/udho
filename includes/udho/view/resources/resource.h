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
#include <udho/net/stream.h>

namespace udho{
namespace view{
namespace resources{

namespace tmpl{
    /**
     * @brief a view template written in a foreign language
     * @ingroup view
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

        const tmpl::resource& resource() const { return _res;}
        tmpl::resource& resource() { return _res;}
        private:
            tmpl::resource _res;

    };
}

namespace asset{
    template <typename Source, bool Owned>
    struct storage;

    template <asset::type AssetType, typename Source, bool Owned>
    struct common_resource;

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

        iterator_type begin() const { return _buffer.begin(); }
        iterator_type end()   const { return _buffer.end();   }
        size_type     size()  const { return _buffer.size();  }


        private:
            std::size_t write(udho::net::stream& stream) const {
                if (!_buffer.empty()) {
                    stream.write(_buffer.data(), _buffer.size());
                }
                return _buffer.size();
            }

        private:
            storage(Iterator begin, Iterator end) {
                auto size = std::distance(begin, end);
                _buffer.reserve(size);
                std::copy(begin, end, _buffer.begin());
            }
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

    template <typename Iterator>
    struct storage<asset::source::memory<Iterator>, false>{
        using source = asset::source::memory<Iterator>;
        static constexpr bool owned           = false;

        template <asset::type AssetType, typename Source, bool Owned>
        friend struct common_resource;

        using iterator_type = Iterator;
        using value_type    = typename std::iterator_traits<Iterator>::value_type;
        using size_type     = std::size_t;

        iterator_type begin() const { return _begin; }
        iterator_type end()   const { return _end;   }
        size_type     size()  const { return _size;  }

        private:
            std::size_t write(udho::net::stream& stream) const {
                if (_size > 0) {
                    stream.write(_begin, _end);
                }
                return _size;
            }

        private:
            storage(iterator_type begin, iterator_type end): _begin(begin), _end(end), _size(std::distance(begin, end)) { }
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

                std::vector<unsigned char> buffer;
                buffer.resize(_size);
                std::copy(_begin, _end, buffer.begin());
                const char* mime_type = magic_buffer(magic, buffer.data(), buffer.size());
                std::string result = mime_type ? mime_type : "application/octet-stream";
                magic_close(magic);
                return result;
            }
        private:
            iterator_type _begin;
            iterator_type _end;
            size_type     _size;
    };

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

        const boost::filesystem::path& path() const { return _path; }

        iterator_type begin() const { return _buffer.begin(); }
        iterator_type end()   const { return _buffer.end();   }
        size_type     size()  const { return _buffer.size();  }

        private:
            std::size_t write(udho::net::stream& stream) const {
                if (!_buffer.empty()) {
                    stream.write(_buffer.data(), _buffer.size());
                }
                return _buffer.size();
            }

        private:
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

    template <typename Path>
    struct storage<asset::source::disk<Path>, false>{
        static_assert("Non owned disk resource is not supported");
    };

    template <>
    struct storage<asset::source::remote, false>{
        using source = asset::source::remote;
        static constexpr bool owned           = false;

        template <asset::type AssetType, typename Source, bool Owned>
        friend struct common_resource;

        const std::string& location() const  { return _location; }

        private:
            std::size_t write(udho::net::stream& stream) const {
                stream.response().result(boost::beast::http::status::moved_permanently);
                stream.set(boost::beast::http::field::location, _location);
                std::string message = "The resource has been moved permanently to " + _location;
                stream.write(message.c_str(), message.size());
                return message.size();
            }

        private:
            storage(const std::string& location): _location(location) { }

        private:
            std::string _location;
    };

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
        asset::source::type source() const { return _source; }
        asset::type type() const { return _type; }

        /**
         * @brief Write the resource to a given output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written to the stream.
         */
        inline virtual std::size_t write(udho::net::stream& stream) const = 0;
        inline virtual std::size_t write_contents(udho::net::stream& stream) const = 0;
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

    template <asset::type AssetType>
    struct asset_policy{
        using basic_type = basic_resource<AssetType>;

        asset_policy(basic_type& res): _res(res) {}

        private:
            basic_type& _res;
    };

    template <>
    struct asset_policy<asset::type::css>{
        using basic_type = basic_resource<asset::type::css>;

        asset_policy(basic_type& res): _res(res), _media("all") {}

        const std::string& media() const { return _media; }
        basic_type& media(const std::string& m) { _media = m; return _res; }

        private:
            basic_type& _res;

            std::string _media;
    };

    template <>
    struct asset_policy<asset::type::js>{
        using basic_type = basic_resource<asset::type::js>;

        asset_policy(basic_type& res): _res(res), _async(false), _defer(false), _nomodule(false) {}

        const bool& is_async() const { return _async; }
        basic_type& is_async(const bool& flag) { _async = flag; return _res; }

        const bool& is_defer() const { return _defer; }
        basic_type& is_defer(const bool& flag) { _defer = flag; return _res; }

        const bool& is_module() const { return _module; }
        basic_type& is_module(const bool& flag) { _module = flag; return _res; }

        const bool& is_nomodule() const { return _nomodule; }
        basic_type& is_nomodule(const bool& flag) { _nomodule = flag; return _res; }

        const std::string& cross_origin() const { return _cross_origin; }
        basic_type& cross_origin(const std::string& v) { _cross_origin = v; return _res; }

        const std::string& referrer_policy() const { return _referrer_policy; }
        basic_type& referrer_policy(const std::string& v) { _referrer_policy = v; return _res; }

        private:
            basic_type& _res;

            bool _async;
            bool _defer;
            bool _module;
            bool _nomodule;
            std::string _cross_origin;
            std::string _referrer_policy;
    };

    template <asset::type AssetType>
    struct basic_resource: abstract_resource, asset_policy<AssetType>{
        using policy_type  = asset_policy<AssetType>;
        using basic_type   = basic_resource<AssetType>;

        basic_resource(const std::string& name, asset::source::type source, bool owned): abstract_resource(name, AssetType, source, owned), asset_policy<AssetType>(*this) {}


        /**
         * @brief Get the MIME type of the resource.
         * @return const std::string& The MIME type of the resource.
         */
        std::string mime() const override { return _mime; }

        /**
         * @brief Set the MIME type of the resource.
         * @param type The MIME type to set.
         */
        basic_type& mime(const std::string& type) { _mime = type; return *this; }

        basic_type& self() { return *this; }

        policy_type& policy() { return *this; }
        const policy_type& policy() const { return *this; }

        private:
            std::string _mime;
    };

    /**
     * @class common_resource
     * @brief Template class for basic resources handling specific types of asset sources.
     *
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
         * @param type The type of the resource.
         * @param mime Mime type
         * @param args Arguments forwarded to the storage constructor.
         */
        template <typename... Args>
        common_resource(const std::string& name, const std::string mime, Args&&... args): basic_resource<AssetType>(name, Source::source, Owned), _storage(std::forward<Args>(args)...) {
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
        std::size_t write(udho::net::stream& stream) const {
            stream.set(boost::beast::http::field::content_type, basic_type::mime());
            stream.set(boost::beast::http::field::content_length, std::to_string(_storage.size()));
            return write_contents(stream);
        }

        std::size_t write_contents(udho::net::stream& stream) const {
            return _storage.write(stream);
        }

        private:
            storage_type _storage;
    };

    /**
     * @class common_resource
     * @brief Template class for basic resources handling url based remote resource.
     */
    template <asset::type AssetType>
    struct common_resource<AssetType, asset::source::remote, false>: basic_resource<AssetType>{
        using storage_type = asset::storage<asset::source::remote, false>;

        /**
         * @brief Construct a new basic resource object.
         *
         * @tparam Args Variadic template for constructor arguments.
         * @param name The name of the resource.
         * @param type The type of the resource.
         * @param args Arguments forwarded to the storage constructor.
         */
        template <typename... Args>
        common_resource(const std::string& name, Args&&... args): basic_resource<AssetType>(name, asset::source::remote::source, false), _storage(std::forward<Args>(args)...) {}

        const storage_type& storage() const { return _storage; }

        /**
         * @brief Write the resource content to a given output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written.
         */
        std::size_t write(udho::net::stream& stream) const {
            return _storage.write(stream);
        }
        std::size_t write_contents(udho::net::stream& stream) const {
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
    inline basic_resource<AssetType>* resource(const std::string& name, Iterator begin, Iterator end, const std::string& mime, bool owned = false) {
        basic_resource<AssetType>* res = 0x0;
        if (owned) {
            res = new common_resource<AssetType, asset::source::memory<Iterator>, true>(name, mime, begin, end);
        } else {
            res = new common_resource<AssetType, asset::source::memory<Iterator>, false>(name, mime, begin, end);
        }
        return res;
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
    inline basic_resource<AssetType>* resource(const std::string& name, Iterator begin, Iterator end, bool owned = false) {
        basic_resource<AssetType>* res = 0x0;
        if (owned) {
            res = new common_resource<AssetType, asset::source::memory<Iterator>, true>(name, "", begin, end);
        } else {
            res = new common_resource<AssetType, asset::source::memory<Iterator>, false>(name, "", begin, end);
        }
        return res;
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
    inline basic_resource<AssetType>* resource(const std::string& name, const boost::filesystem::path& path, const std::string& mime) {
        basic_resource<AssetType>* res = new common_resource<AssetType, asset::source::disk<boost::filesystem::path>, true>(name, mime, path);
        return res;
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
    inline basic_resource<AssetType>* resource(const std::string& name, const boost::filesystem::path& path) {
        basic_resource<AssetType>* res = new common_resource<AssetType, asset::source::disk<boost::filesystem::path>, true>(name, "", path);
        return res;
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
    inline basic_resource<AssetType>* resource(const std::string& name, const std::string& url) {
        basic_resource<AssetType>* res = new common_resource<AssetType, asset::source::remote, false>(name, url);
        return res;
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



}
}
}

#endif // UDHO_VIEW_RESOURCES_RESOURCE_H
