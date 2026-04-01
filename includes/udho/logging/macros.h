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
    message[T::val] = x;
}

template<typename T, typename... Ts>
inline void populate_record(udho::logging::message& message, T&& x, Ts&&... xs){
    message[T::val] = x;
    detail::populate_record(message, std::forward<Ts>(xs)...);
}

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

template<typename... Extra>
bool log(udho::logging::severity severity, const std::string& subsystem, const std::string& message, const char* file, std::size_t line, const char* func, Extra&&... extra) {
    auto record = detail::make_record(severity, subsystem, message, file, line, func, std::forward<Extra>(extra)...);
    return producer::log(record);
}

}
}

#define UDHO_LOG(severity, subsystem, message, ...)          \
            ::udho::logging::log(                            \
                (severity),                                  \
                (subsystem),                                 \
                (message),                                   \
                __FILE__,                                    \
                __LINE__,                                    \
                BOOST_CURRENT_FUNCTION,                      \
                ##__VA_ARGS__                                \
            )

#define UDHO_LOG_FATAL(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::fatal,    subsystem, message, ##__VA_ARGS__)
#define UDHO_LOG_ERROR(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::error,    subsystem, message, ##__VA_ARGS__)
#define UDHO_LOG_WARNING(subsystem, message, ...)   UDHO_LOG(udho::logging::severity::warning,  subsystem, message, ##__VA_ARGS__)
#define UDHO_LOG_INFO(subsystem, message, ...)      UDHO_LOG(udho::logging::severity::info,     subsystem, message, ##__VA_ARGS__)
#define UDHO_LOG_DEBUG(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::debug,    subsystem, message, ##__VA_ARGS__)
#define UDHO_LOG_TRACE(subsystem, message, ...)     UDHO_LOG(udho::logging::severity::trace,    subsystem, message, ##__VA_ARGS__)

#endif // UDHO_LOGGING_MACROS_H
