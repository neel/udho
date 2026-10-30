#ifndef UDHO_UTILS_PROPERTY_TREE_H
#define UDHO_UTILS_PROPERTY_TREE_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <udho/utils/property_map.h>

namespace udho{
namespace utils{

template <typename Key = std::string, typename Value = std::string>
struct property_tree: private property_map<Key, Value>{
    using key_type          = Key;
    using value_type        = Value;
    using label_type        = value_type;
    using tree_type         = property_tree<key_type, value_type>;
    using children_type     = std::vector<tree_type>;
    using const_iterator    = typename children_type::const_iterator;
    using iterator          = typename children_type::iterator;
    using const_range       = std::pair<const_iterator, const_iterator>;
    using range             = std::pair<iterator, iterator>;
    using size_type         = typename children_type::size_type;
    using properties_type   = property_map<key_type, value_type>;
    using path_type         = std::string;

    using properties_type::property;
    using properties_type::operator[];
    using properties_type::values;
    using properties_type::clear;
    using properties_type::count;
    using properties_type::empty;

    property_tree(const value_type& label): _label(label) {}
    const value_type& label() const { return _label; }

    tree_type& add(const value_type& label) {
        auto it = _children.emplace(_children.end(), property_tree<key_type, value_type>{label});
        return *it;
    }

    size_type size() const { return _children.size(); }

    bool exists(const path_type& path) const {
        try {
            at(path);
            return true;
        } catch(const std::out_of_range&) {
            return false;
        }
    }

    value_type at(const path_type& path) const {
        std::vector<path_type> parts = split(path);
        const tree_type* node = this;
        for(size_type index = 0; index + 1 < parts.size(); ++index) node = &node->child(parts[index]);

        auto value = node->properties_type::property(key_type(parts.back()));
        if(!value) throw std::out_of_range("Property path does not exist: " + path);
        return value.value();
    }

    template <typename StreamT>
    StreamT& write(StreamT& stream, std::uint8_t indent = 0) const {
        stream << (int)indent << " " << _label << "\n";
        for(const tree_type& c: _children){
            c.write(stream, indent+1);
        }
        return stream;
    }

private:
    static std::vector<path_type> split(const path_type& path) {
        if(path.empty()) throw std::out_of_range("Property path is empty");

        std::vector<path_type> parts;
        typename path_type::size_type begin = 0;
        while(begin <= path.size()) {
            typename path_type::size_type end = path.find('.', begin);
            if(end == begin) throw std::out_of_range("Property path contains an empty segment: " + path);
            parts.emplace_back(path.substr(begin, end == path_type::npos ? path_type::npos : end - begin));
            if(end == path_type::npos) break;
            begin = end + 1;
        }
        if(parts.empty() || parts.back().empty()) throw std::out_of_range("Property path contains an empty segment: " + path);
        return parts;
    }

    const tree_type& child(const path_type& label) const {
        const tree_type* result = nullptr;
        for(const tree_type& candidate: _children) {
            if(candidate.label() != label_type(label)) continue;
            if(result) throw std::out_of_range("Property path contains an ambiguous child: " + label);
            result = &candidate;
        }
        if(!result) throw std::out_of_range("Property path child does not exist: " + label);
        return *result;
    }

private:
    value_type      _label;
    size_type       _order;
    children_type   _children;
};

}
}

#endif // UDHO_UTILS_PROPERTY_TREE_H
