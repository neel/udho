cmake_minimum_required(VERSION 3.9)

project(udho-tests-session LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-tests-session-session session.cpp)
add_catch_test(udho-tests-session-storage-disk storage_disk.cpp)
