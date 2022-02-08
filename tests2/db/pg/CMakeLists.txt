cmake_minimum_required(VERSION 3.0)

project(udho-tests2-db-pg LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(pg_basic_activity basic_activity.cpp)
target_link_libraries(pg_basic_activity PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(pg_activity activity.cpp)
target_link_libraries(pg_activity PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

catch_discover_tests(pg_basic_activity)
catch_discover_tests(pg_activity)
