#ifndef UDHO_URL_TABLES_H
#define UDHO_URL_TABLES_H

#include <udho/url/action.h>
#include <udho/url/mount.h>
#include <udho/hazo/seq/seq.h>
#include <udho/hazo/string/basic.h>

namespace udho{
namespace url{

template <typename... Actions>
class action_table: private udho::hazo::basic_seq<udho::hazo::by_data, Actions...>{
    using sequence_type = udho::hazo::basic_seq<udho::hazo::by_data, Actions...>;

    template <typename... XActions>
    friend class action_table;

    template <typename T>
    using extend = typename sequence_type::template extend<T>;
    const sequence_type& sequence() const { return *this; }

public:
    using sequence_type::visit;
    using sequence_type::visit_at;
    using sequence_type::depth;

    action_table(Actions&&... actions): sequence_type(std::forward<Actions>(actions)...) {}

    action_table(udho::hazo::basic_seq<udho::hazo::by_data, Actions...>&& seq): sequence_type(std::forward<udho::hazo::basic_seq<udho::hazo::by_data, Actions...>>(seq)) {}

    template <typename RFunctionT, typename RStrT, typename RMatchT>
    action_table<Actions..., basic_action<RFunctionT, RStrT, RMatchT>> append(basic_action<RFunctionT, RStrT, RMatchT>&& right) const {
        using rhs_type = basic_action<RFunctionT, RStrT, RMatchT>;
        using result_type = action_table<Actions..., rhs_type>;
        return result_type{extend<rhs_type>(sequence(), std::move(right))};
    }

    template <typename... XActions>
    action_table<Actions..., XActions...> concat(const action_table<XActions...>& other) const {
        return action_table<Actions..., XActions...>{sequence_type::concat(other.sequence())};
    }

    template <typename CharT, CharT... X>
    auto& operator[](const udho::hazo::string::str<CharT, X...>& key) { return sequence_type::operator[](key); }

    template <typename CharT, CharT... X>
    const auto& operator[](const udho::hazo::string::str<CharT, X...>& key) const { return sequence_type::operator[](key); }

    constexpr std::size_t length() const { return sequence_type::depth; }
};

template <typename... Mountpoints>
class mountpoints_table: private udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>{
    using sequence_type = udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>;

    template <typename... XMountpoints>
    friend class mountpoints_table;

    template <typename T>
    using extend = typename sequence_type::template extend<T>;
    const sequence_type& sequence() const { return *this; }

public:

    using sequence_type::visit;
    using sequence_type::visit_at;

    mountpoints_table(Mountpoints&&... mountpoints): sequence_type(std::forward<Mountpoints>(mountpoints)...) {}

    mountpoints_table(udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>&& seq): sequence_type(std::forward<udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>>(seq)) {}

    template <typename RStrT, typename RActionsT>
    mountpoints_table<Mountpoints..., mount_point<RStrT, RActionsT>> append(mount_point<RStrT, RActionsT>&& right) const {
        using rhs_type = mount_point<RStrT, RActionsT>;
        using result_type = mountpoints_table<Mountpoints..., rhs_type>;
        return result_type{extend<rhs_type>(sequence(), std::move(right))};
    }

    template <typename... XMountpoints>
    mountpoints_table<Mountpoints..., XMountpoints...> concat(const mountpoints_table<XMountpoints...>& other) const {
        return mountpoints_table<Mountpoints..., XMountpoints...>{sequence_type::concat(other.sequence())};
    }

    template <typename CharT, CharT... X>
    auto& operator[](const udho::hazo::string::str<CharT, X...>& key) { return sequence_type::operator[](key); }

    template <typename CharT, CharT... X>
    const auto& operator[](const udho::hazo::string::str<CharT, X...>& key) const { return sequence_type::operator[](key); }

    constexpr std::size_t length() const { return sequence_type::depth; }
};


namespace detail{

template <typename T>
struct is_action_table : std::false_type {};

template <typename... Actions>
struct is_action_table<action_table<Actions...>> : std::bool_constant<std::conjunction_v<detail::is_basic_action<Actions>...>> {};

}

}
}

#endif // UDHO_URL_TABLES_H
