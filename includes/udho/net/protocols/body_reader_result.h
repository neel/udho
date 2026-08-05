#ifndef UDHO_NET_PROTOCOLS_BODY_READER_RESULT_H
#define UDHO_NET_PROTOCOLS_BODY_READER_RESULT_H

#include <udho/net/protocols/form_data.h>

namespace udho{
namespace net{
namespace protocols{

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Holds the body and form data produced by a request body reader.
 * @tparam Buffer Type used to store the request body.
 */
template <typename Buffer>
struct body_reader_result{
    using buffer_type            = Buffer;
    using form_container_type    = detail::form_data::form_container_type;

    /** @brief Return mutable parsed form data. */
    detail::form_data& form() { return _form; }
    /** @brief Return parsed form data. */
    const detail::form_data& form() const { return _form; }

    /** @brief Return the buffered request body. */
    const buffer_type& buffer() const { return _buffer; }
    /** @brief Return the mutable buffered request body. */
    buffer_type& buffer() { return _buffer; }

    /** @brief Move out the parsed form data and reset it. */
    detail::form_data release_form() {
        return std::exchange(_form, {});
    }

    /** @brief Move out the buffered body and reset it. */
    buffer_type release_buffer() {
        return std::exchange(_buffer, {});
    }

private:
    buffer_type           _buffer;
    detail::form_data     _form;
};

/** @} */

}
}
}

#endif // UDHO_NET_PROTOCOLS_BODY_READER_RESULT_H
