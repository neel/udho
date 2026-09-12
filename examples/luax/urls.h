// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_LUAX_URLS_H
#define UDHO_EXAMPLES_LUAX_URLS_H

#include <boost/config.hpp>
#include <string>
#include <udho/url/url.h>
#include <udho/view/bridges/lua.h>
#include <udho/www/www.h>

namespace luax {

namespace actions {

using context = udho::www::context<udho::www::components::resources<udho::view::data::bridges::lua>>;

BOOST_SYMBOL_EXPORT void home(context context);
BOOST_SYMBOL_EXPORT void hello(context context, const std::string& name);

}

inline auto urls() {
    using namespace udho::hazo::string::literals;

    auto actions = udho::url::slot("home"_h, &actions::home)   << udho::url::home(udho::url::verb::get)
                 | udho::url::slot("hello"_h, &actions::hello) << udho::url::regx(udho::url::verb::get, "/hello/(\\w+)", "/hello/{}")
    ;

    return udho::url::mount("root"_h, "/", std::move(actions));
}

}

#endif // UDHO_EXAMPLES_LUAX_URLS_H

