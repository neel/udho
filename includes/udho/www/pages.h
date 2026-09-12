#ifndef UDHO_WWW_PAGES_H
#define UDHO_WWW_PAGES_H

#include <udho/pages/layouts.h>
#include <udho/pages/system.h>
#include <cpptrace/cpptrace.hpp>
#include <udho/utils/detail/libiberty_helper.h>
#include <boost/exception/diagnostic_information.hpp>

namespace udho {
namespace www {

namespace pages{

template <typename ContextT>
struct client_error{
    client_error(ContextT& context): _context(context) {}

    void operator()(const udho::http::error& error){
        auto layout = udho::pages::system::layouts::listing(_context);

        namespace places = udho::pages::system::layouts::places;
        namespace placeholders = udho::pages::system::layouts::placeholders;

        _context.ostream().status(error.status());
        _context.ostream().set(boost::beast::http::field::content_type, "text/html");
        _context.ostream().set(boost::beast::http::field::connection, error.keep_alive() ? std::string("keep-alive") : std::string("close"));

        layout[placeholders::header]    = udho::pages::system::data::listing_header{error.status()};
        layout[placeholders::footer]    = udho::pages::system::data::status_info{};
        layout[places::headline]        = udho::utils::format(R"(
            <div class="headline">
                <div class="msg">{}</div>
                <a href="/">docroot</a>
                <a href="{}">assets</a>
            </div>
        )", error.what(), _context.portal().resources().assets().base());

        if(error.status() == boost::beast::http::status::not_found) {
            layout[places::routes]          = _context.portal().routes();
        }

        layout();
    }


private:
    ContextT& _context;
};

template <typename OStreamT>
struct server_error{
    static const constexpr udho::utils::string_view header_str_template = R"(
        <div class="header">
            <div class="status">{} {}</div>
        </div>
    )";

    server_error(OStreamT& ostream): _ostream(ostream) {}

    template <typename ExceptionT, std::enable_if_t<std::is_base_of_v<std::exception, ExceptionT> || std::is_base_of_v<boost::exception, ExceptionT>, bool> = true>
    void operator()(const ExceptionT& ex, const cpptrace::stacktrace& trace, bool keep_alive){
        boost::beast::http::status status = boost::beast::http::status::internal_server_error;
        std::string message = "Internal Server Error";

        try{
            const udho::http::error& herror = dynamic_cast<const udho::http::error&>(ex);

            status = herror.status();
            std::stringstream stream;
            stream << status;
            message = stream.str();

            keep_alive = herror.keep_alive();
        } catch (const std::bad_cast&) { }

        if(!_ostream.headers_sealed()) {
            _ostream.status(status);
            _ostream.set(boost::beast::http::field::content_type, "text/html");
            _ostream.set(boost::beast::http::field::connection, keep_alive ? std::string("keep-alive") : std::string("close"));
        }

        _ostream._write(udho::utils::format(header_str_template, static_cast<std::underlying_type_t<boost::beast::http::status>>(status), message));
        _ostream._write(html(ex, trace));
        _ostream.finish();
    }

    template <typename ErrorCodeT, std::enable_if_t<std::is_same_v<ErrorCodeT, std::error_code> || std::is_same_v<ErrorCodeT, boost::system::error_code>, bool> = true>
    void operator()(const ErrorCodeT& ec, const cpptrace::stacktrace& trace, bool keep_alive){
        if(!_ostream.headers_sealed()) {
            _ostream.status(boost::beast::http::status::internal_server_error);
            _ostream.set(boost::beast::http::field::content_type, "text/html");
            _ostream.set(boost::beast::http::field::connection, keep_alive ? std::string("keep-alive") : std::string("close"));
        }

        _ostream.write(udho::utils::format(header_str_template, 500, "Internal Server Error"));
        _ostream.write(html(ec, trace));
        _ostream.finish();
    }

private:
    template <typename ExceptionT, std::enable_if_t<std::is_base_of_v<std::exception, ExceptionT> || std::is_base_of_v<boost::exception, ExceptionT>, bool> = true>
    std::string html(const ExceptionT& ex, const cpptrace::stacktrace& trace){
        static constexpr const char* trace_line_template    = R"(
            <div class="stack" tabindex="0">
                <div class="address">{}</div>
                <div class="name">{}</div>
                <div class="file">{}</div>
                <div class="line">{}</div>
            </div>
        )";
        static constexpr const char* full_exception_template = R"(
            <div class="exception">
                <style>
                    {}
                </style>
                <div class="message">{}</div>
                <div class="trace">
                    {}
                </div>
            </div>
        )";

        std::vector<std::string> lines;
        for(const auto& stack: trace.frames) {
            std::string symbol  = cpptrace::prettify_symbol(demangle(stack.symbol));
            std::string address = udho::utils::format("0x{:x}", stack.raw_address);
            std::string line    = udho::utils::format(trace_line_template, address, udho::utils::encode::escape(symbol), stack.filename, stack.line.value_or(0));
            lines.emplace_back(line);
        }
        return udho::utils::format(full_exception_template, css(), udho::utils::encode::escape(exception_message(ex)), boost::algorithm::join(lines, "\n"));
    }

    template <typename ErrorCodeT, std::enable_if_t<std::is_same_v<ErrorCodeT, std::error_code> || std::is_same_v<ErrorCodeT, boost::system::error_code>, bool> = true>
    std::string html(const ErrorCodeT& ec, const cpptrace::stacktrace& trace){
        static constexpr const char* trace_line_template    = R"(
            <div class="stack" tabindex="0">
                <div class="address">{}</div>
                <div class="name">{}</div>
                <div class="file">{}</div>
                <div class="line">{}</div>
            </div>
        )";
        static constexpr const char* full_exception_template = R"(
            <div class="error_code">
                <style>
                    {}
                </style>
                <div class="value">{}</div>
                <div class="message">{}</div>
                <div class="trace">
                    {}
                </div>
            </div>
        )";

        std::vector<std::string> lines;
        for(const auto& stack: trace.frames) {
            std::string symbol  = cpptrace::prettify_symbol(demangle(stack.symbol));
            std::string address = udho::utils::format("0x{:x}", stack.raw_address);
            std::string line    = udho::utils::format(trace_line_template, address, udho::utils::encode::escape(symbol), stack.filename, stack.line.value_or(0));
            lines.emplace_back(line);
        }
        return udho::utils::format(full_exception_template, css(), ec.value(), udho::utils::encode::escape(ec.message()), boost::algorithm::join(lines, "\n"));
    }

    static udho::utils::string_view css() {
        return R"CSS(
        .header {
            margin-bottom: 0.75rem;
            padding-bottom: 0.75rem;
            border-bottom: 1px solid #ddd;
            font-family: system-ui, sans-serif;
        }

        .status {
            font-size: 1.15rem;
            font-weight: 700;
            color: #c0392b;
        }

        .error_code, .exception {
            max-width: 100%;
            margin: 1rem;
            padding: 1rem;
            border: 1px solid #ddd;
            border-left: 4px solid #c0392b;
            border-radius: 6px;
            background: #fafafa;
            font-family: system-ui, sans-serif;
            color: #222;
        }

        .error_code > .value, .exception > .type {
            display: inline-block;
            margin-right: 0.5rem;
            padding: 0.15rem 0.5rem;
            border-radius: 4px;
            background: #c0392b;
            color: #fff;
            font-weight: 700;
        }

        .error_code > .message, .exception > .message {
            display: inline-block;
            font-weight: 600;
            color: #c0392b;
        }

        .error_code > .trace, .exception > .trace {
            margin-top: 1rem;
            border-top: 1px solid #ddd;
            font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
            font-size: 0.85rem;
            line-height: 1.45;
        }

        .stack {
            display: grid;
            grid-template-columns: 16ch minmax(18rem, 1fr) minmax(14rem, 28rem) 5ch;
            gap: 0.75rem;
            padding: 0.45rem 0;
            border-bottom: 1px solid #eee;
            align-items: start;
        }

        .address, .name, .file, .line {
            min-width: 0;
            user-select: text;
        }

        .address {
            color: #555;
            white-space: nowrap;
        }

        .name, .file {
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }

        .name {
            color: #111;
        }

        .file {
            color: #777;
        }

        .line {
            color: #777;
            text-align: right;
            white-space: nowrap;
            font-variant-numeric: tabular-nums;
        }

        .stack:hover {
            background: #fff;
        }

        .stack:hover .name, .stack:hover .file, .stack:focus-within .name, .stack:focus-within .file {
            overflow: visible;
            white-space: normal;
            overflow-wrap: anywhere;
            word-break: break-word;
            background: #fff;
            outline: 1px solid #ddd;
            border-radius: 4px;
            padding: 0.25rem;
            position: relative;
            z-index: 1;
        }

        .name::selection, .file::selection, .address::selection, .line::selection {
            background: #ffe4a3;
            color: #111;
        }

        @media (max-width: 900px) {
            .stack {
                grid-template-columns: 1fr;
                gap: 0.25rem;
                padding: 0.7rem 0;
            }

            .line {
                text-align: left;
            }

            .address::before {
                content: "address: ";
                color: #999;
            }

            .name::before {
                content: "symbol: ";
                color: #999;
            }

            .file::before {
                content: "file: ";
                color: #999;
            }

            .line::before {
                content: "line: ";
                color: #999;
            }
        }
        )CSS";
    }

private:
    static std::string demangle(const std::string& input) {
        if(input.empty()) {
            return {};
        }

#ifdef WITH_LIBIBERTY
        return udho::utils::detail::demangle_with_libiberty(input);
#else
        int status = 0;

        std::unique_ptr<char, void(*)(void*)> demangled(
            abi::__cxa_demangle(input.c_str(), nullptr, nullptr, &status),
            std::free
            );

        if(status == 0 && demangled) {
            return std::string(demangled.get());
        }

        return input;
#endif // WITH_LIBIBERTY

    }

    static std::string exception_message(const boost::exception& ex) { return boost::diagnostic_information_what(ex); }

    static std::string exception_message(const std::exception& ex) { return ex.what(); }
private:
    OStreamT& _ostream;
};

}

}
}

#endif // UDHO_WWW_PAGES_H
