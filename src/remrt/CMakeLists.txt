set(REMRTBIN_INCLUDE_DIRS
  ${REMRT_INCLUDE_DIRS}
  ${OPTICAL_INCLUDE_DIRS}
  ${FB_INCLUDE_DIRS}
  ${PKG_INCLUDE_DIRS}
  )
LIST(REMOVE_DUPLICATES REMRTBIN_INCLUDE_DIRS)
include_directories(${REMRTBIN_INCLUDE_DIRS})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TCL_INCLUDE_DIRS}
  )

BRLCAD_ADDEXEC(remrt "ihost.c;remrt.c" "libremrt;liboptical;libfb")

BRLCAD_ADDEXEC(rtsrv rtsrv.c "libremrt;libfb;liboptical;libpkg;libicv")
SET_TARGET_PROPERTIES(rtsrv PROPERTIES COMPILE_DEFINITIONS "RTSRV")

CMAKEFILES(ihost.h protocol.h)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
