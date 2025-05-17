cmake_minimum_required(VERSION 3.9)

project(udho-tests-activities LANGUAGES CXX VERSION 0.0.1)

find_package(CURL REQUIRED)

add_catch_test(udho-test-activities-data data.cpp)
add_catch_test(udho-test-activities-activity activity.cpp)
add_catch_test(udho-test-activities-subtask subtask.cpp)

add_catch_test(udho-test-activities-application application.cpp)
target_link_libraries(udho-test-activities-application PRIVATE CURL::libcurl)
