// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_PLANETS_URLS_H
#define UDHO_EXAMPLES_PLANETS_URLS_H

#include "actions.h"

#include <udho/url/url.h>

namespace planets {

inline auto urls() {
    using namespace udho::hazo::string::literals;

    auto standard_actions = udho::url::slot("home"_h, &actions::home)   << udho::url::home(udho::url::verb::get)
                 | udho::url::slot("hello"_h, &actions::hello) << udho::url::regx(udho::url::verb::get, "/hello/(\\w+)", "/hello/{}")
    ;

    auto layout_actions = udho::url::slot("home"_h, &actions::layout_home)   << udho::url::home(udho::url::verb::get)
                        | udho::url::slot("hello"_h, &actions::layout_hello) << udho::url::regx(udho::url::verb::get, "/hello/(\\w+)", "/hello/{}")
    ;

    return udho::url::mount("root"_h, "/", std::move(standard_actions))
         | udho::url::mount("layout"_h, "/layout", std::move(layout_actions));
}

}

#endif // UDHO_EXAMPLES_PLANETS_URLS_H

