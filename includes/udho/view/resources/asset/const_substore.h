#ifndef UDHO_VIEW_RESOURCES_ASSET_CONST_SUBSTORE_H
#define UDHO_VIEW_RESOURCES_ASSET_CONST_SUBSTORE_H

#include <udho/view/resources/fwd.h>
#include <udho/view/resources/asset/const_store.h>

namespace udho{
namespace view{
namespace resources{

namespace asset{

/**
 * @addtogroup DoxyG_view_resources_assets
 * @{
 */

/**
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

    asset::proxy get(const std::string& prefix, const std::string& name) const {
        composite_const_iterator it = find(prefix, name);
        if(it.valid()){
            return *it;
        }
        throw std::out_of_range{udho::url::format("resource {}/{} not found in the asset store", prefix, name)};
    }
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
               iter(&self_type::cbegin, &self_type::cend),
               func("get",  &self_type::get),
               fvar("size", &self_type::size);
    }

private:
    const store_type& _store;
};

template <asset::type Type>
struct const_substore: basic_const_substore<Type>{
    using basic_const_store_type = basic_const_substore<Type>;

    using composite_const_iterator = typename basic_const_store_type::composite_const_iterator;

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
    udho::net::ostream_view& importmap(udho::net::ostream_view& stream, It begin, It end, Function&& f) const {
        stream << "<script type=\"importmap\">" << "\n";
        stream << "{" << "\n";
        std::vector<std::string> imports;
        for(It it = begin; it != end; ++it) {
            const auto& asset_proxy = *it;
            const auto& asset_js = asset_proxy.template cast<udho::view::resources::asset::type::js>();
            if(!asset_js.embedded() && f(it)) {
                imports.emplace_back(udho::url::format( "\t\t\"{}/{}\": \"{}\"", asset_proxy.prefix(), asset_proxy.name(), asset_proxy.url() ));
            }
        }
        stream << "\t\"imports\": {" <<"\n";
        if(!imports.empty()) {
            stream << boost::algorithm::join(imports, ",\n") << "\n";
        }
        stream << "\t}\n}\n</script>\n";
        return stream;
    }

    template <typename Function>
    udho::net::ostream_view& importmap(udho::net::ostream_view& stream, Function&& f) const {
        return importmap(stream, basic_const_store_type::begin(), basic_const_store_type::end(), std::forward<Function>(f));
    }

    template <typename Function>
    udho::net::ostream_view& importmap(udho::net::ostream_view& stream, const std::string& prefix, Function&& f) const {
        return importmap(stream, basic_const_store_type::begin(prefix), basic_const_store_type::end(prefix), std::forward<Function>(f));
    }

    udho::net::ostream_view& importmap(udho::net::ostream_view& stream) const {
        return importmap(stream, [](basic_const_store_type::type_const_iterator){ return true; });
    }

    udho::net::ostream_view& importmap(udho::net::ostream_view& stream, const std::string& prefix) const {
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

/** @} */

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_ASSET_CONST_SUBSTORE_H
