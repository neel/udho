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

template <typename OStreamT>
struct server_error{
    server_error(OStreamT& ostream): _ostream(ostream) {}

    void operator()(const std::exception& ex, const boost::stacktrace::stacktrace& trace){
        if(!_ostream.headers_sealed()) {
            _ostream.status(boost::beast::http::status::internal_server_error);
            _ostream.set(boost::beast::http::field::content_type, "text/html");
            _ostream.set(boost::beast::http::field::connection, "keep-alive");
        }

        _ostream.write(html(ex, trace));
        _ostream.finish();
    }

    template <typename ErrorCodeT>
    void operator()(const ErrorCodeT& ec, const boost::stacktrace::stacktrace& trace){
        if(!_ostream.headers_sealed()) {
            _ostream.status(boost::beast::http::status::internal_server_error);
            _ostream.set(boost::beast::http::field::content_type, "text/html");
            _ostream.set(boost::beast::http::field::connection, "keep-alive");
        }

        _ostream.write(html(ec, trace));
        _ostream.finish();
    }

private:
    std::string html(const std::exception& ex, const boost::stacktrace::stacktrace& trace){
        static constexpr const char* trace_line_template    = R"(
            <div class="stack">
                <div class="address">{}</div>
                <div class="name">{}</div>
                <div class="file">{}</div>
                <div class="line">{}</div>
            </div>
        )";
        static constexpr const char* full_exception_template = R"(
            <div class="exception">
                <style>

                </style>
                <div class="message">{}</div>
                <div class="trace">
                    {}
                </div>
            </div>
        )";

        std::vector<std::string> lines;
        for(const auto& stack: trace) {
            std::string line = udho::utils::format(trace_line_template, stack.address(), stack.name(), stack.source_file(), stack.source_line());
            lines.emplace_back(line);
        }
        return udho::utils::format(full_exception_template, ex.what(), boost::algorithm::join(lines, "\n"));
    }

    template <typename ErrorCodeT>
    std::string html(const ErrorCodeT& ec, const boost::stacktrace::stacktrace& trace){
        static constexpr const char* trace_line_template    = R"(
            <div class="stack">
                <div class="address">{}</div>
                <div class="name">{}</div>
                <div class="file">{}</div>
                <div class="line">{}</div>
            </div>
        )";
        static constexpr const char* full_exception_template = R"(
            <div class="error_code">
                <style>

                </style>
                <div class="value">{}</div>
                <div class="message">{}</div>
                <div class="trace">
                    {}
                </div>
            </div>
        )";

        std::vector<std::string> lines;
        for(const auto& stack: trace) {
            std::string line = udho::utils::format(trace_line_template, stack.address(), stack.name(), stack.source_file(), stack.source_line());
            lines.emplace_back(line);
        }
        return udho::utils::format(full_exception_template, ec.value(), ec.message(), boost::algorithm::join(lines, "\n"));
    }

private:
    OStreamT& _ostream;
};

}

}
}

#endif // UDHO_WWW_PAGES_H
