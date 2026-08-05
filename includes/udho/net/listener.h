#ifndef UDHO_NET_LISTENER_H
#define UDHO_NET_LISTENER_H

#include <boost/asio/strand.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <udho/manifold/fwd.h>
#include <boost/asio/bind_executor.hpp>
#include <udho/net/detail.h>
#include <udho/manifold/flow.h>
#include <udho/manifold/terminal.h>
#include <udho/logging/macros.h>

namespace udho{
namespace net{

/** @addtogroup DoxyG_net
 *  @{
 */

/**
 * @brief Asynchronously accepts connections and starts a flow for each one.
 * @tparam WireT Transport protocol type.
 * @tparam RuntimeT Runtime that owns and creates flows.
 */
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
    using flow_type     = typename runtime_type::flow_type;

public:
    /**
     * @brief Construct a listener for an endpoint.
     * @param io I/O context used for asynchronous operations.
     * @param runtime Runtime that owns accepted flows.
     * @param endpoint Endpoint on which to listen.
     */
    basic_listener(boost::asio::io_context& io, runtime_type& runtime, endpoint_type endpoint): _strand(io.get_executor()), _runtime(runtime), _endpoint(endpoint), _acceptor(io.get_executor()), _running(false) {}

    /** @brief Start listening for connections. */
    void start() {
        start([](boost::system::error_code error){
            // noop
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

private:
    /** @brief Begin the next asynchronous accept operation. */
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

    /**
     * @brief Start a flow for an accepted connection.
     * @param error Result of the accept operation.
     * @param socket Accepted socket.
     */
    void on_accept(boost::system::error_code error, socket_type&& socket) {
        if(error) {
            UDHO_LOG_ERROR("udho::net::listener", "Failed to accept with error " + error.message());
        } else {
            auto socket_id = udho::utils::misc::native_handle(socket);
            flow_type& flow = _runtime.spawn(std::move(socket));

            namespace p = udho::logging::params;
            UDHO_LOG_INFO(
                "udho::net::listener", "Accepted incoming connection",
                p::flow_id(flow.id()),
                p::socket_id(socket_id)
            );

            flow.start(); // flows are owned by runtime
        }
    }

private:
    strand_type              _strand;
    runtime_type&            _runtime;
    endpoint_type            _endpoint;
    acceptor_type            _acceptor;
    bool                     _running;
};

/** @brief Empty specialization for stream-based test runtimes. */
template <typename RuntimeT>
struct basic_listener<std::stringstream, RuntimeT>{};

/**
 * @brief Create a listener for a runtime and endpoint.
 * @param io I/O context used for asynchronous operations.
 * @param runtime Runtime that owns accepted flows.
 * @param endpoint Endpoint on which to listen.
 * @return Listener configured for the runtime's transport protocol.
 */
template <typename RuntimeT>
basic_listener<typename RuntimeT::stream_type::protocol_type, RuntimeT> listener(boost::asio::io_context& io, RuntimeT& runtime, typename detail::wire_traits<typename RuntimeT::stream_type::protocol_type>::endpoint_type endpoint) {
    return basic_listener<typename RuntimeT::stream_type::protocol_type, RuntimeT>(io, runtime, endpoint);
}

/** @} */

}
}

#endif // UDHO_NET_LISTENER_H
