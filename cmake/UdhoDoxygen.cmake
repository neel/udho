include(CMakeParseArguments)

# Add one incremental Doxygen invocation. Doxygen behavior comes from the
# source configuration and overlay; this helper only describes the build graph
# and redirects generated files into the build tree.
function(udho_add_doxygen_target target)
    set(one_value_args DOXYFILE OVERLAY WORKING_DIRECTORY BUILD_OUTPUT_DIRECTORY TAG_FILE STRIP_FROM_PATH)
    set(multi_value_args OUTPUTS DEPENDS TAG_MAPPINGS ORDER_DEPENDS)
    cmake_parse_arguments(UDHO_DOXYGEN "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    foreach(required_arg IN ITEMS DOXYFILE OVERLAY WORKING_DIRECTORY BUILD_OUTPUT_DIRECTORY STRIP_FROM_PATH OUTPUTS)
        if(NOT UDHO_DOXYGEN_${required_arg})
            message(FATAL_ERROR "udho_add_doxygen_target(${target}) requires ${required_arg}")
        endif()
    endforeach()

    set(config_dir "${CMAKE_CURRENT_BINARY_DIR}/doxygen/${target}")
    set(config_file "${config_dir}/Doxyfile")
    set(UDHO_DOXYFILE_SOURCE "${UDHO_DOXYGEN_DOXYFILE}")
    set(UDHO_DOXYFILE_OVERLAY "${UDHO_DOXYGEN_OVERLAY}")
    set(UDHO_DOXYGEN_BUILD_OUTPUT "${UDHO_DOXYGEN_BUILD_OUTPUT_DIRECTORY}")
    set(UDHO_DOXYGEN_BUILD_TAG_FILE "${UDHO_DOXYGEN_TAG_FILE}")

    set(UDHO_DOXYGEN_BUILD_TAG_MAPPINGS "")
    foreach(tag_mapping IN LISTS UDHO_DOXYGEN_TAG_MAPPINGS)
        string(APPEND UDHO_DOXYGEN_BUILD_TAG_MAPPINGS " \"${tag_mapping}\"")
    endforeach()

    file(MAKE_DIRECTORY "${config_dir}")
    configure_file("${UDHO_DOXYGEN_TEMPLATE}" "${config_file}" @ONLY)

    set(stamp_file "${config_dir}/.doxygen.stamp")
    set(output_directories "${UDHO_DOXYGEN_BUILD_OUTPUT_DIRECTORY}")
    foreach(output IN LISTS UDHO_DOXYGEN_OUTPUTS)
        get_filename_component(output_directory "${output}" DIRECTORY)
        list(APPEND output_directories "${output_directory}")
    endforeach()
    list(REMOVE_DUPLICATES output_directories)

    add_custom_command(
        OUTPUT "${stamp_file}" ${UDHO_DOXYGEN_OUTPUTS}
        COMMAND "${CMAKE_COMMAND}" -E make_directory ${output_directories}
        COMMAND "${DOXYGEN_EXECUTABLE}" "${config_file}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${stamp_file}"
        COMMENT "Doxygen: generating ${target}"
        WORKING_DIRECTORY "${UDHO_DOXYGEN_WORKING_DIRECTORY}"
        MAIN_DEPENDENCY "${config_file}"
        DEPENDS
            "${UDHO_DOXYGEN_TEMPLATE}"
            "${UDHO_DOXYGEN_DOXYFILE}"
            "${UDHO_DOXYGEN_OVERLAY}"
            ${UDHO_DOXYGEN_DEPENDS}
        VERBATIM
    )

    add_custom_target(${target} DEPENDS "${stamp_file}" ${UDHO_DOXYGEN_OUTPUTS})
    if(UDHO_DOXYGEN_ORDER_DEPENDS)
        add_dependencies(${target} ${UDHO_DOXYGEN_ORDER_DEPENDS})
    endif()
endfunction()
