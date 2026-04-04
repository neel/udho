#ifndef UDHO_LOGGING_CONSUMER_H
#define UDHO_LOGGING_CONSUMER_H

#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <string>
#include <type_traits>
#include <cstring>
#include <chrono>
#include <vector>
#include <iterator>
#include <udho/logging/ipc_queue.h>
#include <udho/logging/message.h>
#include <udho/utils/string_view.h>
#include <udho/utils/format.h>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <udho/logging/macros.h>
#include <udho/logging/protocol.h>
#include <boost/asio.hpp>
#include <boost/asio/local/seq_packet_protocol.hpp>
#include <boost/asio/socket_base.hpp>


namespace udho {
namespace logging {

namespace {

template <typename KeyT>
void add_optional_attr(const udho::logging::message& msg, boost::log::attribute_set& attrs, const KeyT k, const char* name){
    if (msg[k].value().has_value())
        attrs.insert(name, boost::log::attributes::make_constant(msg[k].value().value()));
}

}

/**
 * @brief Single-threaded log consumer and admin-command endpoint.
 *
 * The consumer performs two tasks in the same thread:
 * - drains log messages from the interprocess queue and forwards them into Boost.Log
 * - accepts and handles administrative commands over a Unix-domain seq-packet socket
 *
 * The consumer owns the admin socket path for the lifetime of the object and
 * processes one admin session at a time.
 */
struct consumer{
    using message_type  = udho::logging::message;
    using protocol_type = boost::asio::local::seq_packet_protocol;
    using acceptor_type = protocol_type::acceptor;
    using socket_type   = protocol_type::socket;
    using message_flags = boost::asio::socket_base::message_flags;

    static constexpr const char* default_socket_path = "/tmp/udho-log.sock";

    /**
     * @brief Construct a consumer for the given queue and admin socket path.
     * @param socket_path filesystem path of the Unix-domain admin socket
     * @param name IPC queue name to open
     *
     * The constructor unlinks any stale socket path, binds and listens on the
     * admin socket, and arms the first asynchronous accept operation.
     */
    consumer(const char* socket_path = 0x0, const char* name = 0x0): _ipc_queue(name), _socket_path(socket_path ? socket_path : default_socket_path), _acceptor(_io), _socket(_io), _out_flags(0), _enabled(true) {
        ::unlink(_socket_path.c_str());

        typename protocol_type::endpoint ep(_socket_path);
        _acceptor.open();
        _acceptor.bind(ep);
        _acceptor.listen(1);

        start_accept();
    }

    /**
     * @brief Destroy the consumer and release all owned transport resources.
     *
     * Closes the acceptor and active client socket, then unlinks the socket path.
     */
    ~consumer() {
        boost::system::error_code error_acceptor;
        _acceptor.close(error_acceptor);

        boost::system::error_code errot_socket;
        _socket.close(errot_socket);

        ::unlink(_socket_path.c_str());
    }

    /**
     * @brief Run the consumer loop until stop is requested.
     * @param should_stop external stop flag observed by the loop
     *
     * The loop polls ready admin-socket handlers, drains a bounded number of
     * queued log messages, and uses a small backoff sleep when idle.
     */
    void consume(std::atomic_bool& should_stop) {
        std::size_t backoff = 1, backoff_ceiling = 4;
        while(true) {
            std::size_t ready = _io.poll();

            std::size_t drained = 0;
            for (; drained < 64; ++drained) {
                message_type msg;
                if (!_ipc_queue.try_receive(msg)) break;
                deliver(msg);
            }

            if (should_stop) break;

            if (ready == 0 && drained == 0) {
                if (backoff > backoff_ceiling) backoff = 1;
                std::this_thread::sleep_for(std::chrono::milliseconds(1 << backoff++));
            } else {
                backoff = 1;
            }
        }
    }

private:

    /**
     * @brief Starts an asynchronous accept for the next admin session.
     *
     * On accept success, the consumer starts reading one request packet from the
     * newly accepted socket. On error, the condition is logged and the consumer
     * restarts the session lifecycle.
     */
    void start_accept() {
        _acceptor.async_accept(_socket, [this](const boost::system::error_code& ec) {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) return;

                std::string error_message;
                try{
                    error_message = udho::utils::format("Error while accepting {}", ec.message());
                } catch(const std::exception& ex) {
                    message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                    deliver(msg);
                }

                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
                deliver(msg);

                finish_session();
                return;
            }

            _out_flags = 0;

            start_read();
        });
    }

    /**
     * @brief Start asynchronously receiving one admin request packet.
     *
     * The packet is read into the internal read buffer. On successful receipt,
     * parsing is delegated to @ref parse. On failure, the error is logged and the
     * active session is terminated.
     */
    void start_read() {
        _socket.async_receive(
            boost::asio::buffer(_read_buffer, sizeof(_read_buffer)),
            _out_flags,
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    std::string error_message;
                    try{
                        error_message = udho::utils::format("Error while receiving command {}", ec.message());
                    } catch(const std::exception& ex) {
                        message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                        deliver(msg);
                        finish_session();
                        return;
                    }

                    message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
                    deliver(msg);
                    finish_session();
                    return;
                }

                parse(bytes_transferred);
            }
        );
    }

    /**
     * @brief Send a reply packet to the active admin client.
     * @param success success flag to encode in the reply header
     * @param reply textual reply payload
     *
     * The method serializes a @ref protocol::reply_header followed by the reply
     * payload, then asynchronously sends it over the active client socket.
     * The session is closed when the send completes.
     */
    void start_write(bool success, std::string&& reply) {
        protocol::reply_header hdr(success, reply.size());

        _write_buffer.clear();
        _write_buffer.reserve(sizeof(protocol::reply_header) + reply.size());

        const char* reply_begin = reinterpret_cast<const char*>(&hdr);
        const char* reply_end   = reply_begin + sizeof(hdr);

        std::copy(reply_begin,   reply_end,   std::back_inserter(_write_buffer));
        std::copy(reply.begin(), reply.end(), std::back_inserter(_write_buffer));

        _socket.async_send(
            boost::asio::buffer(_write_buffer),
            0,
            [this](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
                if (ec) {
                    message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", udho::utils::format("Error while sending reply {}", ec.message()), __FILE__, __LINE__, __func__);
                    deliver(msg);
                }

                _write_buffer.clear();
                finish_session();
            }
        );
    }

    /**
     * @brief Close the current admin session and resume accepting new ones.
     *
     * This closes the active client socket and immediately starts a new accept.
     */
    void finish_session() {
        boost::system::error_code ignored;
        _socket.close(ignored);
        start_accept();
    }

private:

    /**
     * @brief Parse one received admin request packet.
     * @param bytes_read number of bytes received into the read buffer
     *
     * The parser validates the request header, checks the packet size, and
     * dispatches the decoded command to @ref command.
     */
    void parse(std::size_t bytes_read) {
        if (bytes_read < sizeof(protocol::request_header)) {
            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Error packet too small", __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, "Error packet too small");
            return;
        }

        protocol::request_header hdr;
        std::memcpy(&hdr, _read_buffer, sizeof(hdr));

        if (hdr.magic != protocol::MAGIC) {
            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Bad Magic", __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, "Bad Magic");
            return;
        }

        if (bytes_read != sizeof(protocol::request_header) + hdr.length) {
            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Malformed command", __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, "Malformed command");
            return;
        }

        if(!command(protocol::command(hdr.cmd), hdr.length))
            return;
    }

    /**
     * @brief Execute one decoded administrative command.
     * @param cmd command identifier
     * @param length payload size in bytes
     * @return @c true when command dispatch completed normally, @c false when
     *         command handling aborted early due to a protocol or execution error
     *
     * Supported commands include filter manipulation and temporary delivery enable/disable.
     */
    bool command(protocol::command cmd, std::uint32_t length) {
        if(length > sizeof(_read_buffer) - sizeof(protocol::request_header)) {
            std::string error_message;
            try{
                error_message = udho::utils::format("Command too big {} bytes", length);
            } catch(const std::exception& ex) {
                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);
                finish_session();
                return false;
            }

            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
            return false;
        }

        if(cmd == protocol::command::filter_set) {
            std::string filter(_read_buffer + sizeof(protocol::request_header), length);
            filter_set(filter);
        } else if(cmd == protocol::command::filter_unset) {
            filter_reset();
        } else if(cmd == protocol::command::filter_show) {
            filter_show();
        } else if(cmd == protocol::command::temporary_enable){
            // read one byte
            auto begin = _read_buffer + sizeof(protocol::request_header);
            if(length != 1) {
                std::string error_message;
                try{
                    error_message = udho::utils::format("Expected exactly 1 byte payload, but received {} bytes", length);
                } catch(const std::exception& ex) {
                    message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                    deliver(msg);
                    finish_session();
                    return false;
                }

                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
                deliver(msg);

                start_write(false, std::move(error_message));
                return false;
            }

            std::uint8_t value = *begin;
            if(value != 0 && value != 1) {
                std::string error_message = "Expected payload byte to be 0 or 1";
                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
                deliver(msg);
                start_write(false, std::move(error_message));
                return false;
            }
            delivery_enable(value == 1);
        } else {
            std::string error_message;
            try{
                error_message = udho::utils::format("Unknown command {} received over admin transport", static_cast<std::uint32_t>(std::underlying_type_t<protocol::command>(cmd)));
            } catch(const std::exception& ex) {
                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);
                finish_session();
                return false;
            }

            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
        }
        return true;
    }

private:

    /**
     * @brief Apply a new Boost.Log filter expression.
     * @param filter_str textual filter expression
     *
     * On success, the filter is installed in Boost.Log core and remembered for
     * later inspection by @ref filter_show.
     */
    void filter_set(const std::string& filter_str) {
        try {
            auto filter = boost::log::parse_filter(filter_str.c_str());
            boost::log::core::get()->set_filter(filter);
            start_write(true, "Applied filter successfully");
            _filter_text = filter_str;
        } catch (const std::exception& e) {
            std::string error_message;
            try{
                error_message = udho::utils::format("Error {} while seting filter {} at consumer", e.what(), filter_str);
            } catch(const std::exception& ex) {
                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);

                finish_session();
                return;
            }

            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
        }
    }

    /**
     * @brief Remove the currently installed Boost.Log filter.
     *
     * On success, the Boost.Log core filter is reset and the stored filter text
     * is cleared.
     */
    void filter_reset() {
        try {
            boost::log::core::get()->reset_filter();
            start_write(true, "Removed filters successfully");
            _filter_text.reset();
        } catch (const std::exception& e) {
            std::string error_message;
            try{
                error_message = udho::utils::format("Error {} while removing filter at consumer", e.what());
            } catch(const std::exception& ex) {
                message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);

                finish_session();
                return;
            }

            message_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
        }
    }

    /**
     * @brief Report the currently installed filter text to the admin client.
     *
     * If no filter is installed, a success reply describing that state is sent.
     */
    void filter_show() {
        if(!_filter_text.has_value()) {
            start_write(true, "No filter is set");
            return;
        }

        std::string filter_str = _filter_text.value();
        start_write(true, std::move(filter_str));
    }

    /**
     * @brief Enable or disable forwarding of consumed log messages into Boost.Log.
     * @param flag @c true to enable delivery, @c false to suppress delivery
     *
     * This affects only consumer-side forwarding. It does not change producer behavior
     * or queueing behavior.
     */
    void delivery_enable(bool flag) {
        _enabled = flag;

        std::string state = flag ? "enabled" : "disabled";
        std::string message = "Logging " + state;

        start_write(true, std::move(message));
    }

private:

    /**
     * @brief Transform one transported message into a Boost.Log record and push it.
     * @param msg consumed log message
     *
     * Mandatory fields are always inserted into the record attribute set.
     * Optional fields are inserted only when present.
     * When consumer-side delivery is disabled, the message is discarded.
     */
    void deliver(const message_type& msg) {
        if(!_enabled) return;

        using namespace udho::logging::params;

        boost::log::attribute_set attrs;

        attrs.insert(names::local_id,  boost::log::attributes::make_constant(msg[local_id::val].value()) );
        attrs.insert(names::timestamp, boost::log::attributes::make_constant(msg[timestamp::val].value()) );
        attrs.insert(names::severity,  boost::log::attributes::make_constant(msg[udho::logging::params::severity::val].value()) );
        attrs.insert(names::thread,    boost::log::attributes::make_constant(msg[thread::val].value()) );
        attrs.insert(names::process,   boost::log::attributes::make_constant(msg[process::val].value()) );
        attrs.insert(names::subsystem, boost::log::attributes::make_constant(msg[subsystem::val].value()) );
        attrs.insert(names::file,      boost::log::attributes::make_constant(msg[file::val].value()) );
        attrs.insert(names::function,  boost::log::attributes::make_constant(msg[function::val].value()) );
        attrs.insert(names::line,      boost::log::attributes::make_constant(msg[line::val].value()) );

        add_optional_attr(msg, attrs, request_id::val,      names::request_id);
        add_optional_attr(msg, attrs, flow_id::val,         names::flow_id);
        add_optional_attr(msg, attrs, session_id::val,      names::session_id);
        add_optional_attr(msg, attrs, user_id::val,         names::user_id);
        add_optional_attr(msg, attrs, client::val,          names::client);
        add_optional_attr(msg, attrs, host::val,            names::host);
        add_optional_attr(msg, attrs, method::val,          names::method);
        add_optional_attr(msg, attrs, uri::val,             names::uri);
        add_optional_attr(msg, attrs, route::val,           names::route);
        add_optional_attr(msg, attrs, query::val,           names::query);
        add_optional_attr(msg, attrs, agent::val,           names::agent);
        add_optional_attr(msg, attrs, status_code::val,     names::status_code);
        add_optional_attr(msg, attrs, bytes_sent::val,      names::bytes_sent);
        add_optional_attr(msg, attrs, latency::val,         names::latency);
        add_optional_attr(msg, attrs, retry_count::val,     names::retry_count);
        add_optional_attr(msg, attrs, error_code::val,      names::error_code);
        add_optional_attr(msg, attrs, error_message::val,   names::error_message);

        if (auto record = boost::log::core::get()->open_record(attrs)) {
            boost::log::record_ostream stream(record);
            stream << msg[udho::logging::params::message::val].value();
            stream.flush();
            boost::log::core::get()->push_record(std::move(record));
        }
    }

private:
    udho::logging::detail::ipc_queue _ipc_queue;
    std::string                      _socket_path;
    char                             _read_buffer[4096];
    std::vector<std::uint8_t>        _write_buffer;
    boost::asio::io_context          _io;
    acceptor_type                    _acceptor;
    socket_type                      _socket;
    message_flags                    _out_flags;
    std::optional<std::string>       _filter_text;
    bool                             _enabled;
};

}
}

#endif // UDHO_LOGGING_CONSUMER_H
