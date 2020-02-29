cmake_minimum_required(VERSION 3.0)

FIND_PACKAGE(Boost COMPONENTS regex unit_test_framework system REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

# ADD_EXECUTABLE(routing routing.cpp)
# TARGET_LINK_LIBRARIES(routing ${Boost_LIBRARIES} udho)

ENABLE_TESTING()

# ADD_TEST(routing routing --report_level=short --log_level=message --show_progress=true)
