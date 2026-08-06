# Public module boundaries are the immediate directories below includes/udho.
# _deleted is intentionally absent because it is not a public module.
set(UDHO_DOCUMENTATION_MODULES
    activities
    cookies
    core
    db
    exceptions
    hazo
    logging
    manifold
    net
    pages
    pretty
    session
    url
    utils
    view
    www
)

# Documentation targets are available whenever Doxygen is installed. Doxygen
# remains optional and documentation is never part of the default build.
find_package(Doxygen QUIET)
if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen not found; documentation targets are unavailable")
    return()
endif()

set(UDHO_DOXYGEN_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.wrapper.in")
set(UDHO_DOXYGEN_SHARED_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
set(UDHO_DOXYGEN_DIAGRAM_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.diagrams")
set(UDHO_DOXYGEN_CLANG_DATABASE_PATH "${CMAKE_BINARY_DIR}")
set(UDHO_DOXYGEN_TAG_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.tags")
set(UDHO_DOXYGEN_HTML_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.html")
include(UdhoDoxygen)

get_target_property(UDHO_SOURCE_DIR udho SOURCE_DIR)
file(GLOB_RECURSE UDHO_DOCUMENTATION_PAGES "${CMAKE_CURRENT_SOURCE_DIR}/pages/*.md")
file(GLOB_RECURSE UDHO_DOCUMENTATION_PARTS "${CMAKE_CURRENT_SOURCE_DIR}/parts/*.dox")
file(GLOB_RECURSE UDHO_DOCUMENTATION_IMAGES "${CMAKE_CURRENT_SOURCE_DIR}/images/*")

set(UDHO_DOXYGEN_MODULAR_ROOT "${CMAKE_CURRENT_BINARY_DIR}/modular")
set(UDHO_DOXYGEN_TAG_DIR "${UDHO_DOXYGEN_MODULAR_ROOT}/tags")
set(UDHO_DOXYGEN_ROOT_TAG "${UDHO_DOXYGEN_TAG_DIR}/udho.tag")

set(UDHO_PLANTUML_VERSION "1.2026.6")
set(UDHO_PLANTUML_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plantuml")
set(UDHO_PLANTUML_JAR "${UDHO_PLANTUML_DIRECTORY}/plantuml.jar")
set(UDHO_PLANTUML_URL "https://github.com/plantuml/plantuml/releases/download/v${UDHO_PLANTUML_VERSION}/plantuml-${UDHO_PLANTUML_VERSION}.jar")
set(UDHO_DOXYGEN_PLANTUML_JAR_PATH "${UDHO_PLANTUML_DIRECTORY}")
set(UDHO_DOXYGEN_TOOL_DEPENDS "${UDHO_PLANTUML_JAR}")

add_custom_command(
    OUTPUT "${UDHO_PLANTUML_JAR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${UDHO_PLANTUML_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}"
        "-DUDHO_PLANTUML_URL=${UDHO_PLANTUML_URL}"
        "-DUDHO_PLANTUML_OUTPUT=${UDHO_PLANTUML_JAR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/DownloadPlantUML.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/DownloadPlantUML.cmake"
    COMMENT "Downloading PlantUML ${UDHO_PLANTUML_VERSION}"
    VERBATIM
)
add_custom_target(docs-plantuml DEPENDS "${UDHO_PLANTUML_JAR}")

# The root topic is generated first so module-level groups can attach to it.
udho_add_doxygen_target(docs-tag-udho
    DOXYFILE "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.root"
    OVERLAY "${UDHO_DOXYGEN_TAG_CONFIG}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    BUILD_OUTPUT_DIRECTORY "${UDHO_DOXYGEN_MODULAR_ROOT}/root"
    STRIP_FROM_PATH "${UDHO_SOURCE_DIR}"
    STRIP_FROM_INC_PATH "${UDHO_SOURCE_DIR}/includes"
    TAG_FILE "${UDHO_DOXYGEN_ROOT_TAG}"
    OUTPUTS
        "${UDHO_DOXYGEN_MODULAR_ROOT}/root/xml/index.xml"
        "${UDHO_DOXYGEN_ROOT_TAG}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/index.dox"
        "${CMAKE_CURRENT_SOURCE_DIR}/module-groups.dox"
        "${UDHO_DOXYGEN_SHARED_CONFIG}"
        "${CMAKE_BINARY_DIR}/compile_commands.json"
)

# Phase one creates tags and per-module XML without rendering HTML or consuming
# external tags.  These targets depend only on their own headers, which is the
# key to module-local incremental rebuilds.
set(UDHO_DOXYGEN_TAG_TARGETS)
set(UDHO_DOXYGEN_TAG_FILES)
foreach(module IN LISTS UDHO_DOCUMENTATION_MODULES)
    set(module_source_dir "${UDHO_SOURCE_DIR}/includes/udho/${module}")
    set(module_config "${module_source_dir}/Doxyfile")
    file(GLOB_RECURSE module_sources CONFIGURE_DEPENDS
        "${module_source_dir}/*.h"
        "${module_source_dir}/*.dox"
        "${module_source_dir}/*.md")
    set(UDHO_DOCUMENTATION_SOURCES_${module} ${module_sources})

    set(tag_target "docs-tag-${module}")
    set(tag_file "${UDHO_DOXYGEN_TAG_DIR}/${module}.tag")
    udho_add_doxygen_target(${tag_target}
        DOXYFILE "${module_config}"
        OVERLAY "${UDHO_DOXYGEN_TAG_CONFIG}"
        WORKING_DIRECTORY "${module_source_dir}"
        BUILD_OUTPUT_DIRECTORY "${UDHO_DOXYGEN_MODULAR_ROOT}/modules/${module}"
        STRIP_FROM_PATH "${UDHO_SOURCE_DIR}/includes"
        STRIP_FROM_INC_PATH "${UDHO_SOURCE_DIR}/includes"
        TAG_FILE "${tag_file}"
        TAG_MAPPINGS "${UDHO_DOXYGEN_ROOT_TAG}"
        ORDER_DEPENDS docs-tag-udho
        OUTPUTS
            "${UDHO_DOXYGEN_MODULAR_ROOT}/modules/${module}/xml/index.xml"
            "${tag_file}"
        DEPENDS
            ${module_sources}
            "${UDHO_DOXYGEN_SHARED_CONFIG}"
            "${UDHO_DOXYGEN_ROOT_TAG}"
            "${CMAKE_BINARY_DIR}/compile_commands.json"
    )
    list(APPEND UDHO_DOXYGEN_TAG_TARGETS ${tag_target})
    list(APPEND UDHO_DOXYGEN_TAG_FILES "${tag_file}")
endforeach()

add_custom_target(docs-tags)
add_dependencies(docs-tags docs-tag-udho ${UDHO_DOXYGEN_TAG_TARGETS})

# Phase two reparses each module to render HTML with every other module's tag
# file available. This resolves documentation-only references without a
# manually maintained module graph. Tag targets are order-only dependencies, so
# changing one module does not force otherwise unchanged module HTML to run.
set(UDHO_DOXYGEN_MODULE_TARGETS)
foreach(module IN LISTS UDHO_DOCUMENTATION_MODULES)
    set(tag_mappings "${UDHO_DOXYGEN_ROOT_TAG}=../../../pages/html")
    foreach(other_module IN LISTS UDHO_DOCUMENTATION_MODULES)
        if(NOT other_module STREQUAL module)
            list(APPEND tag_mappings "${UDHO_DOXYGEN_TAG_DIR}/${other_module}.tag=../../${other_module}/html")
        endif()
    endforeach()

    set(module_target "docs-module-${module}")
    udho_add_doxygen_target(${module_target}
        DOXYFILE "${UDHO_SOURCE_DIR}/includes/udho/${module}/Doxyfile"
        OVERLAY "${UDHO_DOXYGEN_HTML_CONFIG}"
        WORKING_DIRECTORY "${UDHO_SOURCE_DIR}/includes/udho/${module}"
        BUILD_OUTPUT_DIRECTORY "${UDHO_DOXYGEN_MODULAR_ROOT}/modules/${module}"
        STRIP_FROM_PATH "${UDHO_SOURCE_DIR}/includes"
        STRIP_FROM_INC_PATH "${UDHO_SOURCE_DIR}/includes"
        TAG_MAPPINGS ${tag_mappings}
        OUTPUTS "${UDHO_DOXYGEN_MODULAR_ROOT}/modules/${module}/html/index.html"
        ORDER_DEPENDS ${UDHO_DOXYGEN_TAG_TARGETS}
        DEPENDS
            ${UDHO_DOCUMENTATION_SOURCES_${module}}
            "${UDHO_DOXYGEN_SHARED_CONFIG}"
            "${CMAKE_BINARY_DIR}/compile_commands.json"
    )
    list(APPEND UDHO_DOXYGEN_MODULE_TARGETS ${module_target})
endforeach()

add_custom_target(docs-modules)
add_dependencies(docs-modules ${UDHO_DOXYGEN_MODULE_TARGETS})

# Hand-written pages get their own fast Doxygen invocation.  It sees API
# symbols only through tags and never parses the public-header tree itself.
set(page_tag_mappings)
foreach(module IN LISTS UDHO_DOCUMENTATION_MODULES)
    list(APPEND page_tag_mappings
        "${UDHO_DOXYGEN_TAG_DIR}/${module}.tag=../../modules/${module}/html")
endforeach()
list(APPEND page_tag_mappings "${UDHO_DOXYGEN_ROOT_TAG}")

udho_add_doxygen_target(docs-pages
    DOXYFILE "${CMAKE_CURRENT_SOURCE_DIR}/pages/Doxyfile"
    OVERLAY "${UDHO_DOXYGEN_HTML_CONFIG}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pages"
    BUILD_OUTPUT_DIRECTORY "${UDHO_DOXYGEN_MODULAR_ROOT}/pages"
    STRIP_FROM_PATH "${UDHO_SOURCE_DIR}"
    STRIP_FROM_INC_PATH "${UDHO_SOURCE_DIR}/includes"
    TAG_MAPPINGS ${page_tag_mappings}
    OUTPUTS "${UDHO_DOXYGEN_MODULAR_ROOT}/pages/html/index.html"
    ORDER_DEPENDS ${UDHO_DOXYGEN_MODULE_TARGETS}
    DEPENDS
        ${UDHO_DOCUMENTATION_PAGES}
        ${UDHO_DOCUMENTATION_PARTS}
        ${UDHO_DOCUMENTATION_IMAGES}
        "${UDHO_DOXYGEN_ROOT_TAG}"
        ${UDHO_DOXYGEN_TAG_FILES}
        "${UDHO_DOXYGEN_SHARED_CONFIG}"
        "${CMAKE_CURRENT_SOURCE_DIR}/index.dox"
        "${UDHO_SOURCE_DIR}/README.md"
)

add_custom_target(docs)
add_dependencies(docs docs-pages)
