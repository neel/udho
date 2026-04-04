#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <thread>

#include <udho/logging/consumer.h>
#include <udho/logging/ipc_queue.h>
#include <udho/logging/macros.h>
#include <udho/logging/producer.h>

#include "helpers.h"

using namespace udho::logging::params;

TEST_CASE("Consumer Initiation and consumption", "[logging][consumer]") {

    SECTION("lifecycle") {

        SECTION("Consumer stops cleanly while idle") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());

            std::thread worker([&] { consumer.consume(should_stop); });

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                should_stop = true;
                return true;
            }, std::chrono::milliseconds(20), std::chrono::milliseconds(1)));

            worker.join();
            SUCCEED();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer remains idle when producer is inactive and no messages are queued") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            std::this_thread::sleep_for(std::chrono::milliseconds(150));

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.empty());

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

    }

    SECTION("delivery") {

        SECTION("Consumer drains the ipc queue and delivers to Boost.Log") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            std::thread worker([&] {
                udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
                consumer.consume(should_stop);
            });

            REQUIRE(UDHO_LOG_INFO("consumer-test", "first delivered message"));
            REQUIRE(UDHO_LOG_ERROR("consumer-test", "second delivered message"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-test|first delivered message|") != std::string::npos &&
                       content.find("consumer-test|second delivered message|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.find("consumer-test|first delivered message|") != std::string::npos);
            REQUIRE(content.find("consumer-test|second delivered message|") != std::string::npos);

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer can remain idle and later consume produced messages") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            REQUIRE(UDHO_LOG_INFO("consumer-idle", "message after idle"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-idle|message after idle|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer drains messages that were queued before the consumer starts") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            udho::logging::producer::activate(queue_name.c_str());

            REQUIRE(UDHO_LOG_INFO("consumer-backlog", "queued before start 1"));
            REQUIRE(UDHO_LOG_INFO("consumer-backlog", "queued before start 2"));

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] {
                consumer.consume(should_stop);
            });

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-backlog|queued before start 1|") != std::string::npos &&
                       content.find("consumer-backlog|queued before start 2|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer remains stable when producer is deactivated after earlier traffic") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            REQUIRE(UDHO_LOG_INFO("consumer-deactivate", "before deactivate"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-deactivate|before deactivate|") != std::string::npos;
            }));

            udho::logging::producer::deactivate();
            REQUIRE_FALSE(UDHO_LOG_INFO("consumer-deactivate", "after deactivate"));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.find("consumer-deactivate|before deactivate|") != std::string::npos);
            REQUIRE(content.find("consumer-deactivate|after deactivate|") == std::string::npos);

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer can start before producer activation and later consume messages") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            udho::logging::producer::activate(queue_name.c_str());
            REQUIRE(UDHO_LOG_INFO("consumer-late-activate", "message after activate"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-late-activate|message after activate|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer drains backlog first and later consumes live messages") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            REQUIRE(UDHO_LOG_INFO("consumer-mixed", "backlog-1"));
            REQUIRE(UDHO_LOG_INFO("consumer-mixed", "backlog-2"));

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-mixed|backlog-1|") != std::string::npos &&
                       content.find("consumer-mixed|backlog-2|") != std::string::npos;
            }));

            REQUIRE(UDHO_LOG_INFO("consumer-mixed", "live-1"));
            REQUIRE(UDHO_LOG_INFO("consumer-mixed", "live-2"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-mixed|live-1|") != std::string::npos &&
                       content.find("consumer-mixed|live-2|") != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-mixed|backlog-1|") == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-mixed|backlog-2|") == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-mixed|live-1|") == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-mixed|live-2|") == 1);

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }
    }

    SECTION("ordering & completeness") {
        SECTION("Consumer preserves message order for sequential producer messages ensuring no reperation") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            REQUIRE(UDHO_LOG_INFO("consumer-order", "first"));
            REQUIRE(UDHO_LOG_INFO("consumer-order", "second"));
            REQUIRE(UDHO_LOG_INFO("consumer-order", "third"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-order|first|")  != std::string::npos &&
                       content.find("consumer-order|second|") != std::string::npos &&
                       content.find("consumer-order|third|")  != std::string::npos;
            }));

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);

            const auto p1 = content.find("consumer-order|first|");
            const auto p2 = content.find("consumer-order|second|");
            const auto p3 = content.find("consumer-order|third|");

            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-order|first|")  == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-order|second|") == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, "consumer-order|third|") == 1);

            REQUIRE(p1 != std::string::npos);
            REQUIRE(p2 != std::string::npos);
            REQUIRE(p3 != std::string::npos);
            REQUIRE(p1 < p2);
            REQUIRE(p2 < p3);

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

        SECTION("Consumer eventually drains more than one batch of queued messages ensuring no reperation") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);

            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            constexpr std::size_t count = udho::logging::producer::max_messages() *2;
            std::size_t messages_delivered = 0;
            for (std::size_t i = 0; i < count; ++i) {
                messages_delivered += UDHO_LOG_INFO("consumer-batch", "msg-" + std::to_string(i));
            }

            bool expected_contents_found = false;
            std::thread wait_for_completion([log_path, &expected_contents_found](){
                expected_contents_found = udho::logging::test_helpers::wait_until([&] {
                    const auto content = udho::logging::test_helpers::read_file(log_path);
                    return content.find(udho::utils::format("consumer-batch|msg-{}|", 0)) != std::string::npos &&
                           content.find(udho::utils::format("consumer-batch|msg-{}|", count/2)) != std::string::npos &&
                           content.find(udho::utils::format("consumer-batch|msg-{}|", count-1)) != std::string::npos;
                }, std::chrono::seconds(10), std::chrono::milliseconds(10));
            });

            udho::logging::producer::deactivate();

            wait_for_completion.join();
            REQUIRE(expected_contents_found);
            REQUIRE(udho::logging::producer::backlog() == 0);

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);
            REQUIRE(content.find(udho::utils::format("consumer-batch|msg-{}|", 0))  != std::string::npos);
            REQUIRE(content.find(udho::utils::format("consumer-batch|msg-{}|", count/2)) != std::string::npos);
            REQUIRE(content.find(udho::utils::format("consumer-batch|msg-{}|", count-1)) != std::string::npos);

            REQUIRE(udho::logging::test_helpers::count_substring(content, udho::utils::format("consumer-batch|msg-{}|", 0))  == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, udho::utils::format("consumer-batch|msg-{}|", count/2)) == 1);
            REQUIRE(udho::logging::test_helpers::count_substring(content, udho::utils::format("consumer-batch|msg-{}|", count-1)) == 1);

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }
    }

    SECTION("Concurrency") {

        SECTION("Consumer drains and delivers messages produced concurrently") {
            auto queue_name     = udho::logging::test_helpers::unique_queue_name();
            auto socket_path    = udho::logging::test_helpers::unique_socket_path();
            auto log_path       = udho::logging::test_helpers::unique_log_path();
            auto queue          = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::test_helpers::boost_log_guard log_guard;
            udho::logging::test_helpers::producer_state_guard producer_guard;

            udho::logging::test_helpers::configure_file_sink(log_path);
            udho::logging::producer::activate(queue_name.c_str());

            std::atomic_bool should_stop{false};
            udho::logging::consumer consumer(socket_path.c_str(), queue_name.c_str());
            std::thread worker([&] { consumer.consume(should_stop); });

            constexpr std::size_t thread_count = 2;
            constexpr std::size_t per_thread = queue.max_messages/2;

            std::vector<std::thread> threads;
            std::atomic<std::size_t> messages_sent = 0;
            for (std::size_t t = 0; t < thread_count; ++t) {
                threads.emplace_back([t, &messages_sent] {
                    for (std::size_t i = 0; i < per_thread; ++i) {
                        messages_sent += UDHO_LOG_INFO("consumer-concurrent", "t" + std::to_string(t) + "-m" + std::to_string(i));
                    }
                });
            }
            for (auto& th : threads) th.join();
            REQUIRE(messages_sent == per_thread * thread_count);

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                const auto content = udho::logging::test_helpers::read_file(log_path);
                return content.find("consumer-concurrent|t0-m0|") != std::string::npos &&
                       content.find(udho::utils::format("consumer-concurrent|t{}-m{}|", thread_count-1, per_thread-1)) != std::string::npos;
            }, std::chrono::seconds(3), std::chrono::milliseconds(10)));

            should_stop = true;
            worker.join();

            const auto content = udho::logging::test_helpers::read_file(log_path);
            for (std::size_t t = 0; t < thread_count; ++t) {
                for (std::size_t i = 0; i < per_thread; ++i) {
                    const auto needle = std::string("consumer-concurrent|t") + std::to_string(t) + "-m" + std::to_string(i) + "|";
                    REQUIRE(udho::logging::test_helpers::count_substring(content, needle) == 1);
                }
            }

            udho::logging::detail::ipc_queue::remove(queue_name.c_str());
        }

    }
}
