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

set(UDHO_DOXYGEN_TEMPLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.wrapper.in")
set(UDHO_DOXYGEN_SHARED_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
set(UDHO_DOXYGEN_COMPILE_FLAGS "${CMAKE_CURRENT_SOURCE_DIR}/compile_flags.txt")
set(UDHO_DOXYGEN_TAG_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.tags")
set(UDHO_DOXYGEN_HTML_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.html")
include(UdhoDoxygen)

get_target_property(UDHO_SOURCE_DIR udho SOURCE_DIR)
file(GLOB_RECURSE UDHO_DOCUMENTATION_PAGES "${CMAKE_CURRENT_SOURCE_DIR}/pages/*.md")
file(GLOB_RECURSE UDHO_DOCUMENTATION_PARTS "${CMAKE_CURRENT_SOURCE_DIR}/parts/*.dox")
file(GLOB_RECURSE UDHO_DOCUMENTATION_IMAGES "${CMAKE_CURRENT_SOURCE_DIR}/images/*")

set(UDHO_DOXYGEN_MODULAR_ROOT "${CMAKE_CURRENT_BINARY_DIR}/modular")
set(UDHO_DOXYGEN_TAG_DIR "${UDHO_DOXYGEN_MODULAR_ROOT}/tags")

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
        TAG_FILE "${tag_file}"
        OUTPUTS
            "${UDHO_DOXYGEN_MODULAR_ROOT}/modules/${module}/xml/index.xml"
            "${tag_file}"
        DEPENDS
            ${module_sources}
            "${UDHO_DOXYGEN_SHARED_CONFIG}"
            "${UDHO_DOXYGEN_COMPILE_FLAGS}"
    )
    list(APPEND UDHO_DOXYGEN_TAG_TARGETS ${tag_target})
    list(APPEND UDHO_DOXYGEN_TAG_FILES "${tag_file}")
endforeach()

add_custom_target(docs-tags)
add_dependencies(docs-tags ${UDHO_DOXYGEN_TAG_TARGETS})

# Phase two reparses each module to render HTML with every other module's tag
# file available. This resolves documentation-only references without a
# manually maintained module graph. Tag targets are order-only dependencies, so
# changing one module does not force otherwise unchanged module HTML to run.
set(UDHO_DOXYGEN_MODULE_TARGETS)
foreach(module IN LISTS UDHO_DOCUMENTATION_MODULES)
    set(tag_mappings)
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
        TAG_MAPPINGS ${tag_mappings}
        OUTPUTS "${UDHO_DOXYGEN_MODULAR_ROOT}/modules/${module}/html/index.html"
        ORDER_DEPENDS ${UDHO_DOXYGEN_TAG_TARGETS}
        DEPENDS
            ${UDHO_DOCUMENTATION_SOURCES_${module}}
            "${UDHO_DOXYGEN_SHARED_CONFIG}"
            "${UDHO_DOXYGEN_COMPILE_FLAGS}"
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

udho_add_doxygen_target(docs-pages
    DOXYFILE "${CMAKE_CURRENT_SOURCE_DIR}/pages/Doxyfile"
    OVERLAY "${UDHO_DOXYGEN_HTML_CONFIG}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pages"
    BUILD_OUTPUT_DIRECTORY "${UDHO_DOXYGEN_MODULAR_ROOT}/pages"
    TAG_MAPPINGS ${page_tag_mappings}
    OUTPUTS "${UDHO_DOXYGEN_MODULAR_ROOT}/pages/html/index.html"
    ORDER_DEPENDS ${UDHO_DOXYGEN_MODULE_TARGETS}
    DEPENDS
        ${UDHO_DOCUMENTATION_PAGES}
        ${UDHO_DOCUMENTATION_PARTS}
        ${UDHO_DOCUMENTATION_IMAGES}
        ${UDHO_DOXYGEN_TAG_FILES}
        "${UDHO_DOXYGEN_SHARED_CONFIG}"
        "${CMAKE_CURRENT_SOURCE_DIR}/index.h"
        "${UDHO_SOURCE_DIR}/README.md"
)

add_custom_target(docs)
add_dependencies(docs docs-pages)
