#ifndef UDHO_NET_COMMON_H
#define UDHO_NET_COMMON_H

#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
// #include <boost/asio/strand.hpp>
// #include <boost/format.hpp>
// #include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
// #include <udho/url/detail/format.h>

namespace udho{
namespace net{
namespace types{

/** @addtogroup DoxyG_net
 *  @{
 */

/// @brief TCP stream socket using the project's supported Asio executor type.
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket;
#endif
// using strand    = boost::asio::strand<boost::asio::io_context::executor_type>;

// enum class stages{
//     accepted,
//     headers_read,
//     body_read,
//     body_skipped,
//     headers_written,
//     body_written,
//     body_done,
//     closed,
//     rejected,
//     error
// };

/** @} */

namespace headers{
    /** @addtogroup DoxyG_net
     *  @{
     */
    /// @brief HTTP request header type used by protocol readers.
    using request  = boost::beast::http::header<true,  boost::beast::http::fields>;
    // using response = boost::beast::http::header<false, boost::beast::http::fields>;
    /** @} */
}



// enum class buffering_options{
//     first_write_flushes_headers,        // response headers get written to the socket before writting any content to the socket
//     last_write_flushes_headers          // neither contents nor headers are sent over the socket untill all contents have been written
// };

namespace transfer{
    /** @addtogroup DoxyG_net
     *  @{
     */
    /// @brief Framing used to transmit a response body.
    enum class encoding{
        plain,   ///< Send the body without chunk framing.
        chunked  ///< Send the body using HTTP chunk framing.
    };

    /// @brief Compression configured for a response body.
    enum class compression{
        none,       ///< Do not compress the body.
        compress,   ///< Use the compress coding.
        deflate,    ///< Use the deflate coding.
        gzip        ///< Use the gzip coding.
    };

    // struct names{
    //     std::string operator[](encoding e) const {
    //         switch(e){
    //             case encoding::plain:   return "plain";     break;
    //             case encoding::chunked: return "chunked";   break;
    //             default: __builtin_unreachable();
    //         }
    //     }
    //     std::string operator[](compression c) const {
    //         switch(c){
    //             case compression::none:     return "none"; break;
    //             case compression::compress: return "compress"; break;
    //             case compression::deflate:  return "deflate"; break;
    //             case compression::gzip:     return "gzip"; break;
    //             default: __builtin_unreachable();
    //         }
    //     }
    // };
    /** @} */
}

/** @addtogroup DoxyG_net
 *  @{
 */

/** @brief Stores response transfer framing and compression settings. */
class transfer_encoding{
    transfer::encoding    _encoding;
    transfer::compression _compression;
    // transfer::names       _names;

    public:
        /** @brief Construct transfer settings with optional framing and compression. */
        inline explicit transfer_encoding(transfer::encoding enc = transfer::encoding::plain, transfer::compression compress = transfer::compression::none): _encoding(enc), _compression(compress) {}
        /** @brief Set framing and compression together. */
        inline void set(transfer::encoding enc = transfer::encoding::plain, transfer::compression compress = transfer::compression::none) {
            _encoding = enc;
            _compression = compress;
        }
        /** @brief Set the transfer framing. */
        inline void encoding(transfer::encoding enc) { _encoding = enc; }
        /** @brief Return the transfer framing. */
        inline transfer::encoding encoding() const { return _encoding; }
        /** @brief Set the transfer compression. */
        inline void compression(transfer::compression compress) { _compression = compress; }
        /** @brief Return the transfer compression. */
        inline transfer::compression compression() const { return _compression; }
        // inline void prepare(headers::response& response){
        //     if (_encoding == transfer::encoding::plain && _compression == transfer::compression::none) {
        //         return;
        //     } else {
        //         std::string value = (_compression == transfer::compression::none)
        //                                     ? udho::url::format("{}", _names[_encoding])
        //                                     : udho::url::format("{},{}", _names[_compression], _names[_encoding]);
        //         response.set(boost::beast::http::field::transfer_encoding, value);
        //     }
        // }
};

/** @} */

}
}
}

#endif // UDHO_NET_COMMON_H
