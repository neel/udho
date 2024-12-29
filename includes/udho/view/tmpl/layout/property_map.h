#ifndef UDHO_VIEW_LAYOUT_PROPERTY_MAP_H
#define UDHO_VIEW_LAYOUT_PROPERTY_MAP_H

#include <map>
#include <string>
#include <optional>
#include <exception>
#include <exception>

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
        auto range = _properties.equal_range(key);
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
    const_range values(const key_type& key) const {
        return _properties.equal_range(key);
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
        auto pair = values(key);
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

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_PROPERTY_MAP_H
