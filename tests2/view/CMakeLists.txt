cmake_minimum_required(VERSION 3.0)

project(udho-tests2-view LANGUAGES CXX VERSION 0.0.1)

FIND_PACKAGE(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

if(Catch2_VERSION_MAJOR LESS 3)
    add_definitions(-DWITH_CATCH_VERSION_2)
endif()

add_executable(udho-test-view-parser parser.cpp)
target_link_libraries(udho-test-view-parser PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(udho-test-data-meta meta.cpp)
target_link_libraries(udho-test-data-meta PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(udho-test-lua-mt lua-mt.cpp)
target_link_libraries(udho-test-lua-mt PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

add_executable(udho-test-lua lua.cpp)
target_link_libraries(udho-test-lua PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

catch_discover_tests(udho-test-view-parser)
catch_discover_tests(udho-test-data-meta)
catch_discover_tests(udho-test-lua-mt)
catch_discover_tests(udho-test-lua)
