cmake_minimum_required(VERSION 3.0)

project(udho-tests2 LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(meta meta.cpp)
target_link_libraries(meta PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(element element.cpp)
target_link_libraries(element PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(node node.cpp)
target_link_libraries(node PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(seq seq.cpp)
target_link_libraries(seq PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(map map.cpp)
target_link_libraries(map PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(proxy proxy.cpp)
target_link_libraries(proxy PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(legacy legacy.cpp)
target_link_libraries(legacy PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

catch_discover_tests(meta)
catch_discover_tests(element)
catch_discover_tests(node)
catch_discover_tests(proxy)
catch_discover_tests(seq)
catch_discover_tests(map)
catch_discover_tests(legacy)
