// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"
#include "actions.h"

#include <udho/view/tmpl/layout/layout.h>

void hello::actions::home(hello::actions::context context) {
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = layout::attach<placeholders::standard>(context);

    page.preamble().title("Udho Lua example").doclang("en");
    page.css().add("hello", "common.css");

    page.properties(placeholders::header).classes("header");
    page.properties(placeholders::central).classes("content");
    page.properties(placeholders::footer).classes("footer");

    page[placeholders::header] = "<h1>Udho Lua example</h1>";
    page[placeholders::footer] = "<small>Hello example made with Udho C++ framework</small>";

    page.properties(placeholders::central).view("lua://hello/home");
    page[placeholders::central] = hello::data::home();

    page();
}

void hello::actions::hello(hello::actions::context context, const std::string& name) {
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = layout::attach<placeholders::standard>(context);
    page.preamble().title("Hello from Lua").doclang("en");
    page.css().add("hello", "common.css");

    page.properties(placeholders::header).classes("header");
    page.properties(placeholders::central).classes("content");
    page.properties(placeholders::footer).classes("footer");

    page[placeholders::header] = "<h1>Udho Lua example</h1>";
    page[placeholders::footer] = "<small>Hello example made with Udho C++ framework</small>";

    page.properties(placeholders::central).view("lua://hello/hello");
    page[placeholders::central] = hello::data::hello(name);

    page();
}

