#ifndef UDHO_NET_CHANNEL_H
#define UDHO_NET_CHANNEL_H

#include <map>
#include <memory>
#include <utility>
#include <cassert>
#include <cstdint>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/system/error_code.hpp>
#include <udho/net/profile.h>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>

namespace udho{
namespace net{

/**
 * @brief Maps a connection profile to its accepted socket and application stream.
 * @tparam ProfileT Connection profile selected by the framework.
 */
template <typename ProfileT>
struct channel;

/** @brief Plain HTTP channel over TCP. */
template <>
struct channel<profiles::http>{
    using profile_type = profiles::http;
    using socket_type  = typename profile_type::socket_type;
    using stream_type  = typename profile_type::stream_type;

    explicit channel(profile_type& profile): _profile(profile) {}

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        handler(boost::system::error_code{}, std::move(socket));
    }

    static boost::system::error_code shutdown(stream_type& stream) {
        boost::system::error_code error;
        stream.close(error);
        return error;
    }

private:
    profile_type& _profile;
};

/** @brief HTTPS channel over TCP. */
template <>
struct channel<profiles::https>{
    using profile_type      = profiles::https;
    using socket_type       = typename profile_type::socket_type;
    using stream_type       = typename profile_type::stream_type;
    using stream_ptr_type   = std::unique_ptr<stream_type>;

    explicit channel(profile_type& profile): _profile(profile), _counter(0) {}

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        auto executor = boost::asio::get_associated_executor(handler, socket.get_executor());

        std::uint64_t id = _counter++;
        auto result = _staged.emplace(id, std::make_unique<stream_type>(std::move(socket), _profile.ssl()));
        assert(result.second);

        stream_ptr_type& stream_ptr = result.first->second;
        stream_ptr->async_handshake(boost::asio::ssl::stream_base::server,
            boost::asio::bind_executor(executor, [this, id, handler = std::forward<HandlerT>(handler)](boost::system::error_code error) mutable {
                auto it = _staged.find(id);
                assert(it != _staged.end());

                stream_ptr_type stream_ptr = std::move(it->second);
                _staged.erase(it);
                handler(error, std::move(*stream_ptr));
            })
        );
    }

    static boost::system::error_code shutdown(stream_type& stream) {
        boost::system::error_code error;
        stream.next_layer().close(error);
        return error;
    }

private:
    profile_type& _profile;
    std::uint64_t   _counter;
    // TODO Add a strand-serialized periodic TLS-handshake sweep, a staging limit, and stop-time cancellation/draining of staged streams.
    std::map<std::uint64_t, stream_ptr_type> _staged;
};

/** @brief Plain channel over a local stream protocol socket. */
template <>
struct channel<profiles::local>{
    using profile_type = profiles::local;
    using socket_type  = typename profile_type::socket_type;
    using stream_type  = typename profile_type::stream_type;

    explicit channel(profile_type& profile): _profile(profile) {}

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        handler(boost::system::error_code{}, std::move(socket));
    }

    static boost::system::error_code shutdown(stream_type& stream) {
        boost::system::error_code error;
        stream.close(error);
        return error;
    }

private:
    profile_type& _profile;
};

/** @brief Pass-through channel for Beast test streams. */
template <>
struct channel<profiles::test>{
    using profile_type = profiles::test;
    using socket_type  = typename profile_type::socket_type;
    using stream_type  = typename profile_type::stream_type;

    explicit channel(profile_type& profile): _profile(profile) {}

    template <typename HandlerT>
    void async_open(socket_type&& socket, HandlerT&& handler) {
        handler(boost::system::error_code{}, std::move(socket));
    }

    static boost::system::error_code shutdown(stream_type& stream) {
        stream.close();
        stream.close_remote();
        return boost::system::error_code{};
    }

private:
    profile_type& _profile;
};

}
}

#endif // UDHO_NET_CHANNEL_H
