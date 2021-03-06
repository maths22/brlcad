# Minimum required version of CMake
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
  if ("${CMAKE_VERSION}" VERSION_GREATER 2.99)
    CMAKE_POLICY(SET CMP0026 OLD)
  endif ("${CMAKE_VERSION}" VERSION_GREATER 2.99)
endif(COMMAND CMAKE_POLICY)

# Set CMake project name
project(TINYCTHREAD)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

if(NOT BIN_DIR)
  set(BIN_DIR bin)
endif(NOT BIN_DIR)
if(NOT LIB_DIR)
  set(LIB_DIR lib)
endif(NOT LIB_DIR)

if(MSVC)
  add_definitions("-DTINYCTHREAD_DLL_EXPORTS")
endif(MSVC)

add_library(tinycthread SHARED tinycthread.c)
if(MSVC)
  # msvc does not append 'lib' - do it here to have consistent name
  set_target_properties(tinycthread PROPERTIES PREFIX "lib")
  set_target_properties(tinycthread PROPERTIES IMPORT_PREFIX "lib")
endif(MSVC)
install(TARGETS tinycthread
  RUNTIME DESTINATION ${BIN_DIR}
  LIBRARY DESTINATION ${LIB_DIR}
  ARCHIVE DESTINATION ${LIB_DIR})

if(BUILD_STATIC_LIBS)
  add_library(tinycthread-static STATIC tinycthread.c)
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(tinycthread PROPERTIES PREFIX "lib")
  endif(MSVC)
  install(TARGETS tinycthread-static
    RUNTIME DESTINATION ${BIN_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    ARCHIVE DESTINATION ${LIB_DIR})
endif(BUILD_STATIC_LIBS)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
