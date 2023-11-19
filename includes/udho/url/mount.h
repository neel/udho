// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_MOUNT_H
#define UDHO_URL_MOUNT_H

#include <string>
#include <udho/hazo/string/basic.h>
#include <boost/algorithm/string/predicate.hpp>
#include <udho/url/action.h>

namespace udho{
namespace url{

template <typename StrT, typename ActionsT>
struct chain{
    using actions_type = ActionsT;
    using name_type    = StrT;

    struct finder{

        template<typename FunctionT, typename StrXT, typename MatchT>
        void operator()(const basic_action<FunctionT, StrXT, MatchT>& action){

        }
    };

    chain(name_type&& name, actions_type&& actions): _name(std::move(name)), _actions(std::move(actions)) {}
    const name_type& name() const { return _name; }
    const actions_type& actions() const { return _actions; }
    actions_type& actions() { return _actions; }

    template <typename Ch>
    bool find(const std::basic_string<Ch>& pattern) const {
        _actions.visit([pattern](){

        });
        return false;
    }
    template <typename Ch>
    bool invoke(const std::basic_string<Ch>& pattern) {}
    template <typename... X>
    std::string operator()(X&&... x) const {}


    private:
        name_type  _name;
        actions_type _actions;
};


template <typename ActionsT, typename CharT, CharT... C>
chain<udho::hazo::string::str<CharT, C...>, ActionsT> mount(udho::hazo::string::str<CharT, C...>&& name, ActionsT&& actions){
    return chain<udho::hazo::string::str<CharT, C...>, ActionsT>{std::move(name), std::move(actions)};
}

}
}

#endif // UDHO_URL_MOUNT_H
