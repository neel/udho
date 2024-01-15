cmake_minimum_required(VERSION 3.9)
project(udho-examples-simple)

FIND_PACKAGE(Boost COMPONENTS thread REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

SET(EXAMPLE_SOURCES
    manifest.cpp
    main.cpp
)

add_subdirectory(applications)

add_executable(udho-examples-simple ${EXAMPLE_SOURCES})
target_link_libraries(udho-examples-simple PRIVATE ${Boost_LIBRARIES} udho udho-examples-simple-apps)
