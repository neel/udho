#ifndef UDHO_VIEW_RESOURCES_ASSET_CONST_STORE_H
#define UDHO_VIEW_RESOURCES_ASSET_CONST_STORE_H

#include <string>
#include <udho/view/resources/asset/info.h>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/asset/proxy.h>
#include <udho/view/data/data.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{


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
        Iterator    _end;
        const std::string& _base;

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
    inline prefix_const_iterator begin(const std::string& prefix) const { return prefix_const_iterator{_store.by_prefix().lower_bound(prefix), _store.by_prefix().end(), base()}; }
    /**
     * @brief Returns an iterator to the end of the assets of the specified type.
     * @param type The asset type to filter the assets by (e.g., js, css, img).
     * @return An iterator pointing just past the last asset of the specified type.
     */
    inline prefix_const_iterator end(const std::string& prefix)   const { return prefix_const_iterator{_store.by_prefix().upper_bound(prefix), _store.by_prefix().end(), base()}; }
    /**
     * @brief Returns the number of assets of a given type.
     * @param type The asset type to count in the store (e.g., js, css, img).
     * @return The number of assets of the specified type.
     */
    inline size_type size(const std::string& prefix) const { return std::distance(begin(prefix), end(prefix)); }

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
    inline composite_const_iterator find(asset::type type, const std::string& prefix, const std::string& name) const {
        return composite_const_iterator{
            _store.by_composite().find(boost::make_tuple(prefix, type, name)),
            _store.by_composite().end(),
            base()
        };
    }

    /**
     * @brief Returns an iterator to the beginning of all assets
     */
    inline uri_const_iterator begin() const { return uri_const_iterator{_store.by_uri().begin(), _store.by_uri().end(), base()}; }
    inline uri_const_iterator cbegin() const { return begin(); }
    /**
     * @brief Returns an iterator to the end of the all assets
     */
    inline uri_const_iterator end() const { return uri_const_iterator{_store.by_uri().end(), _store.by_uri().end(), base()}; }
    inline uri_const_iterator cend() const { return end(); }

    inline proxy_type at(std::size_t i) const {
        uri_const_iterator it = begin();
        std::size_t size = csize();
        if(i >= csize()){
            throw std::out_of_range{udho::url::format("Index {} out of range, total {}", i, size)};
        }
        std::advance(it, i);
        return *it;
    }

    inline size_type csize() const { return std::distance(begin(), end()); }
    /**
     * @brief find a resource by prefix and name
     * @param prefix string prefix of the asset
     * @param name string name of the asset
     */
    inline uri_const_iterator find(const std::string& prefix, const std::string& name) const { return uri_const_iterator{_store.by_uri().find(boost::make_tuple(prefix, name)), _store.by_uri().end(), base()}; }
    /**
     * @brief find a resource by uri.
     * @param subject /base/prefix/name
     * @pre subject must start with a /
     */
    inline uri_const_iterator find(std::string subject) const {
        std::string prefix, name;
        bool success = udho::url::utils::extract(subject, base(), prefix, name);
        if(!success){
            return uri_const_iterator{end()};
        }
        return find(prefix, name);
    }

    /**
     * @brief server an asset resource through the stream
     * @param stream the response stream
     * @param prefix string prefix of the asset
     * @param name string name of the asset
     */
    template <typename OstreamT>
    inline bool serve(OstreamT& ostream, std::string prefix, std::string name) const {
        auto it = find(prefix, name);
        return serve(ostream, it);
    }

    /**
     * @brief server an asset resource through the stream
     * @param stream the response stream
     * @param subject uri of the asset
     */
    template <typename OstreamT>
    inline bool serve(OstreamT& ostream, std::string subject) const {
        auto it = find(subject);
        return serve(ostream, it);
    }

    /**
     * @brief gets base url of the asset store
     * @details an asset with prefix "blog", name "theme.css" will be served as /base/blog/theme.css
     */
    const std::string& base() const { return _store.base(); }

public:
    /**
         * @brief Proxy class for accessing assets grouped by a specific prefix
         * @tparam Index Type of index used for storing assets (typically prefix_index)
         *
         * Provides iterator access to assets sharing a common prefix in O(1) time complexity.
         */
    template <typename Index>
    class prefixed_proxy_ {
    public:
        using const_iterator = typename Index::const_iterator;
        using size_type      = typename Index::size_type;

        prefixed_proxy_(const Index& index, const std::string& prefix): _index(index), _prefix(prefix) {}
        /**
         * @brief Gets the prefix associated with this proxy group
         * @return Const reference to the prefix string
         */
        const std::string& prefix() const { return _prefix; }

        /**
         * @brief Returns iterator to the first asset in the prefix group
         * @return Const iterator pointing to the first asset with matching prefix
         */
        const_iterator begin() const { return _index.lower_bound(_prefix); }
        /**
         * @brief Returns iterator past the last asset in the prefix group
         * @return Const iterator pointing just after the last asset with matching prefix
         */
        const_iterator end() const { return _index.upper_bound(_prefix); }
        /**
         * @brief Gets the number of assets in the prefix group
         * @return Number of assets sharing this prefix
         */
        size_type size() const { return std::distance(begin(), end()); }

        friend auto metatype(udho::view::data::type<prefixed_proxy_<Index>>){
            using namespace udho::view::data;

            return assoc("resources_asset_const_store_prefix_proxy_"),
                   iter(&prefixed_proxy_<Index>::begin, &prefixed_proxy_<Index>::end),
                   fvar("prefix", &prefixed_proxy_<Index>::prefix),
                   fvar("size",   &prefixed_proxy_<Index>::size);
        }

    private:
        const Index& _index;
        std::string  _prefix;
    };

    /**
     * @brief Custom iterator for grouping assets by their prefixes
     * @tparam Index Type of index used for storing assets (typically prefix_index)
     *
     * Implements forward traversal between different prefix groups in O(log n) time complexity.
     * Dereferences to prefixed_proxy_ objects for each unique prefix group.
     */
    template <typename Index>
    class prefix_group_iterator: public boost::iterator_adaptor<prefix_group_iterator<Index>, typename Index::const_iterator, prefixed_proxy_<Index>, boost::forward_traversal_tag, prefixed_proxy_<Index>>{
    public:
        /**
         * @brief Constructs iterator for a specific index position
         * @param index Reference to the underlying asset index
         * @param it Initial position in the index
         */
        prefix_group_iterator(const Index& index, typename Index::const_iterator it): prefix_group_iterator::iterator_adaptor_(it), _index(index) {}

    private:
        friend class boost::iterator_core_access;

        const Index& _index;

        void increment() {
            if (this->base() != _index.end()) {
                this->base_reference() = _index.upper_bound(this->base()->prefix());
            }
        }

        prefixed_proxy_<Index> dereference() const {
            return prefixed_proxy_<Index>{ _index, this->base()->prefix() };
        }

        bool equal(const prefix_group_iterator& other) const {
            return this->base() == other.base();
        }
    };

    /**
     * @brief Gets a range of prefix groups in the asset store
     * @return boost::iterator_range containing prefix_group_iterators
     *
     * Provides access to all unique prefix groups through:
     * @code
     * for(auto& group: store.prefixes()) {
     *     std::cout << "Prefix: " << group.prefix();
     * }
     * @endcode
     */
    boost::iterator_range<prefix_group_iterator<store_type::prefix_index>> prefixes() const {
        using iterator = prefix_group_iterator<store_type::prefix_index>;

        const auto& index = _store.by_prefix();
        return boost::make_iterator_range(
            iterator(index, index.begin()),
            iterator(index, index.end())
            );
    }

    /**
     * @brief Proxy container for accessing grouped prefix information
     *
     * Exposes iterator access to prefix groups and their sizes through:
     * - begin()/end() for iteration
     * - size() for group count
     */
    struct prefix_proxy{
        using iterator = prefix_group_iterator<store_type::prefix_index>;
        using iterator_range = boost::iterator_range<iterator>;
        using size_type = store_type::prefix_index::size_type;

        explicit inline prefix_proxy(iterator_range range): _range(range) {}
        prefix_proxy(const prefix_proxy&) = default;

        /**
         * @brief Gets iterator to the first prefix group
         * @return iterator pointing to first prefix group
         */
        iterator  begin() const { return _range.begin(); }
        /**
         * @brief Gets iterator past the last prefix group
         * @return iterator pointing after last prefix group
         */
        iterator  end() const   { return _range.end(); }
        /**
         * @brief Gets total number of prefix groups
         * @return Number of unique prefixes in the store
         */
        size_type size() const  { return std::distance(begin(), end()); }

        friend auto metatype(udho::view::data::type<prefix_proxy>){
            using namespace udho::view::data;

            return assoc("resources_asset_const_store_prefix_proxy"),
                   iter(&prefix_proxy::begin, &prefix_proxy::end),
                   fvar("size", &prefix_proxy::size);
        }

    private:
        iterator_range _range;
    };

    /**
     * @brief Creates a prefix grouping proxy for script engine exposure
     * @return prefix_proxy object providing grouped access to prefixes
     *
     * Used to expose prefix grouping functionality to template engines through:
     * @code
     * auto proxy = store.make_prefix_proxy();
     * for(const auto& group: proxy) {
     *     std::cout << "Prefix: " << group.prefix();
     *
     *     for(const auto& asset: group){
     *          std::cout << asset.name() << std::endl;
     *     }
     * }
     * @endcode
     */
    prefix_proxy make_prefix_proxy() const {
        return prefix_proxy{prefixes()};
    }
public:
    /**
     * @brief exposed to lua via the following properties
     * +----------+------------+
     * | ipairs   | function() |
     * +----------+------------+
     *
     * The iterator returns @ref resources::asset::proxy as value type which is also exposed to lua
     */
    friend auto metatype(udho::view::data::type<const_store>){
        using namespace udho::view::data;

        return assoc("resources_asset_const_store"),
               iter(&const_store::cbegin, &const_store::cend),
               index(&const_store::at, &const_store::csize),
               fvar("size", &const_store::csize),
               fvar("prefixes", &const_store::make_prefix_proxy);
    }

private:
    template <typename OstreamT>
    inline bool serve(OstreamT& ostream, uri_const_iterator it) const {
        if(it == end()){
            return false;
        }

        ostream.set(boost::beast::http::field::connection, "keep-alive");
        ostream.status(boost::beast::http::status::ok);
        it->write(ostream);
        ostream.finish();
        return true;
    }

private:
    const store_type& _store;
};



}

}
}
}

#endif // UDHO_VIEW_RESOURCES_ASSET_CONST_STORE_H
