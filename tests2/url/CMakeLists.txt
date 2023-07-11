cmake_minimum_required(VERSION 3.0)

project(udho-tests2-url LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(url url.cpp)
target_link_libraries(url PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

catch_discover_tests(url)
