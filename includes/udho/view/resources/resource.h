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

}

namespace asset{
    template <typename Source, bool Owned>
    struct storage;

    template <typename Source, bool Owned>
    struct basic_resource;

    template <typename Iterator>
    struct storage<asset::source::memory<Iterator>, true>{
        using source = asset::source::memory<Iterator>;
        static constexpr bool owned           = true;

        template <typename Source, bool Owned>
        friend struct basic_resource;

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

        template <typename Source, bool Owned>
        friend struct basic_resource;

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
                if (magic_load(magic, nullptr) != 0) {
                    std::cerr << "Failed to load magic database: " << magic_error(magic) << std::endl;
                    magic_close(magic);
                    return "application/octet-stream"; // Default MIME type if error
                }

                std::vector<char> buffer;
                buffer.reserve(_size);
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

        template <typename Source, bool Owned>
        friend struct basic_resource;

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

        template <typename Source, bool Owned>
        friend struct basic_resource;

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
        inline abstract_resource(const std::string& name, asset::type type, bool owned): _name(name), _type(type), _owned(owned) {}

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

        asset::type type() const { return _type; }

        /**
         * @brief Write the resource to a given output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written to the stream.
         */
        inline virtual std::size_t write(udho::net::stream& stream) const = 0;

        /**
         * @brief Virtual destructor for abstract_resource.
         */
        inline virtual ~abstract_resource() {}

        private:
            std::string   _name;
            asset::type   _type;
            bool          _owned;
    };

    using resource_ptr = std::unique_ptr<abstract_resource>;

    /**
     * @class basic_resource
     * @brief Template class for basic resources handling specific types of asset sources.
     *
     * @tparam Source The source type of the resource.
     * @tparam Owned Flag indicating whether the resource is owned or not.
     */
    template <typename Source, bool Owned>
    struct basic_resource: abstract_resource{
        using storage_type = asset::storage<Source, Owned>;

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
        basic_resource(const std::string& name, asset::type type, const std::string mime, Args&&... args): abstract_resource(name, type, Owned), _mime(mime), _storage(std::forward<Args>(args)...) {
            if(mime.empty()){
                if (type == asset::type::js) {
                    _mime = "application/javascript";
                } else if (type == asset::type::css) {
                    _mime = "text/css";
                } else if (type == asset::type::img) {
                    _mime = _storage.mime();
                } else {
                    _mime = "application/octet-stream";
                }
            }
        }

        /**
         * @brief Construct a new basic resource object.
         *
         * @tparam Args Variadic template for constructor arguments.
         * @param name The name of the resource.
         * @param type The type of the resource.
         * @param args Arguments forwarded to the storage constructor.
         */
        // template <typename... Args>
        // basic_resource(const std::string& name, asset::type type, Args&&... args): abstract_resource(name, type, Owned), _storage(std::forward<Args>(args)...) {
        //     if (type == asset::type::js) {
        //         _mime = "application/javascript";
        //     } else if (type == asset::type::css) {
        //         _mime = "text/css";
        //     } else if (type == asset::type::img) {
        //         _mime = _storage.mime();
        //     } else {
        //         _mime = "application/octet-stream";
        //     }
        // }


        /**
         * @brief Get the MIME type of the resource.
         * @return const std::string& The MIME type of the resource.
         */
        const std::string& mime() const { return _mime; }

        /**
         * @brief Set the MIME type of the resource.
         * @param type The MIME type to set.
         */
        void mime(const std::string type) { _mime = type; }

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
            stream.set(boost::beast::http::field::content_type, mime());
            stream.set(boost::beast::http::field::content_length, std::to_string(_storage.size()));
            return _storage.write(stream);
        }

        private:
            std::string  _mime;
            storage_type _storage;
    };

    /**
     * @class basic_resource
     * @brief Template class for basic resources handling url based remote resource.
     */
    template <>
    struct basic_resource<asset::source::remote, false>: abstract_resource{
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
        basic_resource(const std::string& name, asset::type type, Args&&... args): abstract_resource(name, type, false), _storage(std::forward<Args>(args)...) {}

        const storage_type& storage() const { return _storage; }

        /**
         * @brief Write the resource content to a given output stream.
         * @param stream The stream to write to.
         * @return std::size_t The number of bytes written.
         */
        std::size_t write(udho::net::stream& stream) const {
            return _storage.write(stream);
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
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Iterator>
    inline std::unique_ptr<abstract_resource> resource(const std::string& name, asset::type type, Iterator begin, Iterator end, const std::string& mime, bool owned) {
        if (owned) {
            return std::make_unique<basic_resource<asset::source::memory<Iterator>, true>>(name, type, mime, begin, end);
        } else {
            return std::make_unique<basic_resource<asset::source::memory<Iterator>, false>>(name, type, mime, begin, end);
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
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Iterator>
    inline std::unique_ptr<abstract_resource> resource(const std::string& name, asset::type type, Iterator begin, Iterator end, bool owned) {
        if (owned) {
            return std::make_unique<basic_resource<asset::source::memory<Iterator>, true>>(name, type, "", begin, end);
        } else {
            return std::make_unique<basic_resource<asset::source::memory<Iterator>, false>>(name, type, "", begin, end);
        }
    }

    /**
     * @brief Overload of resource function for creating disk resources.
     *
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param path Path to the file
     * @param mime String mime type
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    inline std::unique_ptr<abstract_resource> resource(const std::string& name, asset::type type, const boost::filesystem::path& path, const std::string& mime) {
        return std::make_unique<basic_resource<asset::source::disk<boost::filesystem::path>, true>>(name, type, mime, path);
    }

    /**
     * @brief Overload of resource function for creating disk resources.
     *
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param path Path to the file
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    inline std::unique_ptr<abstract_resource> resource(const std::string& name, asset::type type, const boost::filesystem::path& path) {
        return std::make_unique<basic_resource<asset::source::disk<boost::filesystem::path>, true>>(name, type, "", path);
    }

    /**
     * @brief Overload of resource function for creating remote resources.
     *
     * @param name Name of the resource.
     * @param type The type of the resource.
     * @param url URL of the remote resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    inline std::unique_ptr<abstract_resource> resource(const std::string& name, asset::type type, const std::string& url) {
        return std::make_unique<basic_resource<asset::source::remote, false>>(name, type, url);
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
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Iterator>
    inline std::unique_ptr<abstract_resource> css(const std::string& name, Iterator begin, Iterator end, bool owned = false){ return resource(name, asset::type::css, begin, end, owned); }


    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Char Type of character.
     * @param name Name of the CSS resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Char>
    inline std::unique_ptr<abstract_resource> css(const std::string& name, const std::basic_string<Char>& str, bool owned = false){ return css(name, str.begin(), str.end(), owned); }

    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Char Type of character.
     * @param name Name of the CSS resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Char>
    inline std::unique_ptr<abstract_resource> css(const std::string& name, const Char* str, bool owned = false){ return css(name, str, str + strlen(str), owned); }

    /**
     * @brief Function to create a CSS resource.
     *
     * This function creates a unique_ptr to an abstract_resource for CSS assets,
     *
     * @tparam Char Type of character.
     * @tparam N Number of characters.
     * @param name Name of the CSS resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Char, std::size_t N>
    inline std::unique_ptr<abstract_resource> css(const std::string& name, const Char (&str)[N], bool owned = false) { return css(name, str, str + N - 1, owned); }


    /**
     * @brief Overload of css function for creating disk-based CSS resources.
     *
     * @param name Name of the CSS resource.
     * @param path Filesystem path to the CSS file.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created CSS resource.
     */
    inline std::unique_ptr<abstract_resource> css(const std::string& name, const boost::filesystem::path& path){ return resource(name, asset::type::css, path); }
    /**
     * @brief Overload of css function for creating remote CSS resources.
     *
     * @param name Name of the CSS resource.
     * @param url URL of the remote CSS resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created CSS resource.
     */
    inline std::unique_ptr<abstract_resource> css(const std::string& name, const std::string& url){ return resource(name, asset::type::css, url); }

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
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Iterator>
    inline std::unique_ptr<abstract_resource> js(const std::string& name, Iterator begin, Iterator end, bool owned = false){ return resource(name, asset::type::js, begin, end, owned); }

    /**
     * @brief Function to create a javascript resource.
     *
     * This function creates a unique_ptr to an abstract_resource for javascript assets,
     *
     * @tparam Char Type of character.
     * @param name Name of the javascript resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Char>
    inline std::unique_ptr<abstract_resource> js(const std::string& name, const Char* str, bool owned = false){ return js(name, str, str + strlen(str), owned); }

    /**
     * @brief Function to create a javascript resource.
     *
     * This function creates a unique_ptr to an abstract_resource for javascript assets,
     *
     * @tparam Char Type of character.
     * @tparam N Number of characters.
     * @param name Name of the javascript resource.
     * @param owned Boolean flag indicating ownership of the resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Char, std::size_t N>
    inline std::unique_ptr<abstract_resource> js(const std::string& name, const Char (&str)[N], bool owned = false) { return js(name, str, str + N - 1, owned); }

    /**
     * @brief Overload of js function for creating disk-based JS resources.
     *
     * @param name Name of the JS resource.
     * @param path Filesystem path to the JS file.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created JS resource.
     */
    inline std::unique_ptr<abstract_resource> js(const std::string& name, const boost::filesystem::path& path){ return resource(name, asset::type::js, path); }
    /**
     * @brief Overload of js function for creating remote JS resources.
     *
     * @param name Name of the JS resource.
     * @param url URL of the remote JS resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created JS resource.
     */
    inline std::unique_ptr<abstract_resource> js(const std::string& name, const std::string& url){ return resource(name, asset::type::js, url); }

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
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created resource.
     */
    template <typename Iterator>
    inline std::unique_ptr<abstract_resource> img(const std::string& name, Iterator begin, Iterator end, bool owned = false){ return resource(name, asset::type::img, begin, end, owned); }
    /**
     * @brief Overload of img function for creating disk-based image resources.
     *
     * @param name Name of the image resource.
     * @param path Filesystem path to the image file.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created image resource.
     */
    inline std::unique_ptr<abstract_resource> img(const std::string& name, const boost::filesystem::path& path){ return resource(name, asset::type::img, path); }
    /**
     * @brief Overload of img function for creating remote image resources.
     *
     * @param name Name of the image resource.
     * @param url URL of the remote image resource.
     * @return std::unique_ptr<abstract_resource> A unique pointer to the created image resource.
     */
    inline std::unique_ptr<abstract_resource> img(const std::string& name, const std::string& url){ return resource(name, asset::type::img, url); }

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_RESOURCE_H
