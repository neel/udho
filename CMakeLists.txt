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
    db/pretty/pretty.h
    db/pretty/detail.h
    db/pretty/type.h
    db/pretty/printer.h
    db/pretty/fwd.h
    db/fieldset.h
    db/pg.h
    db/common.h
    db/pg/pg.h
    db/pg/schema.h
    db/pg/constructs/operators.h
    db/pg/constructs/cast.h
    db/pg/constructs/concat.h
    db/pg/constructs/strings.h
    db/pg/constructs/fwd.h
    db/pg/constructs/constructs.h
    db/pg/constructs/alias.h
    db/pg/constructs/types.h
    db/pg/constructs/functions.h
    db/pg/generators.h
    db/pg/generators/operators.h
    db/pg/generators/group.h
    db/pg/generators/from.h
    db/pg/generators/into.h
    db/pg/generators/limit.h
    db/pg/generators/generators.h
    db/pg/generators/select.h
    db/pg/generators/where.h
    db/pg/generators/set.h
    db/pg/generators/order.h
    db/pg/generators/keys.h
    db/pg/generators/values.h
    db/pg/generators/returning.h
    db/pg/generators/fwd.h
    db/pg/generators/join.h
    db/pg/crud/from.h
    db/pg/crud/into.h
    db/pg/crud/crud.h
    db/pg/crud/remove.h
    db/pg/crud/select.h
    db/pg/crud/builder.h
    db/pg/crud/insert.h
    db/pg/crud/update.h
    db/pg/crud/fwd.h
    db/pg/crud/join.h
    db/pg/activities/traits.h
    db/pg/activities/activity.h
    db/pg/activities/start.h
    db/pg/activities/on.h
    db/pg/activities/after.h
    db/pg/activities/controller.h
    db/pg/activities/failure.h
    db/pg/activities/data.h
    db/pg/activities/subtask.h
    db/pg/features.h
    db/pg/schema/operations.h
    db/pg/schema/basic.h
    db/pg/schema/readonly.h
    db/pg/schema/detail.h
    db/pg/schema/defs.h
    db/pg/schema/schema.h
    db/pg/schema/field.h
    db/pg/schema/column.h
    db/pg/schema/fwd.h
    db/pg/schema/relation.h
    db/pg/model.h
    db/pg/decorators.h
    db/pg/decorators/logical.h
    db/pg/decorators/traits.h
    db/pg/decorators/conjunctions.h
    db/pg/decorators/conjugate.h
    db/pg/decorators/comma.h
    db/pg/decorators/assignments.h
    db/pg/decorators/keys.h
    db/pg/decorators/values.h
    db/pg/decorators/conditions.h
    db/pg/io/pretty.h
    db/pg/io/json.h
    db/pg/io/form.h
    db/pg/ozo/io.h
    db/pg/ozo/connection.h
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
TARGET_LINK_LIBRARIES(udho PUBLIC OpenSSL::SSL certify::core ctti ozo)
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
