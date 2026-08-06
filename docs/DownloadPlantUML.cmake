if(NOT DEFINED UDHO_PLANTUML_URL OR
   NOT DEFINED UDHO_PLANTUML_OUTPUT)
    message(FATAL_ERROR "PlantUML download parameters are incomplete")
endif()

set(download_file "${UDHO_PLANTUML_OUTPUT}.download")
file(DOWNLOAD
    "${UDHO_PLANTUML_URL}"
    "${download_file}"
    TLS_VERIFY ON
    STATUS download_status
)

list(GET download_status 0 download_result)
list(GET download_status 1 download_message)
if(NOT download_result EQUAL 0)
    file(REMOVE "${download_file}")
    message(FATAL_ERROR "PlantUML download failed: ${download_message}")
endif()

file(RENAME "${download_file}" "${UDHO_PLANTUML_OUTPUT}")
