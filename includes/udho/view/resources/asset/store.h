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

#ifndef UDHO_VIEW_RESOURCES_ASSET_SUBSTORE_H
#define UDHO_VIEW_RESOURCES_ASSET_SUBSTORE_H

#include <string>
#include <stdexcept>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <udho/view/data/data.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/results.h>
#include <boost/algorithm/string/predicate.hpp>
#include <udho/view/resources/asset/info.h>
#include <udho/url/utils.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{

struct prefixed_store;

/**
 * @class store
 * @ingroup view
 * @brief The global asset store that holds all assets from all modules.
 *
 * Usage:
 * - Writing must only be done from the main thread and should be completed before the server initialization.
 * - Reading can happen from multiple threads but only after the store has been locked, which is typically done during server initialization or for testing purposes.
 * - Concurrent writing and reading are not allowed to ensure data consistency and to prevent race conditions.
 */
struct store{

    /**
     * @struct tags
     * @brief Provides tags for indexing the resource set.
     */
    struct tags{
        struct prefix{};    ///< Tag for indexing by resource prefix.
        struct name{};      ///< Tag for indexing by resource name.
        struct type{};      ///< Tag for indexing by resource type.
        struct combined{};  ///< Tag for composite indexing by prefix and type
        struct uri{};       ///< Tag for composite indexing by prefix and name
        struct composite{}; ///< Tag for composite indexing by prefix, type and name.
    };

    using resource_set = boost::multi_index_container<
        asset_registration_info,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::composite>,
                boost::multi_index::composite_key<
                    asset_registration_info,
                    boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::prefix>,
                    boost::multi_index::const_mem_fun<asset_registration_info, asset::type, &asset_registration_info::type>,
                    boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::name>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::combined>,
                boost::multi_index::composite_key<
                    asset_registration_info,
                    boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::prefix>,
                    boost::multi_index::const_mem_fun<asset_registration_info, asset::type, &asset_registration_info::type>
                >
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::uri>,
                boost::multi_index::composite_key<
                    asset_registration_info,
                    boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::prefix>,
                    boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::name>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::name>,
                boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::name>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::prefix>,
                boost::multi_index::const_mem_fun<asset_registration_info, const std::string&, &asset_registration_info::prefix>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::type>,
                boost::multi_index::const_mem_fun<asset_registration_info, asset::type, &asset_registration_info::type>
            >
        >
    >; ///< Container for storing and indexing resource information.

    using prefix_index             = typename resource_set::template index<typename tags::prefix>::type;
    using name_index               = typename resource_set::template index<typename tags::name>::type;
    using combined_index           = typename resource_set::template index<typename tags::combined>::type;
    using uri_index                = typename resource_set::template index<typename tags::uri>::type;
    using composite_index          = typename resource_set::template index<typename tags::composite>::type;

    using prefix_iterator          = typename resource_set::template index<typename tags::prefix>::type::iterator;
    using prefix_const_iterator    = typename resource_set::template index<typename tags::prefix>::type::const_iterator;

    using name_iterator            = typename resource_set::template index<typename tags::name>::type::iterator;
    using name_const_iterator      = typename resource_set::template index<typename tags::name>::type::const_iterator;

    using type_iterator            = typename resource_set::template index<typename tags::type>::type::iterator;
    using type_const_iterator      = typename resource_set::template index<typename tags::type>::type::const_iterator;

    using combined_iterator        = typename resource_set::template index<typename tags::combined>::type::iterator;
    using combined_const_iterator  = typename resource_set::template index<typename tags::combined>::type::const_iterator;

    using composite_iterator       = typename resource_set::template index<typename tags::composite>::type::iterator;
    using composite_const_iterator = typename resource_set::template index<typename tags::composite>::type::const_iterator;

    using uri_iterator             = typename resource_set::template index<typename tags::uri>::type::iterator;
    using uri_const_iterator       = typename resource_set::template index<typename tags::uri>::type::const_iterator;

    using size_type                = typename resource_set::size_type;

    /**
     * @brief Constructs a bundle with a specified bridge.
     * @param bridge Reference to the bridge used for resource compilation and execution.
     */
    explicit store(): _locked(false), _base("/") {}
    store(const store&) = delete; ///< Prevents copying.

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
     * @brief Retrieves a modifiable index by type.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::type>::type& by_type() { return by<typename tags::type>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix and type.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::combined>::type& by_combined() { return by<typename tags::combined>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix and name.
     * @return Reference to the requested index.
     */
    typename resource_set::template index<typename tags::uri>::type& by_uri() { return by<typename tags::uri>(); }
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
     * @brief Retrieves a const index by name.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::name>::type& by_name() const { return by<typename tags::name>(); }
    /**
     * @brief Retrieves a const index by type.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::type>::type& by_type() const { return by<typename tags::type>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix and type.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::combined>::type& by_combined() const { return by<typename tags::combined>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix and name.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::uri>::type& by_uri() const { return by<typename tags::uri>(); }
    /**
     * @brief Retrieves a modifiable index by both prefix, type and name.
     * @return Reference to the requested index.
     */
    const typename resource_set::template index<typename tags::composite>::type& by_composite() const { return by<typename tags::composite>(); }

    /**
     * @brief Adds a resource to the bundle and prepares it for use by compiling it through the bridge.
     * @pre expects that the store is locked before adding.
     *      prefix must not contain a leading or trailing slash
     * @note throws exception if resource is being added after the store is locked.
     * @note ownership of the resource is transfered to the store.
     * @param prefix The prefix used in resource identification.
     * @param res The resource to add.
     */
   template <udho::view::resources::asset::type AssetType>
   const asset_registration_info& add(const std::string& prefix, std::unique_ptr<udho::view::resources::asset::basic_resource<AssetType>>&& res) {
        if(!locked()){
           if(prefix.front() == '/' || prefix.back() == '/') {
               throw std::runtime_error{udho::url::format("Restriction: Prefix must not contain a leading or trailing slash, violated by prefix `{}`", prefix)};
           }

            std::string name = res->name();
            auto it = _resources.insert(asset_registration_info{prefix, std::move(res)});
            if(!it.second){
                throw std::runtime_error{udho::url::format("Filed to add asset {}/{}. As another resouorce with the same name already exists.", prefix, name)};
            }
            return *(it.first);
        } else {
            throw std::runtime_error{"Trying to add resources after the store is locked is not permitted."};
        }
    }

    // template <udho::view::resources::asset::type AssetType>
    // const asset_registration_info& add(const std::string& prefix, udho::view::resources::asset::basic_resource<AssetType>& res) {
    //     return add(prefix, res);
    // }

    size_type size() const { return _resources.size(); }

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
     * @brief gets base url of the asset store
     * @details an asset with prefix "blog", name "theme.css" will be served as /base/blog/theme.css
     */
    const std::string& base() const { return _base; }
    /**
     * @brief sets base url of the asset store
     * @details an asset with prefix "blog", name "theme.css" will be served as /base/blog/theme.css
     */
    void base(const std::string& url) {
        _base = udho::url::utils::slash_quote(url);
    }

    prefixed_store operator[](const std::string& prefix);

    // prefixed_store operator[](const std::string& prefix){
    //     return prefixed_store{*this, prefix};
    // }

    private:
        resource_set _resources;
        std::atomic<bool> _locked;
        std::string _base;
};

struct prefixed_store{
    using store_type = udho::view::resources::asset::store;

    inline explicit prefixed_store(store_type& store, const std::string& prefix): _store(store), _prefix(prefix) {}
    inline prefixed_store(const prefixed_store&) = delete;
    inline prefixed_store(prefixed_store&& other): _store(other._store), _prefix(std::move(other._prefix)) {}

    template <asset::type AssetType>
    void add(std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        _store.add(_prefix, std::move(res));
    }

    // template <asset::type AssetType>
    // void add(asset::basic_resource<AssetType>&& res){
    //     _store.add(_prefix, std::unique_ptr<asset::basic_resource<AssetType>>(std::move(res)));
    // }

    template <asset::type AssetType>
    friend prefixed_store& operator<<(prefixed_store& pstore, std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        pstore.add(std::move(res));
        return pstore;
    }

    // template <asset::type AssetType>
    // friend prefixed_store& operator<<(prefixed_store& pstore, asset::basic_resource<AssetType>& res){
    //     pstore.add(res);
    //     return pstore;
    // }

    template <asset::type AssetType>
    friend prefixed_store&& operator<<(prefixed_store&& pstore, std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        pstore.add(std::move(res));
        return std::forward<prefixed_store>(pstore);
    }

    // template <asset::type AssetType>
    // friend prefixed_store&& operator<<(prefixed_store&& pstore, asset::basic_resource<AssetType>&& res){
    //     pstore.add(std::move(res));
    //     return std::forward<prefixed_store>(pstore);
    // }

    private:
        store_type& _store;
        std::string _prefix;
};

inline udho::view::resources::asset::prefixed_store udho::view::resources::asset::store::operator[](const std::string& prefix){
    return udho::view::resources::asset::prefixed_store{*this, prefix};
}

}


}
}
}


#endif // UDHO_VIEW_RESOURCES_ASSET_SUBSTORE_H

