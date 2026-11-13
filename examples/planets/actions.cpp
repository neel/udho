// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"
#include "actions.h"
#include "layout.h"

#include <udho/view/tmpl/layout/layout.h>

namespace {

planets::data::page navigation(planets::data::page data, const planets::actions::context& context, const std::string& mount) {
    const auto& routes = context.portal().route(mount);
    const std::string base = routes.path() == "/" ? "" : routes.path();
    data.home_url(base + routes["home"]())
        .hello_url(base + routes["hello"]("Alice"));
    return data;
}

}

void planets::actions::home(planets::actions::context context) {
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = layout::attach<placeholders::standard>(context);

    page.preamble().title("Udho planets example").doclang("en");
    page.css().add("planets", "common.css");

    page.properties(placeholders::header).classes("header");
    page.properties(placeholders::central).classes("content");
    page.properties(placeholders::footer).classes("footer");

    page[placeholders::header] = "<h1>Udho planets example</h1>";
    page[placeholders::footer] = "<small>Planets example made with Udho C++ framework</small>";

    page.properties(placeholders::central).view("lua://planets/home");
    page[placeholders::central] = navigation(planets::data::home(), context, "root");

    page();
}

void planets::actions::hello(planets::actions::context context, const std::string& name) {
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = layout::attach<placeholders::standard>(context);
    page.preamble().title("Hello from Lua").doclang("en");
    page.css().add("planets", "common.css");

    page.properties(placeholders::header).classes("header");
    page.properties(placeholders::central).classes("content");
    page.properties(placeholders::footer).classes("footer");

    page[placeholders::header] = "<h1>Udho planets example</h1>";
    page[placeholders::footer] = "<small>Planets example made with Udho C++ framework</small>";

    page.properties(placeholders::central).view("lua://planets/hello");
    page[placeholders::central] = navigation(planets::data::hello(name), context, "root");

    page();
}

void planets::actions::layout_home(planets::actions::context context) {
    namespace layout = udho::view::tmpl::layout;
    namespace places = planets::layouts::places;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = layout::attach<planets::layouts::placeholders, planets::layouts::presenter>(context);
    page.preamble().title("Planets with a custom layout").doclang("en");
    page.css().add("planets", "common.css");

    page.properties(places::central).view("lua://planets/home");
    auto data = navigation(planets::data::home(), context, "layout");
    data.message("This page renders the same Lua views inside a custom C++ layout.");
    page[places::central] = std::move(data);

    page();
}

void planets::actions::layout_hello(planets::actions::context context, const std::string& name) {
    namespace layout = udho::view::tmpl::layout;
    namespace places = planets::layouts::places;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = layout::attach<planets::layouts::placeholders, planets::layouts::presenter>(context);
    page.preamble().title("Hello from a custom planets layout").doclang("en");
    page.css().add("planets", "common.css");

    page.properties(places::central).view("lua://planets/hello");
    auto data = navigation(planets::data::hello(name), context, "layout");
    data.message("The custom layout rendered this view in its central placeholder.");
    page[places::central] = std::move(data);

    page();
}

