#!/bin/sh
#			F A S T . S H
#
# A quick way of recompiling LIBRT using multiple processors.
#
#  $Header$

cake  \
 bomb.o \
 nmg_misc.o \
 g_nurb.o \
 g_nmg.o \
 cmd.o \
 complex.o \
 const.o \
 cut.o \
 dir.o \
 &

cake \
 g_arb.o \
 nmg_rt_isect.o \
 g_ars.o \
 db_alloc.o \
 db_anim.o \
 db_io.o \
 db_lookup.o &

cake \
 nmg_rt_segs.o \
 g_ebm.o \
 g_ell.o \
 db_open.o \
 db_path.o \
 db_scan.o \
 nmg_ck.o \
 db_walk.o &

cake \
 g_half.o \
 nmg_index.o \
 nmg_inter.o \
 g_sph.o \
 &

cake \
 g_tgc.o \
 g_torus.o \
 g_part.o \
 g_pipe.o \
 nmg_class.o \
 &

cake \
 nmg_manif.o \
 hist.o \
 htond.o \
 global.o \
 log.o \
 machine.o \
 mat.o \
 mater.o \
 memalloc.o \
 units.o \
 &

cake \
 pr.o \
 db_tree.o \
 g_vol.o \
 nmg_comb.o \
 nmg_eval.o &

cake \
 g_rec.o \
 g_pg.o \
 bool.o \
 nmg_mesh.o \
 nmg_mk.o \
 shoot.o \
 nmg_plot.o \
 &

cake \
 parse.o \
 inout.o \
 plane.o \
 plot3.o \
 polylib.o \
 nmg_bool.o \
 nmg_fuse.o \
 prep.o \
 qmath.o \
 roots.o \
 g_arbn.o \
 &

cake \
 nmg_mod.o \
 sphmap.o \
 storage.o \
 table.o \
 timerunix.o \
 tree.o &

wait
cake
