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

update_submodule(${PROJECT_SOURCE_DIR}/vendor/)

SET(CMAKE_CXX_STANDARD 14)
#SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1 ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 ${CMAKE_CXX_FLAGS}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

option(UDHO_BUILD_TESTS "Build the unit tests" ON)
option(UDHO_BUILD_EXAMPLES "Build the unit tests" ON)
option(UDHO_USE_ICU "Build with ICU" ON)
option(UDHO_USE_PUGIXML "Build with PugiXML" ON)

if(UDHO_USE_ICU)
    add_definitions(-DWITH_ICU)
    IF(UNIX AND APPLE)
        SET(ICU_ROOT "/usr/local/opt/icu4c")
    ENDIF()
    FIND_PACKAGE(ICU COMPONENTS uc in REQUIRED)
endif()

if(UDHO_USE_PUGIXML)
    add_definitions(-DWITH_PUGI)
    FIND_PACKAGE(PugiXML REQUIRED)
endif()

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem regex system serialization REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED)
FIND_PACKAGE(nlohmann_json)

if(nlohmann_json_FOUND)
    add_definitions(-DWITH_JSON_NLOHMANN)
endif()

SET(INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  vendor/certify/include
)

SET(UDHO_HEADERS 
    includes/udho/access.h
    includes/udho/attachment.h
    includes/udho/client.h
    includes/udho/connection.h
    includes/udho/cookie.h
    includes/udho/listener.h
    includes/udho/parser.h
    includes/udho/server.h
    includes/udho/url.h
    includes/udho/watcher.h
    includes/udho/activities.h
    includes/udho/bridge.h
    includes/udho/compositors.h
    includes/udho/context.h
    includes/udho/defs.h
    includes/udho/logging.h
    includes/udho/router.h
    includes/udho/session.h
    includes/udho/util.h
    includes/udho/application.h
    includes/udho/cache.h
    includes/udho/configuration.h
    includes/udho/contexts.h
    includes/udho/forms.h
    includes/udho/page.h
    includes/udho/scope.h
    includes/udho/visitor.h
    includes/udho/hazo.h
)
SET(UDHO_SOURCES 
    page.cpp
)

if(UDHO_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
endif()

add_subdirectory(vendor)

ADD_LIBRARY(udho SHARED ${UDHO_SOURCES} ${UDHO_HEADERS})
TARGET_LINK_LIBRARIES(udho PUBLIC ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(udho PUBLIC OpenSSL::SSL certify::core ctti)
TARGET_INCLUDE_DIRECTORIES(udho PUBLIC ${INCLUDE_DIRS})

if(UDHO_USE_ICU)
    TARGET_LINK_LIBRARIES(udho PUBLIC ICU::uc ICU::in)
endif()

if(UDHO_USE_PUGIXML)
    TARGET_LINK_LIBRARIES(udho PUBLIC ${PUGIXML_LIBRARIES})
endif()

if(nlohmann_json_FOUND)
    TARGET_LINK_LIBRARIES(udho PUBLIC nlohmann_json::nlohmann_json)
endif()

if(UDHO_BUILD_TESTS)
   enable_testing()
   add_subdirectory(tests/)
endif()

add_subdirectory(docs)
