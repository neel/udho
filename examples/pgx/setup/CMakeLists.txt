add_executable(udho-examples-pgx-setup main.cpp activities.cpp)
target_compile_features(udho-examples-pgx-setup PRIVATE cxx_std_17)
target_include_directories(udho-examples-pgx-setup PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(udho-examples-pgx-setup PRIVATE ${Boost_LIBRARIES} udho)
