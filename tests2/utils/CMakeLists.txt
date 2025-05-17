cmake_minimum_required(VERSION 3.9)

project(udho-tests-utils LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-tests-utils-encoding encoding.cpp)
