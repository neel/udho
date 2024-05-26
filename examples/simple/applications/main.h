// SPDX-FileCopyrightText: 2024 <copyright holder> <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_SIMPLE_APPLICATION_H
#define UDHO_EXAMPLES_SIMPLE_APPLICATION_H

#include <udho/core/application.h>
#include <udho/net/stream.h>
#include <udho/hazo/string/basic.h>
#include <udho/url/operators.h>

namespace simple{
namespace apps{

/**
 * @todo write docs
 */
struct main: udho::core::application<apps::main>{
    auto routes() {
        using namespace udho::hazo::string::literals;
        return
            udho::url::slot("home"_h,    &main::home,    this) << udho::url::home  (udho::url::verb::get)               |
            udho::url::slot("contact"_h, &main::contact, this) << udho::url::fixed (udho::url::verb::get, "/contact")
        ;
    }
    void home(udho::net::stream context);
    void contact(udho::net::stream context);
};

}
}

#endif // UDHO_EXAMPLES_SIMPLE_APPLICATION_H
