set(IRPREP_INCLUDE_DIRS
  ${BU_INCLUDE_DIRS}
  ${RT_INCLUDE_DIRS}
  ${X11_INCLUDE_DIR}
  )
LIST(REMOVE_DUPLICATES IRPREP_INCLUDE_DIRS)
include_directories(${IRPREP_INCLUDE_DIRS})

if(BRLCAD_ENABLE_X11)
  BRLCAD_ADDEXEC(ir-X ir-X.c "libbu;${X11_LIBRARIES}")
  BRLCAD_ADDEXEC(irdisp irdisp.c "libbu;${X11_LIBRARIES}")
  BRLCAD_ADDEXEC(pictx pictx.c "libbu;${X11_LIBRARIES}")
else(BRLCAD_ENABLE_X11)
  CMAKEFILES(ir-X.c irdisp.c pictx.c)
endif(BRLCAD_ENABLE_X11)

BRLCAD_ADDEXEC(all_sf all_sf.c "librt;${M_LIBRARY}")
BRLCAD_ADDEXEC(showtherm showtherm.c "librt;${M_LIBRARY}")
BRLCAD_ADDEXEC(firpass "firpass.c;subroutines.c" "librt;${M_LIBRARY}")
BRLCAD_ADDEXEC(secpass "secpass.c;subroutines.c" "librt;${M_LIBRARY}")
BRLCAD_ADDEXEC(shapefact shapefact.c "librt;${M_LIBRARY}")

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
