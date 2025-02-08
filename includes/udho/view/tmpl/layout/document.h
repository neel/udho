#ifndef UDHO_VIEW_LAYOUT_DOCUMENT_H
#define UDHO_VIEW_LAYOUT_DOCUMENT_H

#include <set>
#include <map>
#include <string>
#include <optional>
#include <exception>
#include <udho/url/detail/format.h>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/property_map.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/store.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

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

    template <typename Stream>
    Stream& write(Stream& stream) const {
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

struct document_preamble{
    document_preamble(): _doctype(true) {}

    inline document_preamble& doctype(bool flag) { _doctype = flag; return *this; }
    inline bool doctype() const { return _doctype; }

    inline document_preamble& doclang(const std::string& dl) { _doc_lang = dl; return *this; }
    inline const std::string& doclang() const { return _doc_lang; }

    inline document_preamble& xmlns(const std::string& uri) { _xmlns = uri; return *this; }
    inline const std::string& xmlns() const { return _xmlns; }

    inline document_preamble& dir(const std::string& d) { _dir = d; return *this; }
    inline const std::string& dir() const { return _dir; }

    inline document_preamble& classes(const std::string& classnames) { _doc_classes = classnames; return *this; }
    inline const std::string& classes() const { return _doc_classes; }

    inline document_preamble& title(const std::string& t) { _title = t; return *this; }
    inline const std::string& title() const { return _title; }

    public:
        meta_tags meta;
    private:
        bool        _doctype;
        std::string _doc_lang;
        std::string _xmlns;
        std::string _dir;
        std::string _doc_classes;
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
struct basic_document: protected document_preamble, PlaceholderT{
    using PlaceholderT::operator[];
    using PlaceholderT::properties;

    using placeholders_type = PlaceholderT;

    using loader_js  = asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = asset_loader<udho::view::resources::asset::type::css>;

    template <typename... Bridges>
    basic_document(const udho::view::resources::const_store<Bridges...>& store): _js(store.js()), _css(store.css()) {}
    basic_document(const basic_document&) = delete;
    basic_document(basic_document&& other): _js(std::move(other._js)), _css(std::move(other._css)) {}

    loader_js& js() { return _js; }
    loader_css& css() { return _css; }

    const loader_js& js() const { return _js; }
    const loader_css& css() const { return _css; }

    layout::document_preamble& preamble() { return *this; }
    const layout::document_preamble& preamble() const { return *this; }

    private:
        loader_js  _js;
        loader_css _css;
};

using standard_document = basic_document<placeholders::standard>;

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_DOCUMENT_H
