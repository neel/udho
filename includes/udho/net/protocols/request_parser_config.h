#ifndef UDHO_NET_REQUEST_PARSER_CONFIG_H
#define UDHO_NET_REQUEST_PARSER_CONFIG_H

#include <chrono>

namespace udho{
namespace net{

namespace detail{

struct body_parser_config {
    body_parser_config()
        : _total_timeout(std::chrono::seconds(0))
        , _total_content_limit(0)
        , _field_content_limit(0)
        , _upload_in_buffer(false)
    {}

    std::chrono::seconds total_timeout() const { return _total_timeout; }
    body_parser_config& total_timeout(std::chrono::seconds timeout) {
        _total_timeout = timeout;
        return *this;
    }

    std::size_t total_content_limit() const { return _total_content_limit; }
    body_parser_config& total_content_limit(std::size_t limit) {
        _total_content_limit = limit;
        return *this;
    }

    std::size_t field_content_limit() const { return _field_content_limit; }
    body_parser_config& field_content_limit(std::size_t limit) {
        _field_content_limit = limit;
        return *this;
    }

    bool upload_in_buffer() const { return _upload_in_buffer; }
    body_parser_config& upload_in_buffer(bool flag) {
        _upload_in_buffer = flag;
        return *this;
    }

private:
    std::chrono::seconds _total_timeout;
    std::size_t          _total_content_limit;
    std::size_t          _field_content_limit;
    bool                 _upload_in_buffer;
};

}

}
}

#endif // UDHO_NET_REQUEST_PARSER_CONFIG_H
