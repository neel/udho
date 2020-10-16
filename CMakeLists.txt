cmake_minimum_required(VERSION 3.9)
project(udho)

MACRO(update_submodule submodule_dir)
  EXECUTE_PROCESS(COMMAND git submodule init
                  WORKING_DIRECTORY ${submodule_dir}
                  RESULT_VARIABLE submodule_init_exit_code)
  EXECUTE_PROCESS(COMMAND git submodule update --init --recursive --force
                  WORKING_DIRECTORY ${submodule_dir}
                  RESULT_VARIABLE submodule_update_exit_code)
  IF(NOT(submodule_init_exit_code EQUAL 0 AND submodule_update_exit_code EQUAL 0))
    MESSAGE(FATAL_ERROR "It was not possible update '${submodule_dir}' submodule.")
  ENDIF()
ENDMACRO()

update_submodule(${PROJECT_SOURCE_DIR}/deps/)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1 ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 ${CMAKE_CXX_FLAGS}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

option(UDHO_BUILD_TESTS "Build the unit tests" ON)
option(UDHO_BUILD_EXAMPLES "Build the unit tests" ON)
option(UDHO_USE_ICU "Build with ICU" ON)
option(UDHO_USE_PUGIXML "Build with PugiXML" ON)

if(UDHO_USE_ICU)
    add_definitions(-DWITH_ICU)
    IF(UNIX AND APPLE)
        SET(ICU_ROOT "/USR/LOCAL/OPT/ICU4C")
    ENDIF()
    FIND_PACKAGE(ICU COMPONENTS uc REQUIRED)
endif()

if(UDHO_USE_PUGIXML)
    add_definitions(-DWITH_PUGI)
    FIND_PACKAGE(PugiXML REQUIRED)
endif()

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem regex system serialization REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED)

SET(INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  deps/certify/include
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
    page.cpp
    client.cpp
    url.cpp
)

if(UDHO_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
endif()

add_subdirectory(deps)

ADD_LIBRARY(udho SHARED ${UDHO_SOURCES} ${UDHO_HEADERS})
TARGET_LINK_LIBRARIES(udho ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} OpenSSL::SSL certify::core ctti)
TARGET_INCLUDE_DIRECTORIES(udho PUBLIC ${INCLUDE_DIRS})

if(UDHO_USE_ICU)
    TARGET_LINK_LIBRARIES(udho ICU::uc)
endif()

if(UDHO_USE_PUGIXML)
    TARGET_LINK_LIBRARIES(udho ${PUGIXML_LIBRARIES})
endif()

if(UDHO_BUILD_TESTS)
   enable_testing()
   add_subdirectory(tests/)
endif()
