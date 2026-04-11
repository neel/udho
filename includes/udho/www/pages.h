#ifndef UDHO_WWW_PAGES_H
#define UDHO_WWW_PAGES_H

#include <udho/pages/layouts.h>
#include <udho/pages/system.h>

namespace udho {
namespace www {

namespace pages{

template <typename ContextT>
struct not_found{
    not_found(ContextT& context): _context(context) {}

    void operator()(const std::string& message){
        auto layout = udho::pages::system::layouts::listing(_context);

        namespace places = udho::pages::system::layouts::places;
        namespace placeholders = udho::pages::system::layouts::placeholders;

        _context.ostream().status(boost::beast::http::status::not_found);
        _context.ostream().set(boost::beast::http::field::content_type, "text/html");
        _context.ostream().set(boost::beast::http::field::connection, "keep-alive");

        layout[placeholders::header]    = udho::pages::system::data::listing_header{boost::beast::http::status::not_found};
        layout[places::routes]          = _context.portal().routes();
        layout[placeholders::footer]    = udho::pages::system::data::status_info{};

        layout[places::headline]        = udho::utils::format(R"(
            <div class="headline">
                <div class="msg">{}</div>
                <a href="/">docroot</a>
                <a href="{}">assets</a>
            </div>
        )", message, _context.portal().resources().assets().base());

        layout();
    }


private:
    ContextT& _context;
};

template <typename ContextT>
struct server_error{
    server_error(ContextT& context): _context(context) {}

    void operator()(const std::exception& ex, const boost::stacktrace::stacktrace& trace){
        auto layout = udho::pages::system::layouts::listing(_context);

        namespace places = udho::pages::system::layouts::places;
        namespace placeholders = udho::pages::system::layouts::placeholders;

        _context.ostream().status(boost::beast::http::status::internal_server_error);
        _context.ostream().set(boost::beast::http::field::content_type, "text/html");
        _context.ostream().set(boost::beast::http::field::connection, "keep-alive");

        layout[placeholders::header]    = udho::pages::system::data::listing_header{boost::beast::http::status::internal_server_error};
        layout[placeholders::footer]    = udho::pages::system::data::status_info{};

        std::stringstream trace_stream;
        for(const auto& stack: trace) {
            std::string line = udho::utils::format(
                    R"(
                        {} {} {}:{}
                    )",
                    stack.address(),
                    stack.name(),
                    stack.source_file(),
                    stack.source_line()
                );
            trace_stream << line << std::endl;
        }

        layout[places::headline]        = udho::utils::format(R"(
            <div class="headline">
                {}
            </div>
        )", trace_stream.str());

        layout();
    }


private:
    ContextT& _context;
};

}

}
}

#endif // UDHO_WWW_PAGES_H
