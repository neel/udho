cmake_minimum_required(VERSION 3.9)
project(udho-docs)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen)

if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_XML_DIR ${CMAKE_CURRENT_BINARY_DIR}/xml)
    set(DOXYGEN_XML_INDEX ${DOXYGEN_XML_DIR}/index.xml)
    get_target_property(UDHO_INCLUDE_DIRS udho UDHO_INCLUDE_DIRS)
    file(GLOB_RECURSE UDHO_PAGES ${CMAKE_CURRENT_SOURCE_DIR}/*.md)
    get_target_property(UDHO_SOURCE_DIR udho SOURCE_DIR)
    get_property(UDHO_HEADERS TARGET udho PROPERTY UDHO_HEADERS)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_command(
        OUTPUT ${DOXYGEN_XML_INDEX}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${DOXYGEN_IN} ${UDHO_HEADERS} ${UDHO_PAGES} ${CMAKE_CURRENT_SOURCE_DIR}/index.h
        MAIN_DEPENDENCY ${DOXYGEN_OUT}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
        USES_TERMINAL
    )
    
    add_custom_target(docs DEPENDS ${DOXYGEN_XML_INDEX})

    # --- Boostbook ---
    find_program(XSLTPROC xsltproc)
    if (XSLTPROC)
        set(BOOSTBOOK_DIR                   "/usr/share/boostbook" CACHE PATH "BoostBook root directory containing xsl/ and dtd/")
        set(BOOSTBOOK_DOXYGEN_XSL_DIR       "${BOOSTBOOK_DIR}/xsl/doxygen")
        set(BOOSTBOOK_COLLECT_XSL           "${BOOSTBOOK_DOXYGEN_XSL_DIR}/collect.xsl")
        set(BOOSTBOOK_DOXYGEN2BOOSTBOOK_XSL "${BOOSTBOOK_DOXYGEN_XSL_DIR}/doxygen2boostbook.xsl")
        set(DOXYGEN_XML_DIR                 "${CMAKE_CURRENT_BINARY_DIR}/xml")
        set(DOXYGEN_XML_INDEX               "${DOXYGEN_XML_DIR}/index.xml")
        set(BOOSTBOOK_WORK_DIR              "${CMAKE_CURRENT_BINARY_DIR}/boostbook")
        set(DOXYGEN_COMBINED                "${BOOSTBOOK_WORK_DIR}/udho.doxygen")
        set(BOOSTBOOK_XML                   "${BOOSTBOOK_WORK_DIR}/udho.xml")
        set(BOOSTBOOK_USER_CONFIG_JAM_IN    ${CMAKE_CURRENT_SOURCE_DIR}/user-config.jam.in)
        set(BOOSTBOOK_USER_CONFIG_JAM       ${BOOSTBOOK_WORK_DIR}/user-config.jam)
        set(BOOSTBOOK_JAMROOT               "${BOOSTBOOK_WORK_DIR}/Jamroot.jam")
        set(BOOSTBOOK_HTML_MANIFEST         "${BOOSTBOOK_WORK_DIR}/html/udho_HTML.manifest")
        set(XSLTPROCX_PATH                  ${CMAKE_CURRENT_SOURCE_DIR}/xsltprocx)

        configure_file(${BOOSTBOOK_USER_CONFIG_JAM_IN} ${BOOSTBOOK_USER_CONFIG_JAM} @ONLY)

        file(MAKE_DIRECTORY "${BOOSTBOOK_WORK_DIR}")
        file(WRITE "${BOOSTBOOK_JAMROOT}" [=[
        using boostbook ;

        boostbook udho
            : udho.xml
            ;
        ]=])

        if (NOT EXISTS "${BOOSTBOOK_COLLECT_XSL}")
            message(FATAL_ERROR "BoostBook collect.xsl not found: ${BOOSTBOOK_COLLECT_XSL}")
        endif()

        if (NOT EXISTS "${BOOSTBOOK_DOXYGEN2BOOSTBOOK_XSL}")
            message(FATAL_ERROR "BoostBook doxygen2boostbook.xsl not found: ${BOOSTBOOK_DOXYGEN2BOOSTBOOK_XSL}")
        endif()

        add_custom_command(
            OUTPUT  "${DOXYGEN_COMBINED}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${BOOSTBOOK_WORK_DIR}"
            COMMAND "${XSLTPROC}" --stringparam doxygen.xml.path "${DOXYGEN_XML_DIR}" -o "${DOXYGEN_COMBINED}" "${BOOSTBOOK_COLLECT_XSL}" "${DOXYGEN_XML_INDEX}"
            DEPENDS
                    "${DOXYGEN_XML_INDEX}"
                    "${BOOSTBOOK_COLLECT_XSL}"
            COMMENT "Collecting Doxygen XML for BoostBook"
            VERBATIM
        )


        add_custom_command(
            OUTPUT  "${BOOSTBOOK_XML}"
            COMMAND "${XSLTPROC}" -o "${BOOSTBOOK_XML}" "${BOOSTBOOK_DOXYGEN2BOOSTBOOK_XSL}" "${DOXYGEN_COMBINED}"
            DEPENDS
                    "${DOXYGEN_COMBINED}"
                    "${BOOSTBOOK_DOXYGEN2BOOSTBOOK_XSL}"
            COMMENT "Converting Doxygen XML to BoostBook XML"
            VERBATIM
        )

        add_custom_target(boostbook-xml DEPENDS "${BOOSTBOOK_XML}")

        find_program(B2 b2)

        if (NOT B2)
            message(FATAL_ERROR "b2 not found")
        endif()

        add_custom_command(
                OUTPUT "${BOOSTBOOK_HTML_MANIFEST}"
                COMMAND "${B2}" "--user-config=${BOOSTBOOK_USER_CONFIG_JAM}" html
                WORKING_DIRECTORY "${BOOSTBOOK_WORK_DIR}"
                DEPENDS
                        "${BOOSTBOOK_XML}"
                        "${BOOSTBOOK_JAMROOT}"
                        "${BOOSTBOOK_USER_CONFIG_JAM}"
                COMMENT "Creating BoostBook HTML"
                VERBATIM
                USES_TERMINAL
        )

        add_custom_target(boostbook DEPENDS "${BOOSTBOOK_HTML_MANIFEST}")
    endif()
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

