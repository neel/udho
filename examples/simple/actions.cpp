#include "urls.h"
#include <udho/www/www.h>

void simple::actions::home(udho::www::context<udho::www::components::cookies> context){
    context.ostream().set(boost::beast::http::field::content_type, "text/plain");

    context << "Hello, world!";
    context << "Available URLs:\n";
    context << "  http://localhost:9999/\n";
    context << "  http://localhost:9999/about\n";
    context << "  http://localhost:9999/hello/{name}\n\n";
    context << "Example:\n";
    context << "  http://localhost:9999/hello/Alice\n";

    context.finish();
}

void simple::actions::about(udho::www::context<udho::www::components::cookies> context){
    context << "About this example";
    context.finish();
}

void simple::actions::hello(udho::www::context<udho::www::components::cookies> context, const std::string& name){
    context << "Hello " << name;
    context.finish();
}