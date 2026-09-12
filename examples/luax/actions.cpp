// SPDX-License-Identifier: BSD-3-Clause

#include "data.h"
#include "urls.h"

#include <udho/view/tmpl/layout/layout.h>

namespace {

template <typename ContextT>
auto standard_page(ContextT context, const std::string& title) {
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    layout::standard_layout<ContextT> page(context);
    page.preamble().title(title).doclang("en");
    page.properties(placeholders::header).classes("header");
    page.properties(placeholders::central).classes("content");
    page.properties(placeholders::footer).classes("footer");
    page[placeholders::header] = "<h1>Udho Lua example</h1>";
    page[placeholders::footer] = "<small>The page shell is C++; the central view is Lua.</small>";
    return page;
}

}

void luax::actions::home(luax::actions::context context) {
    namespace placeholders = udho::view::tmpl::layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = standard_page(context, "Udho Lua example");
    page.properties(placeholders::central).view("lua://luax/home");
    page[placeholders::central] = luax::data::home();

    page();
}

void luax::actions::hello(luax::actions::context context, const std::string& name) {
    namespace placeholders = udho::view::tmpl::layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    auto page = standard_page(context, "Hello from Lua");
    page.properties(placeholders::central).view("lua://luax/hello");
    page[placeholders::central] = luax::data::hello(name);

    page();
}

