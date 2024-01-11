#ifndef UDHO_EXCEPTIONS_H
#define UDHO_EXCEPTIONS_H

#include <exception>
#include <sstream>
#include <udho/net/context.h>
#include <boost/beast/http/status.hpp>

namespace udho{
namespace http{

class exception: std::exception{
    boost::asio::ip::address  _remote;
    udho::net::context        _context;
    std::string               _message;
    public:
        inline exception(const boost::asio::ip::address& remote, const udho::net::context& context) noexcept: std::exception(), _remote(remote), _context(context) {}
        exception(const exception& other) = default;
        inline const boost::asio::ip::address remote() const { return _remote; }
        inline udho::net::context& context() { return _context; }
        inline exception& message(const std::string& msg) { _message = msg; return *this; }
        inline std::string message() const { return _message; }
        virtual const char* what() const noexcept { return _message.c_str(); }
};

class error: exception{
    boost::beast::http::status _status;
    public:
        inline error(const boost::asio::ip::address& remote, const udho::net::context& context, boost::beast::http::status status) noexcept: exception(remote, context), _status(status) {}
        error(const error& other) = default;
        inline boost::beast::http::status status() const { return _status; }
        inline boost::beast::http::status_class category() const { return boost::beast::http::to_status_class(_status); }
        inline std::string reason() const {
            std::stringstream stream;
            stream << status();
            return stream.str();
        }
};

}
}

#endif // UDHO_EXCEPTIONS_H
