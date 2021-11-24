cmake_minimum_required(VERSION 3.0)

project(udho-tests2 LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(data data.cpp)
target_link_libraries(data PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(activity activity.cpp)
target_link_libraries(activity PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(subtask subtask.cpp)
target_link_libraries(subtask PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

add_executable(application application.cpp)
target_link_libraries(application PRIVATE ${Boost_LIBRARIES} Catch2::Catch2 udho)

catch_discover_tests(data)
catch_discover_tests(activity)
catch_discover_tests(subtask)
catch_discover_tests(application)