cmake_minimum_required(VERSION 3.9)

project(udho-tests2 LANGUAGES CXX VERSION 0.0.1)

find_package(Boost COMPONENTS regex system REQUIRED)
find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

if(Catch2_VERSION_MAJOR LESS 3)
    add_definitions(-DWITH_CATCH_VERSION_2)
endif()

# Function to create a test executable and register it
function(add_catch_test TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
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
