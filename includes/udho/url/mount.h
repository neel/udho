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

}
}

#endif // UDHO_URL_MOUNT_H
