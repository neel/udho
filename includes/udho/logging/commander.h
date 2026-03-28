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

struct commander{
    using protocol_type = boost::asio::local::seq_packet_protocol;
    using socket_type   = protocol_type::socket;
    using endpoint_type = protocol_type::endpoint;

    /**
     * @brief Result of a command execution.
     */
    struct result {
        enum class status_code : std::uint8_t {
            ok = 0,
            remote_error,
            transport_error,
            protocol_error
        };

        status_code code = status_code::protocol_error;
        std::string message;

        bool success() const noexcept { return code == status_code::ok; }

        explicit operator bool() const noexcept { return success(); }
    };

    explicit commander(std::string socket_path): _socket_path(std::move(socket_path)), _max_packet_size(4096) {}

public:
    /**
     * @brief Send a string payload command.
     * @param cmd command identifier
     * @param payload textual payload
     * @return result
     */
    result execute(udho::logging::protocol::command cmd, std::string_view payload) const { return execute(cmd, payload.data(), payload.size()); }

    /**
     * @brief Send a command with no payload.
     * @param cmd command identifier
     * @return result
     */
    result execute(udho::logging::protocol::command cmd) const { return execute(cmd, nullptr, 0); }

    result filter_set(std::string_view filter_expression) const { return execute(udho::logging::protocol::command::filter_set, filter_expression); }

    result filter_unset() const { return execute(udho::logging::protocol::command::filter_unset); }

    result filter_show() const { return execute(udho::logging::protocol::command::filter_show); }

    /**
     * @brief Set threshold using an opaque string payload.
     *
     * Example payloads:
     * - "trace"
     * - "debug"
     * - "2"
     *
     * The exact interpretation is up to the server side.
     */
    result threshold_set(std::string_view threshold_value) const { return execute(udho::logging::protocol::command::threshold_set, threshold_value); }

    result threshold_show() const { return execute(udho::logging::protocol::command::threshold_show); }

    result sink_list() const { return execute(udho::logging::protocol::command::sink_list); }

    /**
     * @brief Add a sink using an opaque string payload.
     *
     * The exact payload format is defined by the server.
     */
    result sink_add(std::string_view sink_spec) const { return execute(udho::logging::protocol::command::sink_add, sink_spec); }

    /**
     * @brief Remove a sink using an opaque string payload.
     *
     * The exact payload format is defined by the server.
     */
    result sink_remove(std::string_view sink_spec) const { return execute(udho::logging::protocol::command::sink_remove, sink_spec); }

private:
    result execute(udho::logging::protocol::command cmd, const void* payload, std::uint32_t length) const {
        static_assert(std::is_trivially_copyable<udho::logging::protocol::request_header>::value, "request_header must be trivially copyable");
        static_assert(std::is_trivially_copyable<udho::logging::protocol::reply_header>::value, "reply_header must be trivially copyable");

        if (length > max_payload_size()) {
            return {result::status_code::protocol_error, "payload too large for request packet"};
        }

        try {
            boost::asio::io_context io;
            socket_type socket(io);

            endpoint_type endpoint(_socket_path);
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

            std::vector<std::uint8_t> reply(_max_packet_size);
            boost::asio::socket_base::message_flags out_flags = 0;

            std::size_t bytes_received = socket.receive(boost::asio::buffer(reply), out_flags);

            if ((out_flags & boost::asio::socket_base::message_end_of_record) == 0) {
                return {result::status_code::protocol_error, "reply packet truncated or incomplete"};
            }

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
        } catch (const std::exception& e) {
            return {result::status_code::transport_error, e.what()};
        }
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

}
}

#endif // UDHO_LOGGING_COMMANDER_H
