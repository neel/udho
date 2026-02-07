#ifndef UDHO_EXCEPTIONS_H
#define UDHO_EXCEPTIONS_H

#include <stdexcept>
#include <boost/beast/http/status.hpp>

namespace udho{
namespace http{

class error : public std::runtime_error {
    boost::beast::http::status _status;

public:
    explicit error(boost::beast::http::status st, std::string msg = {})
        : std::runtime_error(msg.empty() ? std::string(boost::beast::http::obsolete_reason(st)) : std::move(msg))
        , _status(st)
    {}

    boost::beast::http::status status() const noexcept { return _status; }
    boost::beast::http::status_class status_class() const noexcept { return boost::beast::http::to_status_class(_status); }
};

}
}

#endif // UDHO_EXCEPTIONS_H
