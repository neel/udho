cmake_minimum_required(VERSION 3.0)

FIND_PACKAGE(Boost COMPONENTS regex unit_test_framework system REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

ADD_EXECUTABLE(hazo hazo.cpp)
TARGET_LINK_LIBRARIES(hazo ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(routing routing.cpp)
TARGET_LINK_LIBRARIES(routing ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(parsing parsing.cpp)
TARGET_LINK_LIBRARIES(parsing ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(client client.cpp)
TARGET_LINK_LIBRARIES(client ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(decorators decorators.cpp)
TARGET_LINK_LIBRARIES(decorators ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(generators generators.cpp)
TARGET_LINK_LIBRARIES(generators ${Boost_LIBRARIES} udho)

ADD_EXECUTABLE(sql sql.cpp)
TARGET_LINK_LIBRARIES(sql ${Boost_LIBRARIES} udho)

ENABLE_TESTING()

ADD_TEST(hazo hazo --report_level=short --log_level=message --show_progress=true)
ADD_TEST(routing routing --report_level=short --log_level=message --show_progress=true)
ADD_TEST(parsing parsing --report_level=short --log_level=message --show_progress=true)
ADD_TEST(client client --report_level=short --log_level=message --show_progress=true)
ADD_TEST(decorators decorators --report_level=short --log_level=message --show_progress=true)
ADD_TEST(generators generators --report_level=short --log_level=message --show_progress=true)
ADD_TEST(sql sql --report_level=short --log_level=message --show_progress=true)