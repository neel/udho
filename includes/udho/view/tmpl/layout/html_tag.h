#ifndef UDHO_VIEW_LAYOUT_HTML_TAG_H
#define UDHO_VIEW_LAYOUT_HTML_TAG_H

#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <udho/url/detail/format.h>
#include <udho/utils/property_map.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

/**
 * @struct html_tag
 * @brief Represents an HTML tag with attributes and rendering capabilities.
 *
 * This class inherits from property_map to manage HTML attributes as key-value pairs.
 * It supports generating properly formatted HTML tags, including handling of self-closing tags
 * and HTML entity escaping for attribute values.
 *
 * Example usage of the tag class:
 * @code{.cpp}
 * html_tag div("div");
 * div.add_class({"container", "main-content"});
 * div.property("id", "page-container");
 * div.property("data-info", "user-dashboard");
 * std::cout << div.render() << std::endl;
 * // Output: <div class="container main-content" id="page-container" data-info="user-dashboard"></div>
 *
 * html_tag img("img");
 * img.property("src", "image.jpg?size=large");
 * img.property("alt", "A scenic view");
 * std::cout << img.render() << std::endl;
 * // Output: <img src="image.jpg?size=large" alt="A scenic view" />
 * @endcode
 */
template <bool Modifiable=false>
struct basic_html_tag: udho::utils::property_map<std::string, std::string>{
    using key_type              = std::string;
    using value_type            = std::string;
    using tag_type              = basic_html_tag<Modifiable>;
    using pmap_type             = udho::utils::property_map<key_type, value_type>;

    /**
     * @brief Static list of HTML5 void elements that self-close
     * @see https://html.spec.whatwg.org/multipage/syntax.html#void-elements
     */
    static inline const std::unordered_set<std::string> self_closing_tags = {
        "area", "base", "br", "col", "embed", "hr", "img",
        "input", "link", "meta", "param", "source", "track", "wbr"
    };

    /**
     * @brief Constructs a html_tag with the specified name
     * @param name HTML html_tag name (e.g., "div", "img")
     */
    inline explicit basic_html_tag(const std::string& name): _name(name) {}

    /**
     * @brief Gets the html_tag name
     * @return Current html_tag name as const reference
     */
    inline const std::string& tag() const { return _name; }

    /**
     * @brief checks whether an attribute is set or not
     * @param attr the attribute to check
     * @return bool
     * @code
     * html_tag div("div");
     * div.has("id"); // should return false
     * div.id("div_id");
     * div.has("id"); // should return true
     * @endcode
     */
    inline bool has(const std::string& attr) const {
        return pmap_type::count(attr) > 0;
    }

    /**
     * @brief sets id of the HTML element
     * @details if no tag name is set yet, then sets name to div, to avoid creation of invalid HTML element
     * @param name the id to be set
     * @return Reference to self for method chaining
     */
    inline tag_type& id(const std::string& name){
        pmap_type::property("id", name);
        if(_name.empty()){
            _name = "div";
        }
        return *this;
    }

    /**
     * @brief returns id of the element if set, otherwise returns an empty string
     * @return std::string representing the id of the element (empty string in case no id is set)
     */
    inline std::string id() const {
        optional_value_type v = pmap_type::property("id");
        if(v){
            return *v;
        }
        return std::string{};
    }

    /**
     * @brief Adds multiple class names to the element
     * @param list Initializer list of class names to add
     * @return Reference to self for method chaining
     * @note Preserves existing classes while adding new ones
     * @code{.cpp}
     * html_tag div("div");
     * div.add_class({"container", "active"});
     * @endcode
     */
    inline tag_type& add_class(std::initializer_list<std::string>&& list){
        for(const auto& class_name: list){
            add_class(class_name);
        }
        return *this;
    }

    inline tag_type& classes(std::initializer_list<std::string>&& list){
        return add_class(list);
    }

    /**
     * @brief Adds a single class name to the element
     * @details if no tag name is set yet, then sets name to div, to avoid creation of invalid HTML element
     * @param class_name Class name to add
     * @return Reference to self for method chaining
     * @code{.cpp}
     * html_tag button("button");
     * button.add_class("primary");
     * button.add_class("large");
     * @endcode
     */
    inline tag_type& add_class(const std::string& class_name){
        pmap_type::property("class", class_name);
        if(_name.empty()){
            _name = "div";
        }
        return *this;
    }

    inline tag_type& classes(const std::string& class_name){
        return add_class(class_name);
    }

    /**
     * Iterating through classes:
     * @code{.cpp}
     * html_tag div("div");
     * div.add_class({"container", "active"});
     * for (const std::string& cls : div.classes()) {
     *     std::cout << "Class: " << cls << std::endl;
     * }
     * @endcode
     */
    auto classes() const { return values("class"); }

    /**
     * @brief Renders the HTML html_tag with attributes
     * @return Formatted HTML html_tag string with proper escaping
     *
     * Key Features:
     * - Automatically handles self-closing tags (e.g., <img />)
     * - Escapes HTML entities in attribute values to prevent XSS
     * - Supports boolean attributes (attributes without values)
     * - Merges multiple values for the same attribute key
     * - Trims whitespace from attribute values
     * - Uses XHTML-style closing for self-closing tags
     *
     * @note Attribute Handling Rules:
     * 1. Multiple values for the same attribute are space-joined
     * 2. Empty values after trimming become boolean attributes
     * 3. Values are HTML-entity escaped automatically
     * 4. Attributes are rendered in insertion order
     *
     * Example with boolean attribute and value trimming:
     * @code{.cpp}
     * html_tag input("input");
     * input.property("type", "checkbox");
     * input.property("checked", "");  // Boolean attribute
     * input.property("data-extra", "  ");  // Will be trimmed to empty
     * std::cout << input.open();
     * // Output: <input type="checkbox" checked data-extra />
     * @endcode
     */
    inline std::string open() const {
        std::ostringstream oss;
        oss << "<" << _name;
        std::vector<std::string> attributes;

        auto current = pmap_type::begin();
        while (current != pmap_type::end()) {
            const auto key = current->first;
            const auto range = pmap_type::equal_range(key);

            std::vector<std::string> values;
            for (auto it = range.first; it != range.second; ++it) {
                values.emplace_back(escape_html(it->second));
            }

            if (!values.empty()) {
                std::string vstr = boost::algorithm::join(values, " ");
                boost::algorithm::trim(vstr);
                if(vstr.empty()){
                    attributes.emplace_back(key);
                } else {
                    attributes.emplace_back(udho::url::format("{}=\"{}\"", key, vstr));
                }
            }

            current = range.second;
        }

        if(!attributes.empty()){
            oss << " " << boost::algorithm::join(attributes, " ");
        }

        oss << (self_closing_tags.count(_name) ? " />" : ">");
        return oss.str();
    }

    /**
     * @brief Renders the closing tag
     * @return Closing tag string or empty string for self-closing tags
     * @code{.cpp}
     * html_tag div("div");
     * std::cout << div.open(); // <div>
     * std::cout << "Content";
     * std::cout << div.close(); // </div>
     * @endcode
     */
    inline std::string close() const {
        return self_closing_tags.count(_name) ? "" : "</" + _name + ">";
    }

    protected:
    std::string _name;

    /**
     * @brief Escapes HTML special characters in attribute values
     * @param input Original string
     * @return String with HTML entities escaped
     */
    static std::string escape_html(const std::string& input) {
        std::string output;
        output.reserve(input.size());
        for (char c : input) {
            switch (c) {
            case '&':  output += "&amp;"; break;
            case '\"': output += "&quot;"; break;
            case '\'': output += "&apos;"; break;
            case '<':  output += "&lt;";  break;
            case '>':  output += "&gt;";  break;
            default:   output += c;       break;
            }
        }
        return output;
    }
};

template <>
struct basic_html_tag<true>: basic_html_tag<false>{
    inline explicit basic_html_tag(const std::string& name): basic_html_tag<false>(name) {}
    inline explicit basic_html_tag(): basic_html_tag<false>("") {}

    /**
     * @brief Sets the html_tag name
     * @param name New html_tag name
     * @return Reference to self for method chaining
     */
    inline tag_type& tag(const std::string& name) { _name = name; return *this; }

    inline bool isset() const {return !_name.empty(); }
};

using html_tag = basic_html_tag<true>;
using html_tag_fixed = basic_html_tag<false>;

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_HTML_TAG_H

