cmake_minimum_required(VERSION 3.9)
project(udho-examples-simple)

FIND_PACKAGE(Boost COMPONENTS thread REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

SET(EXAMPLE_SIMPLE_SOURCES
    main.cpp
    actions.cpp
)

add_executable(udho-examples-simple ${EXAMPLE_SIMPLE_SOURCES})
target_link_libraries(udho-examples-simple PRIVATE ${Boost_LIBRARIES} udho)
