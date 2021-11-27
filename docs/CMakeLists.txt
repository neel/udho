cmake_minimum_required(VERSION 3.9)
project(udho-docs)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen)
find_package(Sphinx)

if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_XML_DIR ${CMAKE_CURRENT_BINARY_DIR}/xml)
    set(DOXYGEN_XML_INDEX ${DOXYGEN_XML_DIR}/index.xml)
    get_target_property(UDHO_INCLUDE_DIRS udho UDHO_INCLUDE_DIRS)
    set(FULL_INCLUDE_COMPILER_STRING )
    foreach(dir ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES} ${CMAKE_C_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES} ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES} ${CMAKE_CXX_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES} ${UDHO_INCLUDE_DIRS})
        set(FULL_INCLUDE_COMPILER_STRING ${FULL_INCLUDE_COMPILER_STRING} -I${dir})
    endforeach()
    get_target_property(UDHO_SOURCE_DIR udho SOURCE_DIR)
    get_property(UDHO_HEADERS TARGET udho PROPERTY UDHO_HEADERS)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_command(
        OUTPUT ${DOXYGEN_XML_INDEX} 
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    add_custom_target(docs DEPENDS ${DOXYGEN_XML_INDEX})
        
    if (SPHINX_FOUND)
        set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source)
        set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
        set(SPHINX_INDEX ${SPHINX_BUILD}/index.html)
        file(GLOB SPHINX_PAGES ${SPHINX_SOURCE}/pages/*.rst)
        
        add_custom_command(
            OUTPUT ${SPHINX_INDEX}
            COMMAND ${SPHINX_EXECUTABLE} -b html -Dbreathe_projects.dhyara=${DOXYGEN_XML_DIR} ${SPHINX_SOURCE} ${SPHINX_BUILD}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${SPHINX_SOURCE}/index.rst ${DOXYGEN_XML_INDEX} ${SPHINX_PAGES}
            MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
            COMMENT "Generating documentation with Sphinx"
        )
        add_custom_target(sphinx DEPENDS ${SPHINX_INDEX})
    else (SPHINX_FOUND)
        message("Sphinx need to be installed to generate the sphinx documentation")
    endif (SPHINX_FOUND)
        
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

# See https://github.com/jessevdk/cldoc/issues/141
set(CLDOC /home/neel/.local/bin/cldoc CACHE STRING "User specified '/path/to/cldoc'")
if(CLDOC STREQUAL "")
	find_program(CLDOC cldoc)
elseif(NOT EXISTS ${CLDOC})
	message(FATAL_ERROR "CLDOC was set to '${CLDOC}' but does not exist")
endif()

set(HEADERS ${UDHO_HEADERS})

if(NOT ${CLDOC} STREQUAL "CLDOC-NOTFOUND")
  set(CLDOC_OUT ${CMAKE_CURRENT_BINARY_DIR}/cldoc-docs)
  
  string(STRIP "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}" UDHO_COMPILER_GLAGS)

  message(STATUS "${CLDOC} generate -std=c++17 ${UDHO_COMPILER_GLAGS}
    -DBOOST_BEAST_USE_POSIX_FILE=ON -DBOOST_BEAST_USE_POSIX_FADVISE=ON
    -I/usr/lib/clang/13.0.0/include
    ${FULL_INCLUDE_COMPILER_STRING}
    -- 
    --output ${CLDOC_OUT} 
    ${HEADERS}"
  )
  
  add_custom_target(cldoc COMMAND
    ${CLDOC} generate -std=c++17 ${UDHO_COMPILER_GLAGS} -ferror-limit=0
        -DBOOST_BEAST_USE_POSIX_FILE=ON -DBOOST_BEAST_USE_POSIX_FADVISE=ON -DBOOST_HANA_CONFIG_ENABLE_STRING_UDL=ON
        -I/usr/lib/clang/13.0.0/include
        ${FULL_INCLUDE_COMPILER_STRING} 
        --  
        --output ${CLDOC_OUT} 
        ${HEADERS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  add_custom_target(serve_cldoc COMMAND
    ${CLDOC} serve -- ${CLDOC_OUT}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  add_dependencies(serve_cldoc cldoc)
else()
  message(STATUS "cldoc Documentation will not be generated")
endif()
