cmake_minimum_required(VERSION 3.0)

project(udho-tests2-view LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

if(Catch2_VERSION_MAJOR LESS 3)
    add_definitions(-DWITH_CATCH_VERSION_2)
endif()

add_executable(view-parser parser.cpp)
target_link_libraries(view-parser PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

catch_discover_tests(view-parser)
