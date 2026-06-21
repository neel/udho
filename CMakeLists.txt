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

# update_submodule(${PROJECT_SOURCE_DIR}/vendor/)

SET(CMAKE_CXX_STANDARD 14)
#SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=1 ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=0 -fdiagnostics-color=always -fdiagnostics-show-template-tree  ${CMAKE_CXX_FLAGS}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

option(UDHO_BUILD_TESTS "Build the unit tests" ON)
option(UDHO_ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(UDHO_BUILD_EXAMPLES "Build the unit tests" ON)
option(UDHO_USE_ICU "Build with ICU" OFF)
option(UDHO_USE_PUGIXML "Build with PugiXML" ON)
option(UDHO_USE_NLOHMANN_JSON "Build with Nlohmann JSON Support" ON)
option(UDHO_USE_FCGI "Build with FCGI" ON)
option(UDHO_HEADER_ONLY "Use Udho as Header Only Library" ON)
option(UDHO_USE_LUAJIT "Use LuaJIT is available" OFF)
option(UDHO_ENABLE_STACKTRACE "Enable Stack Trace" ON)
option(UDHO_ENABLE_ASAN  "Enable AddressSanitizer" OFF)
option(UDHO_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

IF(UNIX AND APPLE)
    SET(OPENSSL_ROOT_DIR "/usr/local/Cellar/openssl@3/3.0.1")
ENDIF()

# if(UDHO_HEADER_ONLY)
#     add_compile_definitions(UDHO_HEADER_ONLY)
# endif()

# if(UDHO_USE_ICU)
#     add_definitions(-DWITH_ICU)
#     IF(UNIX AND APPLE)
#         SET(ICU_ROOT "/usr/local/opt/icu4c")
#     ENDIF()
#     FIND_PACKAGE(ICU COMPONENTS uc in REQUIRED)
# endif()

# if(UDHO_USE_PUGIXML)
#     add_definitions(-DWITH_PUGI)
#     FIND_PACKAGE(PUGIXML REQUIRED)
# endif()

find_package(Threads REQUIRED)

set(UDHO_BOOST_COMPONENTS
    filesystem
    regex
    serialization
    iostreams
    log
    log_setup
)
set(UDHO_BOOST_MAX_VERSION "1.90.0")
if(DEFINED BOOST_ROOT AND NOT BOOST_ROOT STREQUAL "")
    message(STATUS "Searching exclusively for Boost under: ${BOOST_ROOT}")
    unset(Boost_DIR CACHE)
    find_package(Boost ${UDHO_BOOST_MIN_VERSION} CONFIG REQUIRED COMPONENTS ${UDHO_BOOST_COMPONENTS} PATHS "${BOOST_ROOT}" NO_DEFAULT_PATH)
else()
    find_package(Boost COMPONENTS ${UDHO_BOOST_COMPONENTS})
    if(Boost_VERSION VERSION_GREATER_EQUAL UDHO_BOOST_MAX_VERSION)
        message(FATAL_ERROR
            "Boost ${Boost_VERSION} is too new. Please configure with -DBOOST_ROOT=/opt/boost-1.89 "
            "or install a supported Boost version. Required range: < ${UDHO_BOOST_MAX_VERSION}."
        )
    endif()
endif()

FIND_PACKAGE(OpenSSL REQUIRED)
find_package(pegtl REQUIRED)
find_package(LibMagic REQUIRED)

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
# ADD_EXECUTABLE(sandbox sandbox.cpp)
# TARGET_LINK_LIBRARIES(sandbox udho)
# ADD_EXECUTABLE(sandbox2 sandbox2.cpp)
# TARGET_LINK_LIBRARIES(sandbox2 udho)

if(UDHO_BUILD_EXAMPLES)
    # ADD_SUBDIRECTORY(examples)
endif()

add_subdirectory(vendor)

ADD_LIBRARY(udho INTERFACE ${UDHO_SOURCES} ${UDHO_HEADERS})
TARGET_LINK_LIBRARIES(udho INTERFACE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(udho INTERFACE OpenSSL::SSL certify::core ctti ozo scn::scn tabulate::tabulate taocpp::pegtl ${LIBMAGIC_LIBRARY})

TARGET_LINK_LIBRARIES(udho INTERFACE fmt)
TARGET_INCLUDE_DIRECTORIES(udho INTERFACE ${INCLUDE_DIRS})

if(UDHO_ENABLE_STACKTRACE)
    include(FetchContent)
    # Protect cpptrace/libdwarf from a parent PACKAGE_VERSION variable.
    set(_UDHO_SAVED_PACKAGE_VERSION "${PACKAGE_VERSION}")
    unset(PACKAGE_VERSION)
    unset(PACKAGE_VERSION CACHE)

    FetchContent_Declare(
        cpptrace
        GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
        GIT_TAG        v1.0.4 # <HASH or TAG>
    )
    FetchContent_MakeAvailable(cpptrace)
    # Restore PACKAGE_VERSION
    if(DEFINED _UDHO_SAVED_PACKAGE_VERSION)
        set(PACKAGE_VERSION "${_UDHO_SAVED_PACKAGE_VERSION}")
    endif()
    target_link_libraries(udho INTERFACE cpptrace::cpptrace)

    find_package(Libiberty)
    if(LIBIBERTY_FOUND)
        target_link_libraries(udho INTERFACE ${LIBIBERTY_LIBRARIES})
        add_compile_definitions(WITH_LIBIBERTY)
    endif()

    add_compile_definitions(WITH_STACKTRACE)
endif()

# IF(UDHO_USE_FCGI)
#     TARGET_LINK_LIBRARIES(udho INTERFACE ${FCGI_LIBRARIES})
# ENDIF()

SET(UDHO_HEADERS_LIST "")
foreach(h IN LISTS UDHO_HEADERS)
    get_filename_component(absolute_h ${h} ABSOLUTE)
    list(APPEND UDHO_HEADERS_LIST ${absolute_h})
endforeach()
set_property(TARGET udho PROPERTY UDHO_HEADERS ${UDHO_HEADERS_LIST})
set_property(TARGET udho PROPERTY UDHO_INCLUDE_DIRS ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ctti/include ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ozo/include ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ozo/contrib/resource_pool/include)
# set_property(TARGET udho PROPERTY UDHO_FULL_INCLUDE_DIRS ${INCLUDE_DIRS} ${INCLUDE_DIRECTORIES})

# if(UDHO_USE_ICU)
#     TARGET_LINK_LIBRARIES(udho INTERFACE ICU::uc ICU::in)
# endif()

# if(UDHO_USE_PUGIXML)
#     TARGET_LINK_LIBRARIES(udho INTERFACE ${PUGIXML_LIBRARIES})
# endif()

if(nlohmann_json_FOUND)
    TARGET_LINK_LIBRARIES(udho INTERFACE nlohmann_json::nlohmann_json)
    target_compile_definitions(udho INTERFACE WITH_JSON_NLOHMANN)
endif()

set(UDHO_LUA_SUPPORTED_VERSIONS 5.4 5.3 5.2 5.1)
function(udho_find_supported_lua)
    foreach(_lua_requested_version IN ITEMS ${UDHO_LUA_SUPPORTED_VERSIONS})
            foreach(_cache_variable IN ITEMS
                LUA_INCLUDE_DIR
                LUA_LIBRARY
                LUA_MATH_LIBRARY
            )
            unset(${_cache_variable} CACHE)
            unset(${_cache_variable})
        endforeach()

        unset(LUA_LIBRARIES)
        unset(Lua_FOUND)
        unset(LUA_FOUND)

        unset(Lua_VERSION)
        unset(Lua_VERSION_MAJOR)
        unset(Lua_VERSION_MINOR)
        unset(Lua_VERSION_PATCH)

        unset(LUA_VERSION_STRING)
        unset(LUA_VERSION_MAJOR)
        unset(LUA_VERSION_MINOR)
        unset(LUA_VERSION_PATCH)

        find_package(Lua ${_lua_requested_version} EXACT QUIET)

        if(Lua_FOUND)
            if(DEFINED Lua_VERSION)
                set(_lua_found_version "${Lua_VERSION}")
            else()
                # Compatibility with CMake versions before 4.2.
                set(_lua_found_version "${LUA_VERSION_STRING}")
            endif()

            set(UDHO_LUA_FOUND TRUE PARENT_SCOPE)
            set(UDHO_LUA_VERSION  "${_lua_found_version}" PARENT_SCOPE)
            set(LUA_INCLUDE_DIR "${LUA_INCLUDE_DIR}" PARENT_SCOPE)
            set(LUA_LIBRARIES "${LUA_LIBRARIES}" PARENT_SCOPE)

            return()
        endif()
    endforeach()
    set(UDHO_LUA_FOUND FALSE PARENT_SCOPE)
endfunction()

if(UDHO_USE_LUAJIT)
    find_package(LuaJIT)
    if(LUAJIT_FOUND)
        message(STATUS "LuaJIT found: ${LUAJIT_INCLUDE_DIR} ${LUAJIT_LIBRARIES}")
        TARGET_LINK_LIBRARIES(udho INTERFACE ${LUAJIT_LIBRARIES})
        TARGET_INCLUDE_DIRECTORIES(udho INTERFACE ${LUAJIT_INCLUDE_DIR})
        add_compile_definitions(WITH_LUA_JIT)
    else()
        udho_find_supported_lua()
        if(NOT UDHO_LUA_FOUND)
            message(FATAL_ERROR "No supported Lua installation found. Udho supports Lua 5.1 through 5.4.")
        endif()
        TARGET_LINK_LIBRARIES(udho INTERFACE ${LUA_LIBRARIES})
        TARGET_INCLUDE_DIRECTORIES(udho INTERFACE ${LUA_INCLUDE_DIR})
    endif()
else()
    udho_find_supported_lua()
    if(NOT UDHO_LUA_FOUND)
        message(FATAL_ERROR "No supported Lua installation found. Udho supports Lua 5.1 through 5.4.")
    endif()
    TARGET_LINK_LIBRARIES(udho INTERFACE ${LUA_LIBRARIES})
    TARGET_INCLUDE_DIRECTORIES(udho INTERFACE ${LUA_INCLUDE_DIR})
endif()

target_compile_definitions(udho INTERFACE SOL_EXCEPTIONS_SAFE_PROPAGATION SOL_ALL_SAFETIES_ON)
TARGET_LINK_LIBRARIES(udho INTERFACE sol2)

FIND_PACKAGE(HiRedis)
IF(HIREDIS_FOUND)
    message(STATUS "HiRedis found: ${HIREDIS_INCLUDE_DIRS} ${HIREDIS_LIBRARIES}")
    add_compile_definitions(WITH_HIREDIS)
    TARGET_LINK_LIBRARIES(udho INTERFACE ${HIREDIS_LIBRARIES})
    TARGET_INCLUDE_DIRECTORIES(udho INTERFACE ${HIREDIS_INCLUDE_DIRS})
ELSE()
    message(STATUS "Hiredis not found")
ENDIF()

function(udho_enable_sanitizers target)
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang"))
        message(WARNING "Sanitizers requested, but unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    set(san_flags "")
    if(UDHO_ENABLE_ASAN)
        list(APPEND san_flags -fsanitize=address)
    endif()
    if(UDHO_ENABLE_UBSAN)
        list(APPEND san_flags -fsanitize=undefined)
    endif()

    if(san_flags)
        # Compile flags
        target_compile_options(${target} PRIVATE ${san_flags} -fno-omit-frame-pointer -g)
        # Link flags (CMake 3.9: LINK_FLAGS)
        foreach(f IN LISTS san_flags)
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${f}")
        endforeach()
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -fno-omit-frame-pointer")
    endif()
endfunction()

udho_enable_sanitizers(udho)

if(UDHO_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(udho PRIVATE -O0 -g --coverage)
        set_property(TARGET udho APPEND_STRING PROPERTY LINK_FLAGS " --coverage")
    else()
        message(WARNING "UDHO_ENABLE_COVERAGE=ON but compiler is not GNU/Clang; skipping coverage flags.")
    endif()
endif()

if(UDHO_ENABLE_COVERAGE)
  find_program(LCOV_EXECUTABLE lcov)
  find_program(GENHTML_EXECUTABLE genhtml)

  if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(LCOV_INFO    "${COVERAGE_DIR}/coverage.info")
    message(STATUS "Coverage: LCOV_INFO='${LCOV_INFO}'")

    set(LCOV_COMMON
        --branch-coverage
        --ignore-errors negative,inconsistent,mismatch,unused
        --rc branch_coverage=1
        --rc geninfo_unexecuted_blocks=1
        --rc check_data_consistency=0
    )

    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory "${COVERAGE_DIR}"
      COMMAND ${LCOV_EXECUTABLE} --directory "${CMAKE_BINARY_DIR}" --zerocounters
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure

      COMMAND ${LCOV_EXECUTABLE} ${LCOV_COMMON}
              --capture
              --directory "${CMAKE_BINARY_DIR}"
              --base-directory "${CMAKE_SOURCE_DIR}"
              --output-file "${LCOV_INFO}"

      COMMAND ${LCOV_EXECUTABLE} ${LCOV_COMMON}
              --remove "${LCOV_INFO}"
              "*/vendor/*" "*/tests/*" "*/tests2/*" "*/build/*"
              "/usr/*" "*/usr/include/*" "*/include/c++/*"
              --output-file "${LCOV_INFO}"

      COMMAND ${GENHTML_EXECUTABLE} "${LCOV_INFO}"
              --ignore-errors inconsistent,corrupt
              --branch-coverage
              --output-directory "${COVERAGE_DIR}"
              --title "udho coverage"
              --show-details

      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endif()
endif()


if(UDHO_BUILD_TESTS)
   enable_testing()
   # add_subdirectory(tests)
   add_subdirectory(tests2)
endif()

if(UNIX AND APPLE)
    target_link_libraries(udho INTERFACE "-framework Security")
    target_link_libraries(udho INTERFACE "-framework CoreServices")
endif()

add_subdirectory(docs)
