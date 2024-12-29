#ifndef UDHO_VIEW_LAYOUT_LAYOUT_H
#define UDHO_VIEW_LAYOUT_LAYOUT_H

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


// Placeholder
// Menu(s)
// Presenter


struct meta_tags: property_map<std::string, std::string>{
    using properties_type = property_map<std::string, std::string>;
    enum http_equiv{
        content_security_policy,
        content_type,
        default_style,
        x_ua_compatible,
        refresh
    };

    auto property(const http_equiv& key) const{
        return http_equiv_.property(key);
    }
    auto property(const http_equiv& key, const std::string& value){
        return http_equiv_.property(key, value);
    }
    auto operator[](const http_equiv& key) const{
        return http_equiv_.property(key);
    }

    std::ostream& write(std::ostream& stream){
        static std::map<http_equiv, std::string> http_equiv_keys = {
            {content_security_policy,   "content-security-policy"},
            {content_type,              "content-type"},
            {default_style,             "default-style"},
            {x_ua_compatible,           "x-ua-compatible"},
            {refresh,                   "refresh"}
        };

        if(properties_type::count("charset") == 1){
            stream << udho::url::format("<meta charset=\"{}\" />", properties_type::property("charset").value()) << "\n";
        }
        if(!http_equiv_.empty()){
            for(auto it = http_equiv_.begin(); it != http_equiv_.end(); ++it){
                stream << udho::url::format("<meta http-equiv=\"{}\" content=\"{}\" />", http_equiv_keys.at(it->first), it->second) << "\n";
            }
        }
        for(auto it = properties_type::begin(); it != properties_type::end(); ++it){
            const auto& key = it->first;
            if(key.rfind("og:", 0) == 0){
                stream << udho::url::format("<meta property=\"{}\" content=\"{}\" />", key, it->second) << "\n";
            } else {
                stream << udho::url::format("<meta name=\"{}\" content=\"{}\" />", key, it->second) << "\n";
            }
        }
        return stream;
    }

    private:
        property_map<http_equiv, std::string> http_equiv_;
};

template <udho::view::resources::asset::type AssetType>
struct common_asset_loader{
    using substore_type     = udho::view::resources::asset::const_substore<AssetType>;
    using selection_type    = std::set<typename substore_type::composite_const_iterator>;

    void add(const std::string& prefix, const std::string& name) {
        typename substore_type::composite_const_iterator it = _store.find(prefix, name);
        if(it != typename substore_type::composite_const_iterator()){
            _selection.emplace(it);
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

    private:
    udho::net::stream& importmap(udho::net::stream& stream){
        return _store.importmap(stream);
    }
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

    private:

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

struct preamble{
    preamble() {}
    preamble& doctype(const std::string& dt) { _doctype = dt; return *this; }
    const std::string& doctype() const { return _doctype; }
    preamble& doclang(const std::string& dl) { _doclang = dl; return *this; }
    const std::string& doclang() const { return _doclang; }
    preamble& title(const std::string& t) { _title = t; return *this; }
    const std::string& title() const { return _title; }

    public:
        meta_tags meta;
    private:
        std::string _doctype;
        std::string _doclang;
        std::string _title;
};

/**
 * @brief A layout wraps the view output into an envelop.
 *
 * Responsibilities
 * -----------------
 * - Wrap the view contents into an envelop e.g. an html envelop will include the begining html, head, body etc.. tags
 * - Store a list of unique assets that have been requsted by the views.
 * - Perform any of the following actions on the assets
 *   - Associate each asset to the layout by the asset url or by inline contents
 *   - Create a bundle of all assets of the same type and associate that bundle either by url or by inlining.
 * - Provide mechanism to set various attributes of the envelop.
 *   - Provides a property map for setting meta attributes
 *   - Provides a property map for setting link attributes
 * - Provide mechanism to set attributes for common document parts such as menus/columns etc..
 */
template <typename PlaceholderT>
struct basic_document: protected preamble, PlaceholderT{
    using preamble::doctype;
    using preamble::doclang;
    using preamble::title;
    using preamble::meta;
    using PlaceholderT::operator[];

    using loader_js  = asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = asset_loader<udho::view::resources::asset::type::css>;

    template <typename... Bridges>
    basic_document(const udho::view::resources::const_store<Bridges...>& store): _js(store.js()), _css(store.css()) {}
    loader_js& js() { return _js; }
    loader_css& css() { return _css; }
    const loader_js& js() const { return _js; }
    const loader_css& css() const { return _css; }

    private:
        loader_js  _js;
        loader_css _css;
};

template <typename DocumentT>
struct basic_presenter{

    std::ostream& operator()(std::ostream& stream) const{
        return stream;
    }
};

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LAYOUT_H
