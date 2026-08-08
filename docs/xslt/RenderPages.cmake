foreach(required IN ITEMS
    UDHO_XSLTPROC
    UDHO_XSLT
    UDHO_MANIFEST
    UDHO_PAGE_INDEX
    UDHO_OUTPUT_DIR
    UDHO_STAMP
    UDHO_PAGE_ASSET_PREFIX)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "RenderPages.cmake requires ${required}")
    endif()
endforeach()

file(READ "${UDHO_PAGE_INDEX}" page_index_xml)
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"page\""
    page_compounds
    "${page_index_xml}"
)

foreach(compound IN LISTS page_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    execute_process(
        COMMAND "${UDHO_XSLTPROC}"
            --nonet
            --stringparam page-type page
            --stringparam selected-compound "${refid}"
            --stringparam page-asset-prefix "${UDHO_PAGE_ASSET_PREFIX}"
            --output "${UDHO_OUTPUT_DIR}/${refid}.html"
            "${UDHO_XSLT}"
            "${UDHO_MANIFEST}"
        RESULT_VARIABLE result
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "xsltproc failed for page ${refid}:\n${error}")
    endif()
endforeach()

file(TOUCH "${UDHO_STAMP}")
