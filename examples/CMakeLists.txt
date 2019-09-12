cmake_minimum_required(VERSION 3.9)
project(udho-examples)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1 ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 ${CMAKE_CXX_FLAGS}")

ADD_EXECUTABLE(udho-example-simple simple.cpp)
TARGET_LINK_LIBRARIES(udho-example-simple udho)
