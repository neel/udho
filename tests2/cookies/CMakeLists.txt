cmake_minimum_required(VERSION 3.9)

project(udho-tests-cookies LANGUAGES CXX VERSION 0.0.1)

find_package(CURL REQUIRED)

add_catch_test(udho-tests-cookies-cookie cookie.cpp)

# add_catch_test(udho-tests-cookies-http http.cpp)
# target_link_libraries(udho-tests-cookies-http PRIVATE CURL::libcurl)
