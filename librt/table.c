/*
 *			T A B L E . C
 *
 *  Tables for the BRL-CAD Package ray-tracing library "librt".
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCStree[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "db.h"
#include "./debug.h"

#if __STDC__ && !alliant && !apollo
# define RT_DECLARE_INTERFACE(name)	\
	RT_EXTERN(int rt_##name##_prep, (struct soltab *stp, \
			union record *rec, struct rt_i *rtip)); \
	RT_EXTERN(int rt_##name##_shot, (struct soltab *stp, struct xray *rp, \
			struct application *ap, struct seg *seghead)); \
	RT_EXTERN(void rt_##name##_print, (struct soltab *stp)); \
	RT_EXTERN(void rt_##name##_norm, (struct hit *hitp, \
			struct soltab *stp, struct xray *rp)); \
	RT_EXTERN(void rt_##name##_uv, (struct application *ap, \
			struct soltab *stp, struct hit *hitp, \
			struct uvcoord *uvp)); \
	RT_EXTERN(void rt_##name##_curve, (struct curvature *cvp, \
			struct hit *hitp, struct soltab *stp)); \
	RT_EXTERN(int rt_##name##_class, ()); \
	RT_EXTERN(void rt_##name##_free, (struct soltab *stp)); \
	RT_EXTERN(int rt_##name##_plot, (union record *rp, mat_t mat, \
			struct vlhead *vhead, struct directory *dp, \
			double abs_tol, double rel_tol, double norm_tol)); \
	RT_EXTERN(void rt_##name##_vshot, (struct soltab *stp[], \
			struct xray *rp[], \
			struct seg segp[], int n, struct resource *resp)); \
	RT_EXTERN(int rt_##name##_tess, (struct nmgregion **r, \
			struct model *m, union record *rp, \
			mat_t mat, struct directory *dp, \
			double abs_tol, double rel_tol, double norm_tol)); \
	RT_EXTERN(int rt_##name##_import, (struct rt_db_internal *ip, \
			struct rt_external *ep, mat_t mat)); \
	RT_EXTERN(int rt_##name##_export, (struct rt_external *ep, \
			struct rt_db_internal *ip)); \
	RT_EXTERN(void rt_##name##_ifree, (struct rt_db_internal *ip)); \
	RT_EXTERN(int rt_##name##_describe, (struct rt_vls *str, \
			struct rt_db_internal *ip, int verbose));
#else
# define RT_DECLARE_INTERFACE(name)	\
	RT_EXTERN(int rt_/**/name/**/_prep, (struct soltab *stp, \
			union record *rec, struct rt_i *rtip)); \
	RT_EXTERN(int rt_/**/name/**/_shot, (struct soltab *stp, struct xray *rp, \
			struct application *ap, struct seg *seghead)); \
	RT_EXTERN(void rt_/**/name/**/_print, (struct soltab *stp)); \
	RT_EXTERN(void rt_/**/name/**/_norm, (struct hit *hitp, \
			struct soltab *stp, struct xray *rp)); \
	RT_EXTERN(void rt_/**/name/**/_uv, (struct application *ap, \
			struct soltab *stp, struct hit *hitp, \
			struct uvcoord *uvp)); \
	RT_EXTERN(void rt_/**/name/**/_curve, (struct curvature *cvp, \
			struct hit *hitp, struct soltab *stp)); \
	RT_EXTERN(int rt_/**/name/**/_class, ()); \
	RT_EXTERN(void rt_/**/name/**/_free, (struct soltab *stp)); \
	RT_EXTERN(int rt_/**/name/**/_plot, (union record *rp, mat_t mat, \
			struct vlhead *vhead, struct directory *dp, \
			double abs_tol, double rel_tol, double norm_tol)); \
	RT_EXTERN(void rt_/**/name/**/_vshot, (struct soltab *stp[], \
			struct xray *rp[], \
			struct seg segp[], int n, struct resource *resp)); \
	RT_EXTERN(int rt_/**/name/**/_tess, (struct nmgregion **r, \
			struct model *m, union record *rp, \
			mat_t mat, struct directory *dp, \
			double abs_tol, double rel_tol, double norm_tol)); \
	RT_EXTERN(int rt_/**/name/**/_import, (struct rt_db_internal *ip, \
			struct rt_external *ep, mat_t mat)); \
	RT_EXTERN(int rt_/**/name/**/_export, (struct rt_external *ep, \
			struct rt_db_internal *ip)); \
	RT_EXTERN(void rt_/**/name/**/_ifree, (struct rt_db_internal *ip)); \
	RT_EXTERN(int rt_/**/name/**/_describe, (struct rt_vls *str, \
			struct rt_db_internal *ip, int verbose));
#endif

/* Note:  no semi-colons at the end of these, please */	
RT_DECLARE_INTERFACE(nul)
RT_DECLARE_INTERFACE(tor)
RT_DECLARE_INTERFACE(tgc)
RT_DECLARE_INTERFACE(ell)
RT_DECLARE_INTERFACE(arb)
RT_DECLARE_INTERFACE(ars)
RT_DECLARE_INTERFACE(hlf)
RT_DECLARE_INTERFACE(rec)
RT_DECLARE_INTERFACE(pg)
RT_DECLARE_INTERFACE(spl)
RT_DECLARE_INTERFACE(sph)
RT_DECLARE_INTERFACE(ebm)
RT_DECLARE_INTERFACE(vol)
RT_DECLARE_INTERFACE(arbn)
RT_DECLARE_INTERFACE(pipe)
RT_DECLARE_INTERFACE(part)

extern void	rt_vstub();	/* XXX vshoot.c */

struct rt_functab rt_functab[ID_MAXIMUM+2] = {
	"ID_NULL",	0,
		rt_nul_prep,	rt_nul_shot,	rt_nul_print, 	rt_nul_norm,
	 	rt_nul_uv,	rt_nul_curve,	rt_nul_class,	rt_nul_free,
		rt_nul_plot,	rt_nul_vshot,	rt_nul_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,
		
	"ID_TOR",	1,
		rt_tor_prep,	rt_tor_shot,	rt_tor_print,	rt_tor_norm,
		rt_tor_uv,	rt_tor_curve,	rt_tor_class,	rt_tor_free,
		rt_tor_plot,	rt_tor_vshot,	rt_tor_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_TGC",	1,
		rt_tgc_prep,	rt_tgc_shot,	rt_tgc_print,	rt_tgc_norm,
		rt_tgc_uv,	rt_tgc_curve,	rt_tgc_class,	rt_tgc_free,
		rt_tgc_plot,	rt_tgc_vshot,	rt_tgc_tess,
		rt_tgc_import,	rt_tgc_export,	rt_tgc_ifree,
		rt_tgc_describe,

	"ID_ELL",	1,
		rt_ell_prep,	rt_ell_shot,	rt_ell_print,	rt_ell_norm,
		rt_ell_uv,	rt_ell_curve,	rt_ell_class,	rt_ell_free,
		rt_ell_plot,	rt_ell_vshot,	rt_ell_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_ARB8",	0,
		rt_arb_prep,	rt_arb_shot,	rt_arb_print,	rt_arb_norm,
		rt_arb_uv,	rt_arb_curve,	rt_arb_class,	rt_arb_free,
		rt_arb_plot,	rt_arb_vshot,	rt_arb_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_ARS",	1,
		rt_ars_prep,	rt_ars_shot,	rt_ars_print,	rt_ars_norm,
		rt_ars_uv,	rt_ars_curve,	rt_ars_class,	rt_ars_free,
		rt_ars_plot,	rt_vstub,	rt_ars_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_HALF",	0,
		rt_hlf_prep,	rt_hlf_shot,	rt_hlf_print,	rt_hlf_norm,
		rt_hlf_uv,	rt_hlf_curve,	rt_hlf_class,	rt_hlf_free,
		rt_hlf_plot,	rt_hlf_vshot,	rt_hlf_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_REC",	1,
		rt_rec_prep,	rt_rec_shot,	rt_rec_print,	rt_rec_norm,
		rt_rec_uv,	rt_rec_curve,	rt_rec_class,	rt_rec_free,
		rt_tgc_plot,	rt_rec_vshot,	rt_tgc_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_POLY",	1,
		rt_pg_prep,	rt_pg_shot,	rt_pg_print,	rt_pg_norm,
		rt_pg_uv,	rt_pg_curve,	rt_pg_class,	rt_pg_free,
		rt_pg_plot,	rt_vstub,	rt_pg_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_BSPLINE",	1,
		rt_spl_prep,	rt_spl_shot,	rt_spl_print,	rt_spl_norm,
		rt_spl_uv,	rt_spl_curve,	rt_spl_class,	rt_spl_free,
		rt_spl_plot,	rt_vstub,	rt_spl_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_SPH",	1,
		rt_sph_prep,	rt_sph_shot,	rt_sph_print,	rt_sph_norm,
		rt_sph_uv,	rt_sph_curve,	rt_sph_class,	rt_sph_free,
		rt_ell_plot,	rt_sph_vshot,	rt_ell_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_STRINGSOL",	0,
		rt_nul_prep,	rt_nul_shot,	rt_nul_print,	rt_nul_norm,
		rt_nul_uv,	rt_nul_curve,	rt_nul_class,	rt_nul_free,
		rt_nul_plot,	rt_nul_vshot,	rt_nul_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_EBM",	0,
		rt_ebm_prep,	rt_ebm_shot,	rt_ebm_print,	rt_ebm_norm,
		rt_ebm_uv,	rt_ebm_curve,	rt_ebm_class,	rt_ebm_free,
		rt_ebm_plot,	rt_vstub,	rt_nul_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_VOL",	0,
		rt_vol_prep,	rt_vol_shot,	rt_vol_print,	rt_vol_norm,
		rt_vol_uv,	rt_vol_curve,	rt_vol_class,	rt_vol_free,
		rt_vol_plot,	rt_vstub,	rt_nul_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe,

	"ID_ARBN",	0,
		rt_arbn_prep,	rt_arbn_shot,	rt_arbn_print,	rt_arbn_norm,
		rt_arbn_uv,	rt_arbn_curve,	rt_arbn_class,	rt_arbn_free,
		rt_arbn_plot,	rt_arbn_vshot,	rt_arbn_tess,
		rt_arbn_import,	rt_arbn_export,	rt_arbn_ifree,
		rt_arbn_describe,

	"ID_PIPE",	0,
		rt_pipe_prep,	rt_pipe_shot,	rt_pipe_print,	rt_pipe_norm,
		rt_pipe_uv,	rt_pipe_curve,	rt_pipe_class,	rt_pipe_free,
		rt_pipe_plot,	rt_pipe_vshot,	rt_pipe_tess,
		rt_pipe_import,	rt_pipe_export,	rt_pipe_ifree,
		rt_pipe_describe,

	"ID_PARTICLE",	0,
		rt_part_prep,	rt_part_shot,	rt_part_print,	rt_part_norm,
		rt_part_uv,	rt_part_curve,	rt_part_class,	rt_part_free,
		rt_part_plot,	rt_part_vshot,	rt_part_tess,
		rt_part_import,	rt_part_export,	rt_part_ifree,
		rt_part_describe,

	">ID_MAXIMUM",	0,
		rt_nul_prep,	rt_nul_shot,	rt_nul_print,	rt_nul_norm,
		rt_nul_uv,	rt_nul_curve,	rt_nul_class,	rt_nul_free,
		rt_nul_plot,	rt_nul_vshot,	rt_nul_tess,
		rt_nul_import,	rt_nul_export,	rt_nul_ifree,
		rt_nul_describe
};
int rt_nfunctab = sizeof(rt_functab)/sizeof(struct rt_functab);

/*
 *  Hooks for unimplemented routines
 */
#define DEF(func)	func() { rt_log("func unimplemented\n"); return; }
#define IDEF(func)	func() { rt_log("func unimplemented\n"); return(0); }
#define NDEF(func)	func() { rt_log("func unimplemented\n"); return(-1); }

int IDEF(rt_nul_prep)
int	 IDEF(rt_nul_shot)
void DEF(rt_nul_print)
void DEF(rt_nul_norm)
void DEF(rt_nul_uv)
void DEF(rt_nul_curve)
int IDEF(rt_nul_class)
void DEF(rt_nul_free)
int NDEF(rt_nul_plot)
void DEF(rt_nul_vshot)
int NDEF(rt_nul_tess)
int NDEF(rt_nul_import)
int NDEF(rt_nul_export)
void DEF(rt_nul_ifree)
int NDEF(rt_nul_describe)

/* Map for database solidrec objects to internal objects */
static char idmap[] = {
	ID_NULL,	/* undefined, 0 */
	ID_NULL,	/* RPP	1 axis-aligned rectangular parallelopiped */
	ID_NULL,	/* BOX	2 arbitrary rectangular parallelopiped */
	ID_NULL,	/* RAW	3 right-angle wedge */
	ID_NULL,	/* ARB4	4 tetrahedron */
	ID_NULL,	/* ARB5	5 pyramid */
	ID_NULL,	/* ARB6	6 extruded triangle */
	ID_NULL,	/* ARB7	7 weird 7-vertex shape */
	ID_NULL,	/* ARB8	8 hexahedron */
	ID_NULL,	/* ELL	9 ellipsoid */
	ID_NULL,	/* ELL1	10 another ellipsoid ? */
	ID_NULL,	/* SPH	11 sphere */
	ID_NULL,	/* RCC	12 right circular cylinder */
	ID_NULL,	/* REC	13 right elliptic cylinder */
	ID_NULL,	/* TRC	14 truncated regular cone */
	ID_NULL,	/* TEC	15 truncated elliptic cone */
	ID_TOR,		/* TOR	16 toroid */
	ID_NULL,	/* TGC	17 truncated general cone */
	ID_TGC,		/* GENTGC 18 supergeneralized TGC; internal form */
	ID_ELL,		/* GENELL 19: V,A,B,C */
	ID_ARB8,	/* GENARB8 20:  V, and 7 other vectors */
	ID_NULL,	/* HACK: ARS 21: arbitrary triangular-surfaced polyhedron */
	ID_NULL,	/* HACK: ARSCONT 22: extension record type for ARS solid */
	ID_NULL,	/* ELLG 23:  gift-only */
	ID_HALF,	/* HALFSPACE 24:  halfspace */
	ID_NULL,	/* HACK: SPLINE 25 */
	ID_NULL		/* n+1 */
};

/*
 *			R T _ I D _ S O L I D
 *
 *  Given a database record, determine the proper rt_functab subscript.
 *  Used by MGED as well as internally to librt.
 *
 *  Returns ID_xxx if successful, or ID_NULL upon failure.
 */
int
rt_id_solid( rec )
register union record *rec;
{
	register int id;

	switch( rec->u_id )  {
	case ID_SOLID:
		id = idmap[rec->s.s_type];
		break;
	case ID_ARS_A:
		id = ID_ARS;
		break;
	case ID_P_HEAD:
		id = ID_POLY;
		break;
	case ID_BSOLID:
		id = ID_BSPLINE;
		break;
	case ID_STRSOL:
		/* XXX This really needs to be some kind of table */
		if( strncmp( rec->ss.ss_str, "ebm", 3 ) == 0 )  {
			id = ID_EBM;
			break;
		} else if( strncmp( rec->ss.ss_str, "vol", 3 ) == 0 )  {
			id = ID_VOL;
			break;
		}
		rt_log("rt_id_solid(%s):  String solid type '%s' unknown\n",
			rec->ss.ss_name, rec->ss.ss_str );
		id = ID_NULL;		/* BAD */
		break;
	case DBID_ARBN:
		id = ID_ARBN;
		break;
	case DBID_PIPE:
		id = ID_PIPE;
		break;
	case DBID_PARTICLE:
		id = ID_PARTICLE;
		break;
	default:
		rt_log("rt_id_solid:  u_id=x%x unknown\n", rec->u_id);
		id = ID_NULL;		/* BAD */
		break;
	}
	if( id < ID_NULL || id > ID_MAXIMUM )  {
		rt_log("rt_id_solid: internal error, id=%d?\n", id);
		id = ID_NULL;		/* very BAD */
	}
	return(id);
}
