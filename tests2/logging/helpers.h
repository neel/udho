#ifndef UDHO_TESTS_LOGGING_HELPERS_H
#define UDHO_TESTS_LOGGING_HELPERS_H

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include <udho/logging/macros.h>

namespace udho::logging::test_helpers {

inline std::string unique_token(const std::string& prefix) {
    static std::atomic<std::uint64_t> counter{0};
    std::ostringstream oss;
    oss << prefix << "_" << ::getpid() << "_" << ++counter;
    return oss.str();
}

inline std::string unique_queue_name(const std::string& prefix = "udho_logging_test_queue") {
    return unique_token(prefix);
}

inline std::filesystem::path unique_socket_path(const std::string& prefix = "udho_logging_test_socket") {
    return std::filesystem::temp_directory_path() / (unique_token(prefix) + ".sock");
}

inline std::filesystem::path unique_log_path(const std::string& prefix = "udho_logging_test_log") {
    return std::filesystem::temp_directory_path() / (unique_token(prefix) + ".log");
}

inline std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline bool file_contains(const std::filesystem::path& path, const std::string& needle) {
    return read_file(path).find(needle) != std::string::npos;
}

template <typename Predicate>
inline bool wait_until(Predicate&& predicate, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000), std::chrono::milliseconds poll = std::chrono::milliseconds(10)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(poll);
    }
    return predicate();
}

struct boost_log_guard {
    boost_log_guard() {
        auto core = boost::log::core::get();
        core->remove_all_sinks();
        core->reset_filter();
    }

    ~boost_log_guard() {
        auto core = boost::log::core::get();
        core->flush();
        core->remove_all_sinks();
        core->reset_filter();
    }
};

inline void configure_file_sink(const std::filesystem::path& path) {
    namespace keywords = boost::log::keywords;
    namespace expr = boost::log::expressions;

    boost::log::add_file_log(
        keywords::file_name = path.string(),
        keywords::open_mode = std::ios_base::out | std::ios_base::trunc,
        keywords::auto_flush = true,
        keywords::format = (
            expr::stream << expr::attr<std::string>("Subsystem") << "|" << expr::attr<std::string>("Message") << "|" << expr::attr<std::underlying_type_t<udho::logging::severity>>("Severity")
        )
    );
}

struct producer_state_guard {
    producer_state_guard() {
        udho::logging::producer::deactivate();
        udho::logging::producer::reset_filter();
        udho::logging::producer::threshold(udho::logging::severity::trace);
    }

    ~producer_state_guard() {
        udho::logging::producer::deactivate();
        udho::logging::producer::reset_filter();
        udho::logging::producer::threshold(udho::logging::severity::trace);
    }
};

} // namespace udho::logging::test_helpers

#endif // UDHO_TESTS_LOGGING_HELPERS_H
