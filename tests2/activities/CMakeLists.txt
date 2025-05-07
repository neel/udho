cmake_minimum_required(VERSION 3.9)

project(udho-tests2-activities LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-test-activities-data data.cpp)
add_catch_test(udho-test-activities-activity activity.cpp)
add_catch_test(udho-test-activities-subtask subtask.cpp)
add_catch_test(udho-test-activities-application application.cpp)
