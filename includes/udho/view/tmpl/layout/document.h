#ifndef UDHO_VIEW_LAYOUT_DOCUMENT_H
#define UDHO_VIEW_LAYOUT_DOCUMENT_H

#include <map>
#include <string>
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

/**
 * @struct meta_tags
 * @brief Represents a collection of HTML meta tags and http-equiv attributes.
 *
 * Inherits from property_map<std::string, std::string> to store standard meta tags.
 * Manages both standard meta tags (name/content) and http-equiv meta tags through an enum interface.
 */
struct meta_tags: property_map<std::string, std::string>{
    using properties_type = property_map<std::string, std::string>;

    /**
     * @enum http_equiv
     * @brief Enumeration of supported http-equiv meta tag types
     */
    enum http_equiv{
        content_security_policy,
        content_type,
        default_style,
        x_ua_compatible,
        refresh
    };

    using properties_type::property;
    using properties_type::operator[];
    using properties_type::count;
    using properties_type::empty;
    using properties_type::begin;
    using properties_type::end;

    /**
     * @brief Get the value of an http-equiv property
     * @param key The http-equiv enum key to retrieve
     * @return Optional string containing the property value
     */
    auto property(const http_equiv& key) const{
        return http_equiv_.property(key);
    }
    /**
     * @brief Set the value of an http-equiv property
     * @param key The http-equiv enum key to modify
     * @param value The value to set
     * @return Reference to the modified property_map entry
     */
    auto property(const http_equiv& key, const std::string& value){
        return http_equiv_.property(key, value);
    }
    /**
     * @brief Access operator for http-equiv properties
     * @param key The http-equiv enum key to access
     * @return Optional string containing the property value
     */
    auto operator[](const http_equiv& key) const{
        return http_equiv_.property(key);
    }

    /**
     * @brief Write all meta tags to a stream
     * @tparam Stream Type of output stream
     * @param stream Output stream to write to
     * @return Reference to the modified output stream
     *
     * Formats and outputs:
     * - charset meta tag if present
     * - http-equiv meta tags using enum mapping
     * - Standard meta tags (name/content)
     * - Open Graph meta tags (property/content)
     */
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

/**
 * @struct document_preamble
 * @brief Manages document declaration and basic structure attributes
 *
 * Contains settings for:
 * - DOCTYPE declaration
 * - Document language
 * - XML namespace
 * - Text direction
 * - Document classes
 * - Title
 * - Meta tags
 */
struct document_preamble{
    inline explicit document_preamble(): _doctype(true) {}

    /**
     * @brief Set whether to include DOCTYPE declaration
     * @param flag Boolean flag to enable/disable DOCTYPE
     * @return Reference to self for chaining
     */
    inline document_preamble& doctype(bool flag) { _doctype = flag; return *this; }
    /**
     * @brief Get current DOCTYPE setting
     * @return Boolean indicating if DOCTYPE is enabled
     */
    inline bool doctype() const { return _doctype; }

    /**
     * @brief Set document language
     * @param dl Language code (e.g., "en-US")
     * @return Reference to self for chaining
     */
    inline document_preamble& doclang(const std::string& dl) { _doc_lang = dl; return *this; }
    /**
     * @brief Get current document language
     * @return Current language code
     */
    inline const std::string& doclang() const { return _doc_lang; }

    /**
     * @brief Get current XML namespace
     * @return Current XML namespace URI
     */
    inline document_preamble& xmlns(const std::string& uri) { _xmlns = uri; return *this; }
    /**
     * @brief Get current XML namespace
     * @return Current XML namespace URI
     */
    inline const std::string& xmlns() const { return _xmlns; }

    /**
     * @brief Set text direction
     * @param d Direction value ("ltr" or "rtl")
     * @return Reference to self for chaining
     */
    inline document_preamble& dir(const std::string& d) { _dir = d; return *this; }
    /**
     * @brief Get current text direction
     * @return Current direction value
     */
    inline const std::string& dir() const { return _dir; }

    /**
     * @brief Set document classes
     * @param classnames Space-separated list of class names
     * @return Reference to self for chaining
     */
    inline document_preamble& classes(const std::string& classnames) { _doc_classes = classnames; return *this; }
    /**
     * @brief Get current document classes
     * @return Current class list
     */
    inline const std::string& classes() const { return _doc_classes; }

    /**
     * @brief Set document title
     * @param t Title text
     * @return Reference to self for chaining
     */
    inline document_preamble& title(const std::string& t) { _title = t; return *this; }
    /**
     * @brief Get current document title
     * @return Current title text
     */
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

template <typename PlaceholderT>
struct basic_document;

/**
 * @brief A document wraps multiple view outputs into an envelop.
 * @tparam PlaceholderT basic_placeholder<Spots...> placeholders for the view contents
 *
 * Responsibilities
 * -----------------
 * - Wrap multiple view contents into an envelop e.g. an html envelop will include the begining html, head, body etc.. tags
 * - Store a list of unique assets required to be loaded, that have been requsted by the views.
 * - Perform any of the following actions on the assets
 *   - Associate each asset to the layout by the asset url or by inline contents
 *   - Create a bundle of all assets of the same type and associate that bundle either by url or by inlining.
 * - Provide mechanism to set various attributes of the envelop.
 *   - Provides a property map for setting meta attributes
 *   - Provides a property map for setting link attributes
 * - Provide mechanism to set attributes for common document parts such as menus/columns etc..
 */
template <typename... Spots>
struct basic_document<basic_placeholder<Spots...>>: protected document_preamble, basic_placeholder<Spots...>{
    using placeholders_type = basic_placeholder<Spots...>;

    using placeholders_type::operator[];
    using placeholders_type::properties;

    using loader_js  = asset_loader<udho::view::resources::asset::type::js>;
    using loader_css = asset_loader<udho::view::resources::asset::type::css>;

    template <typename Key>
    using proxy_type = typename placeholders_type::template proxy_type<Key>;
    template <typename Key>
    using const_proxy_type = typename placeholders_type::template const_proxy_type<Key>;

    template <typename... Bridges>
    basic_document(const udho::view::resources::const_store<Bridges...>& store): _js(store.js()), _css(store.css()), _body("body") {}
    basic_document(const basic_document&) = delete;
    basic_document(basic_document&& other): _js(std::move(other._js)), _css(std::move(other._css)), _body(std::move(other._body)) {}

    loader_js& js() { return _js; }
    loader_css& css() { return _css; }

    const loader_js& js() const { return _js; }
    const loader_css& css() const { return _css; }

    layout::document_preamble& preamble() { return *this; }
    const layout::document_preamble& preamble() const { return *this; }

    udho::view::tmpl::layout::html_tag_fixed body() const { return _body; }
    const udho::view::tmpl::layout::html_tag_fixed& body() { return _body; }

    private:
        loader_js  _js;
        loader_css _css;
        udho::view::tmpl::layout::html_tag_fixed _body;
};

using standard_document = basic_document<placeholders::standard>;

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_DOCUMENT_H
