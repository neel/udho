#ifndef UDHO_UTILS_MISC_H
#define UDHO_UTILS_MISC_H

#include <cstdint>
#include <type_traits>
#include <boost/asio/ip/tcp.hpp>
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
        boost::system::error_code error;
        stream.cancel(error);
        stream.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
        return error;
    }
};

template <>
struct stream_termination<boost::beast::test::stream>{
    static boost::system::error_code apply(boost::beast::test::stream& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
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

}

}
}
}

#endif // UDHO_UTILS_MISC_H
