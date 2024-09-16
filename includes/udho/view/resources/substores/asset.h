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

#ifndef UDHO_VIEW_RESOURCES_SUBSTORE_ASSET_H
#define UDHO_VIEW_RESOURCES_SUBSTORE_ASSET_H

#include <string>
#include <stdexcept>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/results.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{


/**
 * @struct proxy
 * @brief Manages the execution of a resource using a specified bridge and collects the results.
 *
 * This template struct acts as an interface between resource data and the bridge that handles its execution, capturing the output and storing it in a results structure.
 */
struct proxy{

    /**
     * @brief Constructs a proxy for a given resource and bridge.
     * @param name The name of the resource
     * @param prefix The prefix used in resource identification.
     * @param bridge Reference to the bridge used for resource execution.
     */
    proxy(const std::string& name, const std::string& prefix): _name(name), _prefix(prefix) {}

    /**
     * @brief Returns the name of the resource associated with this proxy.
     * @return The name of the resource.
     */
    inline std::string name() const { return _name; }


    private:
        std::string  _name;
        std::string  _prefix;
};

/**
 * @brief description of a view
 */
class description{
    std::string _name;
    std::string _prefix;
    asset::type _type;

    public:
        description() = delete;
        description(const std::string& prefix, const resource_info& info, asset::type type): _name(info.name()), _prefix(prefix), _type(type) {}
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

        asset::type type() const { return _type; }
};

/**
 * @class store
 * @brief The global view store that holds all views from all modules using the same bridge.
 *
 * Usage:
 * - Writing (adding views) must only be done from the main thread and should be completed before the server initialization.
 * - Reading can happen from multiple threads but only after the store has been locked, which is typically done during server initialization or for testing purposes.
 * - Concurrent writing and reading are not allowed to ensure data consistency and to prevent race conditions.
 */
struct substore{
    using proxy_type             = proxy;

    /**
     * @struct tags
     * @brief Provides tags for indexing the resource set.
     */
    struct tags{
        struct prefix{};    ///< Tag for indexing by resource prefix.
        struct name{};      ///< Tag for indexing by resource name.
        struct type{};      ///< Tag for indexing by resource type.
        struct combined{};  ///< Tag for composite indexing by prefix and type
        struct composite{}; ///< Tag for composite indexing by prefix, type and name.
    };

    using resource_set = boost::multi_index_container<
        description,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::composite>,
                boost::multi_index::composite_key<
                    description,
                    boost::multi_index::const_mem_fun<description, const std::string&, &description::prefix>,
                    boost::multi_index::const_mem_fun<description, asset::type, &description::type>,
                    boost::multi_index::const_mem_fun<description, const std::string&, &description::name>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::combined>,
                boost::multi_index::composite_key<
                    description,
                    boost::multi_index::const_mem_fun<description, const std::string&, &description::prefix>,
                    boost::multi_index::const_mem_fun<description, asset::type, &description::type>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::name>,
                boost::multi_index::const_mem_fun<description, const std::string&, &description::name>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::prefix>,
                boost::multi_index::const_mem_fun<description, const std::string&, &description::prefix>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::type>,
                boost::multi_index::const_mem_fun<description, asset::type, &description::type>
            >
        >
    >; ///< Container for storing and indexing resource information.

    using prefix_index             = typename resource_set::template index<typename tags::prefix>::type;
    using name_index               = typename resource_set::template index<typename tags::name>::type;
    using combined_index           = typename resource_set::template index<typename tags::combined>::type;
    using composite_index          = typename resource_set::template index<typename tags::composite>::type;

    using prefix_iterator          = typename resource_set::template index<typename tags::prefix>::type::iterator;
    using prefix_const_iterator    = typename resource_set::template index<typename tags::prefix>::type::const_iterator;
    using name_iterator            = typename resource_set::template index<typename tags::name>::type::iterator;
    using name_const_iterator      = typename resource_set::template index<typename tags::name>::type::const_iterator;
    using combined_iterator        = typename resource_set::template index<typename tags::combined>::type::iterator;
    using combined_const_iterator  = typename resource_set::template index<typename tags::combined>::type::const_iterator;
    using composite_iterator       = typename resource_set::template index<typename tags::composite>::type::iterator;
    using composite_const_iterator = typename resource_set::template index<typename tags::composite>::type::const_iterator;
    using size_type                = typename resource_set::size_type;

    /**
     * @brief Constructs a bundle with a specified bridge.
     * @param bridge Reference to the bridge used for resource compilation and execution.
     */
    explicit substore(): _dirty(false), _locked(false) {}
    substore(const substore&) = delete; ///< Prevents copying.

    /**
     * @brief Retrieves a modifiable index by the specified tag.
     * @tparam Tag The tag type to retrieve the index by.
     * @return Reference to the requested index.
     */
    template <typename Tag>
    typename resource_set::template index<Tag>::type& by() { return _resources.template get<Tag>(); }

    /**
     * @brief Retrieves a modifiable index by the specified tag.
     * @tparam Tag The tag type to retrieve the index by.
     * @return Reference to the requested index.
     */
    template <typename Tag>
    const typename resource_set::template index<Tag>::type& by() const { return _resources.template get<Tag>(); }

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
     * @brief Retrieves a modifiable index by both prefix and type.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::combined>::type& by_combined() { return by<typename tags::combined>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix, type and name.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::composite>::type& by_composite() { return by<typename tags::composite>(); }


    /**
     * @brief Retrieves a modifiable index by prefix.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::prefix>::type& by_prefix() const { return by<typename tags::prefix>(); }
    /**
     * @brief Retrieves a modifiable index by name.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::name>::type& by_name() const { return by<typename tags::name>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix and type.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::combined>::type& by_combined() const { return by<typename tags::combined>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix, type and name.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::composite>::type& by_composite() const { return by<typename tags::composite>(); }

    /**
     * @brief Adds a resource to the bundle and prepares it for use by compiling it through the bridge.
     * @tparam IteratorT The type of the iterator used to define the resource.
     * @param prefix The prefix used in resource identification.
     * @param res The resource to add and compile.
     */
    template <typename IteratorT>
    void add(const std::string& prefix, asset::type type, resource_buffer<udho::view::resources::type::asset, IteratorT>&& res) {
        _resources.insert(description{prefix, res.info(), type});
    }
    /**
     * @brief Adds a resource file to the bundle and prepares it for use by compiling it through the bridge.
     * @param prefix The prefix used in resource identification.
     * @param res The resource file to add and compile.
     */
    void add(const std::string& prefix, asset::type type, resource_file<udho::view::resources::type::asset>&& res) {
        _resources.insert(description{prefix, res.info(), type});
    }


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

    private:
        resource_set _resources;
        std::atomic<bool> _dirty, _locked;
};


/**
 * @brief copiable readonly accessor for the asset substore
 * @details the lifetime of the store must be longer than the readonly accessor as it contains a const reference to the actual store
 */
struct const_substore{
    using store_type  = substore;
    using proxy_type  = typename store_type::proxy_type;

    using prefix_const_iterator    = typename store_type::prefix_const_iterator;
    using name_const_iterator      = typename store_type::name_const_iterator;
    using combined_const_iterator  = typename store_type::combined_const_iterator;
    using composite_const_iterator = typename store_type::composite_const_iterator;
    using size_type                = typename store_type::size_type;

    /**
     * @brief A prefix specific interface to the store
     * @param prefix The prefix to identify a set of resources belonging to the same module
     * @param store Reference to the store.
     */
    explicit const_substore(const store_type& store): _substore(store) {
        if(!store.locked()){
            throw std::runtime_error{udho::url::format("Cannot read, because resource store is not locked.")};
        }
    }
    const_substore(const const_substore&) = default;
    const_substore() = delete;

    typename store_type::combined_const_iterator begin(const std::string& prefix, asset::type type) const { return _substore.by_combined().lower_bound(boost::make_tuple(prefix, type)); }
    typename store_type::combined_const_iterator end(const std::string& prefix, asset::type type)   const { return _substore.by_combined().upper_bound(boost::make_tuple(prefix, type)); }
    typename store_type::size_type size(const std::string& prefix, asset::type type) const { return std::distance(begin(prefix, type), end(prefix, type)); }

    private:
        const store_type& _substore;
};

template <asset::type Type>
struct const_substore_prefixed{
    using store_type = const_substore;
    using proxy_type = typename store_type::proxy_type;

    using prefix_const_iterator    = typename store_type::prefix_const_iterator;
    using name_const_iterator      = typename store_type::name_const_iterator;
    using composite_const_iterator = typename store_type::composite_const_iterator;
    using size_type                = typename store_type::size_type;

    /**
     * @brief A prefix specific interface to the store
     * @param prefix The prefix to identify a set of resources belonging to the same module
     * @param store Reference to the store.
     */
    const_substore_prefixed(const store_type& store, const std::string& prefix): _prefix(prefix), _substore(store) {}
    const_substore_prefixed(const const_substore_prefixed&) = default;
    const_substore_prefixed() = delete;

    typename store_type::combined_const_iterator begin() const { return _substore.begin(_prefix, Type); }
    typename store_type::combined_const_iterator end()   const { return _substore.end(_prefix, Type); }
    typename store_type::size_type size() const { return std::distance(begin(), end()); }

    private:
        std::string _prefix;
        const store_type& _substore;
};


}


}
}
}


#endif // UDHO_VIEW_RESOURCES_SUBSTORE_ASSET_H

