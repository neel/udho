#ifndef UDHO_NET_OSTREAM_DETAIL_CHUNKING_HELPER_H
#define UDHO_NET_OSTREAM_DETAIL_CHUNKING_HELPER_H

#include <array>
#include <charconv>
#include <boost/beast/core/flat_buffer.hpp>

namespace udho{
namespace net{

namespace detail{

/**
 * @brief Helper for HTTP chunked transfer framing.
 * @note Intended to be used only from within a strand.
 */
struct chunking_helper{
    chunking_helper(): _crlf({0x0d, 0x0a}), _last_chunk({'0', 0x0d, 0x0a, 0x0d, 0x0a}) {}

    /**
     * @brief Convert chunk size to hexadecimal ASCII.
     * @param size Payload size.
     * @param buffered_bytes_size_hex Output buffer to write hexadecimal ASCII digits into.
     * @return Number of characters written to the buffer.
     * @pre buffered_bytes_size_hex has sufficient size for the conversion (20 is plenty for size_t).
     */
    std::size_t make_chunk_header(std::size_t size, std::array<char, 20>& buffered_bytes_size_hex) {
        std::size_t buffered_bytes_size = size;
        std::to_chars_result result = std::to_chars(buffered_bytes_size_hex.data(), buffered_bytes_size_hex.data()+buffered_bytes_size_hex.size(), buffered_bytes_size, 16);
        assert (result.ec == std::errc());
        std::size_t buffered_bytes_size_hex_len   = std::distance(buffered_bytes_size_hex.data(), result.ptr);
        return buffered_bytes_size_hex_len;
    }

    /**
     * @brief Prepare `buffer` with "<hex-size>\\r\\n".
     * @param buffer Destination buffer (must be empty).
     * @param size Chunk payload size.
     *
     * @pre buffer.size() == 0
     * @post buffer contains the size header followed by CRLF, committed.
     */
    void prepare(boost::beast::flat_buffer& buffer, std::size_t size) {
        std::array<char, 20> buffered_bytes_size_hex = {0};
        std::size_t buffered_bytes_size_hex_len      = make_chunk_header(size, buffered_bytes_size_hex);
        const char* buffered_bytes_size_hex_begin    = buffered_bytes_size_hex.data();

        assert(buffer.size() == 0);
        auto mutable_buffer = buffer.prepare(buffered_bytes_size_hex_len + 2);
        mutable_buffer += boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(buffered_bytes_size_hex_begin, buffered_bytes_size_hex_len));
        mutable_buffer += boost::asio::buffer_copy(mutable_buffer, boost::asio::buffer(_crlf.data(), 2));
        buffer.commit(buffered_bytes_size_hex_len + 2);
    }
public:
    std::array<char, 2>        _crlf;
    std::array<char, 5>        _last_chunk;
};


}

}
}

#endif // UDHO_NET_OSTREAM_DETAIL_CHUNKING_HELPER_H
