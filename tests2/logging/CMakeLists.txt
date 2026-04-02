cmake_minimum_required(VERSION 3.9)

project(udho-tests-logging LANGUAGES CXX VERSION 0.0.1)

add_executable(udho-tests-logging-example main.cpp)
target_link_libraries(udho-tests-logging-example udho)

add_catch_test(udho-tests-logging-message   message.cpp)
add_catch_test(udho-tests-logging-producer  producer.cpp)
add_catch_test(udho-tests-logging-consumer  consumer.cpp)
add_catch_test(udho-tests-logging-commander commander.cpp)
add_catch_test(udho-tests-logging-setup     setup.cpp)