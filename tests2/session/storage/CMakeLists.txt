cmake_minimum_required(VERSION 3.9)

project(udho-tests-session-storage LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-tests-session-storage-fs fs.cpp)
add_catch_test(udho-tests-session-storage-fs_mem fs_mem.cpp)
add_catch_test(udho-tests-session-storage-redis redis.cpp)
