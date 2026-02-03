#ifndef UDHO_NET_LISTENER_H
#define UDHO_NET_LISTENER_H

#include <boost/asio/strand.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <udho/manifold/fwd.h>
#include <boost/asio/bind_executor.hpp>

namespace udho{
namespace net{

namespace detail{

template <typename WireT>
struct wire_types{
    using protocol_type = WireT;
#if (BOOST_VERSION / 1000 >=1 && BOOST_VERSION / 100 % 1000 >= 70)
    using socket_type   = boost::asio::basic_stream_socket<protocol_type, boost::asio::io_context::executor_type>;
    using acceptor_type = boost::asio::basic_socket_acceptor<protocol_type, boost::asio::io_context::executor_type>;
#else
    using socket_type   = boost::asio::basic_stream_socket<protocol_type>;
    using acceptor_type = boost::asio::basic_socket_acceptor<protocol_type>;
#endif
    using endpoint_type = typename protocol_type::endpoint;
};

template <typename WireT>
struct basic_wire_traits{
    using protocol_type = WireT;
    using wire_types    = detail::wire_types<protocol_type>;
    using socket_type   = typename wire_types::socket_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using endpoint_type = typename wire_types::endpoint_type;

protected:
    boost::system::error_code prepare(acceptor_type&, endpoint_type&) {
        return boost::system::error_code{};
    }
    boost::system::error_code cancel(acceptor_type& acceptor) {
        boost::system::error_code error;
        acceptor.cancel(error);
        acceptor.close(error);
        return error;
    }
};

template <typename WireT>
struct wire_traits: public basic_wire_traits<WireT>{
    using basic_traits_type = basic_wire_traits<WireT>;

    using basic_traits_type::prepare;
    using basic_traits_type::cancel;
};

template <>
struct wire_traits<boost::asio::ip::tcp>: public basic_wire_traits<boost::asio::ip::tcp>{
    using basic_traits_type = basic_wire_traits<boost::asio::ip::tcp>;

    boost::system::error_code prepare(acceptor_type& acceptor, endpoint_type& endpoint) {
        boost::system::error_code error;
        acceptor.set_option(boost::asio::socket_base::reuse_address(true), error);
        return error;
    }
    using basic_traits_type::cancel;
};

template <>
struct wire_traits<boost::asio::local::stream_protocol>: public basic_wire_traits<boost::asio::local::stream_protocol>{
    using basic_traits_type = basic_wire_traits<boost::asio::local::stream_protocol>;

    boost::system::error_code prepare(acceptor_type&, endpoint_type& endpoint) {
        errno = 0;
        if (::unlink(endpoint.path().c_str()) != 0) {
            if (errno == ENOENT) return {}; // no prior socket, OK
            return boost::system::error_code(errno, boost::system::system_category());
        }
        return {};
    }
    using basic_traits_type::cancel;
};


}

template <typename WireT, typename RuntimeT>
struct basic_listener: private detail::wire_traits<WireT>{
    using protocol_type = WireT;
    using wire_types    = detail::wire_types<protocol_type>;
    using traits_type   = detail::wire_traits<WireT>;
    using socket_type   = typename wire_types::socket_type;
    using acceptor_type = typename wire_types::acceptor_type;
    using endpoint_type = typename wire_types::endpoint_type;
    using executor_type = typename socket_type::executor_type;
    using strand_type   = boost::asio::strand<executor_type>;
    using runtime_type  = RuntimeT;

public:
    basic_listener(boost::asio::io_context& io, runtime_type& runtime, endpoint_type endpoint): _strand(io.get_executor()), _runtime(runtime), _endpoint(endpoint), _acceptor(io.get_executor()), _running(false) {}

    void start() {
        boost::asio::post(_strand, [this] {
            if (_running) return;
            _running = true;

            boost::system::error_code error;
            _acceptor.open(_endpoint.protocol(), error);
            if (error) {
                _running = false;
                return;
            }

            error = traits_type::prepare(_acceptor, _endpoint);
            if (error) {
                _running = false;
                return;
            }

            _acceptor.bind(_endpoint, error);
            if (error) {
                _running = false;
                return;
            }

            _acceptor.listen(boost::asio::socket_base::max_listen_connections, error);
            if (error) {
                _running = false;
                return;
            }

            accept();
        });
    }

    void stop() {
        boost::asio::post(_strand, [this] {
            if (!_running) return;
            _running = false;

            boost::system::error_code error = traits_type::cancel(_acceptor);
        });
    }

private:
    void accept() {
        if(!_running) return;
        _acceptor.async_accept(
            boost::asio::bind_executor(
                _strand,
                [this](boost::system::error_code error, socket_type socket) {
                    if (!_running) return;
                    on_accept(error, std::move(socket));

                    if (_running) accept();
                }
            )
        );
    }

    void on_accept(boost::system::error_code error, socket_type&& socket) {
        if(error) {
            // TODO report error
        } else {
            auto flow = _runtime.spawn(std::move(socket));
            flow->start();
            // flows are owned by runtime
        }
    }

private:
    strand_type              _strand;
    runtime_type&            _runtime;
    endpoint_type            _endpoint;
    acceptor_type            _acceptor;
    bool                     _running;
};

template <typename RuntimeT>
struct basic_listener<std::stringstream, RuntimeT>{};

template <typename RuntimeT>
basic_listener<typename RuntimeT::stream_type::protocol_type, RuntimeT> listener(boost::asio::io_context& io, RuntimeT& runtime, typename detail::wire_traits<typename RuntimeT::stream_type::protocol_type>::endpoint_type endpoint) {
    return basic_listener<typename RuntimeT::stream_type::protocol_type, RuntimeT>(io, runtime, endpoint);
}

}
}

#endif // UDHO_NET_LISTENER_H
