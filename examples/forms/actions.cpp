// SPDX-License-Identifier: BSD-3-Clause

#include "actions.h"
#include "data.h"
#include "helpers.h"

#include <udho/forms/forms.h>

namespace forms::actions {

namespace http = boost::beast::http;
namespace validation = udho::forms;
using form_view = udho::net::protocols::detail::form_view;

void home(context context) {
    helpers::render(context, "home", {"Udho forms"});
}

void submit(context context) {
    const auto& body = context.portal().body();
    const helpers::temporary_uploads cleanup{body.form()};
    if(!helpers::media_type(body.mime(), "application/x-www-form-urlencoded")) {
        helpers::render(context, "result", {"Unsupported form type", "Submit URL-encoded fields."}, http::status::unsupported_media_type);
        return;
    }

    auto name = validation::required<std::string>("name")
        .absent("A name is required.")
        .unparsable("A name is required.")
        .constrain<validation::constraints::gte>(2, "Use at least 2 bytes for the name.")
        .constrain<validation::constraints::lte>(60, "Use at most 60 bytes for the name.");

    auto age = validation::required<int>("age")
        .absent("An age is required.")
        .unparsable("Enter age as a whole number.")
        .constrain<validation::constraints::gte>(18, "Age must be at least 18.")
        .constrain<validation::constraints::lte>(120, "Age must be at most 120.");

    auto newsletter = validation::optional<bool>("newsletter", false)
        .unparsable("Choose whether to receive updates.");

    auto result = validation::validate(body.form());
    result >> name >> age >> newsletter;

    data::page page{"Submitted values", "All fields passed validation."};
    page.validation = result;
    page.newsletter = newsletter.valid() && *newsletter;
    if(!result.valid()) {
        page.title = "Check your form";
        page.message = "Please correct the errors and submit again.";
        helpers::render(context, "home", page, http::status::bad_request);
        return;
    }

    page.fields = {
        {"Name", *name},
        {"Age", std::to_string(*age)},
        {"Newsletter", *newsletter ? "yes" : "no"}
    };
    helpers::render(context, "result", page);
}

void json(context context) {
    const auto& body = context.portal().body();
    const helpers::temporary_uploads cleanup{body.form()};
    if(!helpers::media_type(body.mime(), "application/json")) {
        helpers::reply_json(context, {{"error", "Send Content-Type: application/json."}}, http::status::unsupported_media_type);
        return;
    }

    try {
        helpers::reply_json(context, body.json());
    } catch(const nlohmann::json::parse_error&) {
        helpers::reply_json(context, {{"error", "The request body is not valid JSON."}}, http::status::bad_request);
    }
}

void upload(context context) {
    const auto& body = context.portal().body();
    const helpers::temporary_uploads cleanup{body.form()};
    if(!helpers::media_type(body.mime(), "multipart/form-data")) {
        helpers::render(context, "result", {"Unsupported upload type", "Submit a multipart/form-data upload."}, http::status::unsupported_media_type);
        return;
    }

    const form_view form(body.form());
    if(form.count("file") == 0) {
        helpers::render(context, "result", {"Missing file", "Choose a file to upload."}, http::status::bad_request);
        return;
    }

    data::page result{"Uploaded files", "Temporary uploads are removed after processing."};
    const auto files = form.get("file");
    for(auto it = files.first; it != files.second; ++it) {
        const auto& file = it->second;
        if(!file.is_path()) {
            helpers::render(context, "result", {"Missing file", "Choose a file to upload."}, http::status::bad_request);
            return;
        }
        std::error_code error;
        const auto size = helpers::filesystem::file_size(file.path(), error);
        if(error) {
            helpers::render(context, "result", {"Upload error", "Could not inspect the uploaded file."}, http::status::internal_server_error);
            return;
        }
        result.fields.push_back({file.path().filename().string(), std::to_string(size) + " bytes"});
    }
    helpers::render(context, "result", result);
}

}
