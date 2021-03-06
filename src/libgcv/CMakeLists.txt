# Include directories needed by libgcv users
set(GCV_INCLUDE_DIRS
  ${BRLCAD_BINARY_DIR}/include
  ${BRLCAD_SOURCE_DIR}/include
  ${BU_INCLUDE_DIRS}
  ${RT_INCLUDE_DIRS}
  )
BRLCAD_LIB_INCLUDE_DIRS(gcv GCV_INCLUDE_DIRS "")


set(LIBGCV_PLUGINS_DIRECTORY "libgcv_plugins")
add_subdirectory(plugins)


set(LIBGCV_SOURCES
  bot_solidity.c
  bottess.c
  facetize.c
  gcv.c
  region_end.c
  region_end_mc.c
  tri_intersect.c
  )


BRLCAD_ADDLIB(libgcv "${LIBGCV_SOURCES}" "librt;libwdb;libbu" SO_SRCS "${GCV_SO_SRCS}" STATIC_SRCS "${GCV_STATIC_SRCS}")
set_target_properties(libgcv PROPERTIES VERSION 20.0.1 SOVERSION 20)
set_source_files_properties(gcv.c PROPERTIES COMPILE_DEFINITIONS "LIBGCV_PLUGINS_DIRECTORY=\"${LIBGCV_PLUGINS_DIRECTORY}\"")


enable_testing()
BRLCAD_ADDEXEC(test_bottess test_bottess.c libgcv NO_INSTALL)
if(CPP_DLL_DEFINES)
  set_property(TARGET test_bottess APPEND PROPERTY COMPILE_DEFINITIONS "BOTTESS_DLL_IMPORTS")
endif(CPP_DLL_DEFINES)
add_test(NAME bottess_test COMMAND test_bottess)


set(GCV_IGNORE_FILES
  NOTES
  soup.h
  tri_intersect.h
  )
CMAKEFILES(${GCV_IGNORE_FILES})


DISTCLEAN(${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake)


# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
