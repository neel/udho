cmake_minimum_required(VERSION 3.9)

FIND_PACKAGE(Boost COMPONENTS regex unit_test_framework REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

ADD_EXECUTABLE(routing routing.cpp)
TARGET_LINK_LIBRARIES(routing ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(parsing parsing.cpp)
TARGET_LINK_LIBRARIES(parsing ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(client client.cpp)
TARGET_LINK_LIBRARIES(client ${Boost_LIBRARIES} udho)

ENABLE_TESTING()

ADD_TEST(routing routing --report_level=short --log_level=message --show_progress=true)
ADD_TEST(parsing parsing --report_level=short --log_level=message --show_progress=true)
ADD_TEST(client client --report_level=short --log_level=message --show_progress=true)
