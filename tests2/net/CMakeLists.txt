cmake_minimum_required(VERSION 3.0)

project(udho-tests2-net LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)


find_package(CURL REQUIRED)

add_executable(network network.cpp)
target_link_libraries(network PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho CURL::libcurl)

catch_discover_tests(network)

