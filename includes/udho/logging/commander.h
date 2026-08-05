#ifndef UDHO_LOGGING_COMMANDER_H
#define UDHO_LOGGING_COMMANDER_H

#include <boost/asio.hpp>
#include <boost/asio/local/seq_packet_protocol.hpp>
#include <boost/asio/socket_base.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <udho/logging/protocol.h>

namespace udho {
namespace logging {

/** @addtogroup DoxyG_logging
 *  @{
 */

/**
 * @brief Result of one command execution.
 *
 * A result captures transport status, protocol validity, remote success/failure,
 * and any textual reply payload returned by the consumer.
 */
struct result {
    /**
     * @brief High-level completion status of the command.
     */
    enum class status_code : std::uint8_t {
        ok = 0,
        remote_error,
        transport_error,
        protocol_error
    };

    status_code code = status_code::protocol_error;
    std::string message;

    /**
     * @brief Return @c true when the command completed successfully.
     * @return @c true if @ref code equals @ref status_code::ok
     */
    bool success() const noexcept { return code == status_code::ok; }

    /**
     * @brief Boolean convenience conversion.
     * @return same value as @ref success
     */
    explicit operator bool() const noexcept { return success(); }
};

/** @} */

namespace detail {

/** @addtogroup DoxyG_logging
 *  @{
 */

template <typename ProtocolT>
struct sync_write_helper{
    using protocol_type = ProtocolT;
    using socket_type   = typename protocol_type::socket;
    using endpoint_type = typename protocol_type::endpoint;

    static result write(const std::string& socket_path, udho::logging::protocol::command cmd, const void* payload, std::uint32_t length, std::size_t max_reply_size) {
        try{
            boost::asio::io_context io;
            socket_type socket(io);

            endpoint_type endpoint(socket_path);
            socket.connect(endpoint);

            std::vector<std::uint8_t> request;
            request.reserve(sizeof(udho::logging::protocol::request_header) + length);

            udho::logging::protocol::request_header hdr(cmd, length);

            const std::uint8_t* hdr_begin = reinterpret_cast<const std::uint8_t*>(&hdr);
            request.insert(request.end(), hdr_begin, hdr_begin + sizeof(hdr));

            if (payload && length > 0) {
                const auto* payload_begin = reinterpret_cast<const std::uint8_t*>(payload);
                request.insert(request.end(), payload_begin, payload_begin + length);
            }

            socket.send(boost::asio::buffer(request), 0);

            std::vector<std::uint8_t> reply(max_reply_size);
            boost::asio::socket_base::message_flags out_flags = 0;

            std::size_t bytes_received = socket.receive(boost::asio::buffer(reply), out_flags);

            if (bytes_received < sizeof(udho::logging::protocol::reply_header)) {
                return {result::status_code::protocol_error, "reply packet too small"};
            }

            udho::logging::protocol::reply_header rhdr;
            std::memcpy(&rhdr, reply.data(), sizeof(rhdr));

            if (rhdr.magic != udho::logging::protocol::MAGIC) {
                return {result::status_code::protocol_error, "bad reply magic"};
            }

            if (bytes_received != sizeof(udho::logging::protocol::reply_header) + rhdr.length) {
                return {result::status_code::protocol_error, "malformed reply length" };
            }

            std::string message;
            if (rhdr.length > 0) {
                const char* msg_begin = reinterpret_cast<const char*>(reply.data() + sizeof(udho::logging::protocol::reply_header));
                message.assign(msg_begin, msg_begin + rhdr.length);
            }

            return {rhdr.success ? result::status_code::ok : result::status_code::remote_error, std::move(message) };
        } catch(const std::exception& e) {
            return {result::status_code::transport_error, e.what()};
        }
    }

};

/** @} */


}

/** @addtogroup DoxyG_logging
 *  @{
 */

/**
 * @brief Stateless synchronous client for the consumer admin socket.
 *
 * The commander is intended for short-lived tools such as @c udho-log that:
 * - connect to the Unix-domain admin socket
 * - send exactly one request packet
 * - receive exactly one reply packet
 * - exit
 */
struct commander{
    using protocol_type = boost::asio::local::seq_packet_protocol;
    using socket_type   = protocol_type::socket;
    using endpoint_type = protocol_type::endpoint;

    /**
     * @brief Construct a commander bound to a specific admin socket path.
     * @param socket_path filesystem path of the Unix-domain admin socket
     * @param max_packet_size maximum receive buffer size for reply packets
     */
    explicit commander(std::string socket_path): _socket_path(std::move(socket_path)), _max_packet_size(4096) {}

public:
    /**
     * @brief sets a filter for the consumer
     * @param filter_expression
     * @return command execution result
     */
    result filter_set(std::string_view filter_expression) { return execute(udho::logging::protocol::command::filter_set, filter_expression); }

    /**
     * @brief remove filter
     * @return command execution result
     */
    result filter_unset() { return execute(udho::logging::protocol::command::filter_unset); }

    /**
     * @brief shows current filter
     * @return command execution result
     */
    result filter_show() { return execute(udho::logging::protocol::command::filter_show); }

    /**
     * @brief temporarily enable/disable logging
     * @return command execution result
     */
    result temporary_enable(bool flag = true) {
        std::uint8_t state = flag ? 1 : 0;
        return execute(udho::logging::protocol::command::temporary_enable, &state, sizeof(std::uint8_t));
    }

    /**
     * @brief temporarily enable/disable logging
     * @return command execution result
     */
    result temporary_disable(bool flag = true) { return temporary_enable(!flag); }

private:
    /**
     * @brief Execute one command with a textual payload.
     * @param cmd command identifier
     * @param payload string payload
     * @return command execution result
     */
    result execute(udho::logging::protocol::command cmd, std::string_view payload) { return execute(cmd, payload.data(), payload.size()); }

    /**
     * @brief Execute one command with no payload.
     * @param cmd command identifier
     * @return command execution result
     */
    result execute(udho::logging::protocol::command cmd) { return execute(cmd, nullptr, 0); }


    /**
     * @brief Execute one command with a raw binary payload.
     * @param cmd command identifier
     * @param payload pointer to payload bytes, or @c nullptr for no payload
     * @param length payload size in bytes
     * @return command execution result
     *
     * The method opens a connection, sends one request packet, receives one reply
     * packet, validates it, and returns the decoded result.
     */
    result execute(udho::logging::protocol::command cmd, const void* payload, std::uint32_t length) {
        static_assert(std::is_trivially_copyable<udho::logging::protocol::request_header>::value, "request_header must be trivially copyable");
        static_assert(std::is_trivially_copyable<udho::logging::protocol::reply_header>::value, "reply_header must be trivially copyable");

        if (length > max_payload_size()) {
            return {result::status_code::protocol_error, "payload too large for request packet"};
        }

        return detail::sync_write_helper<protocol_type>::write(_socket_path, cmd, payload, length, _max_packet_size);
    }

public:
    /**
     * @brief Maximum payload size that fits in one request packet.
     */
    std::size_t max_payload_size() const noexcept {
        return (_max_packet_size > sizeof(udho::logging::protocol::request_header))
                    ? (_max_packet_size - sizeof(udho::logging::protocol::request_header))
                    : 0;
    }

    /**
     * @brief Socket path used by this commander.
     */
    const std::string& socket_path() const noexcept { return _socket_path; }

private:
    std::string _socket_path;
    std::size_t _max_packet_size;
};

/** @} */

}
}

#endif // UDHO_LOGGING_COMMANDER_H
