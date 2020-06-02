cmake_minimum_required(VERSION 3.9)
project(udho-examples)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1 ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 ${CMAKE_CXX_FLAGS}")

ADD_EXECUTABLE(udho-example-hello hello.cpp)
TARGET_LINK_LIBRARIES(udho-example-hello udho)

# ADD_EXECUTABLE(udho-example-simple simple.cpp)
# TARGET_LINK_LIBRARIES(udho-example-simple udho)
# 
# ADD_EXECUTABLE(udho-example-functor functor.cpp)
# TARGET_LINK_LIBRARIES(udho-example-functor udho)
# 
# ADD_EXECUTABLE(udho-example-application application.cpp)
# TARGET_LINK_LIBRARIES(udho-example-application udho)
# 
add_definitions(-DWWW_PATH="${CMAKE_CURRENT_SOURCE_DIR}/www")
add_definitions(-DTMPL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/templates")

ADD_EXECUTABLE(udho-example-view view.cpp)
TARGET_LINK_LIBRARIES(udho-example-view udho)
