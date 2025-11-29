cmake_minimum_required(VERSION 3.9)

project(udho-tests-url LANGUAGES CXX VERSION 0.0.1)

find_package(CURL REQUIRED)
find_package(Boost REQUIRED)

function(target_enable_symbol_export target)
    # Determine target type
    get_target_property(target_type ${target} TYPE)

    if(UNIX OR APPLE)
        # Always hide symbols by default for libraries/executables
        target_compile_options(${target} PRIVATE -fvisibility=hidden)

        # Handle executables needing symbol export
        if(target_type STREQUAL "EXECUTABLE")
            if(APPLE)
                target_link_options(${target} PRIVATE -Wl,-export_dynamic)
            else()
                target_link_options(${target} PRIVATE -rdynamic)
            endif()
        endif()
    endif()

    # Add Boost headers if using Boost's symbol export
    if(TARGET Boost::headers)
        target_link_libraries(${target} PRIVATE Boost::headers)
    endif()
endfunction()

add_catch_test(udho-test-url-patterns url.cpp)
target_enable_symbol_export(udho-test-url-patterns)

add_catch_test(udho-test-url-options options.cpp)

add_catch_test(udho-test-url-router-asset asset.cpp)
target_link_libraries(udho-test-url-router-asset PRIVATE CURL::libcurl)

add_catch_test(udho-test-url-router-listing url-routes-listing.cpp)
target_enable_symbol_export(udho-test-url-router-listing)
