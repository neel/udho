cmake_minimum_required(VERSION 3.19)
project(udho-examples-hello-https)

find_package(Boost COMPONENTS thread REQUIRED)
find_package(Threads REQUIRED)

udho_rc(hello_https)
udho_rc_view(hello_https home views/home.view)
udho_rc_view(hello_https hello views/hello.view)
udho_rc_view(hello_https planet views/planet.view)
udho_rc_css(hello_https hello.css assets/hello.css)

add_executable(udho-examples-hello-https
    main.cpp
    actions.cpp
    data.cpp
)

target_compile_features(udho-examples-hello-https PRIVATE cxx_std_17)
target_link_libraries(udho-examples-hello-https PRIVATE ${Boost_LIBRARIES} udho udho_rc::hello_https)
