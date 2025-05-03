#ifndef UDHO_VIEW_LAYOUT_LOADER_H
#define UDHO_VIEW_LAYOUT_LOADER_H

#include <string>
#include <udho/url/detail/format.h>
#include <udho/view/tmpl/layout/property_map.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/asset/const_substore.h>
#include <udho/view/tmpl/layout/property_map.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

/**
 * @defgroup AssetLoaders Asset Loader System
 * @brief Manages loading and embedding of web assets (JS/CSS) with dependency tracking
 */


/**
 * @struct common_asset_loader
 * @ingroup AssetLoaders
 * @brief Base class for asset loading functionality
 * @tparam AssetType Type of asset to handle (js/css from udho::view::resources::asset::type)
 *
 * Provides common functionality for:
 * - Tracking selected assets
 * - Preventing duplicate additions
 * - Managing embed/inline vs external reference modes
 */
template <udho::view::resources::asset::type AssetType>
struct common_asset_loader{
    using substore_type     = udho::view::resources::asset::const_substore<AssetType>;
    using composite_const_iterator = typename substore_type::composite_const_iterator;
    using selection_type    = std::map<composite_const_iterator, bool>;

    common_asset_loader(const common_asset_loader&) = delete;
    common_asset_loader(common_asset_loader&& other): _selection(std::move(other._selection)) {}

    /**
     * @brief Add an asset to the loader
     * @pre The specified asset must be added to the store already
     * @post adds the requested asset into the selection unless it has already been added
     * @param prefix Asset namespace/prefix
     * @param name Asset name within the prefix
     * @param embedded boolean value indicating whether the resource is embedded or not
     * @throws std::out_of_range if asset not found in store
     */
    bool add(const std::string& prefix, const std::string& name, bool embedded = false) {
        composite_const_iterator it = _store.find(prefix, name);
        if(it.valid()){
            if(_selection.count(it)){
                return false;
            } else{
                return _selection.insert(std::make_pair(it, embedded)).second;
            }
        } else {
            throw std::out_of_range{udho::url::format("Refering to asset :{}/{} which was never registered to the store", prefix, name)};
        }
    }

    protected:
        common_asset_loader(const substore_type& store): _store(store) {}
        const substore_type& _store;
    protected:
        selection_type       _selection;

};

/**
 * @struct asset_loader
 * @ingroup AssetLoaders
 * @brief Generic asset loader template
 * @tparam AssetType Asset type specialization (js/css)
 *
 * Inherits common functionality from common_asset_loader
 */
template <udho::view::resources::asset::type AssetType>
struct asset_loader: common_asset_loader<AssetType>{
    using common_asset_loader_type = common_asset_loader<AssetType>;

    using common_asset_loader_type::common_asset_loader_type;
};

/**
 * @struct asset_loader<udho::view::resources::asset::type::js>
 * @ingroup AssetLoaders
 * @brief JavaScript-specific asset loader
 *
 * Adds JS-specific output capabilities:
 * - Importmap generation
 * - Script tag output
 */
template <>
struct asset_loader<udho::view::resources::asset::type::js>: common_asset_loader<udho::view::resources::asset::type::js>{
    using common_asset_loader_type = common_asset_loader<udho::view::resources::asset::type::js>;
    using substore_type            = typename common_asset_loader_type::substore_type;

    template <typename PlaceholderT>
    friend struct basic_document;

    asset_loader(const substore_type& store): common_asset_loader_type(store) {}

    public:
    /**
     * @brief write global importmap (includes all javascripts from all prefixes irrespective of whether they are requested or not)
     */
    udho::net::stream& importmap(udho::net::stream& stream) const {
        return _store.importmap(stream);
    }

    /**
     * @brief generate script tags only for the requested javascripts
     */
    udho::net::stream& write(udho::net::stream& stream) const {
        for(auto it: common_asset_loader_type::_selection){
            if(!it.second){ // Not embedded
                const udho::view::resources::asset::proxy& asset_proxy = *(it.first);
                const udho::view::resources::asset::basic_resource<udho::view::resources::asset::type::js>& asset_js = asset_proxy.template cast<udho::view::resources::asset::type::js>();
                const udho::view::resources::asset::asset_policy<udho::view::resources::asset::type::js>& policy = asset_js.policy();

                html_tag_fixed script("script");

                script.property("src", asset_proxy.url());

                if (policy.is_async())      script.property("async", "");
                if (policy.is_defer())      script.property("defer", "");
                if (policy.is_nomodule())   script.property("nomodule", "");

                if (policy.is_module()) {
                    script.property("type", "module");
                }

                if (!policy.cross_origin().empty()) {
                    script.property("crossorigin", policy.cross_origin());
                }

                if (!policy.referrer_policy().empty()) {
                    script.property("referrerpolicy", policy.referrer_policy());
                }

                stream << script.open() << script.close() << "\n";
            }
        }
        return stream;
    }

    /**
     * @brief generate script tags only for the requested javascripts
     */
    udho::net::stream& write_embedded(udho::net::stream& stream) const {
        for(auto it: common_asset_loader_type::_selection){
            if(it.second){ // Embedded
                const udho::view::resources::asset::proxy& asset_proxy = *(it.first);
                const udho::view::resources::asset::basic_resource<udho::view::resources::asset::type::js>& asset_js = asset_proxy.template cast<udho::view::resources::asset::type::js>();
                const udho::view::resources::asset::asset_policy<udho::view::resources::asset::type::js>& policy = asset_js.policy();

                html_tag_fixed script("script");

                if (policy.is_module()) {
                    script.property("type", "module");
                } else {
                    script.property("type", "text/javascript");
                }

                if (policy.is_async()) script.property("async", "");
                if (policy.is_defer()) script.property("defer", "");
                if (policy.is_nomodule()) script.property("nomodule", "");

                if (!policy.cross_origin().empty()) {
                    script.property("crossorigin", policy.cross_origin());
                }

                if (!policy.referrer_policy().empty()) {
                    script.property("referrerpolicy", policy.referrer_policy());
                }

                stream << script.open() << "\n";
                asset_proxy.write_contents(stream);
                stream << "\n" << script.close() << "\n";
            }


        }
        return stream;
    }
};

/**
 * @struct asset_loader<udho::view::resources::asset::type::css>
 * @ingroup AssetLoaders
 * @brief CSS-specific asset loader
 *
 * Handles CSS-specific output considerations:
 * - Media query support
 * - Style embedding vs link tags
 */
template <>
struct asset_loader<udho::view::resources::asset::type::css>: common_asset_loader<udho::view::resources::asset::type::css>{
    using common_asset_loader_type = common_asset_loader<udho::view::resources::asset::type::css>;
    using substore_type            = typename common_asset_loader_type::substore_type;

    template <typename PlaceholderT>
    friend struct basic_document;

    asset_loader(const substore_type& store): common_asset_loader_type(store) {}

    public:
    /**
     * @brief generate link or style tags only for the requested stylesheets
     */
    udho::net::stream& write(udho::net::stream& stream) const {
        for(auto it: common_asset_loader_type::_selection){
            if(!it.second){ // Not embedded
                const udho::view::resources::asset::proxy& asset_proxy = *(it.first);
                const udho::view::resources::asset::basic_resource<udho::view::resources::asset::type::css>& a = asset_proxy.template cast<udho::view::resources::asset::type::css>();
                const auto& policy = a.policy();
                stream << udho::url::format("<link rel=\"stylesheet\" type=\"text/css\" href=\"{}\" media=\"{}\">", asset_proxy.url(), policy.media()) << "\n";
            }
            // stream << udho::url::format("<style media=\"{}\">", policy.media()) << "\n";
            // it->write_contents(stream);
            // stream << "</style>" << "\n";
        }
        return stream;
    }
};



}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LOADER_H
