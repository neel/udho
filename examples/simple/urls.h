// SPDX-FileCopyrightText: 2024 <copyright holder> <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_SIMPLE_URLS_H
#define UDHO_EXAMPLES_SIMPLE_URLS_H

#include <boost/config.hpp>
#include <udho/url/url.h>
#include <udho/www/www.h>

namespace simple{

namespace actions{

BOOST_SYMBOL_EXPORT void home(udho::www::context<udho::www::components::cookies> context);
BOOST_SYMBOL_EXPORT void about(udho::www::context<udho::www::components::cookies> context);
BOOST_SYMBOL_EXPORT void hello(udho::www::context<udho::www::components::cookies> context, const std::string& name);

}


inline auto urls() {
    using namespace udho::hazo::string::literals;

    auto actions = udho::url::slot("home"_h,  &actions::home)   << udho::url::home (udho::url::verb::get)
                 | udho::url::slot("about"_h, &actions::about)  << udho::url::fixed(udho::url::verb::get, "/about", "/about")
                 | udho::url::slot("hello"_h, &actions::hello)  << udho::url::regx (udho::url::verb::get, "/hello/(\\w+)", "/hello/{}")
    ;

    auto mount_point = udho::url::mount("root"_h, "/", std::move(actions));

    return mount_point;
}

}

#endif // UDHO_EXAMPLES_SIMPLE_URLS_H
