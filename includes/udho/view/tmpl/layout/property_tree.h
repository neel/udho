#ifndef UDHO_VIEW_LAYOUT_PROPERTY_TREE_H
#define UDHO_VIEW_LAYOUT_PROPERTY_TREE_H

#include <vector>
#include <string>
#include <cstdint>
#include <udho/view/tmpl/layout/property_map.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

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

    template <typename StreamT>
    StreamT& write(StreamT& stream, std::uint8_t indent = 0) const {
        stream << (int)indent << " " << _label << "\n";
        for(const tree_type& c: _children){
            c.write(stream, indent+1);
        }
        return stream;
    }

    private:
        value_type      _label;
        size_type       _order;
        children_type   _children;

};


// Presenter https://codepen.io/mohnaji94/pen/evbWGW
using menu = property_tree<std::string, std::string>;

}
}
}
}


#endif // UDHO_VIEW_LAYOUT_PROPERTY_TREE_H
