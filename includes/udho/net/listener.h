#ifndef UDHO_NET_LISTENER_H
#define UDHO_NET_LISTENER_H

#include <type_traits>
#include <utility>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <udho/logging/macros.h>
#include <udho/manifold/flow.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/terminal.h>
#include <udho/net/channel.h>
#include <udho/net/detail.h>
#include <udho/net/fwd.h>
#include <udho/utils/misc.h>

namespace udho{
namespace net{

/** @addtogroup DoxyG_net
 *  @{
 */

namespace detail{

/**
 * @brief Common asynchronous accept loop for concrete listener policies.
 * @tparam DerivedT CRTP listener implementing on_accept().
 * @tparam RuntimeT Runtime that owns and creates flows.
 * @tparam SocketT Socket produced by the acceptor.
 * @tparam StreamT Stream transferred to the runtime.
 */
template <typename DerivedT, typename RuntimeT, typename SocketT, typename StreamT>
struct basic_listener: private wire_traits<typename SocketT::protocol_type>{
    using derived_type   = DerivedT;
    using runtime_type   = RuntimeT;
    using socket_type    = SocketT;
    using stream_type    = StreamT;
    using wire_type      = typename socket_type::protocol_type;
    using wire_types     = detail::wire_types<wire_type>;
    using traits_type    = detail::wire_traits<wire_type>;
    using acceptor_type  = typename wire_types::acceptor_type;
    using endpoint_type  = typename wire_types::endpoint_type;
    using executor_type  = typename socket_type::executor_type;
    using strand_type    = boost::asio::strand<executor_type>;
    using flow_type      = typename runtime_type::flow_type;
    using socket_id_type = udho::utils::misc::portable_socket_id;

    static_assert(std::is_same<socket_type, typename wire_types::socket_type>::value, "Listener socket must match the socket produced by the acceptor");
    static_assert(std::is_same<typename runtime_type::stream_type, stream_type>::value, "Listener stream must match runtime stream");

public:
    /** @brief Start listening for connections. */
    void start() {
        start([](boost::system::error_code error){
            // TODO Log error if any
        });
    }

    /**
     * @brief Start listening and report the setup result.
     * @param f Function invoked with the setup error code.
     */
    template <typename Function>
    void start(Function&& f) {
        boost::asio::post(_strand, [this, f = std::move(f)] {
            if (_running) return;
            _running = true;

            boost::system::error_code error;
            _acceptor.open(_endpoint.protocol(), error);
            if (error) {
                _running = false;
                f(error);
                return;
            }

            error = traits_type::prepare(_acceptor, _endpoint);
            if (error) {
                _running = false;
                f(error);
                return;
            }

            _acceptor.bind(_endpoint, error);
            if (error) {
                _running = false;
                f(error);
                return;
            }

            _acceptor.listen(boost::asio::socket_base::max_listen_connections, error);
            if (error) {
                _running = false;
                f(error);
                return;
            }

            f(error);

            UDHO_LOG_INFO("udho::net::listener", "started");
            accept();
        });
    }

    /** @brief Stop accepting connections and stop the runtime. */
    void stop() {
        boost::asio::post(_strand, [this] {
            if (!_running) return;
            _running = false;

            boost::system::error_code error = traits_type::cancel(_acceptor);
            _runtime.stop();

            namespace p = udho::logging::params;
            UDHO_LOG_INFO("udho::net::listener", "stopped");
        });
    }

protected:
    /**
     * @brief Construct the common listener state and take ownership of its endpoint.
     * @param io I/O context used for asynchronous operations.
     * @param runtime Runtime that owns accepted flows.
     * @param endpoint Endpoint on which to listen.
     */
    basic_listener(boost::asio::io_context& io, runtime_type& runtime, endpoint_type&& endpoint): _strand(io.get_executor()), _runtime(runtime), _endpoint(std::move(endpoint)), _acceptor(io.get_executor()), _running(false) {}

    runtime_type& runtime() { return _runtime; }
    strand_type& strand() { return _strand; }
    bool running() const { return _running; }

    /**
     * @brief Transfer an opened stream to the runtime and start its flow.
     * @param stream Ready application stream.
     * @param socket_id Portable identifier captured before transforming the socket.
     */
    void start_flow(stream_type&& stream, socket_id_type socket_id) {
        flow_type& flow = _runtime.spawn(std::move(stream));

        namespace p = udho::logging::params;
        UDHO_LOG_INFO("udho::net::listener", "Accepted incoming connection", p::flow_id(flow.id()), p::socket_id(socket_id));

        flow.start(); // flows are owned by runtime
    }

private:
    derived_type& derived() { return static_cast<derived_type&>(*this); }

    /** @brief Begin the next asynchronous accept operation. */
    void accept() {
        if(!_running) return;
        _acceptor.async_accept(
            boost::asio::bind_executor(_strand, [this](boost::system::error_code error, socket_type socket) {
                if (!_running) return;
                accept();
                derived().on_accept(error, std::move(socket));
            })
        );
    }

private:
    strand_type   _strand;
    runtime_type& _runtime;
    endpoint_type _endpoint;
    acceptor_type _acceptor;
    bool          _running;
};

}

/**
 * @brief Asynchronously accepts sockets, opens profile channels, and starts flows.
 * @tparam ProfileT Connection profile type.
 * @tparam RuntimeT Runtime that owns and creates flows.
 */
template <typename ProfileT, typename RuntimeT>
struct basic_listener: detail::basic_listener<basic_listener<ProfileT, RuntimeT>, RuntimeT, typename ProfileT::socket_type, typename ProfileT::stream_type>{
    using profile_type   = ProfileT;
    using runtime_type   = RuntimeT;
    using socket_type    = typename profile_type::socket_type;
    using stream_type    = typename profile_type::stream_type;
    using channel_type   = udho::net::channel<profile_type>;
    using base_type      = detail::basic_listener<basic_listener<profile_type, runtime_type>, runtime_type, socket_type, stream_type>;
    using endpoint_type  = typename base_type::endpoint_type;
    using socket_id_type = typename base_type::socket_id_type;

public:
    /**
     * @brief Construct a listener for a connection profile.
     * @param io I/O context used for asynchronous operations.
     * @param runtime Runtime that owns accepted flows.
     * @param profile Connection profile containing the listening endpoint.
     */
    basic_listener(boost::asio::io_context& io, runtime_type& runtime, profile_type& profile): base_type(io, runtime, endpoint_type(profile.endpoint())), _channel(profile) {}

private:
    friend base_type;

    /**
     * @brief Open a channel for an accepted connection.
     * @param error Result of the accept operation.
     * @param socket Accepted socket.
     */
    void on_accept(boost::system::error_code error, socket_type&& socket) {
        if(error) {
            UDHO_LOG_ERROR("udho::net::listener", "Failed to accept with error " + error.message());
        } else {
            socket_id_type socket_id = udho::utils::misc::native_handle(socket);
            _channel.async_open(std::move(socket), boost::asio::bind_executor(this->strand(),
                [this, socket_id](boost::system::error_code error, stream_type stream) mutable {
                    on_open(error, std::move(stream), socket_id);
                }
            ));
        }
    }

    /**
     * @brief Start a flow for an opened channel.
     * @param error Result of opening the channel.
     * @param stream Opened stream.
     * @param socket_id Portable identifier of the accepted socket.
     */
    void on_open(boost::system::error_code error, stream_type&& stream, socket_id_type socket_id) {
        if(error) {
            UDHO_LOG_ERROR("udho::net::listener", "Failed to open channel with error " + error.message());
            channel_type::shutdown(stream);
            return;
        }

        if(!this->running()) {
            channel_type::shutdown(stream);
            return;
        }

        this->start_flow(std::move(stream), socket_id);
    }

private:
    channel_type _channel;
};

/** @brief Empty specialization for stream-based test runtimes. */
template <typename RuntimeT>
struct basic_listener<profiles::test, RuntimeT>{};

/**
 * @brief Endpoint-based listener that passes accepted sockets directly to a runtime.
 * @tparam RuntimeT Runtime that owns and creates flows.
 * @tparam SocketT Socket produced by the acceptor and consumed by the runtime.
 */
template <typename RuntimeT, typename SocketT>
struct basic_plain_listener: detail::basic_listener<basic_plain_listener<RuntimeT, SocketT>, RuntimeT, SocketT, SocketT>{
    using runtime_type   = RuntimeT;
    using socket_type    = SocketT;
    using base_type      = detail::basic_listener<basic_plain_listener<runtime_type, socket_type>, runtime_type, socket_type, socket_type>;
    using endpoint_type  = typename base_type::endpoint_type;
    using socket_id_type = typename base_type::socket_id_type;

public:
    basic_plain_listener(boost::asio::io_context& io, runtime_type& runtime, endpoint_type&& endpoint): base_type(io, runtime, std::move(endpoint)) {}

private:
    friend base_type;

    void on_accept(boost::system::error_code error, socket_type&& socket) {
        if(error) {
            UDHO_LOG_ERROR("udho::net::plain_listener", "Failed to accept with error " + error.message());
            return;
        }

        socket_id_type socket_id = udho::utils::misc::native_handle(socket);
        this->start_flow(std::move(socket), socket_id);
    }
};

/**
 * @brief Create a listener for a runtime and connection profile.
 * @param io I/O context used for asynchronous operations.
 * @param runtime Runtime that owns accepted flows.
 * @param profile Connection profile containing the listening endpoint.
 * @return Listener configured for the supplied connection profile.
 */
template <typename ProfileT, typename RuntimeT>
basic_listener<ProfileT, RuntimeT> listener(boost::asio::io_context& io, RuntimeT& runtime, ProfileT& profile) {
    return basic_listener<ProfileT, RuntimeT>(io, runtime, profile);
}

/**
 * @brief Create an endpoint-based pass-through listener for a runtime.
 * @param io I/O context used for asynchronous operations.
 * @param runtime Runtime that owns accepted flows.
 * @param endpoint Endpoint on which to listen.
 * @return Listener that transfers accepted sockets directly to the runtime.
 */
template <typename RuntimeT>
basic_plain_listener<RuntimeT> plain_listener(boost::asio::io_context& io, RuntimeT& runtime, typename basic_plain_listener<RuntimeT>::endpoint_type&& endpoint) {
    return basic_plain_listener<RuntimeT>(io, runtime, std::move(endpoint));
}

/** @} */

}
}

#endif // UDHO_NET_LISTENER_H
