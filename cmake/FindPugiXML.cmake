# Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# - Try to find PugiXML
# Once done this will define
#  PUGIXML_FOUND - System has PugiXML
#  PUGIXML_INCLUDE_DIRS - The PugiXML include directories
#  PUGIXML_LIBRARIES - The libraries needed to use PugiXML
#  PUGIXML_DEFINITIONS - Compiler switches required for using PugiXML

find_package(PkgConfig)
pkg_check_modules(PC_PUGIXML QUIET pugixml)
set(PUGIXML_DEFINITIONS ${PC_PUGIXML_CFLAGS_OTHER})

find_path(PUGIXML_INCLUDE_DIR pugixml.hpp
          HINTS ${PC_PUGIXML_INCLUDEDIR} ${PC_PUGIXML_INCLUDE_DIRS}
          PATH_SUFFIXES pugixml )

find_library(PUGIXML_LIBRARY NAMES libpugixml.so
             HINTS ${PC_PUGIXML_LIBDIR} ${PC_PUGIXML_LIBRARY_DIRS} )

set(PUGIXML_LIBRARIES ${PUGIXML_LIBRARY} )
set(PUGIXML_INCLUDE_DIRS ${PUGIXML_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PUGIXML_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PugiXML  DEFAULT_MSG
                                  PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)
mark_as_advanced(PUGIXML_INCLUDE_DIR PUGIXML_LIBRARY )
# Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# - Try to find PugiXML
# Once done this will define
#  PUGIXML_FOUND - System has PugiXML
#  PUGIXML_INCLUDE_DIRS - The PugiXML include directories
#  PUGIXML_LIBRARIES - The libraries needed to use PugiXML
#  PUGIXML_DEFINITIONS - Compiler switches required for using PugiXML

find_package(PkgConfig)
pkg_check_modules(PC_PUGIXML QUIET pugixml)
set(PUGIXML_DEFINITIONS ${PC_PUGIXML_CFLAGS_OTHER})

find_path(PUGIXML_INCLUDE_DIR pugixml.hpp
          HINTS ${PC_PUGIXML_INCLUDEDIR} ${PC_PUGIXML_INCLUDE_DIRS}
          PATH_SUFFIXES pugixml )

find_library(PUGIXML_LIBRARY NAMES 
             HINTS ${PC_PUGIXML_LIBDIR} ${PC_PUGIXML_LIBRARY_DIRS} )

set(PUGIXML_LIBRARIES ${PUGIXML_LIBRARY} )
set(PUGIXML_INCLUDE_DIRS ${PUGIXML_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PUGIXML_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PugiXML  DEFAULT_MSG
                                  PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)
mark_as_advanced(PUGIXML_INCLUDE_DIR PUGIXML_LIBRARY )
