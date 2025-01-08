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
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/results.h>
#include <boost/algorithm/string/predicate.hpp>

namespace udho{
namespace view{
namespace resources{

namespace asset{

/**
 * @brief description of an asset
 * @ingroup view
 */
class description{
    using resource_ptr = std::unique_ptr<asset::abstract_resource>;

    std::string  _prefix;
    resource_ptr _res;

    public:
        description() = delete;
        description(const std::string& prefix, resource_ptr&& res): _prefix(prefix), _res(std::move(res)) {}
        description(const description&) = delete;
        description(description&& other): _prefix(std::move(other._prefix)), _res(std::move(other._res)) {}
        description& operator=(const description&) = delete;
        description& operator=(description&& other) noexcept {
            _prefix = std::move(other._prefix);
            _res = std::move(other._res);
            return *this;
        }
    public:
        /**
         * @brief Name of the view
         */
        const std::string& name() const { return _res->name(); }
        /**
         * @brief The prefix used in resource identification.
         */
        const std::string& prefix() const { return _prefix; }

        basic_resource<asset::type::css>& css() { return cast<asset::type::css>(); }
        basic_resource<asset::type::js>&  js()  { return cast<asset::type::js> (); }
        basic_resource<asset::type::txt>& txt() { return cast<asset::type::txt>(); }
        basic_resource<asset::type::img>& img() { return cast<asset::type::img>(); }

        std::string mime() const { return _res->mime(); }

        std::size_t write(udho::net::stream& stream) const{
            return _res->write(stream);
        }
        std::size_t write_contents(udho::net::stream& stream) const{
            return _res->write_contents(stream);
        }
        /**
         * @brief type of the asset
         */
        asset::type type() const { return _res->type(); }

        template <asset::type AssetType>
        const udho::view::resources::asset::basic_resource<AssetType>& cast() const {
            return dynamic_cast<const udho::view::resources::asset::basic_resource<AssetType>&>(*_res);
        }
        template <asset::type AssetType>
        udho::view::resources::asset::basic_resource<AssetType>& cast() {
            return dynamic_cast<udho::view::resources::asset::basic_resource<AssetType>&>(*_res);
        }

        friend auto metatype(udho::view::data::type<description>){
            using namespace udho::view::data;

            return assoc("asset_description"),
                fvar("name",   &description::name),
                fvar("prefix", &description::prefix);
        }
};

/**
 * @struct proxy
 * @ingroup view
 * @brief proxies an asset.
 */
struct proxy{

    /**
     * @brief Constructs a proxy for a given resource and bridge.
     * @param name The name of the resource
     * @param prefix The prefix used in resource identification.
     * @param bridge Reference to the bridge used for resource execution.
     */
    inline proxy(const description& desc,  const std::string& base): _desc(desc), _base(base) {}

     /**
     * @brief Returns the name of the resource associated with this proxy.
     * @return The name of the resource.
     */
    inline std::string name() const { return _desc.name(); }

    /**
     * @brief Returns the prefix of the resource associated with this proxy.
     * @return The prefix of the resource.
     */
    inline std::string prefix() const { return _desc.prefix(); }

    /**
     * @brief Returns the prefix of the resource associated with this proxy.
     * @return The prefix of the resource.
     */
    inline std::string mime() const { return _desc.mime(); }

    /**
     * @brief type of the asset
     */
    inline asset::type type() const { return _desc.type(); }

    /**
     * @brief gets base url of the asset store
     * @details an asset with prefix "blog", name "theme.css" will be served as /base/blog/theme.css
     */
    const std::string& base() const { return _base; }

    /**
     * @brief expected url of the asset
     */
    const std::string url() const {
        return udho::url::format("/{}/{}/{}", _base, prefix(), name());
    }

    /**
     * @brief write the asset contents to stream
     */
    std::size_t write(udho::net::stream& stream) const{
        return _desc.write(stream);
    }
    std::size_t write_contents(udho::net::stream& stream) const{
        return _desc.write_contents(stream);
    }

    template <asset::type AssetType>
    const udho::view::resources::asset::basic_resource<AssetType>& cast() const {
        return _desc.cast<AssetType>();
    }

    /**
     * @brief exposed to lua via the following properties
     * +----------+------------+
     * | name     | property   |
     * | prefix   | property   |
     * | url      | property   |
     * +----------+------------+
     */
    friend auto metatype(udho::view::data::type<proxy>){
        using namespace udho::view::data;

        return assoc("resources_asset_proxy"),
            fvar("name",   &proxy::name),
            fvar("prefix", &proxy::prefix),
            fvar("url",    &proxy::url);
    }

    bool less(const proxy& other) const {
        if(type() == other.type()){
            if(prefix() < other.prefix()){
                return name() < other.name();
            }
            return prefix() < other.prefix();
        }
        return type() < other.type();
    }

    private:
        const description& _desc;
        const std::string& _base;
};

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
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::uri>,
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
     * @note throws exception if resource is being added after the store is locked.
     * @note ownership of the resource is transfered to the store.
     * @param prefix The prefix used in resource identification.
     * @param res The resource to add.
     */
   template <udho::view::resources::asset::type AssetType>
   const description& add(const std::string& prefix, udho::view::resources::asset::basic_resource<AssetType>* res) {
        if(!locked()){
            std::string name = res->name();
            auto it = _resources.insert(description{prefix, std::unique_ptr<udho::view::resources::asset::basic_resource<AssetType>>(res)});
            if(!it.second){
                throw std::runtime_error{udho::url::format("Filed to add asset {}/{}. As another resouorce with the same name already exists.", prefix, name)};
            }
            return *(it.first);
        } else {
            throw std::runtime_error{"Trying to add resources after the store is locked is not permitted."};
        }
    }

    template <udho::view::resources::asset::type AssetType>
    const description& add(const std::string& prefix, udho::view::resources::asset::basic_resource<AssetType>& res) {
        return add(prefix, &res);
    }

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
    void base(const std::string& url) { _base = url; }

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
    void add(asset::basic_resource<AssetType>* res){
        _store.add(_prefix, res);
    }
    template <asset::type AssetType>
    void add(asset::basic_resource<AssetType>& res){
        _store.add(_prefix, &res);
    }

    template <asset::type AssetType>
    friend prefixed_store& operator<<(prefixed_store& pstore, asset::basic_resource<AssetType>* res){
        pstore.add(res);
        return pstore;
    }

    template <asset::type AssetType>
    friend prefixed_store& operator<<(prefixed_store& pstore, asset::basic_resource<AssetType>& res){
        pstore.add(res);
        return pstore;
    }

    template <asset::type AssetType>
    friend prefixed_store&& operator<<(prefixed_store&& pstore, asset::basic_resource<AssetType>* res){
        pstore.add(res);
        return std::forward<prefixed_store>(pstore);
    }

    template <asset::type AssetType>
    friend prefixed_store&& operator<<(prefixed_store&& pstore, asset::basic_resource<AssetType>& res){
        pstore.add(res);
        return std::forward<prefixed_store>(pstore);
    }

    private:
        store_type& _store;
        std::string _prefix;
};

inline udho::view::resources::asset::prefixed_store udho::view::resources::asset::store::operator[](const std::string& prefix){
    return udho::view::resources::asset::prefixed_store{*this, prefix};
}


/**
 * @ingroup view
 * @brief copiable readonly accessor for the asset store
 * @details the lifetime of the store must be longer than the readonly accessor as it contains a const reference to the actual store
 */
struct const_store{
    using store_type  = store;
    using proxy_type  = proxy;

    template <typename Iterator>
    struct proxy_iterator : public boost::iterator_adaptor<proxy_iterator<Iterator>, Iterator, proxy_type, boost::use_default, proxy_type> {
        proxy_iterator() : proxy_iterator::iterator_adaptor_() {}
        explicit proxy_iterator(Iterator it, Iterator end, const std::string& base): proxy_iterator::iterator_adaptor_(it), _end(end), _base(base) {}
        bool valid() const { return this->base() != _end; }

        friend bool operator<(const proxy_iterator<Iterator>& left, const proxy_iterator<Iterator>& right){
            return left->less(*right);
        }

        private:
            std::string _base;
            Iterator    _end;

            friend class boost::iterator_core_access;

            proxy_type dereference() const {
                return proxy_type(*this->base_reference(), _base);
            }
    };

    using prefix_const_iterator    = proxy_iterator<typename store_type::prefix_const_iterator>;
    using name_const_iterator      = proxy_iterator<typename store_type::name_const_iterator>;
    using type_const_iterator      = proxy_iterator<typename store_type::type_const_iterator>;
    using combined_const_iterator  = proxy_iterator<typename store_type::combined_const_iterator>;
    using composite_const_iterator = proxy_iterator<typename store_type::composite_const_iterator>;
    using uri_const_iterator       = proxy_iterator<typename store_type::uri_const_iterator>;
    using size_type                = typename store_type::size_type;

    /**
     * @brief Constructs a read-only accessor to the given store.
     * @details This constructor ensures that the store is already locked for modifications, guaranteeing that the read operations are safe and consistent.
     * @param store Reference to the store that this accessor will provide a read-only interface for.
     * @exception std::runtime_error Thrown if the store is not locked, indicating it may still be under modification.
     */
    inline explicit const_store(const store_type& store): _store(store) {
        if(!store.locked()){
            throw std::runtime_error{udho::url::format("Cannot create const_store from unlocked store.")};
        }
    }
    inline const_store(const const_store&) = default;
    inline const_store() = delete;

    /**
     * @brief Returns an iterator to the beginning of the assets of the specified type.
     * @param type The asset type to filter the assets by (e.g., js, css, img).
     * @return An iterator pointing to the first asset of the specified type, or end iterator if no such asset exists.
     */
    inline type_const_iterator begin(asset::type type) const { return type_const_iterator{_store.by_type().lower_bound(type), _store.by_type().end(), base()}; }
    /**
     * @brief Returns an iterator to the end of the assets of the specified type.
     * @param type The asset type to filter the assets by (e.g., js, css, img).
     * @return An iterator pointing just past the last asset of the specified type.
     */
    inline type_const_iterator end(asset::type type)   const { return type_const_iterator{_store.by_type().upper_bound(type), _store.by_type().end(), base()}; }
    /**
     * @brief Returns the number of assets of a given type.
     * @param type The asset type to count in the store (e.g., js, css, img).
     * @return The number of assets of the specified type.
     */
    inline size_type size(asset::type type) const { return std::distance(begin(type), end(type)); }

    /**
     * @brief Returns an iterator to the beginning of the assets of a given type and prefix.
     * @param prefix The prefix that groups assets.
     * @param type The asset type to filter the assets by.
     * @return An iterator pointing to the first asset that matches the specified type and prefix, or end iterator if no such asset exists.
     */
    inline combined_const_iterator begin(const std::string& prefix, asset::type type) const { return combined_const_iterator{_store.by_combined().lower_bound(boost::make_tuple(prefix, type)), _store.by_combined().end(), base()}; }
    /**
     * @brief Returns an iterator to the end of the assets of a given type and prefix.
     * @param prefix The prefix that groups assets.
     * @param type The asset type to filter the assets by.
     * @return An iterator pointing just past the last asset that matches the specified type and prefix.
     */
    inline combined_const_iterator end(const std::string& prefix, asset::type type)   const { return combined_const_iterator{_store.by_combined().upper_bound(boost::make_tuple(prefix, type)), _store.by_combined().end(), base()}; }
    /**
     * @brief Returns the number of assets of a given type and prefix.
     * @param prefix The prefix that groups assets.
     * @param type The asset type to count in the store.
     * @return The number of assets that match the specified type and prefix.
     */
    inline size_type size(const std::string& prefix, asset::type type) const { return std::distance(begin(prefix, type), end(prefix, type)); }

    /**
     * @brief find a resource by type, prefix and name
     * @param type asset type
     * @param prefix string prefix of the asset
     * @param name string name of the asset
     */
    inline composite_const_iterator find(asset::type type, const std::string& prefix, const std::string& name) const { return composite_const_iterator{_store.by_composite().find(boost::make_tuple(prefix, type, name)), _store.by_composite().end(), base()}; }

    /**
     * @brief Returns an iterator to the beginning of all assets
     */
    inline uri_const_iterator begin() const { return uri_const_iterator{_store.by_uri().begin(), _store.by_uri().end(), base()}; }
    /**
     * @brief Returns an iterator to the end of the all assets
     */
    inline uri_const_iterator end() const { return uri_const_iterator{_store.by_uri().end(), _store.by_uri().end(), base()}; }
    /**
     * @brief find a resource by prefix and name
     * @param prefix string prefix of the asset
     * @param name string name of the asset
     */
    inline uri_const_iterator find(const std::string& prefix, const std::string& name) const { return uri_const_iterator{_store.by_uri().find(boost::make_tuple(prefix, name)), _store.by_uri().end(), base()}; }
    /**
     * @brief find a resource by uri.
     * @param subject /base/prefix/name
     */
    inline uri_const_iterator find(std::string subject) const {
        // check if subject starts with base
        // if not return end()
        // else take the part after the base ends
        // split that by the first slash only
        // take the first part as prefix and the last part as name
        // Remember: The name may contain / (just ignore them)

        const std::string& base_url = base();
        if (!boost::starts_with(subject, base_url)) {
            return uri_const_iterator{end()};
        }
        std::size_t begin = base_url.size();
        std::size_t slash = subject.find('/', begin);
        if (slash == std::string::npos) {
            return uri_const_iterator{end()};
        }
        std::string prefix = subject.substr(begin, slash - begin);
        std::string name   = subject.substr(slash + 1);

        return uri_const_iterator{find(prefix, name)};
    }

    /**
     * @brief server an asset resource through the stream
     * @param stream the response stream
     * @param prefix string prefix of the asset
     * @param name string name of the asset
     */
    inline bool serve(udho::net::stream& stream, std::string prefix, std::string name) const {
        return serve(stream, find(prefix, name));
    }

    /**
     * @brief server an asset resource through the stream
     * @param stream the response stream
     * @param subject uri of the asset
     */
    inline bool serve(udho::net::stream& stream, std::string subject) const {
        return serve(stream, find(subject));
    }

    /**
     * @brief gets base url of the asset store
     * @details an asset with prefix "blog", name "theme.css" will be served as /base/blog/theme.css
     */
    const std::string& base() const { return _store.base(); }

    private:
        inline bool serve(udho::net::stream& stream, uri_const_iterator it) const {
            if(it == end()){
                return false;
            }
            it->write(stream);
            stream.finish();
            return true;
        }

    private:
        const store_type& _store;
};

/**
 * @ingroup view
 * @brief copiable readonly accessor (obtained from a const_store) for the asset store for a specific asset type e.g. javascript, css etc..
 * @details the lifetime of the store must be longer than the readonly accessor as it contains a const reference to the actual store
 */
template <asset::type Type>
struct basic_const_substore{
    using store_type = const_store;
    using proxy_type = typename store_type::proxy_type;
    using self_type  = basic_const_substore<Type>;

    using prefix_const_iterator    = typename store_type::prefix_const_iterator;
    using name_const_iterator      = typename store_type::name_const_iterator;
    using type_const_iterator      = typename store_type::type_const_iterator;
    using composite_const_iterator = typename store_type::composite_const_iterator;
    using combined_const_iterator  = typename store_type::combined_const_iterator;
    using size_type                = typename store_type::size_type;

    /**
     * @brief A prefix specific interface to the store
     * @param prefix The prefix to identify a set of resources belonging to the same module
     * @param store Reference to the const_store.
     */
    basic_const_substore(const store_type& store): _store(store) {}
    basic_const_substore(const basic_const_substore&) = default;
    basic_const_substore() = delete;

    /**
     * @brief begin iterator for the asset substore
     */
    typename store_type::type_const_iterator begin() const { return _store.begin(Type); }
    /**
     * @brief end iterator for the asset substore
     */
    typename store_type::type_const_iterator end()   const { return _store.end(Type); }
    /**
     * @brief number of assets in the asset substore
     */
    typename store_type::size_type size() const { return std::distance(begin(), end()); }

    typename store_type::combined_const_iterator begin(const std::string& prefix) const { return _store.begin(prefix, Type); }
    typename store_type::combined_const_iterator end(const std::string& prefix)   const { return _store.end(prefix, Type); }
    typename store_type::size_type count(const std::string& prefix) const { return std::distance(begin(prefix), end(prefix)); }

    /**
     * @brief find a resource by type, prefix and name
     * @param prefix string prefix of the asset
     * @param name string name of the asset
     */
    inline composite_const_iterator find(const std::string& prefix, const std::string& name) const { return _store.find(Type, prefix, name); }

    /**
     * @brief begin iterator for the asset substore
     */
    typename store_type::type_const_iterator cbegin() const { return _store.begin(Type); }
    /**
     * @brief end iterator for the asset substore
     */
    typename store_type::type_const_iterator cend()   const { return _store.end(Type); }

    /**
     * @brief exposed to lua via the following properties
     * +----------+------------+
     * | ipairs   | function() |
     * | size     | property   |
     * +----------+------------+
     *
     * The iterator returns @ref resources::asset::proxy as value type which is also exposed to lua
     */
    friend auto metatype(udho::view::data::type<basic_const_substore<Type>>){
        using namespace udho::view::data;

        return assoc("resources_asset_basic_const_substore"),
            iter (&self_type::cbegin, &self_type::cend),
            fvar("size", &self_type::size);
    }

    private:
        const store_type& _store;
};

template <asset::type Type>
struct const_substore: basic_const_substore<Type>{
    using basic_const_store_type = basic_const_substore<Type>;

    using basic_const_store_type::basic_const_store_type;

    friend auto metatype(udho::view::data::type<const_substore<Type>>){
        using namespace udho::view::data;

        return assoc("resources_asset_const_substore_generic"),
            metatype(udho::view::data::type<basic_const_substore<Type>>());
    }
};

template <>
struct const_substore<asset::type::js>: basic_const_substore<asset::type::js>{
    using basic_const_store_type = basic_const_substore<asset::type::js>;

    using basic_const_store_type::basic_const_store_type;

    template <typename It, typename Function>
    udho::net::stream& importmap(udho::net::stream& stream, It begin, It end, Function&& f) const {
        std::stringstream sstream;
        sstream << "<script type=\"importmap\">" << "\n";
        sstream << "{" << "\n";
        sstream << "\t\"imports\": {" <<"\n";
        for(It it = begin; it != end; ++it){
            if(f(it)){
                sstream << udho::url::format("\t\t\"{}/{}\": \"{}\",", it->prefix(), it->name(), it->url()) << "\n";
            }
        }
        sstream << "\t}" <<"\n";
        sstream << "}" << "\n";
        sstream << "</script>" << "\n";
        stream << sstream.str();
        return stream;
    }

    template <typename Function>
    udho::net::stream& importmap(udho::net::stream& stream, Function&& f) const {
        return importmap(stream, basic_const_store_type::begin(), basic_const_store_type::end(), std::forward<Function>(f));
    }

    template <typename Function>
    udho::net::stream& importmap(udho::net::stream& stream, const std::string& prefix, Function&& f) const {
        return importmap(stream, basic_const_store_type::begin(prefix), basic_const_store_type::end(prefix), std::forward<Function>(f));
    }

    udho::net::stream& importmap(udho::net::stream& stream) const {
        return importmap(stream, [](basic_const_store_type::type_const_iterator){ return true; });
    }

    udho::net::stream& importmap(udho::net::stream& stream, const std::string& prefix) const {
        return importmap(stream, prefix, [](basic_const_store_type::combined_const_iterator){ return true; });
    }

    friend auto metatype(udho::view::data::type<const_substore<asset::type::js>>){
        using namespace udho::view::data;

        return assoc("resources_asset_const_substore_js"),
            metatype(udho::view::data::type<basic_const_substore<asset::type::js>>());
    }

    // template <typename It, typename Function>
    // udho::net::stream& bundle(udho::net::stream& stream, It begin, It end, Function&& f) const {
    //     stream << "<script>" << "\n";
    //     for(It it = begin; it != end; ++it){
    //         if(f(it)){
    //             stream << udho::url::format("// {}/{}", it->prefix(), it->name()) << "\n";
    //             stream << "(function() {" << "\n";
    //             it->write_contents(stream);
    //             stream << "})();" << "\n";
    //         }
    //     }
    //     stream << "</script>" << "\n";
    //     return stream;
    // }
    //
    // template <typename Function>
    // udho::net::stream& bundle(udho::net::stream& stream, const std::string& prefix, Function&& f) const {
    //     return bundle(stream, basic_const_store_type::begin(prefix), basic_const_store_type::end(prefix), std::forward<Function>(f));
    // }
    //
    // template <typename Function>
    // udho::net::stream& bundle(udho::net::stream& stream, Function&& f) const {
    //     return bundle(stream, basic_const_store_type::begin(), basic_const_store_type::end(), std::forward<Function>(f));
    // }
    //
    // udho::net::stream& bundle(udho::net::stream& stream) const {
    //     return bundle(stream, [](basic_const_store_type::type_const_iterator){ return true; });
    // }
    //
    // udho::net::stream& bundle(udho::net::stream& stream, const std::string& prefix) const {
    //     return bundle(stream, prefix, [](basic_const_store_type::combined_const_iterator){ return true; });
    // }
};

// template <asset::type Type>
// struct const_substore_prefixed{
//     using store_type = const_store;
//     using proxy_type = typename store_type::proxy_type;
//
//     using prefix_const_iterator    = typename store_type::prefix_const_iterator;
//     using name_const_iterator      = typename store_type::name_const_iterator;
//     using composite_const_iterator = typename store_type::composite_const_iterator;
//     using size_type                = typename store_type::size_type;
//
//     /**
//      * @brief A prefix specific interface to the store
//      * @param prefix The prefix to identify a set of resources belonging to the same module
//      * @param store Reference to the store.
//      */
//     const_substore_prefixed(const store_type& store, const std::string& prefix): _prefix(prefix), _substore(store) {}
//     const_substore_prefixed(const const_substore_prefixed&) = default;
//     const_substore_prefixed() = delete;
//
//     typename store_type::combined_const_iterator begin() const { return _substore.begin(_prefix, Type); }
//     typename store_type::combined_const_iterator end()   const { return _substore.end(_prefix, Type); }
//     typename store_type::size_type size() const { return std::distance(begin(), end()); }
//
//     private:
//         std::string _prefix;
//         const store_type& _substore;
// };


}


}
}
}


#endif // UDHO_VIEW_RESOURCES_ASSET_SUBSTORE_H

