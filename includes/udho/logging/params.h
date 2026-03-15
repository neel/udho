#ifndef UDHO_LOGGING_PARAMS_H
#define UDHO_LOGGING_PARAMS_H

#include <cstdint>
#include <chrono>
#include <string>
#include <optional>
#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>

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

enum class severity : std::uint32_t {
    trace   = 0,
    debug   = 1,
    info    = 2,
    warning = 3,
    error   = 4,
    fatal   = 5
};

namespace params {

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
UDHO_LOG_PARAM(session_id,      std::optional<std::string>);
UDHO_LOG_PARAM(user_id,         std::optional<std::string>);

UDHO_LOG_PARAM(client_ip,       std::optional<std::string>);
UDHO_LOG_PARAM(host,            std::optional<std::string>);
UDHO_LOG_PARAM(http_method,     std::optional<std::string>);
UDHO_LOG_PARAM(uri,             std::optional<std::string>);
UDHO_LOG_PARAM(route,           std::optional<std::string>);
UDHO_LOG_PARAM(query,           std::optional<std::string>);
UDHO_LOG_PARAM(user_agent,      std::optional<std::string>);

UDHO_LOG_PARAM(status_code,     std::optional<std::uint32_t>);
UDHO_LOG_PARAM(bytes_sent,      std::optional<std::uint64_t>);
UDHO_LOG_PARAM(latency,         std::optional<std::chrono::nanoseconds>);
UDHO_LOG_PARAM(retry_count,     std::optional<std::uint32_t>);

UDHO_LOG_PARAM(error_code,      std::optional<std::int64_t>);
UDHO_LOG_PARAM(error_message,   std::optional<std::string>);

}

}
}

#endif // UDHO_LOGGING_PARAMS_H
