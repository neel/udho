#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include <boost/log/core.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <udho/logging/commander.h>
#include <udho/logging/formatter.h>
#include <udho/logging/macros.h>
#include <udho/logging/setup.h>

#include "helpers.h"

namespace {

struct setup_test_file_sink {
    static std::string& path() {
        static std::string value;
        return value;
    }

    static auto apply() {
        auto sink = boost::log::add_file_log(
            boost::log::keywords::file_name  = path(),
            boost::log::keywords::open_mode  = std::ios_base::out | std::ios_base::trunc,
            boost::log::keywords::auto_flush = true
        );
        sink->set_formatter(udho::logging::formatter{});
        return sink;
    }
};

using test_setup = udho::logging::setup<setup_test_file_sink>;

} // namespace

TEST_CASE("Setup initializes and controls the logging subsystem", "[logging][setup]") {
    auto cleanup = [] (const std::string& queue_name, const std::string& socket_path, const std::string& log_path) {
        test_setup::stop();
        udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        std::error_code ec;
        std::filesystem::remove(socket_path, ec);
        std::filesystem::remove(log_path, ec);
        std::filesystem::remove("consumer.stdout", ec);
    };

    SECTION("lifecycle") {

        SECTION("apply starts the logger child and running reports true") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid > 0);
            REQUIRE(test_setup::running());
            REQUIRE(std::filesystem::exists(socket_path));

            cleanup(queue_name, socket_path, log_path);

            REQUIRE_FALSE(test_setup::running());
        }

        SECTION("apply is idempotent while logger is already running") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid1 = test_setup::apply(queue_name.c_str(), socket_path.c_str());
            auto pid2 = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid1 > 0);
            REQUIRE(pid2 == pid1);
            REQUIRE(test_setup::running());

            cleanup(queue_name, socket_path, log_path);
        }

        SECTION("stop is safe when logger is not running") {
            REQUIRE_NOTHROW(test_setup::stop());
            REQUIRE_FALSE(test_setup::running());
        }

        SECTION("stop terminates the logger and deactivates producer logging") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid > 0);
            REQUIRE(test_setup::running());

            test_setup::stop();

            REQUIRE_FALSE(test_setup::running());
            REQUIRE_FALSE(UDHO_LOG_INFO("setup-test", "after stop should be rejected"));

            cleanup(queue_name, socket_path, log_path);
        }

        SECTION("logger can be started again after stop") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid1 = test_setup::apply(queue_name.c_str(), socket_path.c_str());
            REQUIRE(pid1 > 0);
            REQUIRE(test_setup::running());

            test_setup::stop();
            REQUIRE_FALSE(test_setup::running());

            cleanup(queue_name, socket_path, log_path);

            auto queue_name2  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path2 = udho::logging::test_helpers::unique_socket_path();
            auto log_path2    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path2;

            auto pid2 = test_setup::apply(queue_name2.c_str(), socket_path2.c_str());
            REQUIRE(pid2 > 0);
            REQUIRE(test_setup::running());
            REQUIRE(pid2 != pid1);

            REQUIRE(UDHO_LOG_INFO("setup-restart", "after restart"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path2);
                return content.find("setup-restart") != std::string::npos &&
                       content.find("after restart") != std::string::npos;
            }, std::chrono::seconds(3), std::chrono::milliseconds(20)));

            test_setup::stop();
            REQUIRE_FALSE(test_setup::running());

            std::error_code ec;
            std::filesystem::remove(socket_path2, ec);
            std::filesystem::remove(log_path2, ec);

            cleanup(queue_name, socket_path, log_path);
        }

    }

    SECTION("end-to-end logging") {

        SECTION("setup enables parent-side log messages to reach child consumer sink") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid > 0);
            REQUIRE(test_setup::running());

            REQUIRE(UDHO_LOG_INFO("setup-test", "first setup message"));
            REQUIRE(UDHO_LOG_ERROR("setup-test", "second setup message"));

            bool success = udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);

                bool set_test_found             = (content.find("setup-test") != std::string::npos);
                bool first_setup_message_found  = (content.find("first setup message") != std::string::npos);
                bool second_setup_message_found = (content.find("second setup message") != std::string::npos);

                return set_test_found && first_setup_message_found && second_setup_message_found;

            }, std::chrono::seconds(3), std::chrono::milliseconds(20));

            REQUIRE(success);

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.find("first setup message") != std::string::npos);
            REQUIRE(content.find("second setup message") != std::string::npos);

            cleanup(queue_name, socket_path, log_path);
        }

        SECTION("messages queued through setup are delivered exactly once") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid > 0);
            REQUIRE(test_setup::running());

            const std::size_t count = udho::logging::producer::max_messages() *2;
            std::size_t total_sent = 0;
            for (std::size_t i = 0; i < count; ++i) {
                total_sent += UDHO_LOG_INFO("setup-seq", "msg-" + std::to_string(i)+";");
            }

            bool expected_contents_found = false;
            std::thread wait_for_completion([&]{
                expected_contents_found = udho::logging::test_helpers::wait_until([&] {
                    const auto content = udho::logging::test_helpers::read_file(log_path);
                    return content.find("msg-0;") != std::string::npos &&
                           content.find(udho::utils::format("msg-{};", count-1)) != std::string::npos;
                }, std::chrono::seconds(3), std::chrono::milliseconds(20));
            });

            test_setup::stop();

            wait_for_completion.join();
            REQUIRE(expected_contents_found);
            REQUIRE(udho::logging::producer::backlog() == 0);

            const auto content = udho::logging::test_helpers::read_file(log_path);
            for (std::size_t i = 0; i < count; ++i) {
                const auto needle = std::string("msg-") + std::to_string(i)+ ";";
                CAPTURE(needle);
                CHECK(udho::logging::test_helpers::count_substring(content, needle) == 1);
            }

            cleanup(queue_name, socket_path, log_path);
        }

    }

    SECTION("admin transport via setup-managed child") {

        SECTION("commander can reach the setup-managed consumer and filter commands have effect") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid > 0);
            REQUIRE(test_setup::running());
            REQUIRE(std::filesystem::exists(socket_path));

            udho::logging::commander commander(socket_path);

            const std::string filter_text = udho::utils::format("%{}%", udho::logging::names::subsystem) + " = \"setup-allowed\"";

            auto set_result = commander.filter_set(filter_text);
            REQUIRE(set_result);
            REQUIRE(set_result.message.find("Applied") != std::string::npos);

            auto show_result = commander.filter_show();
            REQUIRE(show_result);
            REQUIRE(show_result.message == filter_text);

            REQUIRE(UDHO_LOG_INFO("setup-allowed", "allowed after setup filter"));
            REQUIRE(UDHO_LOG_INFO("setup-blocked", "blocked after setup filter"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("setup-allowed") != std::string::npos &&
                       content.find("allowed after setup filter") != std::string::npos;
            }, std::chrono::seconds(3), std::chrono::milliseconds(20)));

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                REQUIRE(content.find("setup-blocked") == std::string::npos);
                REQUIRE(content.find("blocked after setup filter") == std::string::npos);
            }

            auto unset_result = commander.filter_unset();
            REQUIRE(unset_result);

            REQUIRE(UDHO_LOG_INFO("setup-blocked", "allowed after setup unset"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("allowed after setup unset") != std::string::npos;
            }, std::chrono::seconds(3), std::chrono::milliseconds(20)));

            cleanup(queue_name, socket_path, log_path);
        }

        SECTION("temporary_enable over setup-managed socket suppresses and restores delivery") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();

            setup_test_file_sink::path() = log_path;

            auto pid = test_setup::apply(queue_name.c_str(), socket_path.c_str());

            REQUIRE(pid > 0);
            REQUIRE(test_setup::running());

            udho::logging::commander commander(socket_path);

            REQUIRE(UDHO_LOG_INFO("setup-enable", "before disable"));
            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("before disable") != std::string::npos;
            }, std::chrono::seconds(3), std::chrono::milliseconds(20)));

            auto disable_result = commander.temporary_enable(false);
            REQUIRE(disable_result);
            REQUIRE(disable_result.message.find("disabled") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("setup-enable", "during disable"));
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                REQUIRE(content.find("during disable") == std::string::npos);
            }

            auto enable_result = commander.temporary_enable(true);
            REQUIRE(enable_result);
            REQUIRE(enable_result.message.find("enabled") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("setup-enable", "after re-enable"));
            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("after re-enable") != std::string::npos;
            }, std::chrono::seconds(3), std::chrono::milliseconds(20)));

            cleanup(queue_name, socket_path, log_path);
        }

    }

}