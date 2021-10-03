cmake_minimum_required(VERSION 3.19)
project(udho-docs)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen)
#find_package(Sphinx)

if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_XML_DIR ${CMAKE_CURRENT_BINARY_DIR}/xml)
    set(DOXYGEN_XML_INDEX ${DOXYGEN_XML_DIR}/index.xml)
    get_target_property(UDHO_SOURCE_DIR udho SOURCE_DIR)

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
        
    #if (SPHINX_FOUND)
        #set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source)
        #set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
        #set(SPHINX_INDEX ${SPHINX_BUILD}/index.html)
        #file(GLOB SPHINX_PAGES ${SPHINX_SOURCE}/pages/*.rst)
        
        #add_custom_command(
            #OUTPUT ${SPHINX_INDEX}
            #COMMAND ${SPHINX_EXECUTABLE} -b html -Dbreathe_projects.dhyara=${DOXYGEN_XML_DIR} ${SPHINX_SOURCE} ${SPHINX_BUILD}
            #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            #DEPENDS ${SPHINX_SOURCE}/index.rst ${DOXYGEN_XML_INDEX} ${SPHINX_PAGES}
            #MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
            #COMMENT "Generating documentation with Sphinx"
        #)
        #add_custom_target(sphinx ALL DEPENDS ${SPHINX_INDEX})
    #else (SPHINX_FOUND)
        #message("Sphinx need to be installed to generate the sphinx documentation")
    #endif (SPHINX_FOUND)
        
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)


#set(CLDOC /home/neel/.espressif/python_env/idf4.4_py3.9_env/bin/cldoc CACHE STRING "User specified '/path/to/cldoc'")
#if(CLDOC STREQUAL "")
	#find_program(CLDOC cldoc)
#elseif(NOT EXISTS ${CLDOC})
	#message(FATAL_ERROR "CLDOC was set to '${CLDOC}' but does not exist")
#endif()

#set(HEADERS
    #${CMAKE_SOURCE_DIR}/includes/udho/activities.h
    #${CMAKE_SOURCE_DIR}/includes/udho/parser.h
    #${CMAKE_SOURCE_DIR}/includes/udho/watcher.h
    #${CMAKE_SOURCE_DIR}/includes/udho/session.h
    #${CMAKE_SOURCE_DIR}/includes/udho/server.h
    #${CMAKE_SOURCE_DIR}/includes/udho/context.h
    #${CMAKE_SOURCE_DIR}/includes/udho/defs.h
    #${CMAKE_SOURCE_DIR}/includes/udho/attachment.h
    #${CMAKE_SOURCE_DIR}/includes/udho/router.h
    #${CMAKE_SOURCE_DIR}/includes/udho/listener.h
    #${CMAKE_SOURCE_DIR}/includes/udho/client.h
    #${CMAKE_SOURCE_DIR}/includes/udho/compositors.h
    #${CMAKE_SOURCE_DIR}/includes/udho/url.h
    #${CMAKE_SOURCE_DIR}/includes/udho/scope.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/activities.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/states.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/activity.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/combinator.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/start.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/joined.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/result_data.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/after.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/require.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/result.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/conveniance.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/fwd.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/perform.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/data.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/subtask.h
    #${CMAKE_SOURCE_DIR}/includes/udho/activities/analyzer.h
    #${CMAKE_SOURCE_DIR}/includes/udho/logging.h
    #${CMAKE_SOURCE_DIR}/includes/udho/bridge.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/seq.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/operations.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/basic.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/helpers.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/hana.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/io.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/fwd.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/tag.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/seq/proxy.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/basic.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/capsule.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/node.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/encapsulate.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/io.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/fwd.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/tag.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node/proxy.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/node.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/detail/has_member.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/detail/has_member_type.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/detail/indices.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/detail/extraction_helper.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/detail/call_helper.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/detail/is_streamable.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/hana.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/hazo.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/rest_of.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/append.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/first_of.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/flatten.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/eliminate.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/fwd.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/operations/prepend.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/operations.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/basic.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/helpers.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/hana.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/io.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/fwd.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/element.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/tag.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/map.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo/map/proxy.h
    #${CMAKE_SOURCE_DIR}/includes/udho/hazo.h
    #${CMAKE_SOURCE_DIR}/includes/udho/visitor.h
    #${CMAKE_SOURCE_DIR}/includes/udho/access.h
    #${CMAKE_SOURCE_DIR}/includes/udho/configuration.h
    #${CMAKE_SOURCE_DIR}/includes/udho/cache.h
    #${CMAKE_SOURCE_DIR}/includes/udho/util.h
    #${CMAKE_SOURCE_DIR}/includes/udho/cookie.h
    #${CMAKE_SOURCE_DIR}/includes/udho/page.h
    #${CMAKE_SOURCE_DIR}/includes/udho/contexts.h
    #${CMAKE_SOURCE_DIR}/includes/udho/application.h
    #${CMAKE_SOURCE_DIR}/includes/udho/forms.h
    #${CMAKE_SOURCE_DIR}/includes/udho/connection.h
#)

#if(NOT ${CLDOC} STREQUAL "CLDOC-NOTFOUND")
  #set(CLDOC_OUT ${CMAKE_CURRENT_BINARY_DIR}/cldoc-docs)
  
  #message(STATUS "${CLDOC} generate ${CMAKE_CXX_FLAGS} -Iincludes -Ideps/ctti/include -Ideps/certify/include -I/usr/include/c++/11.1.0 -I/usr/include/c++/11.1.0/x86_64-pc-linux-gnu -I/usr/local/include -I/usr/lib/clang/12.0.1/include -I/usr/include -- --output ${CLDOC_OUT} ${HEADERS}")
  
  #add_custom_target(cldoc COMMAND
    #${CLDOC} generate ${CMAKE_CXX_FLAGS} -Iincludes -Ideps/ctti/include -Ideps/certify/include -I/usr/include/c++/11.1.0 -I/usr/include/c++/11.1.0/x86_64-pc-linux-gnu -I/usr/local/include -I/usr/lib/clang/12.0.1/include -I/usr/include -- --output ${CLDOC_OUT} ${HEADERS}
    #WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  #)

  #add_custom_target(serve_cldoc COMMAND
    #${CLDOC} serve -- ${CLDOC_OUT}
    #WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  #)

  #add_dependencies(serve_cldoc cldoc)
#else()
  #message(STATUS "cldoc Documentation will not be generated")
#endif()
