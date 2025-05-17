cmake_minimum_required(VERSION 3.9)

project(udho-tests-net LANGUAGES CXX VERSION 0.0.1)

find_package(CURL REQUIRED)
add_catch_test(udho-test-network network.cpp)
target_link_libraries(udho-test-network PRIVATE CURL::libcurl)
