#include "urls.h"
#include <udho/utils/encoding.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/www/www.h>

void simple::actions::home(udho::www::context<udho::www::components::resources<>> context){
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    layout::standard_layout<decltype(context)> page(context);

    page.preamble().title("Udho simple example").doclang("en");

    page.css().add("simple", "simple.css");

    page.properties(placeholders::header).classes("masthead").id("page-header");
    page.properties(placeholders::central).classes("content");
    page.properties(placeholders::footer).classes("footer");

    page[placeholders::header] = "<h1>Hello, world!</h1>";
    page[placeholders::central] = R"(
        <p>Available URLs:</p>
        <ul>
            <li><a href="/">Home</a></li>
            <li><a href="/about">About</a></li>
            <li><a href="/hello/Alice">Hello Alice</a></li>
        </ul>
    )";
    page[placeholders::footer] = "<small>Styled with a local asset served by udho.</small>";

    page();
}

void simple::actions::about(udho::www::context<udho::www::components::navigators::pretty> context){
    const auto& query     = context.portal().query();
    const auto& extension = query.extension();
    const auto& path      = query.path();
    const auto& params    = query.params();

    if(extension == "html") {
        context.ostream().set(boost::beast::http::field::content_type, "text/html");
        context << "<!doctype html><html><head><title>About</title></head><body>"
                << "<h1>About this example</h1>"
                << "<p>Path: <code>" << udho::utils::encode::escape(path) << "</code></p>"
                << "<h2>Query parameters</h2><ul>";
        for(const auto& [key, value]: params) {
            context << "<li><code>" << udho::utils::encode::escape(key) << "</code>: <code>" << udho::utils::encode::escape(value) << "</code></li>";
        }
        context << "</ul></body></html>";
    } else if(extension == "json") {
        context.ostream().set(boost::beast::http::field::content_type, "application/json");
        nlohmann::json document = {
            {"about", "About this example"},
            {"path", path},
            {"query_parameters", nlohmann::json::array()}
        };
        for(const auto& [key, value]: params) {
            document["query_parameters"].push_back({{"name", key}, {"value", value}});
        }
        context << document.dump(2);
    } else if(extension == "xml") {
        context.ostream().set(boost::beast::http::field::content_type, "application/xml");
        context << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                << "<about><description>About this example</description>"
                << "<path>" << udho::utils::encode::escape(path) << "</path>"
                << "<query-parameters>";
        for(const auto& [key, value]: params) {
            context << "<parameter><name>" << udho::utils::encode::escape(key) << "</name><value>" << udho::utils::encode::escape(value) << "</value></parameter>";
        }
        context << "</query-parameters></about>";
    } else {
        context.ostream().set(boost::beast::http::field::content_type, "text/plain");
        context << "About this example, try .html, .json, or .xml\n"
                << "Path: " << path << "\n"
                << "Query parameters:\n";
        for(const auto& [key, value]: params) {
            context << "  " << key << " = " << value << "\n";
        }
    }


    context.finish();
}

void simple::actions::hello(udho::www::context<udho::www::components::resources<>> context, const std::string& name){
    namespace layout = udho::view::tmpl::layout;
    namespace placeholders = layout::placeholders;

    context.ostream().set(boost::beast::http::field::content_type, "text/html");

    layout::standard_layout<decltype(context)> page(context);
    const auto escaped_name = udho::utils::encode::escape(name);

    page.preamble().title("Hello from udho").doclang("en");

    page.css().add("simple", "simple.css");
    page.js().add("cdn", "umbrella.js");
    page.js().add("simple", "hello.js");

    page.properties(placeholders::header) .classes("masthead").id("page-header");
    page.properties(placeholders::central).classes("greeting").id("greeting");
    page.properties(placeholders::footer) .classes("footer");

    page[placeholders::header]  = "<h1>Udho simple example</h1>";
    page[placeholders::central] =
        "<p>Hello, <strong>" + escaped_name + "</strong>!</p>"
        "<button id=\"hello-button\" type=\"button\">Run JavaScript</button>"
        "<p id=\"hello-message\"></p>";

    page[placeholders::footer] = "<small>Rendered with udho's standard C++ layout.</small>";

    page();
}
