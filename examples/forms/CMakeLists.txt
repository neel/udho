cmake_minimum_required(VERSION 3.19)
project(udho-examples-forms)

udho_rc(forms)
udho_rc_view(forms home views/home.view)
udho_rc_view(forms result views/result.view)
udho_rc_css(forms forms.css assets/forms.css)
udho_rc_js(forms forms.js assets/forms.js)

add_executable(udho-examples-forms main.cpp actions.cpp)
target_compile_features(udho-examples-forms PRIVATE cxx_std_17)
target_link_libraries(udho-examples-forms PRIVATE udho udho_rc::forms)
