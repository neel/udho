#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <future>

#include <udho/logging/ipc_queue.h>
#include <udho/logging/macros.h>
#include <udho/logging/producer.h>

#include "helpers.h"

using namespace udho::logging::params;

namespace {

bool reject_all(const udho::logging::message&) {
    return false;
}

bool error_and_above(const udho::logging::message& msg) {
    return msg[severity::val].value() >= udho::logging::severity::error;
}

std::atomic<bool> g_blocking_filter_entered{false};
std::atomic<bool> g_blocking_filter_release{false};
std::atomic<std::size_t> g_blocking_filter_calls{0};

bool blocking_accept_all(const udho::logging::message&) {
    g_blocking_filter_calls.fetch_add(1, std::memory_order_relaxed);
    g_blocking_filter_entered.store(true, std::memory_order_release);

    while (!g_blocking_filter_release.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    return true;
}

void reset_blocking_filter_state() {
    g_blocking_filter_entered.store(false, std::memory_order_relaxed);
    g_blocking_filter_release.store(false, std::memory_order_relaxed);
    g_blocking_filter_calls.store(0, std::memory_order_relaxed);
}


} // namespace

TEST_CASE("Producer admission and queue delivery", "[logging][producer]") {
    udho::logging::test_helpers::producer_state_guard guard;

    SECTION("activation/deactivation") {

        SECTION("log returns false while producer is inactive") {
            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "inactive message"));
        }

        SECTION("activated producer delivers a message to the ipc queue") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());

            REQUIRE(UDHO_LOG_WARNING("producer-test", "hello queue"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[subsystem::val].value() == "producer-test");
            REQUIRE(msg[message::val].value() == "hello queue");
            REQUIRE(msg[severity::val].value() == udho::logging::severity::warning);
        }

        SECTION("deactivate stops further submissions") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            REQUIRE(UDHO_LOG_INFO("producer-test", "before deactivate"));

            udho::logging::producer::deactivate();

            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "after deactivate"));

            std::size_t received = 0;
            udho::logging::message msg;
            while (queue.try_receive(msg)) {
                ++received;
            }
            REQUIRE(received == 1);
        }

        SECTION("producer can be reactivated after deactivation") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            REQUIRE(UDHO_LOG_INFO("producer-test", "before first deactivate"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "before first deactivate");

            udho::logging::producer::deactivate();
            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "while inactive"));

            udho::logging::producer::activate(queue_name.c_str());
            REQUIRE(UDHO_LOG_INFO("producer-test", "after reactivate"));

            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "after reactivate");
        }

        SECTION("deactivate is safe when producer is already inactive") {
            REQUIRE_NOTHROW(udho::logging::producer::deactivate());
            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "still inactive"));
        }

        SECTION("activate is idempotent for an already active producer") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::activate(queue_name.c_str());

            REQUIRE(UDHO_LOG_INFO("producer-test", "delivered after double activate"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "delivered after double activate");
        }

    }

    SECTION("filtering & threshold") {

        SECTION("callback filter can reject all") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::filter(&reject_all);

            REQUIRE_FALSE(UDHO_LOG_ERROR("producer-test", "rejected by callback"));

            udho::logging::message msg;
            REQUIRE_FALSE(queue.try_receive(msg));
        }

        SECTION("callback filter can accept only errors and above") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::filter(&error_and_above);

            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "filtered info"));
            REQUIRE(UDHO_LOG_ERROR("producer-test", "accepted error"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "accepted error");
            REQUIRE(msg[severity::val].value() == udho::logging::severity::error);
        }

        SECTION("thsreshold applies when no callback filter is installed") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::reset_filter();
            udho::logging::producer::threshold(udho::logging::severity::warning);

            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "below threshold"));
            REQUIRE(UDHO_LOG_ERROR("producer-test", "above threshold"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "above threshold");
            REQUIRE(msg[severity::val].value() == udho::logging::severity::error);
        }

        SECTION("callback filter takes precedence over threshold") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::threshold(udho::logging::severity::fatal);
            udho::logging::producer::filter(&error_and_above);

            REQUIRE(UDHO_LOG_ERROR("producer-test", "accepted by callback despite fatal threshold"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "accepted by callback despite fatal threshold");
        }

        SECTION("reset_filter restores threshold behavior") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::threshold(udho::logging::severity::error);
            udho::logging::producer::filter(&reject_all);
            udho::logging::producer::reset_filter();

            REQUIRE_FALSE(UDHO_LOG_WARNING("producer-test", "below restored threshold"));
            REQUIRE(UDHO_LOG_ERROR("producer-test", "above restored threshold"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "above restored threshold");
        }

        SECTION("threshold getter returns configured value") {
            udho::logging::producer::threshold(udho::logging::severity::warning);
            REQUIRE(udho::logging::producer::threshold() == udho::logging::severity::warning);
        }

    }

    SECTION("concurrency") {

        SECTION("multiple threads can submit concurrently") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());

            constexpr std::size_t thread_count = 8;
            std::atomic<std::size_t> accepted{0};
            std::vector<std::thread> threads;
            threads.reserve(thread_count);

            for (std::size_t i = 0; i < thread_count; ++i) {
                threads.emplace_back([&accepted, i] {
                    if (UDHO_LOG_INFO("producer-test", "concurrent message", request_id(std::string("rid-") + std::to_string(i)))) {
                        ++accepted;
                    }
                });
            }

            for (auto& t : threads) t.join();

            std::size_t received = 0;
            udho::logging::message msg;
            while (received < accepted.load() && udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }, std::chrono::milliseconds(200))) {
                ++received;
            }

            REQUIRE(received == accepted.load());
        }

        SECTION("deactivate waits for an in-flight log call before resetting the queue") {
            reset_blocking_filter_state();

            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::filter(&blocking_accept_all);

            std::atomic<bool> log_result{false};
            std::thread logger([&] {
                log_result.store(UDHO_LOG_INFO("producer-test", "blocked in filter"), std::memory_order_release);
            });

            REQUIRE(udho::logging::test_helpers::wait_until([] {
                return g_blocking_filter_entered.load(std::memory_order_acquire);
            }));

            auto deactivator = std::async(std::launch::async, [] {
                udho::logging::producer::deactivate();
            });

            REQUIRE(deactivator.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);

            g_blocking_filter_release.store(true, std::memory_order_release);

            REQUIRE(deactivator.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
            deactivator.get();
            logger.join();

            REQUIRE(log_result.load(std::memory_order_acquire));
            REQUIRE(g_blocking_filter_calls.load(std::memory_order_acquire) == 1);

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "blocked in filter");

            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "after synchronized deactivate"));
        }

        SECTION("deactivate completes under concurrent logging pressure") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::reset_filter();
            udho::logging::producer::threshold(udho::logging::severity::trace);

            constexpr std::size_t thread_count = 6;
            std::atomic<bool> keep_running{true};
            std::atomic<std::size_t> accepted{0};
            std::atomic<std::size_t> rejected{0};
            std::vector<std::thread> threads;
            threads.reserve(thread_count);

            for (std::size_t i = 0; i < thread_count; ++i) {
                threads.emplace_back([&, i] {
                    while (keep_running.load(std::memory_order_acquire)) {
                        if (UDHO_LOG_INFO("producer-test", "storm", request_id(std::string("storm-") + std::to_string(i)))) {
                            accepted.fetch_add(1, std::memory_order_relaxed);
                        } else {
                            rejected.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                });
            }

            REQUIRE(udho::logging::test_helpers::wait_until([&] {
                return accepted.load(std::memory_order_relaxed) > 0;
            }, std::chrono::milliseconds(1000), std::chrono::milliseconds(1)));

            auto deactivator = std::async(std::launch::async, [] {
                udho::logging::producer::deactivate();
            });

            REQUIRE(deactivator.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
            deactivator.get();

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            keep_running.store(false, std::memory_order_release);
            for (auto& t : threads) t.join();

            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "after log storm deactivate"));
            REQUIRE(rejected.load(std::memory_order_relaxed) > 0);

            std::size_t received = 0;
            udho::logging::message msg;
            while (queue.try_receive(msg)) {
                ++received;
            }
            REQUIRE(received == accepted.load(std::memory_order_relaxed));
        }

        SECTION("concurrent deactivate calls serialize without deadlock") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());

            auto f1 = std::async(std::launch::async, [] {
                udho::logging::producer::deactivate();
            });

            auto f2 = std::async(std::launch::async, [] {
                udho::logging::producer::deactivate();
            });

            REQUIRE(f1.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
            REQUIRE(f2.wait_for(std::chrono::seconds(2)) == std::future_status::ready);

            f1.get();
            f2.get();

            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "after double deactivate"));
        }

        SECTION("new log calls are rejected while deactivate waits for an in-flight logger") {
            reset_blocking_filter_state();

            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());
            udho::logging::producer::filter(&blocking_accept_all);

            std::atomic<bool> first_result{false};
            std::thread logger([&] {
                first_result.store(
                    UDHO_LOG_INFO("producer-test", "first blocked log"),
                    std::memory_order_release
                    );
            });

            REQUIRE(udho::logging::test_helpers::wait_until([] {
                return g_blocking_filter_entered.load(std::memory_order_acquire);
            }));

            auto deactivator = std::async(std::launch::async, [] {
                udho::logging::producer::deactivate();
            });

            REQUIRE(deactivator.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);

            // At this point deactivate should have acquired the admin gate and be waiting
            // for the blocked logger to leave.
            REQUIRE_FALSE(UDHO_LOG_INFO("producer-test", "must be rejected while deactivate is pending"));

            g_blocking_filter_release.store(true, std::memory_order_release);

            REQUIRE(deactivator.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
            deactivator.get();
            logger.join();

            REQUIRE(first_result.load(std::memory_order_acquire));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "first blocked log");
        }

        SECTION("concurrent activate calls serialize without deadlock") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            auto f1 = std::async(std::launch::async, [&] {
                udho::logging::producer::activate(queue_name.c_str());
            });

            auto f2 = std::async(std::launch::async, [&] {
                udho::logging::producer::activate(queue_name.c_str());
            });

            REQUIRE(f1.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
            REQUIRE(f2.wait_for(std::chrono::seconds(2)) == std::future_status::ready);

            f1.get();
            f2.get();

            REQUIRE(UDHO_LOG_INFO("producer-test", "after concurrent activate"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "after concurrent activate");
        }

        SECTION("activate and deactivate do not deadlock when raced") {
            auto queue_name = udho::logging::test_helpers::unique_queue_name();
            auto queue = udho::logging::detail::ipc_queue::create(queue_name.c_str());

            udho::logging::producer::activate(queue_name.c_str());

            auto f1 = std::async(std::launch::async, [] {
                udho::logging::producer::deactivate();
            });

            auto f2 = std::async(std::launch::async, [&] {
                udho::logging::producer::activate(queue_name.c_str());
            });

            REQUIRE(f1.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
            REQUIRE(f2.wait_for(std::chrono::seconds(2)) == std::future_status::ready);

            f1.get();
            f2.get();

            // Normalize final state explicitly, then verify usability.
            udho::logging::producer::activate(queue_name.c_str());

            REQUIRE(UDHO_LOG_INFO("producer-test", "after activate/deactivate race"));

            udho::logging::message msg;
            REQUIRE(udho::logging::test_helpers::wait_until([&] { return queue.try_receive(msg); }));
            REQUIRE(msg[message::val].value() == "after activate/deactivate race");
        }
    }

}