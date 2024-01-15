cmake_minimum_required(VERSION 3.0)

project(udho-tests2-db-pg LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)


if(Catch2_VERSION_MAJOR LESS 3)
    add_definitions(-DWITH_CATCH_VERSION_2)
endif()

add_executable(pg_basic_activity basic_activity.cpp)
target_link_libraries(pg_basic_activity PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(pg_activity activity.cpp)
target_link_libraries(pg_activity PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(pg_decorators decorators.cpp)
target_link_libraries(pg_decorators PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(pg_generators generators.cpp)
target_link_libraries(pg_generators PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(pg_join join.cpp)
target_link_libraries(pg_join PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(pg_model model.cpp)
target_link_libraries(pg_model PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

catch_discover_tests(pg_basic_activity)
catch_discover_tests(pg_activity)
catch_discover_tests(pg_decorators)
catch_discover_tests(pg_generators)
catch_discover_tests(pg_join)
catch_discover_tests(pg_model)
