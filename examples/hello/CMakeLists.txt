cmake_minimum_required(VERSION 3.19)
project(udho-examples-hello)

find_package(Boost COMPONENTS thread REQUIRED)
find_package(Threads REQUIRED)

udho_rc(hello)
udho_rc_view(hello home views/home.view)
udho_rc_view(hello hello views/hello.view)
udho_rc_view(hello planet views/planet.view)
udho_rc_css(hello hello.css assets/hello.css)

add_executable(udho-examples-hello
    main.cpp
    actions.cpp
    data.cpp
)

target_compile_features(udho-examples-hello PRIVATE cxx_std_17)
target_link_libraries(udho-examples-hello PRIVATE ${Boost_LIBRARIES} udho udho_rc::hello)
