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

#ifndef UDHO_VIEW_RESOURCES_BUNDLE_H
#define UDHO_VIEW_RESOURCES_BUNDLE_H

#include <string>
#include <stdexcept>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <udho/view/resources/resource.h>
#include <udho/view/tmpl/sections.h>

namespace udho{
namespace view{
namespace resources{

namespace view{

template <typename BridgeT>
struct proxy;

/**
 * @struct results
 * @brief Encapsulates the output of a resource execution, including metadata like name, size, and type.
 *
 * This structure is used to store and access the results of executing a resource, such as a rendered view. It is not constructible directly but through friend classes that manage resource execution.
 */
struct results{
    template <typename BridgeT>
    friend struct proxy;  ///< Allows proxy to construct and modify results.

    results() = delete;  ///< Prevents direct construction of results instances.

    /**
     * @brief Returns the name of the resource associated with these results.
     * @return The resource name as a string.
     */
    inline std::string name() const { return _name; }
    /**
     * @brief Returns the size of the output data.
     * @return Size of the output.
     */
    inline std::size_t size() const { return _size; }
    /**
     * @brief Returns the type of the resource.
     * @return Resource type as a string.
     */
    inline std::string type() const { return _type; }
    /**
     * @brief Provides access to the string output of the resource execution.
     * @return A const reference to the output string.
     */
    inline const std::string& str() const { return _output; }
    /**
     * @brief Returns an iterator to the beginning of the output string.
     * @return A const iterator to the start of the output string.
     */
    inline std::string::const_iterator begin() const { return _output.begin(); }
    /**
     * @brief Returns an iterator to the end of the output string.
     * @return A const iterator to the end of the output string.
     */
    inline std::string::const_iterator end() const { return _output.end(); }

    private:
        inline explicit results(const std::string& name): _name(name) {}
        inline void type(const std::string& t) { _type = t; }
        inline void size(const std::size_t& s) { _size = s; }
        inline std::string& output() { return _output; }
    private:
        std::string _name;
        std::string _output;
        std::string _type;
        std::size_t _size;
};

/**
 * @struct proxy
 * @brief Manages the execution of a resource using a specified bridge and collects the results.
 *
 * This template struct acts as an interface between resource data and the bridge that handles its execution, capturing the output and storing it in a results structure.
 *
 * @tparam BridgeT The type of the bridge used for resource execution.
 */
template <typename BridgeT>
struct proxy{
    using bridge_type = BridgeT;

    /**
     * @brief Constructs a proxy for a given resource and bridge.
     * @param name The name of the resource.
     * @param prefix The prefix used in resource identification.
     * @param bridge Reference to the bridge used for resource execution.
     */
    proxy(const std::string& name, const std::string& prefix, bridge_type& bridge): _name(name), _prefix(prefix), _bridge(bridge) {}

    /**
     * @brief Returns the name of the resource associated with this proxy.
     * @return The name of the resource.
     */
    inline std::string name() const { return _name; }

    /**
     * @brief Executes the resource using the stored bridge and returns the results.
     * @tparam T The type of the data passed to the resource during execution.
     * @param data The data to be used during the resource execution.
     * @return A results object containing the output from the execution.
     */
    template <typename T>
    view::results eval(T&& data){
        results res(_name);
        std::size_t size = _bridge.exec(_name, _prefix, std::forward<T>(data), res.output());
        res.size(size);
        return res;
    }

    /**
     * @brief Function call operator that executes the resource using provided data.
     * @tparam T The type of the data passed to the resource during execution.
     * @param data The data to be used during the execution.
     * @return A results object containing the output from the execution.
     */
    template <typename T>
    view::results operator()(T&& data){
        return eval(std::forward<T>(data));
    }

    private:
        std::string  _name;
        std::string  _prefix;
        bridge_type& _bridge;
};

}

template <typename BridgeT>
struct bundle;

namespace detail {
    template<typename BundleT, typename ProxyT, typename TypeIndex, typename CompositeIndex, resources::type type>
    struct bundle_proxy;

    /**
     * @struct bundle_proxy
     * @brief A proxy class for accessing and managing a specific type of resources within a bundle.
     *
     * This class template provides methods for accessing resources by type and name, facilitating operations such as counting, iterating, and fetching specific resources.
     *
     * @tparam BundleT The specific type of bundle containing the resources.
     * @tparam ProxyT The proxy type used for individual resource access.
     * @tparam TypeIndex The index type used for accessing resources by their type.
     * @tparam CompositeIndex The index type used for accessing resources by a composite key of type and name.
     * @tparam type The resource type identifier.
     */
    template<typename BridgeT, typename ProxyT, typename TypeIndex, typename CompositeIndex, resources::type type>
    struct bundle_proxy<bundle<BridgeT>, ProxyT, TypeIndex, CompositeIndex, type> {
        using bundle_type        = bundle<BridgeT>;                          ///< Type of the resource bundle.
        using proxy_type         = ProxyT;                                   ///< Type of the proxy used for individual resource access.
        using type_iterator      = typename TypeIndex::const_iterator;       ///< Iterator type for accessing resources by type.
        using composite_iterator = typename CompositeIndex::const_iterator;  ///< Iterator type for accessing resources by composite key.
        using size_type          = typename TypeIndex::size_type;            ///< Type for representing sizes and counts.

        /**
         * @brief Constructs a proxy for accessing resources of a specific type within a bundle.
         * @param bundle Reference to the bundle containing the resources.
         */
        bundle_proxy(bundle_type& bundle) : _bundle(bundle) { }
        /**
        * @brief Copy constructor.
        * @param other The other bundle_proxy instance from which to copy.
        */
        bundle_proxy(const bundle_proxy& other) = default;

        /**
         * @brief Returns an iterator pointing to the first resource of the specified type.
         * @return An iterator to the beginning of the resources of the specified type.
         */
        type_iterator begin() const  {
            auto range = _bundle.by_type().equal_range(type);
            return range.first;
        }
        /**
         * @brief Returns an iterator pointing to the end of the resources of the specified type.
         * @return An iterator to the end of the resources of the specified type.
         */
        type_iterator end() const    {
            auto range = _bundle.by_type().equal_range(type);
            return range.second;
        }
        /**
         * @brief Returns the count of resources of the specified type.
         * @return The number of resources of the specified type.
         */
        size_type count() const      {
            auto range = _bundle.by_type().equal_range(type);
            if (range.first == range.second) return 0;
            return std::distance(range.first, range.second);
        }

        /**
         * @brief Finds a resource by its name.
         * @param name The name of the resource to find.
         * @return An iterator to the resource, if found; otherwise, an iterator to the end of the resource index.
         */
        composite_iterator find(const std::string& name) const {
            return _bundle.by_composite().find(boost::make_tuple(type, name));
        }

        /**
         * @brief Accesses a resource by its name and returns its associated proxy.
         * @param name The name of the resource to access.
         * @return A proxy associated with the named resource.
         * @throws std::out_of_range if the resource with the specified name is not found.
         */
        typename proxy_type::result_type operator[](const std::string& name) const {
            composite_iterator it = find(name);
            if (it != _bundle.by_composite().end()) {
                proxy_type proxy{*it, _bundle};
                return proxy();
            } else {
                throw std::out_of_range("Resource with name '" + name + "' not found");
            }
        }

        /**
         * @brief Executes the resource with the specified name using the provided argument.
         * @tparam T The type of the argument to pass to the resource.
         * @param name The name of the resource to execute.
         * @param arg The argument to pass to the resource execution function.
         * @return The results from executing the resource.
         */
        template <typename T>
        view::results operator()(const std::string& name, T&& arg){
            typename proxy_type::result_type v = operator[](name);
            return v(std::forward<T>(arg));
        }

    private:
        bundle_type& _bundle;
    };

    /**
     * @struct view_proxy
     * @brief Provides a proxy for managing a single view resource within a bundle.
     *
     * This class simplifies the access and execution of view resources, delegating the task to a specific bridge and managing prefixes for resource identification.
     *
     * @tparam BundleT The specific type of bundle containing the view resources.
     */
    template<typename BundleT>
    struct view_proxy{
        using bridge_type = typename BundleT::bridge_type;  ///< The type of the bridge associated with the bundle.
        using result_type = view::proxy<bridge_type>; ///< The type of the result proxy provided by this proxy.

        /**
         * @brief Constructs a view proxy for a specific resource within a bundle.
         * @param res Reference to the resource_info describing the resource.
         * @param bundle Reference to the bundle containing the resource.
         */
        view_proxy(const resource_info& res, BundleT& bundle): _res(res), _bridge(bundle.bridge()), _prefix(bundle.prefix()) {}
        /**
         * @brief Activates the proxy, resulting in the creation of a view proxy specific to the resource.
         * @return A view proxy for the specified resource.
         */
        result_type operator()() { return result_type{_res.name(), _prefix, _bridge}; }

        private:
            const resource_info& _res;
            bridge_type&        _bridge;
            std::string         _prefix;
    };
}

/**
 * @struct bundle
 * @brief Manages a collection of resources and interfaces them with a scripting bridge for processing.
 *
 * This structure is responsible for storing, managing, and interfacing resources such as views and assets with the designated bridge. It uses a multi-index container to organize resources by various criteria and provides efficient access and management capabilities.
 *
 * @tparam BridgeT The type of the bridge that compiles and executes resources.
 */
template <typename BridgeT>
struct bundle{
    using bridge_type = BridgeT; ///< The type of the bridge used for processing resources.
    using self_type   = bundle<bridge_type>; ///< Alias for the type of this bundle.
    using view_proxy  = view::proxy<bridge_type>; ///< The proxy type used for accessing individual view resources.

    /**
     * @struct tags
     * @brief Provides tags for indexing the resource set.
     */
    struct tags{
        struct type{}; ///< Tag for indexing by resource type.
        struct name{}; ///< Tag for indexing by resource type.
        struct composite{}; ///< Tag for composite indexing by type and name.
    };

    using resource_set = boost::multi_index_container<
        resource_info,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::composite>,
                boost::multi_index::composite_key<
                    resource_info,
                    boost::multi_index::const_mem_fun<resource_info, resources::type, &resource_info::type>,
                    boost::multi_index::const_mem_fun<resource_info, std::string, &resource_info::name>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::name>,
                boost::multi_index::const_mem_fun<resource_info, std::string, &resource_info::name>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::type>,
                boost::multi_index::const_mem_fun<resource_info, resources::type, &resource_info::type>
            >
        >
    >; ///< Container for storing and indexing resource information.

    using type_index               = typename resource_set::template index<typename tags::type>::type;
    using name_index               = typename resource_set::template index<typename tags::name>::type;
    using composite_index          = typename resource_set::template index<typename tags::composite>::type;

    using type_iterator            = typename resource_set::template index<typename tags::type>::type::iterator;
    using type_const_iterator      = typename resource_set::template index<typename tags::type>::type::const_iterator;
    using name_iterator            = typename resource_set::template index<typename tags::name>::type::iterator;
    using name_const_iterator      = typename resource_set::template index<typename tags::name>::type::const_iterator;
    using composite_iterator       = typename resource_set::template index<typename tags::composite>::type::iterator;
    using composite_const_iterator = typename resource_set::template index<typename tags::composite>::type::const_iterator;
    using size_type                = typename resource_set::size_type;

    template <resources::type type>
    using bundle_proxy = detail::bundle_proxy<
        self_type,
        detail::view_proxy<self_type>,
        type_index,
        composite_index,
        type
    >;

    using bundle_proxy_view = bundle_proxy<resources::type::view>;

    friend bundle_proxy_view;


    /**
     * @brief Constructs a bundle with a specified bridge and prefix.
     * @param bridge Reference to the bridge used for resource compilation and execution.
     * @param prefix Prefix used to identify resources within the bundle.
     */
    explicit bundle(bridge_type& bridge, const std::string& prefix): _bridge(bridge), _prefix(prefix), views(*this) {}
    bundle(const bundle&) = delete; ///< Prevents copying of the bundle.


    /**
     * @brief Retrieves a modifiable index by the specified tag.
     * @tparam Tag The tag type to retrieve the index by.
     * @return Reference to the requested index.
     */
    template <typename Tag>
    typename resource_set::template index<Tag>::type& by() { return _resources.template get<Tag>(); }
    typename resource_set::template index<typename tags::type>::type& by_type() { return by<typename tags::type>(); }
    typename resource_set::template index<typename tags::name>::type& by_name() { return by<typename tags::name>(); }
    typename resource_set::template index<typename tags::composite>::type& by_composite() { return by<typename tags::composite>(); }


    /**
     * @brief Adds a resource to the bundle and prepares it for use by compiling it through the bridge.
     * @tparam IteratorT The type of the iterator used to define the resource.
     * @param res The resource to add and compile.
     */
    template <typename IteratorT>
    void add(resource_buffer<udho::view::resources::type::view, IteratorT>&& res) {
        _resources.insert(res.info());
        prepare_view(std::forward<resource_buffer<udho::view::resources::type::view, IteratorT>>(res));
    }
    /**
     * @brief Adds a resource file to the bundle and prepares it for use by compiling it through the bridge.
     * @param res The resource file to add and compile.
     */
    void add(resource_file<udho::view::resources::type::view>&& res) {
        _resources.insert(res.info());
        prepare_view(std::forward<resource_file<udho::view::resources::type::view>>(res));
    }

    /**
     * @brief Returns the prefix used for resource identification within the bundle.
     * @return The prefix as a string.
     */
    std::string prefix() const { return _prefix; }

    /**
     * @brief Inserts a resource buffer into the bundle and compiles it.
     * @tparam type The specific type of the resource.
     * @tparam IteratorT The type of iterator defining the resource buffer.
     * @param res The resource buffer to be added.
     * @return A reference to this bundle.
     */
    template <udho::view::resources::type type, typename IteratorT>
    friend bundle& operator<<(bundle& b, resource_buffer<type, IteratorT>& res){
        b.add(res);
        return b;
    }

    /**
     * @brief Inserts a resource file into the bundle and compiles it.
     * @tparam type The specific type of the resource.
     * @param res The resource file to be moved and added.
     * @return A reference to this bundle.
     */
    template <udho::view::resources::type type>
    friend bundle& operator<<(bundle& b, resource_file<type>&& res){
        b.add(std::forward<resource_file<type>>(res));
        return b;
    }

    /**
     * @brief Retrieves a view proxy for a specified resource by name.
     * @param name The name of the resource to retrieve.
     * @return A view proxy associated with the named resource.
     * @throws std::out_of_range if the resource is not found within the bundle.
     */
    view_proxy view(const std::string& name){
        auto it = by_composite().find(boost::make_tuple(resources::type::view, name));
        if (it != by_composite().end()) {
            return view_proxy{name, _prefix, _bridge};
        } else {
            throw std::out_of_range("Resource with name '" + name + "' not found");
        }
    }

    /**
     * @brief Retrieves the bridge associated with this bundle.
     * @return Reference to the bridge.
     */
    bridge_type& bridge() { return _bridge; }

    private:
        template <typename ResourceT>
        bool prepare_view(ResourceT&& res){
            return _bridge.compile(std::forward<ResourceT>(res), _prefix);
        }
    private:
        bridge_type& _bridge;
        std::string  _prefix;
        resource_set _resources;
    public:
        bundle_proxy_view views; ///< Direct access to the view proxy
        // detail::bundle_asset_proxy<bridge_type> assets;
};

}
}
}


#endif // UDHO_VIEW_RESOURCES_BUNDLE_H
