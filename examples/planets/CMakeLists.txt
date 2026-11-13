cmake_minimum_required(VERSION 3.19)
project(udho-examples-planets)

udho_rc(planets)
udho_rc_view(planets home views/home.view)
udho_rc_view(planets hello views/hello.view)
udho_rc_view(planets planet views/planet.view)
udho_rc_css(planets common.css assets/common.css)
udho_rc_css(planets planets.css assets/planets.css)
udho_rc_css(planets planet.css assets/planet.css)

set(PLANETS_COMMON_SOURCES
    actions.cpp
    data.cpp
)

add_library(examples-planets-common STATIC ${PLANETS_COMMON_SOURCES})
target_compile_features(examples-planets-common PUBLIC cxx_std_17)
target_link_libraries(examples-planets-common PUBLIC udho)

add_executable(udho-examples-planets http.cpp)
target_link_libraries(udho-examples-planets PRIVATE examples-planets-common udho_rc::planets)

add_executable(udho-examples-planets-https https.cpp)
target_link_libraries(udho-examples-planets-https PRIVATE examples-planets-common udho_rc::planets)

configure_file(planets-https-gen.sh planets-https-gen.sh COPYONLY)
