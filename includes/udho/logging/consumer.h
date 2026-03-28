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

struct consumer{
    using msg_type      = udho::logging::message;
    using protocol_type = boost::asio::local::seq_packet_protocol;
    using acceptor_type = protocol_type::acceptor;
    using socket_type   = protocol_type::socket;
    using message_flags = boost::asio::socket_base::message_flags;

    consumer(const char* socket_path, const char* name = 0x0): _ipc_queue(name), _socket_path(socket_path), _acceptor(_io), _socket(_io), _out_flags(0) {
        ::unlink(_socket_path.c_str());

        typename protocol_type::endpoint ep(_socket_path);
        _acceptor.open();
        _acceptor.bind(ep);
        _acceptor.listen(1);

        start_accept();
    }

    ~consumer() {
        boost::system::error_code error_acceptor;
        _acceptor.close(error_acceptor);

        boost::system::error_code errot_socket;
        _socket.close(errot_socket);

        ::unlink(_socket_path.c_str());
    }

    void consume(std::atomic_bool& should_stop) {
        std::size_t backoff = 1, backoff_ceiling = 4;
        while(true) {
            std::size_t ready = _io.poll();

            std::size_t drained = 0;
            for (; drained < 64; ++drained) {
                msg_type msg;
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
    void start_accept() {
        _acceptor.async_accept(_socket, [this](const boost::system::error_code& ec) {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) return;

                std::string error_message;
                try{
                    error_message = udho::utils::format("Error while accepting {}", ec.message());
                } catch(const std::exception& ex) {
                    msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                    deliver(msg);
                }

                msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
                deliver(msg);

                finish_session();
                return;
            }

            _out_flags = 0;

            start_read();
        });
    }

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
                        msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                        deliver(msg);
                        finish_session();
                        return;
                    }

                    msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
                    deliver(msg);
                    finish_session();
                    return;
                }

                if ((_out_flags & boost::asio::socket_base::message_end_of_record) == 0) {
                    msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Error packet too big", __FILE__, __LINE__, __func__);
                    deliver(msg);
                    start_write(false, "Error packet too big");
                    return;
                }
                parse(bytes_transferred);
            }
        );
    }

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
                    msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", udho::utils::format("Error while sending reply {}", ec.message()), __FILE__, __LINE__, __func__);
                    deliver(msg);
                }

                _write_buffer.clear();
                finish_session();
            }
        );
    }

    void finish_session() {
        boost::system::error_code ignored;
        _socket.close(ignored);
        start_accept();
    }

private:
    void parse(std::size_t bytes_read) {
        if (bytes_read < sizeof(protocol::request_header)) {
            msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Error packet too small", __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, "Error packet too small");
            return;
        }

        protocol::request_header hdr;
        std::memcpy(&hdr, _read_buffer, sizeof(hdr));

        if (hdr.magic != protocol::MAGIC) {
            msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Bad Magic", __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, "Bad Magic");
            return;
        }

        if (bytes_read != sizeof(protocol::request_header) + hdr.length) {
            msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", "Malformed command", __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, "Malformed command");
            return;
        }

        if(!command(protocol::command(hdr.cmd), hdr.length))
            return;
    }

    bool command(protocol::command cmd, std::uint32_t length) noexcept {
        if(length > sizeof(_read_buffer) - sizeof(protocol::request_header)) {
            std::string error_message;
            try{
                error_message = udho::utils::format("Command too big {} bytes", length);
            } catch(const std::exception& ex) {
                msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);
                finish_session();
                return false;
            }

            msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
            return false;
        }

        if(cmd == protocol::command::filter_set) {
            std::string filter(_read_buffer + sizeof(protocol::request_header), length);
            reconfigure(filter);
        } else {
            std::string error_message;
            try{
                error_message = udho::utils::format("Unknown command {} received over admin transport", static_cast<std::uint32_t>(std::underlying_type_t<protocol::command>(cmd)));
            } catch(const std::exception& ex) {
                msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);
                finish_session();
                return false;
            }

            msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
        }
        return true;
    }

    void reconfigure(const std::string& filter_str) noexcept {
        try {
            auto filter = boost::log::parse_filter(filter_str.c_str());
            boost::log::core::get()->set_filter(filter);
            start_write(true, "Applied filter successfully");
        } catch (const std::exception& e) {
            std::string error_message;
            try{
                error_message = udho::utils::format("Error {} while seting filter {} at consumer", e.what(), filter_str);
            } catch(const std::exception& ex) {
                msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", (std::string("Exception in format ") + ex.what()), __FILE__, __LINE__, __func__);
                deliver(msg);

                finish_session();
                return;
            }

            msg_type msg = udho::logging::detail::make_record(udho::logging::severity::error, "logging", error_message, __FILE__, __LINE__, __func__);
            deliver(msg);

            start_write(false, std::move(error_message));
        }
    }

    void deliver(const msg_type& msg) {
        using namespace udho::logging::params; // brings all PARAM tags into scope

        // Build attribute set
        boost::log::attribute_set attrs;


        // ----- Mandatory fields -----
        attrs.insert("LocalID",     boost::log::attributes::make_constant(msg[local_id::val].value()) );
        attrs.insert("TimeStamp",   boost::log::attributes::make_constant(msg[timestamp::val].value()) );
        attrs.insert("Severity",    boost::log::attributes::make_constant(static_cast<std::underlying_type_t<udho::logging::severity>>(msg[udho::logging::params::severity::val].value())) );
        attrs.insert("ThreadID",    boost::log::attributes::make_constant(msg[thread::val].value()) );
        attrs.insert("ProcessID",   boost::log::attributes::make_constant(msg[process::val].value()) );
        attrs.insert("Subsystem",   boost::log::attributes::make_constant(msg[subsystem::val].value()) );
        attrs.insert("Message",     boost::log::attributes::make_constant(msg[udho::logging::params::message::val].value()) );
        attrs.insert("File",        boost::log::attributes::make_constant(msg[file::val].value()) );
        attrs.insert("Function",    boost::log::attributes::make_constant(msg[function::val].value()) );
        attrs.insert("Line",        boost::log::attributes::make_constant(msg[line::val].value()) );

        // ----- Optional fields -----
        if (msg[request_id::val].value().has_value())
            attrs.insert("RequestID", boost::log::attributes::make_constant(msg[request_id::val].value()));

        if (msg[flow_id::val].value().has_value())
            attrs.insert("FlowID", boost::log::attributes::make_constant(msg[flow_id::val].value()));

        if (msg[session_id::val].value().has_value())
            attrs.insert("SessionID", boost::log::attributes::make_constant(msg[session_id::val].value()));

        if (msg[user_id::val].value().has_value())
            attrs.insert("UserID", boost::log::attributes::make_constant(msg[user_id::val].value()));

        if (msg[client_ip::val].value().has_value())
            attrs.insert("ClientIP", boost::log::attributes::make_constant(msg[client_ip::val].value()));

        if (msg[host::val].value().has_value())
            attrs.insert("Host", boost::log::attributes::make_constant(msg[host::val].value()));

        if (msg[http_method::val].value().has_value())
            attrs.insert("HTTPMethod", boost::log::attributes::make_constant(msg[http_method::val].value()));

        if (msg[uri::val].value().has_value())
            attrs.insert("URI", boost::log::attributes::make_constant(msg[uri::val].value()));

        if (msg[route::val].value().has_value())
            attrs.insert("Route", boost::log::attributes::make_constant(msg[route::val].value()));

        if (msg[query::val].value().has_value())
            attrs.insert("Query", boost::log::attributes::make_constant(msg[query::val].value()));

        if (msg[user_agent::val].value().has_value())
            attrs.insert("UserAgent", boost::log::attributes::make_constant(msg[user_agent::val].value()));

        if (msg[status_code::val].value().has_value())
            attrs.insert("StatusCode", boost::log::attributes::make_constant(msg[status_code::val].value()));

        if (msg[bytes_sent::val].value().has_value())
            attrs.insert("BytesSent", boost::log::attributes::make_constant(msg[bytes_sent::val].value()));

        if (msg[latency::val].value().has_value())
            attrs.insert("LatencyNS", boost::log::attributes::make_constant(msg[latency::val].value()));

        if (msg[retry_count::val].value().has_value())
            attrs.insert("RetryCount", boost::log::attributes::make_constant(msg[retry_count::val].value()));

        if (msg[error_code::val].value().has_value())
            attrs.insert("ErrorCode", boost::log::attributes::make_constant(msg[error_code::val].value()));

        if (msg[error_message::val].value().has_value())
            attrs.insert("ErrorMsg", boost::log::attributes::make_constant(msg[error_message::val].value()));

        // Open a record with these attributes
        if (auto record = boost::log::core::get()->open_record(attrs)) {
            boost::log::record_ostream strm(record);
            strm << msg[udho::logging::params::message::val].value(); // stream the msg text
            strm.flush();
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
};

}
}

#endif // UDHO_LOGGING_CONSUMER_H
