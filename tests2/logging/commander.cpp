#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include <iostream>

#include <udho/logging/commander.h>
#include <udho/logging/consumer.h>
#include <udho/logging/ipc_queue.h>
#include <udho/logging/macros.h>
#include <udho/logging/producer.h>
#include <udho/logging/protocol.h>

#include "helpers.h"

namespace {

std::string subsystem_equals(std::string value) {
    return udho::utils::format("%{}%", udho::logging::names::subsystem) + " = \"" + value + "\"";
}

} // namespace

TEST_CASE("Commander admin commands and consumer control", "[logging][commander]") {
    SECTION("filter commands") {

        SECTION("filter_set applies the configured filter and filter_show reports it") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            const std::string filter_text = subsystem_equals("commander-allowed");

            auto set_result = commander.filter_set(filter_text);
            REQUIRE(set_result);
            REQUIRE(set_result.message.find("Applied") != std::string::npos);

            auto show_result = commander.filter_show();
            REQUIRE(show_result);
            REQUIRE(show_result.message == filter_text);

            REQUIRE(UDHO_LOG_INFO("commander-allowed", "allowed after filter_set"));
            REQUIRE(UDHO_LOG_INFO("commander-blocked", "blocked after filter_set"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-allowed|allowed after filter_set|") != std::string::npos;
            }));

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.find("commander-allowed|allowed after filter_set|") != std::string::npos);
            REQUIRE(content.find("commander-blocked|blocked after filter_set|") == std::string::npos);

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("filter_unset restores delivery after a restrictive filter") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            const std::string filter_text = subsystem_equals("commander-only");

            auto set_result = commander.filter_set(filter_text);
            REQUIRE(set_result);

            REQUIRE(UDHO_LOG_INFO("commander-other", "blocked before unset"));
            REQUIRE(UDHO_LOG_INFO("commander-only", "allowed before unset"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-only|allowed before unset|") != std::string::npos;
            }));

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                REQUIRE(content.find("commander-other|blocked before unset|") == std::string::npos);
            }

            auto unset_result = commander.filter_unset();
            REQUIRE(unset_result);
            REQUIRE(unset_result.message.find("Removed") != std::string::npos);

            auto show_result = commander.filter_show();
            REQUIRE(show_result.message.find("No filter") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-other", "allowed after unset"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-other|allowed after unset|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("filter_show without configured filter reports unset state and does not alter delivery") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            auto show_result = commander.filter_show();
            REQUIRE(show_result.message.find("No filter") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-show", "before filter_show"));
            REQUIRE(UDHO_LOG_INFO("commander-show", "after filter_show"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-show|before filter_show|") != std::string::npos &&
                       content.find("commander-show|after filter_show|")  != std::string::npos;
            }));

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "commander-show|before filter_show|") == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "commander-show|after filter_show|") == 1);

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

    }

    SECTION("temporary enable command") {

        SECTION("temporary_enable disables delivery and later re-enables it") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            REQUIRE(UDHO_LOG_INFO("commander-enable", "before disable"));
            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-enable|before disable|") != std::string::npos;
            }));

            auto disable_result = commander.temporary_disable();
            REQUIRE(disable_result);
            REQUIRE(disable_result.message.find("disabled") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-enable", "during disable"));

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                REQUIRE(content.find("commander-enable|during disable|") == std::string::npos);
            }

            std::uint8_t enable = 1;
            auto enable_result = commander.temporary_enable();
            REQUIRE(enable_result);
            REQUIRE(enable_result.message.find("enabled") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-enable", "after re-enable"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-enable|after re-enable|") != std::string::npos;
            }));

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.find("commander-enable|before disable|") != std::string::npos);
            REQUIRE(content.find("commander-enable|during disable|") == std::string::npos);
            REQUIRE(content.find("commander-enable|after re-enable|") != std::string::npos);

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("temporary_enable rejects malformed payload and delivery continues") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            const std::array<std::uint8_t, 2> bad_payload{{0, 1}};
            auto bad_result = udho::logging::detail::sync_write_helper<udho::logging::commander::protocol_type>::write(
                socket_path,
                udho::logging::protocol::command::temporary_enable,
                bad_payload.data(),
                bad_payload.size(),
                commander.max_payload_size()
            );
            REQUIRE_FALSE(bad_result);
            REQUIRE(bad_result.message.find("1 byte") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-enable", "after malformed temporary_enable"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-enable|after malformed temporary_enable|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

    }

    SECTION("admin command failures do not break consumption") {

        SECTION("invalid filter_set command reports failure and later messages are still delivered") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            auto bad_result = commander.filter_set("this is not a valid boost log filter [[");
            REQUIRE_FALSE(bad_result);
            REQUIRE(bad_result.message.find("filter") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-invalid-filter", "delivery after invalid filter"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-invalid-filter|delivery after invalid filter|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("unknown command reports failure and later messages are still delivered") {
            auto queue_name  = udho::logging::test_helpers::unique_queue_name();
            auto socket_path = udho::logging::test_helpers::unique_socket_path();
            auto log_path    = udho::logging::test_helpers::unique_log_path();
            auto queue       = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            udho::logging::commander commander(socket_path);

            auto unknown_result = udho::logging::detail::sync_write_helper<udho::logging::commander::protocol_type>::write(
                socket_path,
                static_cast<udho::logging::protocol::command>(999u),
                nullptr,
                0,
                commander.max_payload_size()
            );
            REQUIRE_FALSE(unknown_result);
            REQUIRE(unknown_result.message.find("Unknown command") != std::string::npos);

            REQUIRE(UDHO_LOG_INFO("commander-unknown", "delivery after unknown command"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("commander-unknown|delivery after unknown command|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

    }
}