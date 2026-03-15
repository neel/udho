#ifndef UDHO_LOGGING_CONSUMER_H
#define UDHO_LOGGING_CONSUMER_H

#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <atomic>
#include <thread>
#include <udho/logging/ipc_queue.h>
#include <udho/logging/message.h>

namespace udho {
namespace logging {

struct consumer{
    using msg_type  = udho::logging::message;

    consumer(const char* name = 0x0): _ipc_queue(name) {}

    void consume(std::atomic_bool& should_stop) {
        while(true) {
            msg_type msg;
            bool success = _ipc_queue.try_receive(msg);
            if(success) {
                deliver(msg);
            } else {
                if(should_stop) {
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
    }

    void deliver(const msg_type& msg) {
        using namespace udho::logging::params; // brings all PARAM tags into scope

        // Build attribute set
        boost::log::attribute_set attrs;

        char pid_str[40];
        std::sprintf(pid_str, "%x", msg[process::val].value());
        char tid_str[40];
        std::sprintf(tid_str, "%zx", msg[thread::val].value());


        // ----- Mandatory fields -----
        attrs.insert("LocalID",     boost::log::attributes::make_constant(msg[local_id::val].value()) );
        attrs.insert("TimeStamp",   boost::log::attributes::make_constant(msg[timestamp::val].value()) );
        attrs.insert("Severity",    boost::log::attributes::make_constant(static_cast<std::underlying_type_t<udho::logging::severity>>(msg[udho::logging::params::severity::val].value())) );
        attrs.insert("ThreadID",    boost::log::attributes::make_constant(tid_str) );
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
};

}
}

#endif // UDHO_LOGGING_CONSUMER_H
