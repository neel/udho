cmake_minimum_required(VERSION 3.9)

project(udho-tests2-utils LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-tests2-utils-encoding encoding.cpp)
