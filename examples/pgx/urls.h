// SPDX-License-Identifier: BSD-3-Clause
#ifndef UDHO_EXAMPLES_PGX_URLS_H
#define UDHO_EXAMPLES_PGX_URLS_H

#include "controllers/api.h"
#include "controllers/ui.h"
#include <udho/url/url.h>

namespace pgx {

inline auto urls(controllers::api& api, controllers::ui& ui) {
    using namespace udho::hazo::string::literals;

    return udho::url::mount("root"_h, "/", ui.routes()) | udho::url::mount("api"_h, "/api", api.routes());
}

}
#endif
