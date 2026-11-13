// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_FORMS_HELPERS_H
#define UDHO_EXAMPLES_FORMS_HELPERS_H

#include "actions.h"
#include "data.h"

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <system_error>
#include <vector>
#include <udho/net/protocols/form_data.h>
#include <udho/utils/filesystem.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/www/www.h>

namespace forms::helpers {

namespace http = boost::beast::http;
namespace filesystem = udho::utils::filesystem;

inline bool media_type(const std::string& value, const std::string& expected) {
    return boost::iequals(boost::trim_copy(value.substr(0, value.find(';'))), expected);
}

inline void render(actions::context context, const std::string& view, const data::page& data,
                   http::status status = http::status::ok) {
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(http::field::content_type, "text/html; charset=utf-8");
    context.ostream().status(status);
    auto page = layout::attach<placeholders::standard>(context);
    page.preamble().title(data.title).doclang("en");
    page.properties(placeholders::central).view("lua://forms/" + view);
    page[placeholders::central] = data;
    page();
}

inline void reply_json(actions::context context, const nlohmann::json& data,
                       http::status status = http::status::ok) {
    context.ostream().set(http::field::content_type, "application/json");
    context.ostream().status(status);
    context << data.dump(2);
    context.finish();
}

// The framework creates upload files; this guard removes them after processing.
struct temporary_uploads {
    std::vector<filesystem::path> paths;

    explicit temporary_uploads(const udho::net::protocols::detail::form_data& data) {
        for(const auto& [name, value] : data.fields()) {
            if(value.is_path()) paths.push_back(value.path());
        }
    }

    ~temporary_uploads() {
        for(const auto& path : paths) {
            std::error_code ignored;
            filesystem::remove(path, ignored);
        }
    }
};

}

#endif // UDHO_EXAMPLES_FORMS_HELPERS_H
