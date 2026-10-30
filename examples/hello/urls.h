// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_HELLO_URLS_H
#define UDHO_EXAMPLES_HELLO_URLS_H

#include "actions.h"

#include <udho/url/url.h>

namespace hello {

inline auto urls() {
    using namespace udho::hazo::string::literals;

    auto actions = udho::url::slot("home"_h, &actions::home)   << udho::url::home(udho::url::verb::get)
                 | udho::url::slot("hello"_h, &actions::hello) << udho::url::regx(udho::url::verb::get, "/hello/(\\w+)", "/hello/{}")
    ;

    return udho::url::mount("root"_h, "/", std::move(actions));
}

}

#endif // UDHO_EXAMPLES_HELLO_URLS_H

