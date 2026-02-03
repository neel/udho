cmake_minimum_required(VERSION 3.9)

project(udho-tests-manifold LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-test-manifold-composition composition.cpp)
add_catch_test(udho-test-manifold-pipeline pipeline.cpp)
add_catch_test(udho-test-manifold-config config.cpp)
add_catch_test(udho-test-manifold-components-protocol components/protocol.cpp)
add_catch_test(udho-test-manifold-components-navigator components/navigator.cpp)
add_catch_test(udho-test-manifold-components-stage0 components/stage0.cpp)
add_catch_test(udho-test-manifold-components-stage01 components/stage01.cpp)
add_catch_test(udho-test-manifold-www www.cpp)
