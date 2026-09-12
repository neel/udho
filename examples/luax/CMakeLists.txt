cmake_minimum_required(VERSION 3.9)
project(udho-examples-luax)

find_package(Boost COMPONENTS thread REQUIRED)
find_package(Threads REQUIRED)

add_executable(udho-examples-luax
    main.cpp
    actions.cpp
    data.cpp
)

target_compile_features(udho-examples-luax PRIVATE cxx_std_17)
target_link_libraries(udho-examples-luax PRIVATE ${Boost_LIBRARIES} udho)

