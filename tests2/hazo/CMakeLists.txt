cmake_minimum_required(VERSION 3.9)

project(udho-tests2-hazo LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-test-hazo-meta meta.cpp)
add_catch_test(udho-test-hazo-element element.cpp)
add_catch_test(udho-test-hazo-node node.cpp)
add_catch_test(udho-test-hazo-seq seq.cpp)
add_catch_test(udho-test-hazo-map map.cpp)
add_catch_test(udho-test-hazo-proxy proxy.cpp)
add_catch_test(udho-test-hazo-legacy legacy.cpp)
