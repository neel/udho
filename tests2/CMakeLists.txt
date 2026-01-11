cmake_minimum_required(VERSION 3.9)

project(udho-tests2 LANGUAGES CXX VERSION 0.0.1)

find_package(Boost COMPONENTS regex REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

if(Catch2_VERSION_MAJOR LESS 3)
    add_definitions(-DWITH_CATCH_VERSION_2)
endif()

# Coverage flags helper (CMake 3.9 compatible)
function(udho_enable_coverage TARGET)
    if(MSVC)
        message(WARNING "Coverage is not configured for MSVC in this setup.")
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${TARGET} PRIVATE -O0 -g --coverage)
        set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_FLAGS " --coverage")
    else()
        message(WARNING "Unknown compiler '${CMAKE_CXX_COMPILER_ID}': coverage flags not applied.")
    endif()
endfunction()

# Function to create a test executable and register it
function(add_catch_test TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    if(UDHO_ENABLE_COVERAGE)
        udho_enable_coverage(${TARGET_NAME})
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE ${Boost_LIBRARIES} Catch2::Catch2WithMain udho)

    get_filename_component(FULL_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}" ABSOLUTE)
    file(RELATIVE_PATH RELATIVE_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${FULL_SOURCE_PATH}")
    get_filename_component(SOURCE_DIR "${RELATIVE_SOURCE_PATH}" DIRECTORY)
    set(RESULTS_DIR "${CMAKE_BINARY_DIR}/results/${SOURCE_DIR}")
    file(MAKE_DIRECTORY ${RESULTS_DIR})
    catch_discover_tests(${TARGET_NAME}
        REPORTER junit
        OUTPUT_DIR ${RESULTS_DIR}
        OUTPUT_PREFIX "${TARGET_NAME}_"
        OUTPUT_SUFFIX ".xml"
        TEST_ARGS "--reporter console"
    )
endfunction()

add_subdirectory(hazo)
add_subdirectory(activities)
add_subdirectory(db)
add_subdirectory(url)
add_subdirectory(net)
add_subdirectory(view)
add_subdirectory(utils)
add_subdirectory(cookies)
add_subdirectory(session)
add_subdirectory(manifold)
