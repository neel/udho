cmake_minimum_required(VERSION 3.0)

project(udho-tests2-net LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

if(Catch2_VERSION_MAJOR LESS 3)
    add_definitions(-DWITH_CATCH_VERSION_2)
endif()

find_package(CURL REQUIRED)

add_executable(network network.cpp)
target_link_libraries(network PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho CURL::libcurl)

catch_discover_tests(network)

