cmake_minimum_required(VERSION 3.0)

project(udho-tests2-url LANGUAGES CXX VERSION 0.0.1)

find_package(CURL REQUIRED)

add_catch_test(udho-test-url-patterns url.cpp)
add_catch_test(udho-test-url-router-asset asset.cpp)

target_link_libraries(udho-test-url-router-asset PRIVATE CURL::libcurl)
