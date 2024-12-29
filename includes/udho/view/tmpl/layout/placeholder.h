#ifndef UDHO_VIEW_LAYOUT_PLACEHOLDER_H
#define UDHO_VIEW_LAYOUT_PLACEHOLDER_H

#include <map>
#include <string>
#include <optional>
#include <exception>
#include <udho/url/detail/format.h>

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

// { std::map
template <typename KeyT>
struct content<std::map<KeyT, std::string>>{
    using self_type         = content<std::map<KeyT, std::string>>;
    using container_type    = std::map<KeyT, std::string>;
    using key_type          = typename container_type::key_type;
    using value_type        = typename container_type::value_type;
    using size_type         = typename container_type::size_type;

    bool exists() const { return _spots.count(_key) > 0; }
    size_type size() const { return exists() ? value().size() : 0; }
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
    private:
        container_type& _spots;
        key_type        _key;
};
template <typename KeyT>
struct const_content<std::map<KeyT, std::string>>{
    using container_type    = std::map<KeyT, std::string>;
    using key_type          = typename container_type::key_type;
    using value_type        = typename container_type::value_type;
    using size_type         = typename container_type::size_type;

    bool exists() const { return _spots.count(_key) > 0; }
    size_type size() const { return exists() ? value().size() : 0; }
    const value_type& value() const{
        if(!exists()){
            throw std::out_of_range{"Error fetching content for layout placeholder. No value is set for the placeholder."};
        }
        return _spots.at(_key);
    }
    const value_type& operator*() const { return value(); }

    public:
        const_content(const container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
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
    using value_type            = typename container_type::value_type;
    using size_type             = typename container_type::size_type;
    using iterator_type         = typename container_type::iterator;
    using const_iterator_type   = typename container_type::const_iterator;

    bool exists() const { return _spots.find(_key) != _spots.end(); }
    size_type count() const { return _spots.count(_key); }
    iterator_type begin() { return _spots.lower_bound(_key); }
    iterator_type end() { return _spots.upper_bound(_key); }
    const_iterator_type begin() const { return _spots.lower_bound(_key); }
    const_iterator_type end() const  { return _spots.upper_bound(_key); }
    self_type& operator+=(const value_type& value) {
        _spots.emplace(std::make_pair(_key, value));
        return *this;
    }

    public:
        content(container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
    private:
        container_type& _spots;
        key_type        _key;
};
template <typename KeyT>
struct const_content<std::multimap<KeyT, std::string>>{
    using container_type        = std::multimap<KeyT, std::string>;
    using key_type              = typename container_type::key_type;
    using value_type            = typename container_type::value_type;
    using size_type             = typename container_type::size_type;
    using const_iterator_type   = typename container_type::const_iterator;

    bool exists() const { return _spots.find(_key) != _spots.end(); }
    size_type count() const { return _spots.count(_key); }
    const_iterator_type begin() const { return _spots.lower_bound(_key); }
    const_iterator_type end() const  { return _spots.upper_bound(_key); }

    public:
        const_content(const container_type& spots, key_type&& key): _spots(spots), _key(std::move(key)) {}
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
    size_type size() const { return exists() ? 1 : 0; }
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
    size_type size() const { return exists() ? 1 : 0; }
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

    proxy_type operator[](key_type&& key) {
        return proxy_type{_container, std::move(key)};
    }
    const_proxy_type operator[](key_type&& key) const {
        return const_proxy_type{_container, std::move(key)};
    }

    template <typename F>
    void apply(F&& f) const {
        f(_container);
    }

  private:
    container_type _container;
};

template <bool Multi, typename KeyT>
struct basic_placeholder_container<Multi, KeyT, std::enable_if_t<!has_less_than_operator_v<KeyT>>>{
    using key_type          = KeyT;
    using container_type    = std::conditional_t<Multi, std::vector<std::string>, std::optional<std::string>>;
    using proxy_type        = proxy::content<container_type>;
    using const_proxy_type  = proxy::const_content<container_type>;

    proxy_type operator[](key_type&&) {
        return proxy_type{_container};
    }
    const_proxy_type operator[](key_type&&) const {
        return const_proxy_type{_container};
    }
    proxy_type operator[](const key_type&) {
        return proxy_type{_container};
    }
    const_proxy_type operator[](const key_type&) const {
        return const_proxy_type{_container};
    }

    template <typename F>
    void apply(F&& f) const {
        f(_container);
    }

  private:
    container_type _container;
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
    using basic_placeholder<Spots...>::operator[];

    template <typename F>
    void apply(F&& f) const {
        auto&& lf = std::forward<F>(f);

        basic_placeholder_container<Multi, KeyT>::apply(lf);
        basic_placeholder<Spots...>::apply(lf);
    }

    template <typename F>
    void operator()(F&& f) const{
        apply(std::forward<F>(f));
    }
};

template <>
struct basic_placeholder<nullspot>: protected basic_placeholder_container<true, std::nullptr_t> {
    using basic_placeholder_container<true, std::nullptr_t>::operator[];

    template <typename F>
    void apply(F&& f) const {
        basic_placeholder_container<true, std::nullptr_t>::apply(std::forward<F>(f));
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
    spot<segments::central>,
    spot<segments::header>,
    spot<segments::footer>,
    multispot<segments::left>,
    multispot<segments::right>
>;

// using slim = basic_placeholder<
//     spot<segments::central>,
//     spot<segments::header>,
//     spot<segments::footer>
// >;
//
// using minimal = basic_placeholder<
//     spot<segments::central>
// >;

enum spots{
    main,
    north,
    south,
    east,
    west
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
