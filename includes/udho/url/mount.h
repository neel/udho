// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_MOUNT_H
#define UDHO_URL_MOUNT_H

#include <string>
#include <udho/hazo/string/basic.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <udho/url/action.h>

namespace udho{
namespace url{

/**
 * chain of actions labeled with a compile time string
 * @tparam StrT compile time string
 * @tparam ActionsT udho::hazo::basic_seq_d<basic_action<F, StrT, MatchT>, ...>
 */
template <typename StrT, typename ActionsT>
struct mount_point{
    using actions_type = ActionsT;
    using name_type    = StrT;

    mount_point(name_type&& name, const std::string& path, actions_type&& actions): _name(std::move(name)), _path(path), _actions(std::move(actions)) { check(); }
    const name_type& name() const { return _name; }
    const std::string& path() const { return _path; }
    const actions_type& actions() const { return _actions; }
    actions_type& actions() { return _actions; }

    template <typename XStrT>
    auto& operator[](XStrT&& xstr) { return _actions[std::move(xstr)]; }

    template <typename XStrT>
    const auto& operator[](XStrT&& xstr) const { return _actions[std::move(xstr)]; }

    template <typename Ch>
    bool find(const std::basic_string<Ch>& pattern) const {
        auto pos = find_name<Ch>(pattern);
        if(pos == pattern.npos)
            return false;
        auto rest = pattern.substr(pos);

        bool found = false;
        _actions.visit([&rest, &found](const auto& action){
            if(found) return;
            found = action.find(rest);
        });
        return found;
    }
    template <typename Ch>
    bool invoke(const std::basic_string<Ch>& pattern) {
        auto pos = find_name<Ch>(pattern);
        if(pos == pattern.npos)
            return false;
        auto rest = pattern.substr(pos);

        bool found = false;
        _actions.visit([&rest, &found](auto& action){
            if(found) return;
            found = action.invoke(rest);
        });
        return found;
    }

    template <typename XStrT, typename... X>
    std::string operator()(XStrT&& xstr, X&&... x) const {
        return format("{}{}", _path, operator[](std::move(xstr))(x...));
    }

    template <typename XStrT, typename... X>
    std::string fill(XStrT&& xstr, std::tuple<X...>&& x) const {
        return format("{}{}", _path, operator[](std::move(xstr)).fill(x));
    }

    private:
        /**
         * if the pattern starts with _name followed by a slash then return the rest of the pattern including the slash.
         * returns an empty string otherwise.
         * @param pattern
         */
        template <typename Ch>
        typename std::basic_string<Ch>::size_type find_name(const std::basic_string<Ch>& pattern) const {
            bool prefixed = boost::starts_with(pattern, _path);
            return prefixed ? _path.size() : std::basic_string<Ch>::npos;
        }

        void check(){
            if(_path.back() != '/'){
                throw std::invalid_argument(format("the mountpoint path ({}) must end with a /", _path));
            }
        }
    private:
        name_type    _name;
        std::string  _path;
        actions_type _actions;
};


template <typename ActionsT, typename CharT, CharT... C>
mount_point<udho::hazo::string::str<CharT, C...>, ActionsT> mount(udho::hazo::string::str<CharT, C...>&& name, ActionsT&& actions){
    return mount_point<udho::hazo::string::str<CharT, C...>, ActionsT>{std::move(name), std::move(actions)};
}

template <typename LStrT, typename LActionsT, typename RStrT, typename RActionsT>
auto operator|(mount_point<LStrT, LActionsT>&& left, mount_point<RStrT, RActionsT>&& right){
    return udho::hazo::make_seq_d(std::move(left), std::move(right));
}

template <typename... Args, typename RStrT, typename RActionsT>
auto operator|(udho::hazo::basic_seq<udho::hazo::by_data, Args...>&& left, mount_point<RStrT, RActionsT>&& right){
    using lhs_type = udho::hazo::basic_seq<udho::hazo::by_data, Args...>;
    using rhs_type = mount_point<RStrT, RActionsT>;
    return typename lhs_type::template extend<rhs_type>(left, std::move(right));
}


template <typename StrT, typename ActionsT>
tabulate::Table& operator<<(tabulate::Table& table, const mount_point<StrT, ActionsT>& point){
    table.add_row({point.name().c_str(), point.path()});
    tabulate::Table chain_table;
    chain_table << point.actions();
    table.add_row({"", chain_table});
    return table;
}

template <typename StrT, typename ActionsT>
std::ostream& operator<<(std::ostream& stream, const mount_point<StrT, ActionsT>& point){
    tabulate::Table table;
    table << point;
    stream << table;
    return stream;
}

template <typename StrT, typename ActionsT, typename... TailT>
tabulate::Table& operator<<(tabulate::Table& table, const udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, TailT...>& chain){
    tabulize tab(table);
    chain.visit(tab);
    return table;
}

template <typename StrT, typename ActionsT, typename... TailT>
std::ostream& operator<<(std::ostream& stream, const udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, TailT...>& chain){
    tabulate::Table tab;
    tab << chain;
    stream << tab;
    return stream;
}


}
}

#endif // UDHO_URL_MOUNT_H
