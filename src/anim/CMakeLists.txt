set(ANIM_INCLUDE_DIRS
  ${BU_INCLUDE_DIRS}
  ${BN_INCLUDE_DIRS}
  )
LIST(REMOVE_DUPLICATES ANIM_INCLUDE_DIRS)
include_directories(${ANIM_INCLUDE_DIRS})

BRLCAD_ADDEXEC(anim_cascade anim_cascade.c "libbn;libbu")

BRLCAD_ADDEXEC(anim_fly anim_fly.c "libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(anim_hardtrack anim_hardtrack.c "libbn;libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(anim_keyread anim_keyread.c "libbn;libbu")

BRLCAD_ADDEXEC(anim_lookat anim_lookat.c "libbn;libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(anim_offset anim_offset.c "libbn;libbu")

BRLCAD_ADDEXEC(anim_orient anim_orient.c "libbn;libbu")

BRLCAD_ADDEXEC(anim_script anim_script.c "libbn;libbu")

BRLCAD_ADDEXEC(anim_sort anim_sort.c libbu)

BRLCAD_ADDEXEC(anim_time anim_time.c "libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(anim_track "anim_track.c;cattrack.c" "libbn;libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(anim_turn anim_turn.c "libbn;libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(chan_add chan_add.c libbu)

BRLCAD_ADDEXEC(chan_mult chan_mult.c libbu)

BRLCAD_ADDEXEC(chan_permute chan_permute.c libbu)

CMAKEFILES(cattrack.h)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
