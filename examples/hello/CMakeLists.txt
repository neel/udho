cmake_minimum_required(VERSION 3.19)
project(udho-examples-hello)

udho_rc(hello)
udho_rc_view(hello home views/home.view)
udho_rc_view(hello hello views/hello.view)
udho_rc_view(hello planet views/planet.view)
udho_rc_css(hello hello.css assets/hello.css)

set(HELLO_COMMON_SOURCES
    actions.cpp
    data.cpp
)

add_library(examples-hello-common STATIC ${HELLO_COMMON_SOURCES})
target_compile_features(examples-hello-common PUBLIC cxx_std_17)
target_link_libraries(examples-hello-common PUBLIC udho)

add_executable(udho-examples-hello http.cpp)
target_link_libraries(udho-examples-hello PRIVATE examples-hello-common udho_rc::hello)

add_executable(udho-examples-hello-https https.cpp)
target_link_libraries(udho-examples-hello-https PRIVATE examples-hello-common udho_rc::hello)

configure_file(hello-https-gen.sh hello-https-gen.sh COPYONLY)
