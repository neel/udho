foreach(required IN ITEMS
    UDHO_XSLTPROC
    UDHO_XSLT
    UDHO_MANIFEST
    UDHO_MODULE
    UDHO_MODULE_INDEX
    UDHO_OUTPUT_DIR
    UDHO_STAMP
    UDHO_HTML_DIR
    UDHO_DIAGRAM_PREFIX)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "RenderModule.cmake requires ${required}")
    endif()
endforeach()

# The XML output contains several graph topologies, but call/caller graph data
# is only materialized by Doxygen's HTML renderer. Index the generated SVGs so
# XSLT can conditionally reference both kinds without copying the large asset
# set into the custom output directory.
file(GLOB diagram_assets LIST_DIRECTORIES false "${UDHO_HTML_DIR}/*.svg")
list(SORT diagram_assets)
set(diagram_manifest "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-diagrams.xml")
set(diagram_manifest_xml "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<diagrams>\n")
foreach(diagram IN LISTS diagram_assets)
    get_filename_component(diagram_name "${diagram}" NAME)
    string(APPEND diagram_manifest_xml "  <diagram name=\"${diagram_name}\"/>\n")
endforeach()
string(APPEND diagram_manifest_xml "</diagrams>\n")
file(WRITE "${diagram_manifest}" "${diagram_manifest_xml}")

file(READ "${UDHO_MODULE_INDEX}" index_xml)
get_filename_component(module_xml_dir "${UDHO_MODULE_INDEX}" DIRECTORY)

# Doxygen references identify their targets but assume its own single-page or
# fragment-oriented HTML layout. Build an explicit lookup for the standalone
# pages emitted by this renderer, including the hashed owner key used for class
# member-function pages.
set(link_manifest "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-links.xml")
set(link_manifest_xml "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<links>\n")
set(link_target_ids)
macro(udho_add_link link_refid link_href)
    list(FIND link_target_ids "${link_refid}" link_target_index)
    if(link_target_index EQUAL -1)
        list(APPEND link_target_ids "${link_refid}")
        string(APPEND link_manifest_xml "  <target refid=\"${link_refid}\" href=\"${link_href}\"/>\n")
    endif()
endmacro()

macro(udho_member_suffix member_id output_variable)
    string(LENGTH "${member_id}" member_id_length)
    math(EXPR member_suffix_start "${member_id_length} - 33")
    string(SUBSTRING "${member_id}" ${member_suffix_start} 33 ${output_variable})
endmacro()

string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"[^\"]+\""
    indexed_compounds
    "${index_xml}"
)
foreach(compound IN LISTS indexed_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    string(REGEX REPLACE ".*kind=\"([^\"]+)\".*" "\\1" kind "${compound}")
    if(kind MATCHES "^(class|struct|union|group|file|namespace|dir)$")
        udho_add_link("${refid}" "${UDHO_MODULE}-${refid}.html")
    else()
        udho_add_link("${refid}" "${UDHO_MODULE}.html#${refid}")
    endif()
endforeach()

# Record the namespace that canonically owns each free function. Group and file
# compounds repeat those functions, while the namespace determines the unique
# standalone page name.
set(namespace_member_ids)
set(namespace_member_owners)
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"namespace\""
    link_namespace_compounds
    "${index_xml}"
)
foreach(compound IN LISTS link_namespace_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" namespace_refid "${compound}")
    file(READ "${module_xml_dir}/${namespace_refid}.xml" namespace_xml)
    string(REGEX MATCHALL
        "<member refid=\"[^\"]+\" kind=\"[^\"]+\""
        namespace_members
        "${namespace_xml}"
    )
    string(REGEX MATCHALL
        "<memberdef kind=\"[^\"]+\" id=\"[^\"]+\""
        direct_namespace_members
        "${namespace_xml}"
    )
    foreach(member IN LISTS namespace_members direct_namespace_members)
        if(member MATCHES "refid=\"([^\"]+)\"")
            set(member_id "${CMAKE_MATCH_1}")
        else()
            string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" member_id "${member}")
        endif()
        list(FIND namespace_member_ids "${member_id}" namespace_member_index)
        if(namespace_member_index EQUAL -1)
            list(APPEND namespace_member_ids "${member_id}")
            list(APPEND namespace_member_owners "${namespace_refid}")
        endif()
    endforeach()

    foreach(member IN LISTS direct_namespace_members)
        string(REGEX REPLACE ".*kind=\"([^\"]+)\".*" "\\1" member_kind "${member}")
        string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" member_id "${member}")
        if(member_kind STREQUAL "function" OR member_kind STREQUAL "signal" OR member_kind STREQUAL "slot")
            udho_member_suffix("${member_id}" member_suffix)
            udho_add_link("${member_id}" "${UDHO_MODULE}-free-${namespace_refid}-${member_suffix}.html")
        else()
            udho_add_link("${member_id}" "${UDHO_MODULE}-${namespace_refid}.html#${member_id}")
        endif()
    endforeach()
endforeach()

# Class members take priority because their dedicated function pages include a
# hash of the owning compound. Non-function members remain anchors on the class
# page, as do enumerators nested inside an enum declaration.
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"(class|struct|union)\""
    link_type_compounds
    "${index_xml}"
)
foreach(compound IN LISTS link_type_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    string(SHA256 compound_hash "${refid}")
    string(SUBSTRING "${compound_hash}" 0 16 compound_key)
    file(READ "${module_xml_dir}/${refid}.xml" compound_xml)
    string(REGEX MATCHALL
        "<memberdef kind=\"[^\"]+\" id=\"[^\"]+\""
        compound_members
        "${compound_xml}"
    )
    foreach(member IN LISTS compound_members)
        string(REGEX REPLACE ".*kind=\"([^\"]+)\".*" "\\1" member_kind "${member}")
        string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" member_id "${member}")
        if(member_kind STREQUAL "function" OR member_kind STREQUAL "signal" OR member_kind STREQUAL "slot")
            udho_member_suffix("${member_id}" member_suffix)
            udho_add_link("${member_id}" "${UDHO_MODULE}-member-${compound_key}-${member_suffix}.html")
        else()
            udho_add_link("${member_id}" "${UDHO_MODULE}-${refid}.html#${member_id}")
        endif()
    endforeach()
    string(REGEX MATCHALL
        "<member refid=\"[^\"]+\" kind=\"(function|signal|slot)\""
        referenced_compound_members
        "${compound_xml}"
    )
    foreach(member IN LISTS referenced_compound_members)
        string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" member_id "${member}")
        udho_member_suffix("${member_id}" member_suffix)
        udho_add_link("${member_id}" "${UDHO_MODULE}-member-${compound_key}-${member_suffix}.html")
    endforeach()
    string(REGEX MATCHALL "<enumvalue id=\"[^\"]+\"" enum_values "${compound_xml}")
    foreach(enum_value IN LISTS enum_values)
        string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" enum_id "${enum_value}")
        udho_add_link("${enum_id}" "${UDHO_MODULE}-${refid}.html#${enum_id}")
    endforeach()
endforeach()

# Resolve group definitions before file definitions; Doxygen duplicates grouped
# free declarations in their source-file compounds, but the group is the more
# useful canonical destination for non-function declarations.
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"group\""
    definition_group_compounds
    "${index_xml}"
)
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"file\""
    definition_file_compounds
    "${index_xml}"
)
set(definition_compounds ${definition_group_compounds} ${definition_file_compounds})
foreach(compound IN LISTS definition_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    file(READ "${module_xml_dir}/${refid}.xml" compound_xml)
    string(REGEX MATCHALL
        "<memberdef kind=\"[^\"]+\" id=\"[^\"]+\""
        compound_members
        "${compound_xml}"
    )
    foreach(member IN LISTS compound_members)
        string(REGEX REPLACE ".*kind=\"([^\"]+)\".*" "\\1" member_kind "${member}")
        string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" member_id "${member}")
        if(member_kind STREQUAL "function" OR member_kind STREQUAL "signal" OR member_kind STREQUAL "slot")
            list(FIND namespace_member_ids "${member_id}" namespace_member_index)
            if(NOT namespace_member_index EQUAL -1)
                list(GET namespace_member_owners ${namespace_member_index} namespace_refid)
                udho_member_suffix("${member_id}" member_suffix)
                udho_add_link("${member_id}" "${UDHO_MODULE}-free-${namespace_refid}-${member_suffix}.html")
            else()
                udho_add_link("${member_id}" "${UDHO_MODULE}-${refid}.html#${member_id}")
            endif()
        else()
            udho_add_link("${member_id}" "${UDHO_MODULE}-${refid}.html#${member_id}")
        endif()
    endforeach()
    string(REGEX MATCHALL "<enumvalue id=\"[^\"]+\"" enum_values "${compound_xml}")
    foreach(enum_value IN LISTS enum_values)
        string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" enum_id "${enum_value}")
        udho_add_link("${enum_id}" "${UDHO_MODULE}-${refid}.html#${enum_id}")
    endforeach()
endforeach()

string(APPEND link_manifest_xml "</links>\n")
file(WRITE "${link_manifest}" "${link_manifest_xml}")

function(udho_run_xslt output page_type)
    set(arguments
        --nonet
        --stringparam page-type "${page_type}"
        --stringparam selected-module "${UDHO_MODULE}"
        --stringparam diagram-manifest "${diagram_manifest}"
        --stringparam diagram-prefix "${UDHO_DIAGRAM_PREFIX}"
        --stringparam link-manifest "${link_manifest}"
    )
    if(ARGC GREATER 2)
        list(APPEND arguments --stringparam selected-compound "${ARGV2}")
    endif()
    if(ARGC GREATER 3 AND NOT "${ARGV3}" STREQUAL "")
        list(APPEND arguments --stringparam selected-member "${ARGV3}")
    endif()
    if(ARGC GREATER 4 AND NOT "${ARGV4}" STREQUAL "")
        list(APPEND arguments --stringparam selected-compound-key "${ARGV4}")
    endif()
    if(ARGC GREATER 5 AND NOT "${ARGV5}" STREQUAL "")
        list(APPEND arguments --stringparam selected-owner-ref "${ARGV5}")
    endif()
    if(ARGC GREATER 6 AND NOT "${ARGV6}" STREQUAL "")
        list(APPEND arguments --stringparam selected-owner-name "${ARGV6}")
    endif()
    list(APPEND arguments
        --output "${output}"
        "${UDHO_XSLT}"
        "${UDHO_MANIFEST}"
    )
    execute_process(
        COMMAND "${UDHO_XSLTPROC}" ${arguments}
        RESULT_VARIABLE result
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "xsltproc failed for ${output}:\n${error}")
    endif()
endfunction()

file(MAKE_DIRECTORY "${UDHO_OUTPUT_DIR}")
udho_run_xslt("${UDHO_OUTPUT_DIR}/${UDHO_MODULE}.html" module)

# XSLT 1.0 cannot create multiple result documents. Discover type compounds in
# the Doxygen index and invoke the same stylesheet once for each standalone
# class/struct/union page, including template specializations.
# Render every Doxygen group, including the module root and nested groups, as
# an independent page. Parent/child navigation is derived from innergroup
# references in the XML rather than flattened into the module page.
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"group\""
    group_compounds
    "${index_xml}"
)
foreach(compound IN LISTS group_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    udho_run_xslt(
        "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-${refid}.html"
        group
        "${refid}"
    )
endforeach()

# File compounds get dedicated pages so their complete source listings are
# available on demand without being embedded in the module overview.
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"file\""
    file_compounds
    "${index_xml}"
)
foreach(compound IN LISTS file_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    udho_run_xslt(
        "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-${refid}.html"
        file
        "${refid}"
    )
endforeach()

string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"dir\""
    directory_compounds
    "${index_xml}"
)
foreach(compound IN LISTS directory_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    udho_run_xslt(
        "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-${refid}.html"
        directory
        "${refid}"
    )
endforeach()

string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"(class|struct|union)\""
    type_compounds
    "${index_xml}"
)
foreach(compound IN LISTS type_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    string(SHA256 compound_hash "${refid}")
    string(SUBSTRING "${compound_hash}" 0 16 compound_key)
    udho_run_xslt(
        "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-${refid}.html"
        compound
        "${refid}"
        ""
        "${compound_key}"
    )

    file(READ "${module_xml_dir}/${refid}.xml" compound_xml)
    string(REGEX MATCHALL
        "<memberdef kind=\"(function|signal|slot)\" id=\"[^\"]+\""
        function_members
        "${compound_xml}"
    )
    set(rendered_compound_members)
    foreach(member IN LISTS function_members)
        string(REGEX REPLACE ".*id=\"([^\"]+)\".*" "\\1" member_id "${member}")
        list(APPEND rendered_compound_members "${member_id}")
        udho_member_suffix("${member_id}" member_suffix)
        udho_run_xslt(
            "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-member-${compound_key}-${member_suffix}.html"
            member
            "${refid}"
            "${member_id}"
            "${compound_key}"
            "${refid}"
        )
    endforeach()
    string(REGEX MATCHALL
        "<member refid=\"[^\"]+\" kind=\"(function|signal|slot)\""
        referenced_function_members
        "${compound_xml}"
    )
    foreach(member IN LISTS referenced_function_members)
        string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" member_id "${member}")
        list(FIND rendered_compound_members "${member_id}" already_rendered)
        if(already_rendered EQUAL -1)
            list(APPEND rendered_compound_members "${member_id}")
            if(member_id MATCHES "^(.+)_1[ag][a-f0-9]+$")
                set(member_compound "${CMAKE_MATCH_1}")
            else()
                message(FATAL_ERROR "Cannot determine definition compound for member function ${member_id}")
            endif()
            udho_member_suffix("${member_id}" member_suffix)
            udho_run_xslt(
                "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-member-${compound_key}-${member_suffix}.html"
                member
                "${member_compound}"
                "${member_id}"
                "${compound_key}"
                "${refid}"
            )
        endif()
    endforeach()
endforeach()

# Namespace-level functions use their canonical Doxygen member id. A function
# can also appear in group and file compounds, so namespace compounds are the
# single source used here and duplicate ids are rendered only once.
string(REGEX MATCHALL
    "<compound refid=\"[^\"]+\" kind=\"namespace\""
    namespace_compounds
    "${index_xml}"
)
set(rendered_free_functions)
foreach(compound IN LISTS namespace_compounds)
    string(REGEX REPLACE ".*refid=\"([^\"]+)\".*" "\\1" refid "${compound}")
    udho_run_xslt(
        "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-${refid}.html"
        namespace
        "${refid}"
    )
    file(READ "${module_xml_dir}/${refid}.xml" compound_xml)
    string(REGEX MATCH "<compoundname>([^<]+)</compoundname>" namespace_name_match "${compound_xml}")
    set(namespace_name "${CMAKE_MATCH_1}")
    string(REGEX MATCHALL
        "<member refid=\"[^\"]+\" kind=\"function\""
        free_function_members
        "${compound_xml}"
    )
    string(REGEX MATCHALL
        "<memberdef kind=\"function\" id=\"[^\"]+\""
        direct_free_function_members
        "${compound_xml}"
    )
    list(APPEND free_function_members ${direct_free_function_members})
    foreach(member IN LISTS free_function_members)
        if(member MATCHES "refid=\"([^\"]+)\"")
            set(member_id "${CMAKE_MATCH_1}")
        elseif(member MATCHES "id=\"([^\"]+)\"")
            set(member_id "${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR "Cannot read free-function id from ${member}")
        endif()
        list(FIND rendered_free_functions "${member_id}" already_rendered)
        if(already_rendered EQUAL -1)
            list(APPEND rendered_free_functions "${member_id}")
            string(LENGTH "${member_id}" member_id_length)
            math(EXPR member_suffix_start "${member_id_length} - 33")
            string(SUBSTRING "${member_id}" ${member_suffix_start} 33 member_suffix)
            if(member_id MATCHES "^(.+)_1g[a-f0-9]+$")
                set(member_compound "${CMAKE_MATCH_1}")
            elseif(member_id MATCHES "^(.+)_1a[a-f0-9]+$")
                set(member_compound "${CMAKE_MATCH_1}")
            else()
                message(FATAL_ERROR "Cannot determine compound for free function ${member_id}")
            endif()
            udho_run_xslt(
                "${UDHO_OUTPUT_DIR}/${UDHO_MODULE}-free-${refid}-${member_suffix}.html"
                free-member
                "${member_compound}"
                "${member_id}"
                ""
                "${refid}"
                "${namespace_name}"
            )
        endif()
    endforeach()
endforeach()

file(TOUCH "${UDHO_STAMP}")
