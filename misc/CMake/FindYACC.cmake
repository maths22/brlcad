#                  F I N D Y A C C . C M A K E
# BRL-CAD
#
# Copyright (c) 2010-2013 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###
# - Find yacc executable and provides macros to generate custom build rules
# The module defines the following variables
#
#  YACC_EXECUTABLE - path to the yacc program
#  YACC_FOUND - true if the program was found
#
# If yacc is found, the module defines the macros:
#  YACC_TARGET(<Name> <YaccInput> <CodeOutput> [VERBOSE <file>]
#              [COMPILE_FLAGS <string>])
# which will create  a custom rule to generate  a parser. <YaccInput> is
# the path to  a yacc file. <CodeOutput> is the name  of the source file
# generated by yacc.  A header file is also  be generated, and contains
# the  token  list.  If  COMPILE_FLAGS  option is  specified,  the  next
# parameter is  added in the yacc command line.  if  VERBOSE option is
# specified, <file> is created  and contains verbose descriptions of the
# grammar and parser. The macro defines a set of variables:
#  YACC_${Name}_DEFINED - true is the macro ran successfully
#  YACC_${Name}_INPUT - The input source file, an alias for <YaccInput>
#  YACC_${Name}_OUTPUT_SOURCE - The source file generated by yacc
#  YACC_${Name}_OUTPUT_HEADER - The header file generated by yacc
#  YACC_${Name}_OUTPUTS - The sources files generated by yacc
#  YACC_${Name}_COMPILE_FLAGS - Options used in the yacc command line
#
#  ====================================================================
#  Example:
#
#   find_package(YACC)
#   YACC_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
#   add_executable(Foo main.cpp ${YACC_MyParser_OUTPUTS})
#  ====================================================================
#
#=============================================================================
# Copyright (c) 2010-2013 United States Government as represented by
#                the U.S. Army Research Laboratory.
# Copyright 2009 Kitware, Inc.
# Copyright 2006 Tristan Carel
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * The names of the authors may not be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================


find_program(YACC_EXECUTABLE bison DOC "path to the yacc executable")
if(NOT YACC_EXECUTABLE)
  find_program(YACC_EXECUTABLE yacc DOC "path to the yacc executable")
endif(NOT YACC_EXECUTABLE)
mark_as_advanced(YACC_EXECUTABLE)

if(YACC_EXECUTABLE)
  # internal macro
  macro(YACC_TARGET_option_verbose Name YaccOutput filename)
    list(APPEND YACC_TARGET_cmdopt "--verbose")
    get_filename_component(YACC_TARGET_output_path "${YaccOutput}" PATH)
    get_filename_component(YACC_TARGET_output_name "${YaccOutput}" NAME_WE)
    add_custom_command(OUTPUT ${filename}
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy
      "${YACC_TARGET_output_path}/${YACC_TARGET_output_name}.output"
      "${filename}"
      DEPENDS
      "${YACC_TARGET_output_path}/${YACC_TARGET_output_name}.output"
      COMMENT "[YACC][${Name}] Copying yacc verbose table to ${filename}"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(YACC_${Name}_VERBOSE_FILE ${filename})
    list(APPEND YACC_TARGET_extraoutputs
      "${YACC_TARGET_output_path}/${YACC_TARGET_output_name}.output")
  endmacro(YACC_TARGET_option_verbose)

  # internal macro
  macro(YACC_TARGET_option_extraopts Options)
    set(YACC_TARGET_extraopts "${Options}")
    SEPARATE_ARGUMENTS(YACC_TARGET_extraopts)
    list(APPEND YACC_TARGET_cmdopt ${YACC_TARGET_extraopts})
  endmacro(YACC_TARGET_option_extraopts)

  #============================================================
  # YACC_TARGET (public macro)
  #============================================================
  #
  macro(YACC_TARGET Name YaccInput YaccOutput)
    set(YACC_TARGET_output_header "")
    set(YACC_TARGET_command_opt "")
    set(YACC_TARGET_outputs "${YaccOutput}")
    if(NOT ${ARGC} EQUAL 3 AND NOT ${ARGC} EQUAL 5 AND NOT ${ARGC} EQUAL 7)
      message(SEND_ERROR "Usage")
    else()
      # Parsing parameters
      if(${ARGC} GREATER 5 OR ${ARGC} EQUAL 5)
        if("${ARGV3}" STREQUAL "VERBOSE")
          YACC_TARGET_option_verbose(${Name} ${YaccOutput} "${ARGV4}")
        endif()
        if("${ARGV3}" STREQUAL "COMPILE_FLAGS")
          YACC_TARGET_option_extraopts("${ARGV4}")
        endif()
      endif()

      if(${ARGC} EQUAL 7)
        if("${ARGV5}" STREQUAL "VERBOSE")
          YACC_TARGET_option_verbose(${Name} ${YaccOutput} "${ARGV6}")
        endif()

        if("${ARGV5}" STREQUAL "COMPILE_FLAGS")
          YACC_TARGET_option_extraopts("${ARGV6}")
        endif()
      endif()

      # Header's name generated by yacc (see option -d)
      list(APPEND YACC_TARGET_cmdopt "-d")
      string(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\2" _fileext "${ARGV2}")
      string(REPLACE "c" "h" _fileext ${_fileext})
      string(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\1${_fileext}"
        YACC_${Name}_OUTPUT_HEADER "${ARGV2}")
      list(APPEND YACC_TARGET_outputs "${YACC_${Name}_OUTPUT_HEADER}")

      add_custom_command(OUTPUT ${YACC_TARGET_outputs}
        ${YACC_TARGET_extraoutputs}
        COMMAND ${YACC_EXECUTABLE}
        ARGS ${YACC_TARGET_cmdopt} -o ${ARGV2} ${ARGV1}
        DEPENDS ${ARGV1}
	COMMENT "[YACC][${Name}] Building parser with ${YACC_EXECUTABLE}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

      # define target variables
      set(YACC_${Name}_DEFINED TRUE)
      set(YACC_${Name}_INPUT ${ARGV1})
      set(YACC_${Name}_OUTPUTS ${YACC_TARGET_outputs})
      set(YACC_${Name}_COMPILE_FLAGS ${YACC_TARGET_cmdopt})
      set(YACC_${Name}_OUTPUT_SOURCE "${YaccOutput}")

    endif(NOT ${ARGC} EQUAL 3 AND NOT ${ARGC} EQUAL 5 AND NOT ${ARGC} EQUAL 7)
  endmacro(YACC_TARGET)
  #
  #============================================================

endif(YACC_EXECUTABLE)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(YACC DEFAULT_MSG YACC_EXECUTABLE)

# FindYACC.cmake ends here

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
