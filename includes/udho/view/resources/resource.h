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

#include <string>
#include <fstream>
#include <boost/filesystem/path.hpp>
#include <udho/url/detail/format.h>
#include <boost/iostreams/device/mapped_file.hpp>

namespace udho{
namespace view{
namespace resources{

/**
 * @enum type
 * @brief Enumerates different types of resources managed within the application.
 */
enum class type {
    none, ///< No specific type.
    view, ///< Resource is a view.
    asset ///< Resource is an asset.
};

/**
 * @struct resource_info
 * @brief Holds metadata about a resource.
 *
 * This structure encapsulates details about a resource such as its type and name.
 */
struct resource_info{
    /**
     * @brief Construct a new resource_info object with specified type and name.
     * @param type Type of the resource.
     * @param name Name of the resource.
     */
    inline explicit resource_info(udho::view::resources::type type, const std::string& name): _type(type), _name(name) {}
    /**
     * @brief Returns the type of the resource.
     * @return Type of the resource as udho::view::resources::type.
     */
    inline udho::view::resources::type type() const { return _type; }
    /**
     * @brief Returns the name of the resource.
     * @return Name of the resource as a std::string.
     */
    inline std::string name() const { return _name; }
    private:
        udho::view::resources::type _type;
        std::string _name;
};

/**
 * @struct resource_buffer
 * @brief Represents an on-memory resource.
 *
 * @tparam R The type of the resource, as defined in the type enum.
 * @tparam IteratorT The type of the iterator for navigating the resource content.
 */
template <resources::type R, typename IteratorT>
struct resource_buffer{
    static constexpr udho::view::resources::type type = R;
    using iterator_type = IteratorT;

    /**
     * @brief Constructs a resource buffer with named content spanning specified iterators.
     * @param name The name of the resource.
     * @param begin Iterator pointing to the beginning of the resource content.
     * @param end Iterator pointing to the end of the resource content.
     */
    resource_buffer(const std::string& name, iterator_type begin, iterator_type end): _name(name), _begin(begin), _end(end) {}

    /**
     * @brief Returns the name of the resource.
     * @return The name of the resource.
     */
    std::string name() const { return _name; }
    /**
     * @brief Gets an iterator to the beginning of the resource content.
     * @return An iterator to the start.
     */
    iterator_type begin() const { return _begin; }
    /**
     * @brief Gets an iterator to the end of the resource content.
     * @return An iterator to the end.
     */
    iterator_type end() const { return _end; }

    /**
     * @brief Generates resource metadata.
     * @return A resource_info object encapsulating details about the resource.
     */
    resource_info info() const {
        return resource_info{type, _name};
    }

    private:
        std::string _name; ///< The name of the resource.
        iterator_type _begin, _end;
};

/**
 * @struct resource_file
 * @brief Represents a file-based resource.
 *
 * @tparam R The type of the resource, as defined in the type enum.
 */
template <type R>
struct resource_file{
    static constexpr udho::view::resources::type type = R;
    using iterator_type = const char*;

    /**
     * @brief Constructs a resource file object.
     * @param name The name of the resource.
     * @param path The filesystem path to the resource file.
     */
    template <typename PathT = boost::filesystem::path>
    resource_file(const std::string& name, const PathT& path): _name(name), _mmap(path.c_str(), boost::iostreams::mapped_file::readonly){}

    /**
     * @brief Returns the name of the resource.
     * @return The name of the resource.
     */
    std::string name() const { return _name; }
    /**
     * @brief Gets an iterator to the beginning of the file content.
     * @return An iterator to the start of the mapped file data.
     */
    iterator_type begin() const { return _mmap.const_data(); }
    /**
     * @brief Gets an iterator to the end of the file content.
     * @return An iterator to the end of the mapped file data.
     */
    iterator_type end() const { return begin() + _mmap.size(); }

    /**
     * @brief Generates resource metadata.
     * @return A resource_info object encapsulating details about the resource.
     */
    resource_info info() const {
        return resource_info{type, _name};
    }

    private:
        std::string _name; ///< The name of the resource.
        boost::iostreams::mapped_file _mmap;
};

// template <type R, typename IteratorT>
// resource_buffer<R, IteratorT> resource(const std::string& name, IteratorT begin, IteratorT end){
//     return resource_buffer<R, IteratorT>{name, begin, end};
// }
//
// template <type R, typename PathT = boost::filesystem::path>
// resource_file<R> resource(const std::string& name, const PathT& path){
//     return resource_file<R>{name, path};
// }

namespace resource{
    /**
    * @brief Creates a resource_buffer instance for a view resource.
    * @details Constructs a view-type resource buffer by specifying its name and the range it occupies in memory.
    *
    * @tparam IteratorT The type of the iterator for navigating the resource content.
    * @param name The name of the view resource.
    * @param begin Iterator pointing to the beginning of the view content.
    * @param end Iterator pointing to the end of the view content.
    * @return A resource_buffer object configured as a view.
    */
    template <typename IteratorT>
    static resource_buffer<udho::view::resources::type::view, IteratorT> view(const std::string& name, IteratorT begin, IteratorT end){
        return resource_buffer<type::view, IteratorT>{name, begin, end};
    }

    /**
    * @brief Creates a resource_buffer instance for an asset resource.
    * @details Constructs an asset-type resource buffer by specifying its name and the range it occupies in memory.
    *
    * @tparam IteratorT The type of the iterator for navigating the resource content.
    * @param name The name of the asset resource.
    * @param begin Iterator pointing to the beginning of the asset content.
    * @param end Iterator pointing to the end of the asset content.
    * @return A resource_buffer object configured as an asset.
    */
    template <typename IteratorT>
    static resource_buffer<udho::view::resources::type::asset, IteratorT> asset(const std::string& name, IteratorT begin, IteratorT end){
        return resource_buffer<type::asset, IteratorT>{name, begin, end};
    }

    /**
    * @brief Creates a resource_file instance for a view resource from a file path.
    * @details Constructs a view-type resource file by specifying its name and the file system path.
    *
    * @tparam PathT The type of the file path, defaults to boost::filesystem::path.
    * @param name The name of the view resource.
    * @param path File system path to the resource.
    * @return A resource_file object configured as a view.
    */
    template <typename PathT = boost::filesystem::path>
    static resource_file<udho::view::resources::type::view> view(const std::string& name, const PathT& path){
        return resource_file<type::view>{name, path};
    }

    /**
    * @brief Creates a resource_file instance for an asset resource from a file path.
    * @details Constructs an asset-type resource file by specifying its name and the file system path.
    *
    * @tparam PathT The type of the file path, defaults to boost::filesystem::path.
    * @param name The name of the asset resource.
    * @param path File system path to the resource.
    * @return A resource_file object configured as an asset.
    */
    template <typename PathT = boost::filesystem::path>
    static resource_file<udho::view::resources::type::asset> asset(const std::string& name, PathT& path){
        return resource_file<type::asset>{name, path};
    }

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_RESOURCE_H
