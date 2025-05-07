#ifndef UDHO_VIEW_LAYOUT_PROPERTY_MAP_H
#define UDHO_VIEW_LAYOUT_PROPERTY_MAP_H

#include <map>
#include <vector>
#include <unordered_set>
#include <string>
#include <sstream>
#include <optional>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <udho/url/detail/format.h>
#include <boost/range/adaptors.hpp>
#include <boost/range/iterator_range.hpp>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

/**
 * @brief A template class for mapping keys to multiple values.
 *
 * This class allows storing multiple values for each key using a multimap.
 * It provides methods to access, modify, and manage these values effectively.
 *
 * @tparam Key The type of the keys. Default is std::string.
 * @tparam Value The type of the values. Default is std::string.
 * @example Example usage of property_map and value_proxy:
 * @code
 * property_map<std::string, std::string> props;
 * props["color"] = "blue";    // Sets the color property clears all other values added for color key before blue
 * props["color"] += " red";   // Adds another value to the color property
 * if (props["color"]) {    // checks whether any value has been set for key color
 *     std::cout << "Color is set to " << *props["color"].value() << std::endl;
 * }
 * for(auto& color: props.values("color")) {
 *     std::cout << "Available color: " << color.second << std::endl;
 * }
 * @endcode
 */
template <typename Key = std::string, typename Value = std::string>
struct property_map{
    using key_type              = Key;
    using value_type            = Value;
    using optional_value_type   = std::optional<value_type>;
    using map_type              = std::multimap<Key, Value>;
    using const_iterator        = typename map_type::const_iterator;
    using iterator              = typename map_type::iterator;
    using const_range           = std::pair<const_iterator, const_iterator>;
    using range                 = std::pair<iterator, iterator>;
    using size_type             = typename map_type::size_type;

    /**
     * @brief A proxy class to provide value access and manipulation capabilities for a specific key within the property_map.
     *
     * This proxy class enables adding, setting, and getting values for a specific key. It supports both assignment and addition operations.
     */
    struct value_proxy{
         /**
          * @brief Constructs a value proxy for a given key within a property map.
          *
          * @param map Reference to the property map.
          * @param key The key associated with this proxy.
          */
        value_proxy(property_map& map, const key_type& key): _map(map), _key(key) {}
        /**
         * @brief Adds a value to the property map under the associated key.
         *
         * @param value The value to add.
         * @return Reference to the parent property_map for chaining.
         */
        property_map& operator+=(const key_type& value){
            return _map.property(_key, value);
        }
        /**
         * @brief Sets a new value for the associated key, replacing any existing values.
         *
         * @param value The value to set.
         * @return Reference to the parent property_map for chaining.
         */
        property_map& operator=(const key_type& value){
            _map.clear(_key);
            return _map.property(_key, value);
        }
        /**
         * @brief Retrieves the current value associated with the key, if any.
         *
         * @return An optional containing the value if it exists; otherwise, std::nullopt.
         */
        optional_value_type value() const{
            return _map.property(_key);
        }
        /**
         * @brief Checks if the key has any associated value in the property map.
         *
         * @return True if the key has a value, false otherwise.
         */
        operator bool() const {
            return _map.property(_key).has_value();
        }
        /**
         * @brief Dereferences the current value associated with the key.
         *
         * @return A reference to the value associated with the key.
         */
        const value_type& operator*() const&{
            return *(_map.property(_key));
        }
        private:
            property_map& _map;
            Key           _key;
    };

    /**
     * @brief Retrieves a property value by key.
     *
     * @param key The key for which to retrieve the value.
     * @return An optional value associated with the key.
     */
    optional_value_type property(const key_type& key) const {
        auto range = equal_range(key);
        if (range.first != range.second) {
            return range.first->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Accesses or modifies properties associated with a given key.
     *
     * @param key The key to access or modify.
     * @return A value_proxy object which can be used to manipulate values.
     */
    value_proxy operator[](const key_type& key) const {
        return value_proxy{*this, key};
    }
    /**
     * @brief Gets the range of values associated with a key.
     *
     * @param key The key whose values are to be accessed.
     * @return A const_range of values associated with the key.
     */
    const_range equal_range(const key_type& key) const {
        return _properties.equal_range(key);
    }

    /**
     * @brief Gets a range of all values associated with the same key
     * @return Boost range of class name strings
     * @note Returns a transformed range that directly provides class names
     * @example Iterating through classes:
     * @code
     * for (const std::string& cls : tag.values("class")) {
     *     std::cout << "Class: " << cls << std::endl;
     * }
     *
     * // Convert to vector
     * std::vector<std::string> classList{tag.values("class").begin(), tag.values("class").end()};
     * @endcode
     */
    auto values(const key_type& key) const {
        auto raw_range = equal_range(key);
        return boost::make_iterator_range(raw_range.first, raw_range.second) | boost::adaptors::transformed([](const auto& pair) {
             return pair.second;
         });
    }
    /**
     * @brief Adds a property value under a specified key.
     *
     * @param key The key under which to store the value.
     * @param val The value to store.
     * @return A reference to this property_map for chaining.
     */
    property_map& property(const key_type& key, const value_type& val){
        _properties.insert(std::make_pair(key, val));
        return *this;
    }
    /**
     * @brief Clears a specific value from a property.
     *
     * @param key The key from which to clear the value.
     * @param val The value to clear.
     */
    void clear(const key_type& key, const value_type& val) {
        auto range = _properties.equal_range(key);
        for (auto it = range.first; it != range.second; ) {
            if (it->second == val) {
                it = _properties.erase(it);
            } else {
                ++it;
            }
        }
    }
    /**
     * @brief Clears all values associated with a key.
     *
     * @param key The key whose values are to be cleared.
     */
    void clear(const key_type& key) {
        auto range = _properties.equal_range(key);
        if (range.first != range.second) {
            _properties.erase(range.first, range.second);
        }
    }
    /**
     * @brief Counts the number of values associated with a key.
     *
     * @param key The key whose values are to be counted.
     * @return The number of values associated with the key.
     */
    size_type count(const key_type& key) const {
        auto pair = equal_range(key);
        return std::distance(pair.first, pair.second);
    }

    /**
     * @brief Checks whether the map is empty
     *
     * @return boolean
     */
    bool empty() const {
        return _properties.size() == 0;
    }

    const_iterator begin() const { return _properties.begin(); }
    const_iterator end() const   { return _properties.end();   }

    private:
        map_type _properties;
};

/**
 * @struct html_tag
 * @brief Represents an HTML tag with attributes and rendering capabilities.
 *
 * This class inherits from property_map to manage HTML attributes as key-value pairs.
 * It supports generating properly formatted HTML tags, including handling of self-closing tags
 * and HTML entity escaping for attribute values.
 *
 * @example Example usage of the tag class:
 * @code
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
struct basic_html_tag: property_map<std::string, std::string>{
    using key_type              = std::string;
    using value_type            = std::string;
    using tag_type              = basic_html_tag<Modifiable>;
    using pmap_type             = property_map<key_type, value_type>;

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
     * @example
     * html_tag div("div");
     * div.add_class({"container", "active"});
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
     * @example
     * html_tag button("button");
     * button.add_class("primary");
     * button.add_class("large");
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
     * @example Iterating through classes:
     * @code
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
     * @example Example with boolean attribute and value trimming:
     * @code
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
     * @example
     * html_tag div("div");
     * std::cout << div.open(); // <div>
     * std::cout << "Content";
     * std::cout << div.close(); // </div>
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

#endif // UDHO_VIEW_LAYOUT_PROPERTY_MAP_H
