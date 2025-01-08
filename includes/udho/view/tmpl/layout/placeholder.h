#ifndef UDHO_VIEW_LAYOUT_PLACEHOLDER_H
#define UDHO_VIEW_LAYOUT_PLACEHOLDER_H

#include <map>
#include <string>
#include <optional>
#include <exception>
#include <udho/url/detail/format.h>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/algorithm/string/join.hpp>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

namespace detail {
    template <typename T, typename = void>
    struct has_less_than_operator : std::false_type {};

    template <typename T>
    struct has_less_than_operator<T, std::void_t<decltype(std::declval<T>() < std::declval<T>())>> : std::true_type {};
}

template <typename T>
inline constexpr bool has_less_than_operator_v = detail::has_less_than_operator<T>::value;

namespace proxy{

template <typename ContainerT>
struct content;

template <typename ContainerT>
struct const_content;

struct placeholder_properties{
    placeholder_properties(): _tag("div") {}

    placeholder_properties& tag(const std::string& tagname) { _tag = tagname; return *this; }
    const std::string& tag() const { return _tag; }

    placeholder_properties& id(const std::string& i) { _id = i; return *this; }
    const std::string& id() const { return _id; }

    placeholder_properties& classes(const std::string& classnames) { _classes = classnames; return *this; }
    const std::string& classes() const { return _classes; }

    placeholder_properties& wrapper_tag(const std::string& wrapper_tag) { _wrapper_tag = wrapper_tag; return *this; }
    const std::string& wrapper_tag() const { return _wrapper_tag; }

    placeholder_properties& wrapper_classes(const std::string& wrapper_classes) { _wrapper_classes = wrapper_classes; return *this; }
    const std::string& wrapper_classes() const { return _wrapper_classes; }

    std::string opening() const {
        std::vector<std::string> attr;
        if(!_id.empty())        attr.emplace_back(udho::url::format("id=\"{}\"", _id));
        if(!_classes.empty())   attr.emplace_back(udho::url::format("class=\"{}\"", _classes));
        std::string joined = boost::algorithm::join(attr, " ");

        std::string tag = "<"+_tag;
        if(!joined.empty()) tag += " "+joined;
        tag += ">";

        return tag;
    }
    bool styled() const { return !_id.empty() || !_classes.empty() || !_wrapper_classes.empty(); }
    std::string closing() const {
        return "</" +_tag+ ">";
    }

    private:
        std::string _tag;
        std::string _id;
        std::string _classes;
        std::string _wrapper_tag;
        std::string _wrapper_classes;
};

// { std::map
template <typename KeyT>
struct content<std::map<KeyT, std::string>>{
    using self_type         = content<std::map<KeyT, std::string>>;
    using container_type    = std::map<KeyT, std::string>;
    using key_type          = typename container_type::key_type;
    using value_type        = typename container_type::mapped_type;
    using size_type         = typename container_type::size_type;

    bool exists() const { return _spots.count(_key) > 0; }
    size_type count() const { return exists() ? value().size() : 0; }
    value_type& value() {
        if(!exists()){
            throw std::out_of_range{"Error fetching content for layout placeholder. No value is set for the placeholder."};
        }
        return _spots.at(_key);
    }
    const value_type& value() const{
        if(!exists()){
            throw std::out_of_range{"Error fetching content for layout placeholder. No value is set for the placeholder."};
        }
        return _spots.at(_key);
    }
    value_type& operator*(){ return value(); }
    const value_type& operator*() const { return value(); }
    self_type& operator=(const value_type& value) {
        _spots.emplace(std::make_pair(_key, value));
        return *this;
    }

    public:
        content(container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
        content(container_type& spots, const key_type& key): _spots(spots), _key(key) {}
    private:
        container_type& _spots;
        key_type        _key;
};
template <typename KeyT>
struct const_content<std::map<KeyT, std::string>>{
    using container_type    = std::map<KeyT, std::string>;
    using key_type          = typename container_type::key_type;
    using value_type        = typename container_type::mapped_type;
    using size_type         = typename container_type::size_type;

    bool exists() const { return _spots.count(_key) > 0; }
    size_type count() const { return exists() ? value().size() : 0; }
    const value_type& value() const{
        if(!exists()){
            throw std::out_of_range{"Error fetching content for layout placeholder. No value is set for the placeholder."};
        }
        return _spots.at(_key);
    }
    const value_type& operator*() const { return value(); }

    public:
        const_content(const container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
        const_content(container_type& spots, const key_type& key): _spots(spots), _key(key) {}
    private:
        const container_type& _spots;
        key_type        _key;
};
// }

// { std::multimap
template <typename KeyT>
struct content<std::multimap<KeyT, std::string>>{
    using self_type             = content<std::multimap<KeyT, std::string>>;
    using container_type        = std::multimap<KeyT, std::string>;
    using key_type              = typename container_type::key_type;
    using value_type            = typename container_type::mapped_type;
    using size_type             = typename container_type::size_type;

    template <typename Iterator>
    struct value_iterator_ : public boost::iterator_adaptor<value_iterator_<Iterator>, Iterator, typename std::iterator_traits<Iterator>::value_type::second_type, boost::use_default, typename std::iterator_traits<Iterator>::value_type::second_type&> {
        explicit value_iterator_(Iterator it): value_iterator_::iterator_adaptor_(it) {}
        private:
            friend class boost::iterator_core_access;
            typename value_iterator_::reference& dereference() const {
                return this->base_reference()->second;
            }
    };
    template <typename Iterator>
    struct const_value_iterator_ : public boost::iterator_adaptor<const_value_iterator_<Iterator>, Iterator, typename std::iterator_traits<Iterator>::value_type::second_type, boost::use_default, const typename std::iterator_traits<Iterator>::value_type::second_type&> {
        explicit const_value_iterator_(Iterator it): const_value_iterator_::iterator_adaptor_(it) {}
        private:
            friend class boost::iterator_core_access;
            const typename const_value_iterator_::reference& dereference() const {
                return this->base_reference()->second;
            }
    };

    using value_iterator        = value_iterator_<typename container_type::iterator>;
    using const_value_iterator  = const_value_iterator_<typename container_type::const_iterator>;
    using iterator_type         = value_iterator;
    using const_iterator_type   = const_value_iterator;

    bool exists() const { return _spots.find(_key) != _spots.end(); }
    size_type count() const { return _spots.count(_key); }
    iterator_type begin() { return iterator_type{_spots.lower_bound(_key)}; }
    iterator_type end() { return iterator_type{_spots.upper_bound(_key)}; }
    const_iterator_type begin() const { return const_iterator_type{_spots.lower_bound(_key)}; }
    const_iterator_type end() const  { return const_iterator_type{_spots.upper_bound(_key)}; }
    self_type& operator+=(const value_type& value) {
        _spots.emplace(std::make_pair(_key, value));
        return *this;
    }

    value_type& operator[](const size_type i) {
        size_type total = count();
        if(i >= total){
            throw std::out_of_range{udho::url::format("index {} out of range, size = {}", i, total)};
        }
        auto it = begin();
        std::advance(it, i);
        return *it;
    }
    const value_type& operator[](const size_type i) const {
        size_type total = count();
        if(i >= total){
            throw std::out_of_range{udho::url::format("index {} out of range, size = {}", i, total)};
        }
        auto it = begin();
        std::advance(it, i);
        return *it;
    }

    public:
        content(container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
        content(container_type& spots, const key_type& key): _spots(spots), _key(key) {}
    private:
        container_type& _spots;
        key_type        _key;
};
template <typename KeyT>
struct const_content<std::multimap<KeyT, std::string>>{
    using container_type        = std::multimap<KeyT, std::string>;
    using key_type              = typename container_type::key_type;
    using value_type            = typename container_type::mapped_type;
    using size_type             = typename container_type::size_type;

    template <typename Iterator>
    struct const_value_iterator_ : public boost::iterator_adaptor<const_value_iterator_<Iterator>, Iterator, typename std::iterator_traits<Iterator>::value_type::second_type, boost::use_default, const typename std::iterator_traits<Iterator>::value_type::second_type&> {
        explicit const_value_iterator_(Iterator it): const_value_iterator_::iterator_adaptor_(it) {}
        private:
            friend class boost::iterator_core_access;
            const typename const_value_iterator_::reference& dereference() const {
                return this->base_reference()->second;
            }
    };

    using const_value_iterator  = const_value_iterator_<typename container_type::const_iterator>;
    // using value_iterator        = const_value_iterator;
    using const_iterator_type   = const_value_iterator;
    // using iterator_type         = const_iterator_type;


    bool exists() const { return _spots.find(_key) != _spots.end(); }
    size_type count() const { return _spots.count(_key); }
    const_iterator_type begin() const { return const_iterator_type{_spots.lower_bound(_key)}; }
    const_iterator_type end() const  { return const_iterator_type{_spots.upper_bound(_key)}; }
    const value_type& operator[](const size_type i) const {
        size_type total = count();
        if(i >= total){
            throw std::out_of_range{udho::url::format("index {} out of range, size = {}", i, total)};
        }
        auto it = begin();
        std::advance(it, i);
        return *it;
    }

    public:
        const_content(const container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
        const_content(const container_type& spots, const key_type& key): _spots(spots), _key(key) {}
    private:
        const container_type& _spots;
        key_type        _key;
};
// }

// { std::optional
template <>
struct content<std::optional<std::string>>{
    using self_type         = content<std::optional<std::string>>;
    using container_type    = std::optional<std::string>;
    using value_type        = typename container_type::value_type;
    using size_type         = std::size_t;

    bool exists() const { return _spots.has_value(); }
    size_type count() const { return exists() ? 1 : 0; }
    value_type& value() { return _spots.value(); }
    const value_type& value() const{ return _spots.value(); }
    value_type& operator*(){ return value(); }
    const value_type& operator*() const { return value(); }
    self_type& operator=(const value_type& value) {
        _spots = value;
        return *this;
    }

    public:
        content(container_type& spots): _spots(spots) {}
    private:
        container_type& _spots;
};
template <>
struct const_content<std::optional<std::string>>{
    using self_type         = content<std::optional<std::string>>;
    using container_type    = std::optional<std::string>;
    using value_type        = typename container_type::value_type;
    using size_type         = std::size_t;

    bool exists() const { return _spots.has_value(); }
    size_type count() const { return exists() ? 1 : 0; }
    const value_type& value() const{ return _spots.value(); }
    const value_type& operator*() const { return value(); }

    public:
        const_content(const container_type& spots): _spots(spots) {}
    private:
        const container_type& _spots;
};
// }

// { std::vector
template <>
struct content<std::vector<std::string>>{
    using self_type             = content<std::vector<std::string>>;
    using container_type        = std::vector<std::string>;
    using value_type            = typename container_type::value_type;
    using size_type             = typename container_type::size_type;
    using iterator_type         = typename container_type::iterator;
    using const_iterator_type   = typename container_type::const_iterator;

    bool exists() const { return !_spots.empty(); }
    size_type count() const { return _spots.size(); }
    iterator_type begin() { return _spots.begin(); }
    iterator_type end() { return _spots.end(); }
    const_iterator_type begin() const { return _spots.begin(); }
    const_iterator_type end() const  { return _spots.end(); }
    self_type& operator+=(const value_type& value) {
        _spots.emplace_back(value);
        return *this;
    }
    value_type& operator[](const size_type i) { return _spots.at(i); }
    const value_type& operator[](const size_type i) const { return _spots.at(i); }
    public:
        content(container_type& spots): _spots(spots) {}
    private:
        container_type& _spots;
};
template <>
struct const_content<std::vector<std::string>>{
    using self_type             = content<std::vector<std::string>>;
    using container_type        = std::vector<std::string>;
    using value_type            = typename container_type::value_type;
    using size_type             = typename container_type::size_type;
    using const_iterator_type   = typename container_type::const_iterator;

    bool exists() const { return !_spots.empty(); }
    size_type count() const { return _spots.size(); }
    const_iterator_type begin() const { return _spots.begin(); }
    const_iterator_type end() const  { return _spots.end(); }
    const value_type& operator[](const size_type i) const { return _spots.at(i); }

    public:
        const_content(const container_type& spots): _spots(spots) {}
    private:
        const container_type& _spots;
};
// }

}

template <bool Multi, typename KeyT, typename Enable = void>
struct basic_placeholder_container;

/**
 * If KeyT doesn't have operator< overloaded then either uses std::optional or uses std::vector depending on the value of Multi.
 */
template <bool Multi, typename KeyT>
struct basic_placeholder_container<Multi, KeyT, std::enable_if_t<has_less_than_operator_v<KeyT>>>{
    using key_type          = KeyT;
    using container_type    = std::conditional_t<Multi, std::multimap<key_type, std::string>, std::map<key_type, std::string>>;
    using proxy_type        = proxy::content<container_type>;
    using const_proxy_type  = proxy::const_content<container_type>;
    using properties_type   = std::map<KeyT, proxy::placeholder_properties>;

    proxy_type operator[](const key_type& key) {
        if(!_properties.count(key)){
            _properties[key] = proxy::placeholder_properties{};
        }
        return proxy_type{_container, key};
    }
    const_proxy_type operator[](const key_type& key) const {
        return const_proxy_type{_container, key};
    }

    proxy::placeholder_properties& properties(const key_type& key) { return _properties[key]; }
    const proxy::placeholder_properties& properties(const key_type& key) const { return _properties.at(key); }

    template <typename F, typename Stream>
    void apply(F&& f, Stream& stream) const {
        if constexpr (!Multi){
            for(const auto& pair: _container){
                f(pair.first, pair.second, stream);
            }
        } else {
            for(typename container_type::const_iterator it = _container.begin(), end = _container.end(); it != end; it = _container.upper_bound(it->first)){
                const KeyT& key = it->first;
                const_proxy_type proxy{_container, key};
                std::size_t index = 0;
                std::size_t count = proxy.count();
                for (auto it = proxy.begin(); it != proxy.end(); ++it){
                    f(key, *it, stream, index++, count);
                }
            }
        }
    }
  private:
    container_type  _container;
    properties_type _properties;

};

template <bool Multi, typename KeyT>
struct basic_placeholder_container<Multi, KeyT, std::enable_if_t<!has_less_than_operator_v<KeyT>>>{
    using key_type          = KeyT;
    using container_type    = std::conditional_t<Multi, std::vector<std::string>, std::optional<std::string>>;
    using proxy_type        = proxy::content<container_type>;
    using const_proxy_type  = proxy::const_content<container_type>;
    using properties_type   = proxy::placeholder_properties;

    proxy_type operator[](const key_type&) {
        return proxy_type{_container};
    }
    const_proxy_type operator[](const key_type&) const {
        return const_proxy_type{_container};
    }

    properties_type& properties(const key_type&) { return _properties; }
    const properties_type& properties(const key_type&) const { return _properties; }

    template <typename F, typename Stream>
    void apply(F&& f, Stream& stream) const {
        static key_type empty_key;
        if constexpr (!Multi){
            f(empty_key, *_container, stream);
        } else {
            std::size_t index = 0;
            std::size_t count = _container.size();
            for(const auto& v: _container){
                f(empty_key, v, stream, index++, count);
            }
        }
    }
  private:
    container_type  _container;
    properties_type _properties;
};

template <typename KeyT, bool Multi = false>
struct spot{
    static constexpr bool multiple = Multi;
    using key_type = KeyT;
};

template <typename KeyT>
using multispot = spot<KeyT, true>;

using nullspot = spot<std::nullptr_t, true>;

template <typename Spot = nullspot, typename... Spots>
struct basic_placeholder;

template <typename KeyT, bool Multi, typename... Spots>
struct basic_placeholder<spot<KeyT, Multi>, Spots...>: protected basic_placeholder_container<Multi, KeyT>, basic_placeholder<Spots...> {
    using basic_placeholder_container<Multi, KeyT>::operator[];
    using basic_placeholder_container<Multi, KeyT>::properties;
    using basic_placeholder<Spots...>::operator[];
    using basic_placeholder<Spots...>::properties;

    template <typename F, typename Stream>
    void apply(F&& f, Stream& stream) const {
        auto&& lf = std::forward<F>(f);

        basic_placeholder_container<Multi, KeyT>::apply(lf, stream);
        basic_placeholder<Spots...>::apply(lf, stream);
    }

    template <typename F>
    void operator()(F&& f) const{
        apply(std::forward<F>(f));
    }
};

template <>
struct basic_placeholder<nullspot>: protected basic_placeholder_container<true, std::nullptr_t> {
    using basic_placeholder_container<true, std::nullptr_t>::operator[];

    template <typename F, typename Stream>
    void apply(F&& f, Stream& stream) const {
        basic_placeholder_container<true, std::nullptr_t>::apply(std::forward<F>(f), stream);
    }
    template <typename F>
    void operator()(F&& f) const{
        apply(std::forward<F>(f));
    }
};

namespace placeholders{

namespace segments{
    struct central{};
    struct header{};
    struct footer{};
    struct left{};
    struct right{};
}


static segments::central central;
static segments::header  header;
static segments::footer  footer;
static segments::left    left;
static segments::right   right;

using standard = basic_placeholder<
    spot<segments::header>,
    multispot<segments::left>,
    spot<segments::central>,
    multispot<segments::right>,
    spot<segments::footer>
>;

enum spots{
    north,
    west,
    main,
    east,
    south
};

using common = basic_placeholder<
    spot<spots>
>;

using multi = basic_placeholder<
    multispot<spots>
>;

}

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_PLACEHOLDER_H
