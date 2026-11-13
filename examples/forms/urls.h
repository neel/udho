// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_FORMS_URLS_H
#define UDHO_EXAMPLES_FORMS_URLS_H

#include "actions.h"

#include <utility>
#include <udho/url/url.h>
#include <udho/www/components/params.h>

namespace forms {

inline auto urls() {
    using namespace udho::hazo::string::literals;
    namespace protocol = udho::www::params::protocol;

    auto routes =
        udho::url::slot("home"_h, &actions::home) << udho::url::home(udho::url::verb::get)
        | udho::url::slot("submit"_h, &actions::submit) <<
            udho::url::fixed(udho::url::verb::post, "/submit", "/submit").options(
                protocol::body_memory_limit(16 * 1024),
                protocol::field_memory_limit(16 * 1024),
                protocol::body_time_limit(30)
            )
        | udho::url::slot("json"_h, &actions::json) <<
            udho::url::fixed(udho::url::verb::post, "/json", "/json").options(
                protocol::body_memory_limit(16 * 1024),
                protocol::field_memory_limit(16 * 1024),
                protocol::body_time_limit(30)
            )
        | udho::url::slot("upload"_h, &actions::upload) <<
            udho::url::fixed(udho::url::verb::post, "/upload", "/upload").options(
                protocol::body_memory_limit(1024 * 1024),
                protocol::field_memory_limit(1024 * 1024),
                protocol::body_time_limit(30)
            )
    ;

    return udho::url::mount("root"_h, "/", std::move(routes));
}

}

#endif // UDHO_EXAMPLES_FORMS_URLS_H
