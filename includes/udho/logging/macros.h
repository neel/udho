#ifndef UDHO_LOGGING_MACROS_H
#define UDHO_LOGGING_MACROS_H

#include <thread>
#include <atomic>
#include <udho/logging/message.h>
#include <udho/logging/producer.h>
#include <boost/current_function.hpp>

namespace udho {
namespace logging {
namespace detail {

inline std::uint64_t next_local_id() {
    static std::atomic<std::uint64_t> counter{0};
    return ++counter;
}

inline std::uint64_t current_thread_id() {
    static std::hash<std::thread::id> hasher;
    return hasher(std::this_thread::get_id());
}

inline void populate_record(udho::logging::message& message){}

template<typename T>
inline void populate_record(udho::logging::message& message, T&& x){
    message[T::val] = std::forward<T>(x);
}

template<typename T, typename... Ts>
inline void populate_record(udho::logging::message& message, T&& x, Ts&&... xs){
    message[T::val] = std::forward<T>(x);
    detail::populate_record(message, std::forward<Ts>(xs)...);
}

/**
 * @brief Build a fully populated transported log record.
 * @tparam Extra Optional parameter wrapper types appended to the record.
 * @param severity Log severity.
 * @param subsystem Logical subsystem name.
 * @param message Final log message text.
 * @param file Source file path, typically @c __FILE__.
 * @param line Source line number, typically @c __LINE__.
 * @param func Source function name, typically @c __func__.
 * @param extra Additional typed optional fields to append to the record.
 * @return Fully constructed transported log record.
 *
 * The function always populates the mandatory fields:
 * - local record ID
 * - timestamp
 * - severity
 * - thread identifier
 * - process ID
 * - subsystem
 * - message text
 * - source file
 * - source function
 * - source line
 *
 * Additional typed fields are appended through @ref populate_record.
 */
template<typename... Extra>
udho::logging::message make_record(udho::logging::severity severity, const std::string& subsystem, const std::string& message, const char* file, std::size_t line, const char* func, Extra&&... extra) {
    auto record = udho::logging::message(
        udho::logging::params::local_id     (detail::next_local_id()),
        udho::logging::params::timestamp    (std::chrono::system_clock::now()),
        udho::logging::params::severity     (severity),
        udho::logging::params::thread       (detail::current_thread_id()),
        udho::logging::params::process      (::getpid()),
        udho::logging::params::subsystem    (subsystem),
        udho::logging::params::message      (message),
        udho::logging::params::file         (file),
        udho::logging::params::function     (func),
        udho::logging::params::line         (line)
    );

    detail::populate_record(record, std::forward<Extra>(extra)...);

    return record;
}

}

/**
 * @brief Create and submit a log record to the producer.
 * @tparam Extra Optional parameter wrapper types appended to the record.
 * @param severity Log severity.
 * @param subsystem Logical subsystem name.
 * @param message Final log message text.
 * @param file Source file path, typically @c __FILE__.
 * @param line Source line number, typically @c __LINE__.
 * @param func Source function name, typically @c __func__.
 * @param extra Additional typed optional fields to append to the record.
 * @return Number of messages transferred to the IPC queue during this call.
 *
 * This is the function behind the public @c UDHO_LOG* macros.
 *
 * @note The returned count includes backlog messages drained during this call
 *       and may or may not include the current record itself.
 *
 * @note A return value of 0 does not necessarily mean the current record was
 *       dropped. It may have been accepted into the producer backlog because
 *       the IPC queue was full.
 */
template<typename... Extra>
std::size_t log(udho::logging::severity severity, const std::string& subsystem, const std::string& message, const char* file, std::size_t line, const char* func, Extra&&... extra) {
    auto record = detail::make_record(severity, subsystem, message, file, line, func, std::forward<Extra>(extra)...);
    return producer::log(record);
}

}
}


/**
 * @brief Emit a log record with explicit severity.
 * @param severity Severity value from @ref udho::logging::severity.
 * @param subsystem Logical subsystem name.
 * @param message Final log message text.
 * @param ... Optional typed extra fields such as @c params::request_id(...).
 * @return Number of messages transferred to the IPC queue during this call.
 *
 * This macro captures source location automatically through @c __FILE__,
 * @c __LINE__, and @c __func__ and forwards the call to
 * @ref udho::logging::log.
 */
#define UDHO_LOG(severity, subsystem, message, ...)          \
            ::udho::logging::log(                            \
                (severity),                                  \
                (subsystem),                                 \
                (message),                                   \
                __FILE__,                                    \
                __LINE__,                                    \
                __func__,                                    \
                ##__VA_ARGS__                                \
            )

/** @brief Emit a fatal-severity log record. */
#define UDHO_LOG_FATAL(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::fatal,    subsystem, message, ##__VA_ARGS__)
/** @brief Emit an error-severity log record. */
#define UDHO_LOG_ERROR(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::error,    subsystem, message, ##__VA_ARGS__)
/** @brief Emit a warning-severity log record. */
#define UDHO_LOG_WARNING(subsystem, message, ...)   UDHO_LOG(udho::logging::severity::warning,  subsystem, message, ##__VA_ARGS__)
/** @brief Emit an info-severity log record. */
#define UDHO_LOG_INFO(subsystem, message, ...)      UDHO_LOG(udho::logging::severity::info,     subsystem, message, ##__VA_ARGS__)
/** @brief Emit a debug-severity log record. */
#define UDHO_LOG_DEBUG(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::debug,    subsystem, message, ##__VA_ARGS__)
/** @brief Emit a trace-severity log record. */
#define UDHO_LOG_TRACE(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::trace,    subsystem, message, ##__VA_ARGS__)

#endif // UDHO_LOGGING_MACROS_H
