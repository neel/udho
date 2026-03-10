#ifndef UDHO_NET_PROTOCOLS_BODY_READER_RESULT_H
#define UDHO_NET_PROTOCOLS_BODY_READER_RESULT_H

#include <udho/net/protocols/form_data.h>

namespace udho{
namespace net{
namespace protocols{

template <typename Buffer>
struct body_reader_result{
    using buffer_type            = Buffer;
    using form_container_type    = detail::form_data::form_container_type;

    detail::form_data& form() { return _form; }
    const detail::form_data& form() const { return _form; }

    const buffer_type& buffer() const { return _buffer; }
    buffer_type& buffer() { return _buffer; }

    detail::form_data release_form() {
        return std::exchange(_form, {});
    }

    buffer_type release_buffer() {
        return std::exchange(_buffer, {});
    }

private:
    buffer_type           _buffer;
    detail::form_data     _form;
};

}
}
}

#endif // UDHO_NET_PROTOCOLS_BODY_READER_RESULT_H
