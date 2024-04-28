cmake_minimum_required(VERSION 3.9)
project(udho)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

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
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 -fdiagnostics-color=always -fdiagnostics-show-template-tree  ${CMAKE_CXX_FLAGS}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

option(UDHO_BUILD_TESTS "Build the unit tests" ON)
option(UDHO_BUILD_EXAMPLES "Build the unit tests" ON)
option(UDHO_USE_ICU "Build with ICU" OFF)
option(UDHO_USE_PUGIXML "Build with PugiXML" ON)
option(UDHO_USE_NLOHMANN_JSON "Build with Nlohmann JSON Support" ON)
option(UDHO_USE_FCGI "Build with FCGI" ON)
option(UDHO_HEADER_ONLY "Use Udho as Header Only Library" ON)

IF(UNIX AND APPLE)
    SET(OPENSSL_ROOT_DIR "/usr/local/Cellar/openssl@3/3.0.1")
ENDIF()

if(UDHO_HEADER_ONLY)
    add_compile_definitions(UDHO_HEADER_ONLY)
endif()

if(UDHO_USE_ICU)
    add_definitions(-DWITH_ICU)
    IF(UNIX AND APPLE)
        SET(ICU_ROOT "/usr/local/opt/icu4c")
    ENDIF()
    FIND_PACKAGE(ICU COMPONENTS uc in REQUIRED)
endif()

if(UDHO_USE_PUGIXML)
    add_definitions(-DWITH_PUGI)
    FIND_PACKAGE(PUGIXML REQUIRED)
endif()

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem regex system serialization iostreams REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED)

IF(UDHO_USE_NLOHMANN_JSON)
    SET(JSON_BuildTests OFF CACHE INTERNAL "")
    FIND_PACKAGE(nlohmann_json)
    if(nlohmann_json_FOUND)
        add_compile_definitions(WITH_JSON_NLOHMANN)
        message(STATUS "With nlohmann json")
    else()
        message(STATUS "Without nlohmann json")
    endif()
ENDIF()

SET(INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  ${CMAKE_CURRENT_SOURCE_DIR}/impl
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/certify/include
)

file(GLOB_RECURSE UDHO_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h)

SET(UDHO_SOURCES 
    sources/udho/page.cpp
)

ADD_EXECUTABLE(sandbox sandbox.cpp)
TARGET_LINK_LIBRARIES(sandbox udho)

if(UDHO_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
endif()

add_subdirectory(vendor)

ADD_LIBRARY(udho SHARED ${UDHO_SOURCES} ${UDHO_HEADERS})
TARGET_LINK_LIBRARIES(udho PUBLIC ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(udho PUBLIC OpenSSL::SSL certify::core ctti ozo scn::scn tabulate::tabulate)
IF (CMAKE_CXX_STANDARD LESS 20)
    TARGET_LINK_LIBRARIES(udho PUBLIC fmt)
ENDIF()
TARGET_INCLUDE_DIRECTORIES(udho PUBLIC ${INCLUDE_DIRS})
IF(UDHO_USE_FCGI)
    TARGET_LINK_LIBRARIES(udho PUBLIC ${FCGI_LIBRARIES})
ENDIF()

SET(UDHO_HEADERS_LIST "")
foreach(h IN LISTS UDHO_HEADERS)
    get_filename_component(absolute_h ${h} ABSOLUTE)
    list(APPEND UDHO_HEADERS_LIST ${absolute_h})
endforeach()
set_property(TARGET udho PROPERTY UDHO_HEADERS ${UDHO_HEADERS_LIST})
set_property(TARGET udho PROPERTY UDHO_INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ctti/include ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ozo/include ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ozo/contrib/resource_pool/include)
# set_property(TARGET udho PROPERTY UDHO_FULL_INCLUDE_DIRS ${INCLUDE_DIRS} ${INCLUDE_DIRECTORIES})

if(UDHO_USE_ICU)
    TARGET_LINK_LIBRARIES(udho PUBLIC ICU::uc ICU::in)
endif()

if(UDHO_USE_PUGIXML)
    TARGET_LINK_LIBRARIES(udho PUBLIC ${PUGIXML_LIBRARIES})
endif()

if(nlohmann_json_FOUND)
    TARGET_LINK_LIBRARIES(udho PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(udho PUBLIC WITH_JSON_NLOHMANN)
endif()

TARGET_LINK_LIBRARIES(udho PUBLIC sol2)

if(UDHO_BUILD_TESTS)
   enable_testing()
   add_subdirectory(tests)
   add_subdirectory(tests2)
endif()

if(UNIX AND APPLE)
    target_link_libraries(udho PUBLIC "-framework Security")
    target_link_libraries(udho PUBLIC "-framework CoreServices")
endif()

add_subdirectory(docs)
