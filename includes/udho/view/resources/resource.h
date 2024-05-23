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
#include <boost/filesystem/path.hpp>
#include <udho/url/detail/format.h>

namespace udho{
namespace view{
namespace resources{

/**
 * @brief Represents a resource with a specific type, name, and path.
 */
struct resource {
    /**
     * @brief The type of the resource.
     */
    enum class resource_type {
        none, ///< No specific type.
        view, ///< Resource is a view.
        asset ///< Resource is an asset.
    };

    /**
     * @brief Constructs a resource with the specified type, name, and path.
     * @param type The type of the resource.
     * @param name The name of the resource.
     * @param path The path to the resource.
     */
    inline resource(resource_type type, const std::string& name, const boost::filesystem::path& path): _type(type), _name(name), _path(path) {}

    /**
     * @brief Constructs a resource with the specified type and path. Derives the name from the filename of the path.
     * @param type The type of the resource.
     * @param path The path to the resource.
     */
    inline resource(resource_type type, const boost::filesystem::path& path): _type(type), _name(path.filename().string()), _path(path) {}

    /**
     * @brief Returns the type of the resource.
     * @return The type of the resource.
     */
    inline resource_type type() const { return _type; }

    inline bool is_view() const { return _type == resource_type::view; }
    inline bool is_asset() const { return _type == resource_type::asset; }

    /**
     * @brief Returns the name of the resource.
     * @return The name of the resource.
     */
    inline std::string name() const { return _name; }

    /**
     * @brief Returns the path to the resource.
     * @return The path to the resource.
     */
    inline const boost::filesystem::path& path() const { return _path; }

    /**
     * @brief Creates and returns a view resource with the specified name and path.
     * @param name The name of the view resource.
     * @param path The path to the view resource.
     * @return A view resource.
     */
    inline static resource view(const std::string& name, const boost::filesystem::path& path){
        return resource{resource_type::view, name, path};
    }

    /**
     * @brief Creates and returns an asset resource with the specified name and path.
     * @param name The name of the asset resource.
     * @param path The path to the asset resource.
     * @return An asset resource.
     */
    inline static resource asset(const std::string& name, const boost::filesystem::path& path){
        return resource{resource_type::asset, name, path};
    }

    /**
     * @brief Creates and returns a view resource with the specified path.
     * Derives the name from the filename of the path.
     * @param path The path to the view resource.
     * @return A view resource.
     */
    inline static resource view(const boost::filesystem::path& path){
        return resource{resource_type::view, path};
    }

    /**
     * @brief Creates and returns an asset resource with the specified path.
     * Derives the name from the filename of the path.
     * @param path The path to the asset resource.
     * @return An asset resource.
     */
    inline static resource asset(const boost::filesystem::path& path){
        return resource{resource_type::asset, path};
    }

    inline bool operator<(const resource& other) const {
        return _name < other._name;
    }

    private:
        resource_type _type; ///< The type of the resource.
        std::string _name; ///< The name of the resource.
        boost::filesystem::path _path; ///< The path to the resource.
};

}
}
}

#endif // UDHO_VIEW_RESOURCES_RESOURCE_H
