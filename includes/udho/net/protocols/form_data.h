#ifndef UDHO_NET_FORM_DATA_H
#define UDHO_NET_FORM_DATA_H

#include <map>
#include <string>
#include <variant>
#include <stdexcept>
#include <boost/beast/core/multi_buffer.hpp>

#include <boost/filesystem.hpp>

#include <udho/utils/filesystem.h>
#include <udho/utils/format.h>

namespace udho{
namespace net{
namespace protocols{

namespace detail{

struct field_value_type{
    using path_type = udho::utils::filesystem::path;
    using variant_type = std::variant<
            std::string,
            path_type,
            boost::beast::multi_buffer // no need for flat_buffer, if we want contigous memory then std::string can be used
        >;

    enum class types{
        none,
        string,
        path,
        buffer
    };

    explicit field_value_type(const std::string& name, types t): _name(name), _type(t) {
        if (t == types::string) _value = std::string{};
        if (t == types::path)   _value = path_type{};
        if (t == types::buffer) _value = boost::beast::multi_buffer{};
    }
    field_value_type(const std::string& name, const std::string& str): _name(name), _type(types::string) {
        value(str);
    }
    field_value_type(const std::string& name, const path_type& path): _name(name), _type(types::path) {
        value(path);
    }
    field_value_type(const std::string& name, boost::beast::multi_buffer&& buffer): _name(name), _type(types::buffer) {
        value(std::forward<boost::beast::multi_buffer>(buffer));
    }
    field_value_type(const std::string& name, std::string&& str): _name(name), _type(types::string) {
        value(std::forward<std::string>(str));
    }
    field_value_type(const std::string& name, path_type&& path): _name(name), _type(types::path) {
        value(std::forward<path_type>(path));
    }

    field_value_type(const field_value_type&) = default;
    field_value_type(field_value_type&&) = default;

    types type() const { return _type; }
    const std::string& name() const { return _name; }
    const variant_type& value() const { return _value; }

    void value(const std::string& str) {
        if(!is_string()) {
            throw std::domain_error{udho::utils::format("field {} expects string values only", _name)};
        }
        _value = str;
    }
    void value(const path_type& path)  {
        if(!is_path()) {
            throw std::domain_error{udho::utils::format("field {} expects path values only", _name)};
        }
        _value = path;
    }
    void value(std::string&& str) {
        if(!is_string()) {
            throw std::domain_error{udho::utils::format("field {} expects string values only", _name)};
        }
        _value = std::move(str);
    }
    void value(path_type&& path)  {
        if(!is_path()) {
            throw std::domain_error{udho::utils::format("field {} expects path values only", _name)};
        }
        _value = std::move(path);
    }
    void value(boost::beast::multi_buffer&& buffer)  {
        if(!is_buffer()) {
            throw std::domain_error{udho::utils::format("field {} expects buffer values only", _name)};
        }
        _value = std::move(buffer);
    }

    template <typename V>
    field_value_type& operator=(const V& v) {
        value(v);
        return *this;
    }

    template <typename V>
    field_value_type& operator=(V&& v) {
        value(std::forward<V>(v));
        return *this;
    }

    bool has_string() const { return std::holds_alternative<std::string>(_value); }
    bool has_path() const { return std::holds_alternative<path_type>(_value); }
    bool has_buffer() const { return std::holds_alternative<boost::beast::multi_buffer>(_value); }

    bool is_string() const { return _type == types::string; }
    bool is_path() const { return _type == types::path; }
    bool is_buffer() const { return _type == types::buffer; }

    std::string& string() {
        if(is_string()) {
            if(has_string()) {
                return std::get<std::string>(_value);
            } else {
                throw std::length_error{udho::utils::format("Trying to call string() on field {} which does not contain a string", _name)};
            }
        } else {
            throw std::domain_error{udho::utils::format("Trying to call string() on field {} which is not string type", _name)};
        }
    }

    path_type& path() {
        if(is_path()) {
            if(has_path()) {
                return std::get<path_type>(_value);
            } else {
                throw std::length_error{udho::utils::format("Trying to call path() on field {} which does not contain a path", _name)};
            }
        } else {
            throw std::domain_error{udho::utils::format("Trying to call path() on field {} which is not path type", _name)};
        }
    }

    boost::beast::multi_buffer& buffer() {
        if(is_buffer()) {
            if(has_buffer()) {
                return std::get<boost::beast::multi_buffer>(_value);
            } else {
                throw std::length_error{udho::utils::format("Trying to call buffer() on field {} which does not contain a buffer", _name)};
            }
        } else {
            throw std::domain_error{udho::utils::format("Trying to call buffer() on field {} which is not buffer type", _name)};
        }
    }

    const std::string& string() const {
        if(is_string()) {
            if(has_string()) {
                return std::get<std::string>(_value);
            } else {
                throw std::length_error{udho::utils::format("Trying to call string() on field {} which does not contain a string", _name)};
            }
        } else {
            throw std::domain_error{udho::utils::format("Trying to call string() on field {} which is not string type", _name)};
        }
    }

    const path_type& path() const {
        if(is_path()) {
            if(has_path()) {
                return std::get<path_type>(_value);
            } else {
                throw std::length_error{udho::utils::format("Trying to call path() on field {} which does not contain a path", _name)};
            }
        } else {
            throw std::domain_error{udho::utils::format("Trying to call path() on field {} which is not path type", _name)};
        }
    }

    const boost::beast::multi_buffer& buffer() const {
        if(is_buffer()) {
            if(has_buffer()) {
                return std::get<boost::beast::multi_buffer>(_value);
            } else {
                throw std::length_error{udho::utils::format("Trying to call buffer() on field {} which does not contain a buffer", _name)};
            }
        } else {
            throw std::domain_error{udho::utils::format("Trying to call buffer() on field {} which is not buffer type", _name)};
        }
    }

private:
    std::string     _name;
    types           _type;
    variant_type    _value;

};

struct form_view;

/**
 * @brief Container for parsed multipart form fields and uploaded files.
 *
 * This class holds the results of parsing a `multipart/form-data` body.
 * Fields are stored as strings, file uploads as `boost::filesystem::path`
 * pointing to a temporary file on disk. The class also tracks an iterator
 * to the currently active part during incremental parsing.
 *
 * The container is a `std::multimap<std::string, field_value_type>` to
 * allow multiple fields with the same name (e.g., multiple file uploads).
 */
struct form_data{
    friend struct form_view;

    /// Underlying multimap container.
    using form_container_type   = std::multimap<std::string, field_value_type>;
    /// Iterator (mutable).
    using form_iterator         = form_container_type::iterator;
    /// Const iterator.
    using form_const_iterator   = form_container_type::const_iterator;

    /// Constructor – initialises with no fields and an end iterator as current.
    form_data(): _field_it(_fields.end()) {}

    /// Returns a const reference to the entire multimap of parsed fields.
    const form_container_type& fields() const { return _fields; }

    /**
     * @brief Insert a new field or file entry.
     * @param name  The part name (from Content-Disposition).
     * @param value The value (string or path).
     * @return Iterator pointing to the newly inserted element.
     *
     * Also updates the internal current iterator to point to this new element.
     */
    form_iterator emplace(std::string name, field_value_type&& value) {
        _field_it = _fields.emplace(name, std::move(value));
        return _field_it;
    }

    /// Returns a const iterator to the end of the container (for comparison).
    form_const_iterator end() const { return _fields.end(); }

    /**
     * @brief Get an iterator to the currently active part.
     * @return Iterator to the part being parsed, or `end()` if none.
     *
     * This is used by the multipart parser to append data incrementally.
     */
    form_iterator current() { return _field_it; }

    /// Clear all fields and reset the current iterator to `end()`.
    void reset() {
        _fields.clear();
        _field_it = _fields.end();
    }

private:
    form_container_type             _fields;
    form_iterator                   _field_it;
};

struct form_view{
    explicit form_view(const form_data& form): _form(form) {}

    std::pair<form_data::form_const_iterator, form_data::form_const_iterator> get(const std::string& name) const {
        return _form._fields.equal_range(name);
    }

    std::vector<field_value_type> values(const std::string& name) const {
        auto range = get(name);
        std::vector<field_value_type> field_values;
        for(auto it = range.first; it != range.second; ++it) {
            field_values.push_back(it->second);
        }
        return field_values;
    }

    const field_value_type& field(const std::string& name, std::size_t index = 0) const {
        auto range = get(name);
        std::size_t count = std::distance(range.first, range.second);

        if(count == 0) {
            throw std::out_of_range{udho::utils::format("Tried to get value of nonexistent field {}", name)};
        }

        if(index >= count) {
            throw std::out_of_range{udho::utils::format("Tried to get value of field {} at index {}, but there are only {} values", name, index, count)};
        }

        auto it = range.first;
        std::advance(it, index);
        return it->second;
    }

    std::size_t count(const std::string& name) const {
        auto range = get(name);
        std::size_t count = std::distance(range.first, range.second);
        return count;
    }

    std::size_t count() const {
        return _form._fields.size();
    }

private:
    const form_data& _form;
};

}

}
}
}

#endif // UDHO_NET_FORM_DATA_H
