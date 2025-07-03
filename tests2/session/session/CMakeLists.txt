cmake_minimum_required(VERSION 3.9)

project(udho-tests-session-session LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-tests-session-session-fs fs.cpp)
add_catch_test(udho-tests-session-session-fs_mem fs_mem.cpp)
add_catch_test(udho-tests-session-session-redis redis.cpp)
