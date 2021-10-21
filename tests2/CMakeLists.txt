cmake_minimum_required(VERSION 3.0)

project(udho-tests2 LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(hazo-meta hazo/meta.cpp)
target_link_libraries(hazo-meta PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(hazo-element hazo/element.cpp)
target_link_libraries(hazo-element PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(hazo-node hazo/node.cpp)
target_link_libraries(hazo-node PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(hazo-seq hazo/seq.cpp)
target_link_libraries(hazo-seq PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(hazo-map hazo/map.cpp)
target_link_libraries(hazo-map PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(hazo-proxy hazo/proxy.cpp)
target_link_libraries(hazo-proxy PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(activities-data activities/data.cpp)
target_link_libraries(activities-data PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(activities-activity activities/activity.cpp)
target_link_libraries(activities-activity PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

catch_discover_tests(hazo-meta)
catch_discover_tests(hazo-element)
catch_discover_tests(hazo-node)
catch_discover_tests(hazo-proxy)
catch_discover_tests(hazo-seq)
catch_discover_tests(hazo-map)
catch_discover_tests(activities-data)
catch_discover_tests(activities-activity)