cmake_minimum_required(VERSION 3.9)
project(udho-examples-simple-apps)

FIND_PACKAGE(Boost COMPONENTS thread REQUIRED)
FIND_PACKAGE(Threads REQUIRED)


SET(SOURCES
    main.cpp
    user.cpp
)

add_library(udho-examples-simple-apps SHARED ${SOURCES})
target_link_libraries(udho-examples-simple-apps PRIVATE ${Boost_LIBRARIES} udho)
