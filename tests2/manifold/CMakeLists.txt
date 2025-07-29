cmake_minimum_required(VERSION 3.9)

project(udho-tests-kernel LANGUAGES CXX VERSION 0.0.1)


# add_catch_test(udho-test-middleware-facade facade.cpp)

add_catch_test(udho-test-manifold-composition composition.cpp)
