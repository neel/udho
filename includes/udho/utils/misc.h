#ifndef UDHO_UTILS_MISC_H
#define UDHO_UTILS_MISC_H

#include <cstdint>
#include <sstream>
#include <type_traits>
#include <utility>
#include <boost/asio/post.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

namespace udho{
namespace utils{
namespace misc{

using portable_socket_id = std::uint64_t;

inline portable_socket_id socket_portable_native_handle(const boost::asio::ip::tcp::socket::native_handle_type& handle) {
    static_assert(std::is_integral_v<portable_socket_id>);

    if constexpr (std::is_integral_v<boost::asio::ip::tcp::socket::native_handle_type> || std::is_enum_v<boost::asio::ip::tcp::socket::native_handle_type>) {
        return static_cast<portable_socket_id>(handle);
    } else if constexpr (std::is_convertible_v<boost::asio::ip::tcp::socket::native_handle_type, std::uintptr_t>) {
        return static_cast<portable_socket_id>(static_cast<std::uintptr_t>(handle));
    } else {
        static_assert(
            std::is_convertible_v<boost::asio::ip::tcp::socket::native_handle_type, std::uintptr_t>,
            "boost::asio::ip::tcp::socket::native_handle_type is not normalizable to an integer log id"
        );
        return 0;
    }
}

namespace detail{

template <typename StreamT>
struct get_native_handle{
    static portable_socket_id apply(StreamT& stream) {
        return socket_portable_native_handle(stream.native_handle());
    }
};

template <>
struct get_native_handle<boost::beast::test::stream>{
    static portable_socket_id apply(boost::beast::test::stream&) {
        return 0;
    }
};

template <typename NextLayerT>
struct get_native_handle<boost::asio::ssl::stream<NextLayerT>>{
    static portable_socket_id apply(boost::asio::ssl::stream<NextLayerT>& stream) {
        return get_native_handle<NextLayerT>::apply(stream.next_layer());
    }
};

template <typename CharT>
struct get_native_handle<std::basic_stringstream<CharT>>{
    static portable_socket_id apply(std::basic_stringstream<CharT>&) {
        return 0;
    }
};

}

template <typename StreamT>
inline portable_socket_id native_handle(StreamT& stream) {
    return detail::get_native_handle<StreamT>::apply(stream);
}

namespace detail{

template <typename StreamT>
struct stream_termination{
    static boost::system::error_code apply(StreamT& stream) {
        boost::system::error_code result;
        boost::system::error_code error;
        stream.cancel(error);
        if(!result && error) result = error;
        stream.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
        if(!result && error) result = error;
        stream.close(error);
        if(!result && error) result = error;
        return result;
    }

    template <typename HandlerT>
    static void async_apply(StreamT& stream, HandlerT&& handler) {
        boost::asio::post(stream.get_executor(), [&stream, handler = std::forward<HandlerT>(handler)]() mutable {
            boost::system::error_code error = stream_termination<StreamT>::apply(stream);
            handler(error);
        });
    }
};

template <>
struct stream_termination<boost::beast::test::stream>{
    static boost::system::error_code apply(boost::beast::test::stream& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
    }

    template <typename HandlerT>
    static void async_apply(boost::beast::test::stream& stream, HandlerT&& handler) {
        boost::asio::post(stream.get_executor(), [&stream, handler = std::forward<HandlerT>(handler)]() mutable {
            boost::system::error_code error = stream_termination<boost::beast::test::stream>::apply(stream);
            handler(error);
        });
    }
};

template <typename NextLayerT>
struct stream_termination<boost::asio::ssl::stream<NextLayerT>>{
    using stream_type = boost::asio::ssl::stream<NextLayerT>;

    static boost::system::error_code apply(stream_type& stream) {
        return stream_termination<NextLayerT>::apply(stream.next_layer());
    }

    template <typename HandlerT>
    static void async_apply(stream_type& stream, HandlerT&& handler) {
        stream.async_shutdown([&stream, handler = std::forward<HandlerT>(handler)](boost::system::error_code error) mutable {
            boost::system::error_code close_error = stream_termination<NextLayerT>::apply(stream.next_layer());
            if(!error) error = close_error;
            handler(error);
        });
    }
};

template <typename CharT>
struct stream_termination<std::basic_stringstream<CharT>>{
    using stream_type = std::basic_stringstream<CharT>;

    static boost::system::error_code apply(stream_type&) {
        return boost::system::error_code{};
    }

    template <typename HandlerT>
    static void async_apply(stream_type&, HandlerT&& handler) {
        // Non-I/O manifold streams have no executor and complete inline.
        std::forward<HandlerT>(handler)(boost::system::error_code{});
    }
};

template <typename StreamT>
static boost::system::error_code terminate_stream(StreamT& stream){
    return stream_termination<StreamT>::apply(stream);
}

template <typename StreamT>
struct stream_available{
    static std::size_t apply(StreamT& stream) {
        return stream.available();
    }
};

template <>
struct stream_available<boost::beast::test::stream>{
    static std::size_t apply(boost::beast::test::stream& stream) {
        return 256;
    }
};

template <typename NextLayerT>
struct stream_available<boost::asio::ssl::stream<NextLayerT>>{
    static std::size_t apply(boost::asio::ssl::stream<NextLayerT>&) {
        return 0;
    }
};

}

}
}
}

#endif // UDHO_UTILS_MISC_H
