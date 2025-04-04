cmake_minimum_required(VERSION 3.9)

project(udho-tests2-view LANGUAGES CXX VERSION 0.0.1)

add_catch_test(udho-test-view-parser parser.cpp)
add_catch_test(udho-test-view-data-meta meta.cpp)
add_catch_test(udho-test-view-lua-mt lua-mt.cpp)
add_catch_test(udho-test-view-lua lua.cpp)
add_catch_test(udho-test-view-resources resources.cpp)
add_catch_test(udho-test-view-assets assets.cpp)
add_catch_test(udho-test-view-layout layout.cpp)
add_catch_test(udho-test-view-layout-placeholders placeholders.cpp)
add_catch_test(udho-test-view-layout-loaders loader.cpp)

if(UDHO_USE_NLOHMANN_JSON)
    find_package(nlohmann_json)
    add_catch_test(udho-test-view-lua-context context.cpp)
    target_link_libraries(udho-test-view-lua-context PRIVATE nlohmann_json::nlohmann_json)
endif()
