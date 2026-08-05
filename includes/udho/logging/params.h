#ifndef UDHO_LOGGING_PARAMS_H
#define UDHO_LOGGING_PARAMS_H

#include <cstdint>
#include <chrono>
#include <string>
#include <optional>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/verb.hpp>
#include <udho/session/defs.h>
#include <boost/asio/ip/tcp.hpp>
#include <udho/utils/misc.h>

#define UDHO_LOG_PARAM(Name, Type)                                          \
struct Name: udho::hazo::element<Name , Type> {                             \
        using base_element = udho::hazo::element<Name , Type>;              \
        using base_element::base_element;                                   \
        inline Name(): base_element() {}                                    \
        using base_element::operator=;                                      \
        inline static constexpr auto key() {                                \
            using namespace udho::hazo::string::literals;                   \
            return #Name##_h;                                               \
    }                                                                       \
}

namespace udho {
namespace logging {

/** @addtogroup DoxyG_logging_params
 *  @{
 */

/**
 * @brief Log severity levels.
 */
enum class severity : std::uint32_t {
    trace   = 0,
    debug   = 1,
    info    = 2,
    warning = 3,
    error   = 4,
    fatal   = 5
};

inline std::ostream& operator<<(std::ostream& stream, udho::logging::severity severity){
    switch(severity) {
        case udho::logging::severity::trace:
            stream << "trace";
            break;
        case udho::logging::severity::debug:
            stream << "debug";
            break;
        case udho::logging::severity::info:
            stream << "info";
            break;
        case udho::logging::severity::warning:
            stream << "warning";
            break;
        case udho::logging::severity::error:
            stream << "error";
            break;
        case udho::logging::severity::fatal:
            stream << "fatal";
            break;
        default:
            stream << "UNKNOWN";
    }
    return stream;
}

/** @} */

/**
 * @brief Compile‑time keys for log message fields.
 *
 * Each UDHO_LOG_PARAM defines a type that can be used as a key in the message
 * container. Example: `msg[params::local_id::val] = 42;`
 */
namespace params {

/** @addtogroup DoxyG_logging_params
 *  @{
 */

UDHO_LOG_PARAM(local_id,        std::uint64_t);   // process-local monotonically increasing id
UDHO_LOG_PARAM(timestamp,       std::chrono::system_clock::time_point);    // unix epoch nanoseconds
UDHO_LOG_PARAM(severity,        udho::logging::severity);
UDHO_LOG_PARAM(thread,          std::size_t);
UDHO_LOG_PARAM(process,         std::uint32_t);   // normalized pid
UDHO_LOG_PARAM(subsystem,       std::string);     // http / router / session / upload / app / ...
UDHO_LOG_PARAM(message,         std::string);     // final log text
UDHO_LOG_PARAM(file,            std::string);     // __FILE__
UDHO_LOG_PARAM(function,        std::string);     // __func__
UDHO_LOG_PARAM(line,            std::uint64_t);   // __LINE__

UDHO_LOG_PARAM(request_id,      std::optional<std::string>);
UDHO_LOG_PARAM(flow_id,         std::optional<std::size_t>);
UDHO_LOG_PARAM(socket_id,       std::optional<udho::utils::misc::portable_socket_id>);
UDHO_LOG_PARAM(session_id,      std::optional<udho::session::id>);
UDHO_LOG_PARAM(user_id,         std::optional<std::string>);

UDHO_LOG_PARAM(client,          std::optional<boost::asio::ip::address>);
UDHO_LOG_PARAM(host,            std::optional<std::string>);
UDHO_LOG_PARAM(method,          std::optional<boost::beast::http::verb>);
UDHO_LOG_PARAM(uri,             std::optional<std::string>);
UDHO_LOG_PARAM(route,           std::optional<std::string>);
UDHO_LOG_PARAM(query,           std::optional<std::string>);
UDHO_LOG_PARAM(agent,           std::optional<std::string>);

UDHO_LOG_PARAM(status_code,     std::optional<std::uint32_t>);
UDHO_LOG_PARAM(bytes_sent,      std::optional<std::uint64_t>);
UDHO_LOG_PARAM(latency,         std::optional<std::chrono::nanoseconds>);
UDHO_LOG_PARAM(retry_count,     std::optional<std::uint32_t>);

/** @} */

}

/**
 * @brief String literals used as Boost.Log attribute names.
 *
 * These correspond one‑to‑one with the keys in @ref params.
 */
namespace names {

/** @addtogroup DoxyG_logging_params
 *  @{
 */

inline constexpr char local_id[]      = "LineID";
inline constexpr char timestamp[]     = "TimeStamp";
inline constexpr char severity[]      = "Severity";
inline constexpr char thread[]        = "ThreadID";
inline constexpr char process[]       = "ProcessID";
inline constexpr char subsystem[]     = "Subsystem";
inline constexpr char message[]       = "Message";
inline constexpr char file[]          = "file";
inline constexpr char function[]      = "function";
inline constexpr char line[]          = "line";

inline constexpr char request_id[]    = "Request";
inline constexpr char flow_id[]       = "Flow";
inline constexpr char socket_id[]     = "Socket";
inline constexpr char session_id[]    = "Session";
inline constexpr char user_id[]       = "User";

inline constexpr char client[]        = "Client";
inline constexpr char host[]          = "Host";
inline constexpr char method[]        = "Method";
inline constexpr char uri[]           = "URI";
inline constexpr char route[]         = "Route";
inline constexpr char query[]         = "Query";
inline constexpr char agent[]         = "Agent";

inline constexpr char status_code[]   = "Status";
inline constexpr char bytes_sent[]    = "BytesSent";
inline constexpr char latency[]       = "Latency";
inline constexpr char retry_count[]   = "Retry";

/** @} */

}

}
}

#endif // UDHO_LOGGING_PARAMS_H
