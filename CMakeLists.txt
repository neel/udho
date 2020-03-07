cmake_minimum_required(VERSION 3.9)
project(udho)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1 ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 ${CMAKE_CXX_FLAGS}")

option(UDHO_BUILD_TESTS "Build the unit tests" ON)
option(UDHO_BUILD_EXAMPLES "Build the unit tests" ON)

if(UNIX AND APPLE)
    SET(ICU_ROOT "/usr/local/opt/icu4c")
endif()

FIND_PACKAGE(Threads REQUIRED)
find_package(ICU COMPONENTS uc REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem regex system REQUIRED) 

SET(INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

SET(UDHO_HEADERS 
    includes/udho/connection.h 
    includes/udho/listener.h 
    includes/udho/router.h 
    includes/udho/util.h 
    includes/udho/page.h
    includes/udho/application.h
)
SET(UDHO_SOURCES 
    connection.cpp 
    util.cpp 
    page.cpp
)

if(UDHO_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
endif()

ADD_LIBRARY(udho SHARED ${UDHO_SOURCES} ${UDHO_HEADERS})
TARGET_LINK_LIBRARIES(udho ${Boost_LIBRARIES} ICU::uc ${CMAKE_THREAD_LIBS_INIT}) 
TARGET_INCLUDE_DIRECTORIES(udho PUBLIC ${INCLUDE_DIRS})

if(UDHO_BUILD_TESTS)
   enable_testing()
   add_subdirectory(tests/)
endif()
