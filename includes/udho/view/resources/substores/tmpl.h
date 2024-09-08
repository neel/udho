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

#ifndef UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_H
#define UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_H

#include <string>
#include <stdexcept>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <udho/view/resources/resource.h>

namespace udho{
namespace view{
namespace resources{

namespace tmpl{

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
    tmpl::results eval(T&& data){
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
    tmpl::results operator()(T&& data){
        return eval(std::forward<T>(data));
    }

    private:
        std::string  _name;
        std::string  _prefix;
        bridge_type& _bridge;
};

template <typename StoreT>
struct mutable_subset;

template <typename StoreT>
struct readonly_subset;

/**
 * @brief description of a view
 */
class description{
    std::string _name;
    std::string _prefix;

    public:
        description() = delete;
        description(const std::string& prefix, const resource_info& info): _name(info.name()), _prefix(prefix) {}
        description(const description&) = default;
    public:
        /**
         * @brief Name of the view
         */
        const std::string& name() const { return _name; }
        /**
         * @brief The prefix used in resource identification.
         */
        const std::string& prefix() const { return _prefix; }
};

/**
 * @class store
 * @brief The global view store that holds all views from all modules using the same bridge.
 *
 * Usage:
 * - Writing (adding views) must only be done from the main thread and should be completed before the server initialization.
 * - Reading can happen from multiple threads but only after the store has been locked, which is typically done during server initialization or for testing purposes.
 * - Concurrent writing and reading are not allowed to ensure data consistency and to prevent race conditions.
 *
 * @tparam BridgeT Type of the foreign language bridge, facilitating interactions with resources.
 */
template <typename BridgeT>
struct substore{
    using bridge_type = BridgeT;
    using proxy_type  = proxy<bridge_type>;
    using mutable_accessor_type = mutable_subset<substore<BridgeT>>;
    using readonly_accessor_type = readonly_subset<substore<BridgeT>>;

    template <typename StoreT>
    friend struct mutable_subset;

    template <typename StoreT>
    friend struct readonly_subset;

    /**
     * @struct tags
     * @brief Provides tags for indexing the resource set.
     */
    struct tags{
        struct prefix{}; ///< Tag for indexing by resource prefix.
        struct name{}; ///< Tag for indexing by resource name.
        struct composite{}; ///< Tag for composite indexing by prefix and name.
    };

    using resource_set = boost::multi_index_container<
        description,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::composite>,
                boost::multi_index::composite_key<
                    description,
                    boost::multi_index::const_mem_fun<description, const std::string&, &description::prefix>,
                    boost::multi_index::const_mem_fun<description, const std::string&, &description::name>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::name>,
                boost::multi_index::const_mem_fun<description, const std::string&, &description::name>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::prefix>,
                boost::multi_index::const_mem_fun<description, const std::string&, &description::prefix>
            >
        >
    >; ///< Container for storing and indexing resource information.

    using prefix_index             = typename resource_set::template index<typename tags::prefix>::type;
    using name_index               = typename resource_set::template index<typename tags::name>::type;
    using composite_index          = typename resource_set::template index<typename tags::composite>::type;

    using prefix_iterator          = typename resource_set::template index<typename tags::prefix>::type::iterator;
    using prefix_const_iterator    = typename resource_set::template index<typename tags::prefix>::type::const_iterator;
    using name_iterator            = typename resource_set::template index<typename tags::name>::type::iterator;
    using name_const_iterator      = typename resource_set::template index<typename tags::name>::type::const_iterator;
    using composite_iterator       = typename resource_set::template index<typename tags::composite>::type::iterator;
    using composite_const_iterator = typename resource_set::template index<typename tags::composite>::type::const_iterator;
    using size_type                = typename resource_set::size_type;

    /**
     * @brief Constructs a bundle with a specified bridge.
     * @param bridge Reference to the bridge used for resource compilation and execution.
     */
    explicit substore(bridge_type& bridge): _bridge(bridge), _dirty(false), _locked(false) {}
    substore(const substore&) = delete; ///< Prevents copying.

    /**
     * @brief Retrieves a modifiable index by the specified tag.
     * @tparam Tag The tag type to retrieve the index by.
     * @return Reference to the requested index.
     */
    template <typename Tag>
    typename resource_set::template index<Tag>::type& by() { return _resources.template get<Tag>(); }

    /**
     * @brief Retrieves a modifiable index by prefix.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::prefix>::type& by_prefix() { return by<typename tags::prefix>(); }
    /**
     * @brief Retrieves a modifiable index by name.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::name>::type& by_name() { return by<typename tags::name>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix and name.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::composite>::type& by_composite() { return by<typename tags::composite>(); }

    /**
     * @brief Adds a resource to the bundle and prepares it for use by compiling it through the bridge.
     * @tparam IteratorT The type of the iterator used to define the resource.
     * @param prefix The prefix used in resource identification.
     * @param res The resource to add and compile.
     */
    template <typename IteratorT>
    void add(const std::string& prefix, resource_buffer<udho::view::resources::type::view, IteratorT>&& res) {
        _resources.insert(description{prefix, res.info()});
        _bridge.compile(std::forward<resource_buffer<udho::view::resources::type::view, IteratorT>>(res), prefix);
    }
    /**
     * @brief Adds a resource file to the bundle and prepares it for use by compiling it through the bridge.
     * @param prefix The prefix used in resource identification.
     * @param res The resource file to add and compile.
     */
    void add(const std::string& prefix, resource_file<udho::view::resources::type::view>&& res) {
        _resources.insert(description{prefix, res.info()});
        _bridge.compile(std::forward<resource_file<udho::view::resources::type::view>>(res), prefix);
    }

    /**
     * @brief Retrieves a view proxy for a specified resource by name.
     * @param prefix The prefix used in resource identification.
     * @param name The name of the resource to retrieve.
     * @return A view proxy associated with the named resource.
     * @throws std::out_of_range if the resource is not found within the bundle.
     */
    proxy_type view(const std::string& prefix, const std::string& name){
        auto it = by_composite().find(boost::make_tuple(prefix, name));
        if (it != by_composite().end()) {
            return proxy_type{name, prefix, _bridge};
        } else {
            throw std::out_of_range("Resource with name '" + name + "' not found");
        }
    }

    /**
     * @brief Retrieves the bridge associated with this bundle.
     * @return Reference to the bridge.
     */
    bridge_type& bridge() { return _bridge; }
    /**
     * @brief Checks if the store is locked.
     * @return True if the store is locked, false otherwise.
     */
    bool locked() const { return _locked; }
    /**
     * @brief Locks the store to transition it to a read-only state.
     * This function should be called after all writing operations are done, typically just before server initialization.
     */
    void lock() { _locked = true; }
    /**
     * @brief Retrieves a mutable accessor to add resources to the store.
     * @param prefix The prefix to identify a set of resources.
     * @return mutable_accessor_type A mutable subset accessor for adding resources.
     * @throw std::runtime_error if the store is already locked.
     */
    mutable_accessor_type writer(const std::string& prefix) {
        if(!_dirty){
            _dirty = true;
        }
        if(_locked){
            throw std::runtime_error{udho::url::format("Cannot write, because resource store is already locked.")};
        }
        return mutable_accessor_type{prefix, *this};
    }
    /**
     * @brief Retrieves a read-only accessor to access resources from the store.
     * @param prefix The prefix to identify a set of resources.
     * @return readonly_accessor_type A read-only subset accessor for accessing resources.
     * @throw std::runtime_error if the store is not yet locked.
     */
    readonly_accessor_type reader(const std::string& prefix) {
        if(_dirty){
            throw std::runtime_error{udho::url::format("Cannot create reading interface on view store until the write interface is destroyed")};
        }
        return readonly_accessor_type{prefix, *this};
    }

    private:
        resource_set _resources;
        bridge_type& _bridge;
        std::atomic<bool> _dirty, _locked;
};

template <typename BridgeT>
struct mutable_subset<substore<BridgeT>>{
    using bridge_type = BridgeT;
    using store_type  = substore<bridge_type>;
    using proxy_type  = typename store_type::proxy_type;

    /**
     * @brief A prefix specific interface to the store
     * @param prefix The prefix to identify a set of resources belonging to the same module
     * @param store Reference to the store.
     */
    mutable_subset(const std::string& prefix, store_type& store): _prefix(prefix), _substore(store) {
        if(store.locked()){
            throw std::runtime_error{udho::url::format("Cannot write, because resource store is locked.")};
        }
    }
    mutable_subset(const mutable_subset&) = default;
    mutable_subset() = delete;
    ~mutable_subset() {
        _substore._dirty = false;
    }

    /**
     * @brief Adds a resource to the bundle and prepares it for use by compiling it through the bridge.
     * @tparam IteratorT The type of the iterator used to define the resource.
     * @param res The resource to add and compile.
     */
    template <typename IteratorT>
    void add(resource_buffer<udho::view::resources::type::view, IteratorT>&& res) {
        _substore.add(_prefix, std::forward<resource_buffer<udho::view::resources::type::view, IteratorT>>(res));
    }
    /**
     * @brief Adds a resource file to the bundle and prepares it for use by compiling it through the bridge.
     * @param res The resource file to add and compile.
     */
    void add(resource_file<udho::view::resources::type::view>&& res) {
        _substore.add(_prefix, std::forward<resource_file<udho::view::resources::type::view>>(res));
    }

    template <typename IteratorT>
    friend mutable_subset& operator<<(mutable_subset& subset, resource_buffer<udho::view::resources::type::view, IteratorT>&& res) {
        subset.add(std::move(res));
        return subset;
    }

    friend mutable_subset& operator<<(mutable_subset& subset, resource_file<udho::view::resources::type::view>&& res) {
        subset.add(std::move(res));
        return subset;
    }

    private:
        std::string _prefix;
        store_type& _substore;
};

template <typename BridgeT>
struct readonly_subset<substore<BridgeT>>{
    using bridge_type = BridgeT;
    using store_type  = substore<bridge_type>;
    using proxy_type  = typename store_type::proxy_type;

    /**
     * @brief A prefix specific interface to the store
     * @param prefix The prefix to identify a set of resources belonging to the same module
     * @param store Reference to the store.
     */
    readonly_subset(const std::string& prefix, store_type& store): _prefix(prefix), _substore(store) {
        if(!store.locked()){
            throw std::runtime_error{udho::url::format("Cannot read, because resource store is not locked.")};
        }
        if(store._dirty){
            throw std::runtime_error{udho::url::format("Cannot create reading interface on view store until the write interface is destroyed")};
        }
    }
    readonly_subset(const readonly_subset&) = default;
    readonly_subset() = delete;

    typename store_type::prefix_const_iterator begin() const { return _substore.by_prefix().lower_bound(_prefix); }
    typename store_type::prefix_const_iterator end()   const { return _substore.by_prefix().upper_bound(_prefix); }
    typename store_type::size_type size() const { return std::distance(begin(), end()); }

    /**
     * @brief Retrieves a view proxy for a specified resource by name.
     * @param name The name of the resource to retrieve.
     * @return A view proxy associated with the named resource.
     * @throws std::out_of_range if the resource is not found within the bundle.
     */
    proxy_type view(const std::string& name){
        auto it = _substore.by_composite().find(boost::make_tuple(_prefix, name));
        if (it != _substore.by_composite().end()) {
            return proxy_type{name, _prefix, _substore.bridge()};
        } else {
            throw std::out_of_range("Resource with name '" + name + "' not found");
        }
    }

    proxy_type operator[](const std::string& name){ return view(name); }

    private:
        std::string _prefix;
        store_type& _substore;
};

}


}
}
}


#endif // UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_H
