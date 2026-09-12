cmake_minimum_required(VERSION 3.9)
project(udho-examples-pgx)

find_package(Boost COMPONENTS thread REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(view)
add_subdirectory(controllers)
add_subdirectory(setup)

add_executable(udho-examples-pgx main.cpp)
target_compile_features(udho-examples-pgx PRIVATE cxx_std_17)
target_link_libraries(udho-examples-pgx PRIVATE udho-examples-pgx-controllers udho-examples-pgx-view ${Boost_LIBRARIES} udho)
