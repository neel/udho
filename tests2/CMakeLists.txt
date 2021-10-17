cmake_minimum_required(VERSION 3.0)

project(udho-tests2 LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(hazo-meta hazo/meta.cpp)
target_link_libraries(hazo-meta PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(hazo-proxy hazo/proxy.cpp)
target_link_libraries(hazo-proxy PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(activity-data activity/data.cpp)
target_link_libraries(activity-data PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

catch_discover_tests(hazo-meta)
catch_discover_tests(hazo-proxy)
catch_discover_tests(activity-data)