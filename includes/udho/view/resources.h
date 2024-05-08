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

#ifndef UDHO_VIEW_RESOURCES_H
#define UDHO_VIEW_RESOURCES_H

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>


namespace udho{
namespace view{
namespace data{

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

struct bundle{
    struct tags{
        struct type{};
        struct name{};
    };

    using resource_set = boost::multi_index_container<
        resource,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::identity<resource>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<tags::name>,
                boost::multi_index::const_mem_fun<resource, std::string, &resource::name>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<tags::type>,
                boost::multi_index::const_mem_fun<resource, resource::resource_type, &resource::type>
            >
        >
    >;

    inline explicit bundle(const std::string& prefix): _prefix(prefix) {}

    inline void add(const resource& res) { _resources.insert(res); }
    inline resource_set::index<tags::type>::type::iterator lower(resource::resource_type type) { return _resources.get<tags::type>().lower_bound(type); }
    inline resource_set::index<tags::type>::type::iterator upper(resource::resource_type type) { return _resources.get<tags::type>().upper_bound(type); }
    inline resource_set::index<tags::type>::type::iterator find(resource::resource_type type) { return _resources.get<tags::type>().find(type); }
    inline resource_set::index<tags::type>::type::size_type count(resource::resource_type type) { return _resources.get<tags::type>().count(type); }

    inline resource_set::index<tags::type>::type::iterator lower(resource::resource_type type) const { return _resources.get<tags::type>().lower_bound(type); }
    inline resource_set::index<tags::type>::type::iterator upper(resource::resource_type type) const { return _resources.get<tags::type>().upper_bound(type); }
    inline resource_set::index<tags::type>::type::iterator find(resource::resource_type type) const { return _resources.get<tags::type>().find(type); }
    inline resource_set::index<tags::type>::type::size_type count(resource::resource_type type) const { return _resources.get<tags::type>().count(type); }

    inline resource_set::iterator begin() { return _resources.begin(); }
    inline resource_set::iterator end() { return _resources.end(); }

    inline resource_set::iterator begin() const { return _resources.begin(); }
    inline resource_set::iterator end() const { return _resources.end(); }

    inline resource_set::size_type size() const { return _resources.size(); }

    inline std::string prefix() const { return _prefix; }

    inline const resource& operator[](const std::string& name){
        auto& name_index = _resources.get<tags::name>();
        auto it = name_index.find(name);
        if (it != name_index.end()) {
            return *it;
        } else {
            throw std::out_of_range("Resource with name '" + name + "' not found");
        }
    }

    private:
        std::string  _prefix;
        resource_set _resources;
};


}
}
}

#endif // UDHO_VIEW_RESOURCES_H
