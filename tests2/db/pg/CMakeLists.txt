cmake_minimum_required(VERSION 3.9)

project(udho-tests-db-pg LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-test-db-pg-pg_basic_activity basic_activity.cpp)
# add_catch_test(udho-test-db-pg-pg_activity activity.cpp)
# add_catch_test(udho-test-db-pg-pg_decorators decorators.cpp)
# add_catch_test(udho-test-db-pg-pg_generators generators.cpp)
# add_catch_test(udho-test-db-pg-pg_join join.cpp)
# add_catch_test(udho-test-db-pg-pg_model model.cpp)
