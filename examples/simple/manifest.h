// SPDX-FileCopyrightText: 2024 <copyright holder> <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_SIMPLE_MANIFEST_H
#define UDHO_EXAMPLES_SIMPLE_MANIFEST_H

#include <udho/core/manifest.h>
#include <udho/url/url.h>
#include <udho/url/operators.h>
#include "applications/main.h"
#include "applications/user.h"

namespace simple{

/**
 * @todo write docs
 */
struct manifest: udho::core::manifest<simple::manifest>{
    auto routes() {
        using namespace udho::hazo::string::literals;
        return
            _main.root() |
            _user.mount("user"_h, "/users")
        ;
    }

    private:
        simple::apps::main _main;
        simple::apps::user _user;
};

}

#endif // UDHO_EXAMPLES_SIMPLE_MANIFEST_H
