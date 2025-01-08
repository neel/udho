#ifndef UDHO_VIEW_LAYOUT_LOADER_H
#define UDHO_VIEW_LAYOUT_LOADER_H

#include <set>
#include <map>
#include <string>
#include <optional>
#include <exception>
#include <udho/url/detail/format.h>
#include <udho/view/tmpl/layout/property_map.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/asset/store.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

template <udho::view::resources::asset::type AssetType>
struct common_asset_loader{
    using substore_type     = udho::view::resources::asset::const_substore<AssetType>;
    using selection_type    = std::set<typename substore_type::composite_const_iterator>;

    common_asset_loader(const common_asset_loader&) = delete;
    common_asset_loader(common_asset_loader&& other): _selection(std::move(other._selection)) {}

    void add(const std::string& prefix, const std::string& name) {
        typename substore_type::composite_const_iterator it = _store.find(prefix, name);
        if(it.valid()){
            _selection.emplace(it);
        } else {
            throw std::out_of_range{udho::url::format("Refering to asset :{}/{} which was nevered registered to the store", prefix, name)};
        }
    }

    void embed(bool flag) { _embed = flag; }
    bool embed() const { return _embed; }

    protected:
        common_asset_loader(const substore_type& store): _store(store), _embed(false) {}
        const substore_type& _store;
    private:
        bool _embed;
    protected:
        selection_type       _selection;

};

template <udho::view::resources::asset::type AssetType>
struct asset_loader: common_asset_loader<AssetType>{
    using common_asset_loader_type = common_asset_loader<AssetType>;

    using common_asset_loader_type::common_asset_loader_type;
};

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
    udho::net::stream& importmap(udho::net::stream& stream){
        return _store.importmap(stream);
    }
    /**
     * @brief generate script tags only for the requested javascripts
     */
    udho::net::stream& write(udho::net::stream& stream){
        for(auto it: common_asset_loader_type::_selection){
            if(!embed()){
                stream << udho::url::format("<script src=\"{}\"></script>", it->url()) << "\n";
            } else {
                stream << "<script>" << "\n";
                it->write_contents(stream);
                stream << "</script>" << "\n";
            }
        }
        return stream;
    }
};

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
    udho::net::stream& write(udho::net::stream& stream){
        for(auto it: common_asset_loader_type::_selection){
            const udho::view::resources::asset::basic_resource<udho::view::resources::asset::type::css>& a = it->template cast<udho::view::resources::asset::type::css>();
            const auto& policy = a.policy();
            if(!embed()){
                stream << udho::url::format("<link rel=\"stylesheet\" type=\"text/css\" href=\"{}\" media=\"{}\">", it->url(), policy.media()) << "\n";
            } else {
                stream << udho::url::format("<style media=\"{}\">", policy.media()) << "\n";
                it->write_contents(stream);
                stream << "</style>" << "\n";
            }
        }
        return stream;
    }
};



}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LOADER_H
