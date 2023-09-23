#ifndef UDHO_NET_COMMON_H
#define UDHO_NET_COMMON_H

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <udho/url/detail/format.h>

namespace udho{
namespace net{
namespace types{

#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::io_context::executor_type> socket;
#else
    typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp> socket;
#endif
using strand    = boost::asio::strand<boost::asio::io_context::executor_type>;

enum class stages{
    accepted,
    headers_read,
    body_read,
    body_skipped,
    headers_written,
    body_written,
    body_done,
    closed,
    rejected,
    error
};

namespace headers{
    using request  = boost::beast::http::header<true,  boost::beast::http::fields>;
    using response = boost::beast::http::header<false, boost::beast::http::fields>;
}



enum class buffering_options{
    first_write_flushes_headers,        // response headers get written to the socket before writting any content to the socket
    last_write_flushes_headers          // neither contents nor headers are sent over the socket untill all contents have been written
};

namespace transfer{
    enum class encoding{
        plain,
        chunked
    };

    enum class compression{
        none,
        compress,
        deflate,
        gzip
    };

    struct names{
        std::string operator[](encoding e) const {
            switch(e){
                case encoding::plain:   return "plain";     break;
                case encoding::chunked: return "chunked";   break;
            }
        }
        std::string operator[](compression c) const {
            switch(c){
                case compression::none:     return "plain"; break;
                case compression::compress: return "compress"; break;
                case compression::deflate:  return "deflate"; break;
                case compression::gzip:     return "gzip"; break;
            }
        }
    };
}

class transfer_encoding{
    transfer::encoding    _encoding;
    transfer::compression _compression;
    transfer::names       _names;

    public:
        inline explicit transfer_encoding(transfer::encoding enc = transfer::encoding::plain, transfer::compression compress = transfer::compression::none): _encoding(enc), _compression(compress) {}
        inline void set(transfer::encoding enc = transfer::encoding::plain, transfer::compression compress = transfer::compression::none) {
            _encoding = enc;
            _compression = compress;
        }
        inline void encoding(transfer::encoding enc) { _encoding = enc; }
        inline transfer::encoding encoding() const { return _encoding; }
        inline void compression(transfer::compression compress) { _compression = compress; }
        inline transfer::compression compression() const { return _compression; }
        inline void prepare(headers::response& response){
            std::string value = udho::url::format("{},{}", _names[_encoding], _names[_compression]);
            response.set(boost::beast::http::field::transfer_encoding, value);
        }
};

}
}
}

#endif // UDHO_NET_COMMON_H
