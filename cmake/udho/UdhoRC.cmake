include_guard(GLOBAL)

if(NOT DEFINED UDHO_CMAKE_DIR)
    set(UDHO_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

function(udho_rc BUNDLE)
    if(NOT BUNDLE MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "udho_rc: bundle name '${BUNDLE}' is not a valid C++ identifier")
    endif()
    get_property(_declared GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_DECLARED")
    if(_declared)
        message(FATAL_ERROR "udho_rc: bundle '${BUNDLE}' already exists")
    endif()

    foreach(_t IN ITEMS view css js txt img)
        set_property(GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_${_t}_NAME" "")
        set_property(GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_${_t}_PATH" "")
    endforeach()
    set_property(GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_DECLARED" TRUE)

    add_library(udho_rc_${BUNDLE} OBJECT)
    add_library(udho_rc::${BUNDLE} ALIAS udho_rc_${BUNDLE})
    target_link_libraries(udho_rc_${BUNDLE} PRIVATE udho)
    set_target_properties(udho_rc_${BUNDLE} PROPERTIES
        CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON)

    set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")   # capture now
    cmake_language(EVAL CODE
        "cmake_language(DEFER CALL _udho_rc_finalize [[${BUNDLE}]] [[${_bindir}]])")
endfunction()

function(udho_rc_view BUNDLE RESNAME FILE)
    _udho_rc_add("${BUNDLE}" view "${RESNAME}" "${FILE}")
endfunction()
function(udho_rc_js BUNDLE RESNAME FILE)
    _udho_rc_add("${BUNDLE}" js "${RESNAME}" "${FILE}")
endfunction()
function(udho_rc_css BUNDLE RESNAME FILE)
    _udho_rc_add("${BUNDLE}" css "${RESNAME}" "${FILE}")
endfunction()
function(udho_rc_txt BUNDLE RESNAME FILE)
    _udho_rc_add("${BUNDLE}" txt "${RESNAME}" "${FILE}")
endfunction()
function(udho_rc_img BUNDLE RESNAME FILE)
    _udho_rc_add("${BUNDLE}" img "${RESNAME}" "${FILE}")
endfunction()

function(_udho_rc_add BUNDLE TYPE RESNAME FILE)
    get_property(_ok GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_DECLARED")
    if(NOT _ok)
        message(FATAL_ERROR "udho_rc_*: bundle '${BUNDLE}' not declared")
    endif()

    if(NOT IS_ABSOLUTE "${FILE}")
        set(_abs "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}")
    else()
        set(_abs "${FILE}")
    endif()

    file(TO_CMAKE_PATH "${_abs}" _abs)

    if(NOT EXISTS "${_abs}")
        message(FATAL_ERROR "udho_rc_*: file not found: '${_abs}'")
    endif()

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_abs}")

    # one slot per (bundle, type) — path stored alongside name so the
    # generator can pair them by index.
    set_property(GLOBAL APPEND PROPERTY "UDHO_RC_${BUNDLE}_${TYPE}_NAME" "${RESNAME}")
    set_property(GLOBAL APPEND PROPERTY "UDHO_RC_${BUNDLE}_${TYPE}_PATH" "${_abs}")
endfunction()

function(_udho_rc_finalize BUNDLE BINDIR)
    get_property(_ok GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_DECLARED")
    if(NOT _ok)
        return()
    endif()

    set(_dir      "${BINDIR}/udho_rc")
    set(_cpp      "${_dir}/${BUNDLE}.cpp")
    set(_manifest "${_dir}/${BUNDLE}.manifest.cmake")
    set(_gen      "${UDHO_CMAKE_DIR}/UdhoGen.cmake")
    file(MAKE_DIRECTORY "${_dir}")

    # Flatten the five (NAME, PATH) list pairs into three parallel lists:
    #   types[i], names[i], paths[i]
    set(_m "set(UDHO_RC_TYPES)\nset(UDHO_RC_NAMES)\nset(UDHO_RC_PATHS)\n")
    set(_deps "")

    foreach(_t IN ITEMS view css js txt img)
        get_property(_names GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_${_t}_NAME")
        get_property(_paths GLOBAL PROPERTY "UDHO_RC_${BUNDLE}_${_t}_PATH")
        if(NOT _names)
            continue()
        endif()
        list(LENGTH _names _n)
        math(EXPR _last "${_n} - 1")
        foreach(_i RANGE 0 ${_last})
            list(GET _names ${_i} _rn)
            list(GET _paths ${_i} _rp)
            string(APPEND _m
                "list(APPEND UDHO_RC_TYPES \"${_t}\")\n"
                "list(APPEND UDHO_RC_NAMES \"${_rn}\")\n"
                "list(APPEND UDHO_RC_PATHS \"${_rp}\")\n")
            list(APPEND _deps "${_rp}")
        endforeach()
    endforeach()
    file(WRITE "${_manifest}" "${_m}")

    add_custom_command(
        OUTPUT  "${_cpp}"
        COMMAND "${CMAKE_COMMAND}"
                "-DUDHO_RC_MANIFEST=${_manifest}"
                "-DUDHO_RC_BUNDLE=${BUNDLE}"
                "-DUDHO_RC_OUTPUT=${_cpp}"
                -P "${_gen}"
        DEPENDS ${_deps} "${_manifest}" "${_gen}"
        COMMENT "udho: generating bundle '${BUNDLE}'"
        VERBATIM
    )

    target_sources(udho_rc_${BUNDLE} PRIVATE "${_cpp}")
endfunction()
