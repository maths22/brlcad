/*
 *			E D S O L . C
 *
 * Functions -
 *	init_sedit	set up for a Solid Edit
 *	sedit		Apply Solid Edit transformation(s)
 *	pscale		Partial scaling of a solid
 *	init_objedit	set up for object edit?
 *	f_eqn		change face of GENARB8 to new equation
 *
 *  Authors -
 *	Keith A. Applin
 *	Bob Suckling
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "db.h"
#include "nmg.h"
#include "raytrace.h"
#include "nurb.h"
#include "rtgeom.h"

#include "./ged.h"
#include "./solid.h"
#include "./sedit.h"
#include "./dm.h"
#include "./menu.h"

extern struct rt_tol		mged_tol;	/* from ged.c */

static int	new_way = 0;	/* Set 1 for import/export handling */

static void	arb8_edge(), ars_ed(), ell_ed(), tgc_ed(), tor_ed(), spline_ed();
static void	nmg_ed();
static void	rpc_ed(), rhc_ed(), epa_ed(), ehy_ed(), eto_ed();
static void	arb7_edge(), arb6_edge(), arb5_edge(), arb4_point();
static void	arb8_mv_face(), arb7_mv_face(), arb6_mv_face();
static void	arb5_mv_face(), arb4_mv_face(), arb8_rot_face(), arb7_rot_face();
static void 	arb6_rot_face(), arb5_rot_face(), arb4_rot_face(), arb_control();

void pscale();
void	calc_planes();
static short int fixv;		/* used in ECMD_ARB_ROTATE_FACE,f_eqn(): fixed vertex */

/* data for solid editing */
int			sedraw;	/* apply solid editing changes */

struct rt_external	es_ext;
struct rt_db_internal	es_int;
struct rt_db_internal	es_int_orig;

union record es_rec;		/* current solid record */
int     es_edflag;		/* type of editing for this solid */
fastf_t	es_scale;		/* scale factor */
fastf_t	es_peqn[7][4];		/* ARBs defining plane equations */
fastf_t	es_m[3];		/* edge(line) slope */
mat_t	es_mat;			/* accumulated matrix of path */ 
mat_t 	es_invmat;		/* inverse of es_mat   KAA */

point_t	es_keypoint;		/* center of editing xforms */
char	*es_keytag;		/* string identifying the keypoint */
int	es_keyfixed;		/* keypoint specified by user? */

vect_t		es_para;	/* keyboard input param. Only when inpara set.  */
extern int	inpara;		/* es_para valid.  es_mvalid mus = 0 */
static vect_t	es_mparam;	/* mouse input param.  Only when es_mvalid set */
static int	es_mvalid;	/* es_mparam valid.  inpara must = 0 */

static int	spl_surfno;	/* What surf & ctl pt to edit on spline */
static int	spl_ui;
static int	spl_vi;

static struct edgeuse	*es_eu;	/* Currently selected NMG edgeuse */

/* XXX This belongs in sedit.h */
#define ECMD_VTRANS		17	/* vertex translate */
#define ECMD_NMG_EPICK		19	/* edge pick */
#define ECMD_NMG_EMOVE		20	/* edge move */
#define ECMD_NMG_EDEBUG		21	/* edge debug */
#define ECMD_NMG_FORW		22	/* next eu */
#define ECMD_NMG_BACK		23	/* prev eu */
#define ECMD_NMG_RADIAL		24	/* radial+mate eu */

/*  These values end up in es_menu, as do ARB vertex numbers */
int	es_menu;		/* item selected from menu */
#define MENU_TOR_R1		21
#define MENU_TOR_R2		22
#define MENU_TGC_ROT_H		23
#define MENU_TGC_ROT_AB 	24
#define	MENU_TGC_MV_H		25
#define MENU_TGC_MV_HH		26
#define MENU_TGC_SCALE_H	27
#define MENU_TGC_SCALE_A	28
#define MENU_TGC_SCALE_B	29
#define MENU_TGC_SCALE_C	30
#define MENU_TGC_SCALE_D	31
#define MENU_TGC_SCALE_AB	32
#define MENU_TGC_SCALE_CD	33
#define MENU_TGC_SCALE_ABCD	34
#define MENU_ARB_MV_EDGE	35
#define MENU_ARB_MV_FACE	36
#define MENU_ARB_ROT_FACE	37
#define MENU_ELL_SCALE_A	38
#define MENU_ELL_SCALE_B	39
#define MENU_ELL_SCALE_C	40
#define MENU_ELL_SCALE_ABC	41
#define MENU_RPC_B		42
#define MENU_RPC_H		43
#define MENU_RPC_R		44
#define MENU_RHC_B		45
#define MENU_RHC_H		46
#define MENU_RHC_R		47
#define MENU_RHC_C		48
#define MENU_EPA_H		49
#define MENU_EPA_R1		50
#define MENU_EPA_R2		51
#define MENU_EHY_H		52
#define MENU_EHY_R1		53
#define MENU_EHY_R2		54
#define MENU_EHY_C		55
#define MENU_ETO_R		56
#define MENU_ETO_RD		57
#define MENU_ETO_SCALE_C	58
#define MENU_ETO_ROT_C		59

extern int arb_faces[5][24];	/* from edarb.c */

struct menu_item  edge8_menu[] = {
	{ "ARB8 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb8_edge, 0 },
	{ "move edge 23", arb8_edge, 1 },
	{ "move edge 34", arb8_edge, 2 },
	{ "move edge 14", arb8_edge, 3 },
	{ "move edge 15", arb8_edge, 4 },
	{ "move edge 26", arb8_edge, 5 },
	{ "move edge 56", arb8_edge, 6 },
	{ "move edge 67", arb8_edge, 7 },
	{ "move edge 78", arb8_edge, 8 },
	{ "move edge 58", arb8_edge, 9 },
	{ "move edge 37", arb8_edge, 10 },
	{ "move edge 48", arb8_edge, 11 },
	{ "RETURN",       arb8_edge, 12 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  edge7_menu[] = {
	{ "ARB7 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb7_edge, 0 },
	{ "move edge 23", arb7_edge, 1 },
	{ "move edge 34", arb7_edge, 2 },
	{ "move edge 14", arb7_edge, 3 },
	{ "move edge 15", arb7_edge, 4 },
	{ "move edge 26", arb7_edge, 5 },
	{ "move edge 56", arb7_edge, 6 },
	{ "move edge 67", arb7_edge, 7 },
	{ "move edge 37", arb7_edge, 8 },
	{ "move edge 57", arb7_edge, 9 },
	{ "move edge 45", arb7_edge, 10 },
	{ "move point 5", arb7_edge, 11 },
	{ "RETURN",       arb7_edge, 12 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  edge6_menu[] = {
	{ "ARB6 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb6_edge, 0 },
	{ "move edge 23", arb6_edge, 1 },
	{ "move edge 34", arb6_edge, 2 },
	{ "move edge 14", arb6_edge, 3 },
	{ "move edge 15", arb6_edge, 4 },
	{ "move edge 25", arb6_edge, 5 },
	{ "move edge 36", arb6_edge, 6 },
	{ "move edge 46", arb6_edge, 7 },
	{ "move point 5", arb6_edge, 8 },
	{ "move point 6", arb6_edge, 9 },
	{ "RETURN",       arb6_edge, 10 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  edge5_menu[] = {
	{ "ARB5 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb5_edge, 0 },
	{ "move edge 23", arb5_edge, 1 },
	{ "move edge 34", arb5_edge, 2 },
	{ "move edge 14", arb5_edge, 3 },
	{ "move edge 15", arb5_edge, 4 },
	{ "move edge 25", arb5_edge, 5 },
	{ "move edge 35", arb5_edge, 6 },
	{ "move edge 45", arb5_edge, 7 },
	{ "move point 5", arb5_edge, 8 },
	{ "RETURN",       arb5_edge, 9 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  point4_menu[] = {
	{ "ARB4 POINTS", (void (*)())NULL, 0 },
	{ "move point 1", arb4_point, 0 },
	{ "move point 2", arb4_point, 1 },
	{ "move point 3", arb4_point, 2 },
	{ "move point 4", arb4_point, 4 },
	{ "RETURN",       arb4_point, 5 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  tgc_menu[] = {
	{ "TGC MENU", (void (*)())NULL, 0 },
	{ "scale H",	tgc_ed, MENU_TGC_SCALE_H },
	{ "scale A",	tgc_ed, MENU_TGC_SCALE_A },
	{ "scale B",	tgc_ed, MENU_TGC_SCALE_B },
	{ "scale c",	tgc_ed, MENU_TGC_SCALE_C },
	{ "scale d",	tgc_ed, MENU_TGC_SCALE_D },
	{ "scale A,B",	tgc_ed, MENU_TGC_SCALE_AB },
	{ "scale C,D",	tgc_ed, MENU_TGC_SCALE_CD },
	{ "scale A,B,C,D", tgc_ed, MENU_TGC_SCALE_ABCD },
	{ "rotate H",	tgc_ed, MENU_TGC_ROT_H },
	{ "rotate AxB",	tgc_ed, MENU_TGC_ROT_AB },
	{ "move end H(rt)", tgc_ed, MENU_TGC_MV_H },
	{ "move end H", tgc_ed, MENU_TGC_MV_HH },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  tor_menu[] = {
	{ "TORUS MENU", (void (*)())NULL, 0 },
	{ "scale radius 1", tor_ed, MENU_TOR_R1 },
	{ "scale radius 2", tor_ed, MENU_TOR_R2 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  eto_menu[] = {
	{ "ELL-TORUS MENU", (void (*)())NULL, 0 },
	{ "scale r", eto_ed, MENU_ETO_R },
	{ "scale D", eto_ed, MENU_ETO_RD },
	{ "scale C", eto_ed, MENU_ETO_SCALE_C },
	{ "rotate C", eto_ed, MENU_ETO_ROT_C },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  ell_menu[] = {
	{ "ELLIPSOID MENU", (void (*)())NULL, 0 },
	{ "scale A", ell_ed, MENU_ELL_SCALE_A },
	{ "scale B", ell_ed, MENU_ELL_SCALE_B },
	{ "scale C", ell_ed, MENU_ELL_SCALE_C },
	{ "scale A,B,C", ell_ed, MENU_ELL_SCALE_ABC },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  ars_menu[] = {
	{ "ARS MENU", (void (*)())NULL, 0 },
	{ "not implemented", ars_ed, 1 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  spline_menu[] = {
	{ "SPLINE MENU", (void (*)())NULL, 0 },
	{ "pick vertex", spline_ed, -1 },
	{ "move vertex", spline_ed, ECMD_VTRANS },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  nmg_menu[] = {
	{ "NMG MENU", (void (*)())NULL, 0 },
	{ "pick edge", nmg_ed, ECMD_NMG_EPICK },
	{ "move edge", nmg_ed, ECMD_NMG_EMOVE },
	{ "debug edge", nmg_ed, ECMD_NMG_EDEBUG },
	{ "next eu", nmg_ed, ECMD_NMG_FORW },
	{ "prev eu", nmg_ed, ECMD_NMG_BACK },
	{ "radial eu", nmg_ed, ECMD_NMG_RADIAL },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv8_menu[] = {
	{ "ARB8 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb8_mv_face, 1 },
	{ "move face 5678", arb8_mv_face, 2 },
	{ "move face 1584", arb8_mv_face, 3 },
	{ "move face 2376", arb8_mv_face, 4 },
	{ "move face 1265", arb8_mv_face, 5 },
	{ "move face 4378", arb8_mv_face, 6 },
	{ "RETURN",         arb8_mv_face, 7 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv7_menu[] = {
	{ "ARB7 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb7_mv_face, 1 },
	{ "move face 2376", arb7_mv_face, 4 },
	{ "RETURN",         arb7_mv_face, 7 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv6_menu[] = {
	{ "ARB6 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb6_mv_face, 1 },
	{ "move face 2365", arb6_mv_face, 2 },
	{ "move face 1564", arb6_mv_face, 3 },
	{ "move face 125" , arb6_mv_face, 4 },
	{ "move face 346" , arb6_mv_face, 5 },
	{ "RETURN",         arb6_mv_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv5_menu[] = {
	{ "ARB5 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb5_mv_face, 1 },
	{ "move face 125" , arb5_mv_face, 2 },
	{ "move face 235" , arb5_mv_face, 3 },
	{ "move face 345" , arb5_mv_face, 4 },
	{ "move face 145" , arb5_mv_face, 5 },
	{ "RETURN",         arb5_mv_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv4_menu[] = {
	{ "ARB4 FACES", (void (*)())NULL, 0 },
	{ "move face 123" , arb4_mv_face, 1 },
	{ "move face 124" , arb4_mv_face, 2 },
	{ "move face 234" , arb4_mv_face, 3 },
	{ "move face 134" , arb4_mv_face, 4 },
	{ "RETURN",         arb4_mv_face, 5 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot8_menu[] = {
	{ "ARB8 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb8_rot_face, 1 },
	{ "rotate face 5678", arb8_rot_face, 2 },
	{ "rotate face 1584", arb8_rot_face, 3 },
	{ "rotate face 2376", arb8_rot_face, 4 },
	{ "rotate face 1265", arb8_rot_face, 5 },
	{ "rotate face 4378", arb8_rot_face, 6 },
	{ "RETURN",         arb8_rot_face, 7 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot7_menu[] = {
	{ "ARB7 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb7_rot_face, 1 },
	{ "rotate face 567" , arb7_rot_face, 2 },
	{ "rotate face 145" , arb7_rot_face, 3 },
	{ "rotate face 2376", arb7_rot_face, 4 },
	{ "rotate face 1265", arb7_rot_face, 5 },
	{ "rotate face 4375", arb7_rot_face, 6 },
	{ "RETURN",         arb7_rot_face, 7 },
	{ "", (void (*)())NULL, 0 }
};



struct menu_item rot6_menu[] = {
	{ "ARB6 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb6_rot_face, 1 },
	{ "rotate face 2365", arb6_rot_face, 2 },
	{ "rotate face 1564", arb6_rot_face, 3 },
	{ "rotate face 125" , arb6_rot_face, 4 },
	{ "rotate face 346" , arb6_rot_face, 5 },
	{ "RETURN",         arb6_rot_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot5_menu[] = {
	{ "ARB5 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb5_rot_face, 1 },
	{ "rotate face 125" , arb5_rot_face, 2 },
	{ "rotate face 235" , arb5_rot_face, 3 },
	{ "rotate face 345" , arb5_rot_face, 4 },
	{ "rotate face 145" , arb5_rot_face, 5 },
	{ "RETURN",         arb5_rot_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot4_menu[] = {
	{ "ARB4 FACES", (void (*)())NULL, 0 },
	{ "rotate face 123" , arb4_rot_face, 1 },
	{ "rotate face 124" , arb4_rot_face, 2 },
	{ "rotate face 234" , arb4_rot_face, 3 },
	{ "rotate face 134" , arb4_rot_face, 4 },
	{ "RETURN",         arb4_rot_face, 5 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item cntrl_menu[] = {
	{ "ARB MENU", (void (*)())NULL, 0 },
	{ "move edges", arb_control, MENU_ARB_MV_EDGE },
	{ "move faces", arb_control, MENU_ARB_MV_FACE },
	{ "rotate faces", arb_control, MENU_ARB_ROT_FACE },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  rpc_menu[] = {
	{ "RPC MENU", (void (*)())NULL, 0 },
	{ "scale B", rpc_ed, MENU_RPC_B },
	{ "scale H", rpc_ed, MENU_RPC_H },
	{ "scale r", rpc_ed, MENU_RPC_R },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  rhc_menu[] = {
	{ "RHC MENU", (void (*)())NULL, 0 },
	{ "scale B", rhc_ed, MENU_RHC_B },
	{ "scale H", rhc_ed, MENU_RHC_H },
	{ "scale r", rhc_ed, MENU_RHC_R },
	{ "scale c", rhc_ed, MENU_RHC_C },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  epa_menu[] = {
	{ "EPA MENU", (void (*)())NULL, 0 },
	{ "scale H", epa_ed, MENU_EPA_H },
	{ "scale A", epa_ed, MENU_EPA_R1 },
	{ "scale B", epa_ed, MENU_EPA_R2 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  ehy_menu[] = {
	{ "EHY MENU", (void (*)())NULL, 0 },
	{ "scale H", ehy_ed, MENU_EHY_H },
	{ "scale A", ehy_ed, MENU_EHY_R1 },
	{ "scale B", ehy_ed, MENU_EHY_R2 },
	{ "scale c", ehy_ed, MENU_EHY_C },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item *which_menu[] = {
	point4_menu,
	edge5_menu,
	edge6_menu,
	edge7_menu,
	edge8_menu,
	mv4_menu,
	mv5_menu,
	mv6_menu,
	mv7_menu,
	mv8_menu,
	rot4_menu,
	rot5_menu,
	rot6_menu,
	rot7_menu,
	rot8_menu
};

short int arb_vertices[5][24] = {
	{ 1,2,3,0, 1,2,4,0, 2,3,4,0, 1,3,4,0, 0,0,0,0, 0,0,0,0 },	/* arb4 */
	{ 1,2,3,4, 1,2,5,0, 2,3,5,0, 3,4,5,0, 1,4,5,0, 0,0,0,0 },	/* arb5 */
	{ 1,2,3,4, 2,3,6,5, 1,5,6,4, 1,2,5,0, 3,4,6,0, 0,0,0,0 },	/* arb6 */
	{ 1,2,3,4, 5,6,7,0, 1,4,5,0, 2,3,7,6, 1,2,6,5, 4,3,7,5 },	/* arb7 */
	{ 1,2,3,4, 5,6,7,8, 1,5,8,4, 2,3,7,6, 1,2,6,5, 4,3,7,8 }	/* arb8 */
};

static void
arb8_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 12)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb7_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 11) {
		/* move point 5 */
		es_edflag = PTARB;
		es_menu = 4;	/* location of point */
	}
	if(arg == 12)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb6_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 8) {
		/* move point 5   location = 4 */
		es_edflag = PTARB;
		es_menu = 4;
	}
	if(arg == 9) {
		/* move point 6   location = 6 */
		es_edflag = PTARB;
		es_menu = 6;
	}
	if(arg == 10)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb5_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 8) {
		/* move point 5 at loaction 4 */
		es_edflag = PTARB;
		es_menu = 4;
	}
	if(arg == 9)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb4_point( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PTARB;
	if(arg == 5)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
tgc_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
	if(arg == MENU_TGC_ROT_H )
		es_edflag = ECMD_TGC_ROT_H;
	if(arg == MENU_TGC_ROT_AB)
		es_edflag = ECMD_TGC_ROT_AB;
	if(arg == MENU_TGC_MV_H)
		es_edflag = ECMD_TGC_MV_H;
	if(arg == MENU_TGC_MV_HH)
		es_edflag = ECMD_TGC_MV_HH;
}


static void
tor_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
}

static void
eto_ed( arg )
int arg;
{
	es_menu = arg;
	if(arg == MENU_ETO_ROT_C )
		es_edflag = ECMD_ETO_ROT_C;
	else
		es_edflag = PSCALE;
}

static void
rpc_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
}

static void
rhc_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
}

static void
epa_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
}

static void
ehy_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
}

static void
ell_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
}

static void
arb8_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb7_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}		

static void
arb6_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb5_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb4_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 5)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb8_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb7_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}		

static void
arb6_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb5_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb4_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 5)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb_control( arg )
int arg;
{
	es_menu = arg;
	es_edflag = ECMD_ARB_SPECIFIC_MENU;
	sedraw = 1;
}

/*ARGSUSED*/
static void
ars_ed( arg )
int arg;
{
	(void)printf("NOT IMPLEMENTED YET\n");
}


/*ARGSUSED*/
static void
spline_ed( arg )
int arg;
{
	/* XXX Why wasn't this done by setting es_edflag = ECMD_SPLINE_VPICK? */
	if( arg < 0 )  {
		/* Enter picking state */
		chg_state( ST_S_EDIT, ST_S_VPICK, "Vertex Pick" );
		return;
	}
	/* For example, this will set es_edflag = ECMD_VTRANS */
	es_edflag = arg;
	sedraw = 1;
}

/*
 *			N M G _ E D
 *
 *  Handler for events in the NMG menu.
 *  Mostly just set appropriate state flags to prepare us for user's
 *  next event.
 */
/*ARGSUSED*/
static void
nmg_ed( arg )
int arg;
{
	switch(arg)  {
	default:
		(void)printf("nmg_ed: undefined menu event?\n");
		return;
	case ECMD_NMG_EPICK:
	case ECMD_NMG_EMOVE:
		break;
	case ECMD_NMG_EDEBUG:
		if( !es_eu )  {
			(void)printf("nmg_ed: no edge selected yet\n");
			return;
		}
		nmg_pr_fu_around_eu( es_eu, &mged_tol );
		{
			struct model		*m;
			struct rt_vlblock	*vbp;
			long			*tab;

			m = nmg_find_model( &es_eu->l.magic );
			NMG_CK_MODEL(m);

			/* get space for list of items processed */
			tab = (long *)rt_calloc( m->maxindex+1, sizeof(long),
				"nmg_ed tab[]");
			vbp = rt_vlblock_init();

			nmg_vlblock_around_eu(vbp, es_eu, tab, 1, &mged_tol);
			cvt_vlblock_to_solids( vbp, "_EU_", 0 );	/* swipe vlist */

			rt_vlblock_free(vbp);
			rt_free( (char *)tab, "nmg_ed tab[]" );
			dmaflag = 1;
		}
		if( *es_eu->up.magic_p == NMG_LOOPUSE_MAGIC )  {
			nmg_veu( &es_eu->up.lu_p->down_hd, es_eu->up.magic_p );
		}
		/* no change of state or es_edflag */
		return;
	case ECMD_NMG_FORW:
		if( !es_eu )  {
			(void)printf("nmg_ed: no edge selected yet\n");
			return;
		}
		NMG_CK_EDGEUSE(es_eu);
		es_eu = RT_LIST_PNEXT_CIRC(edgeuse, es_eu);
		(void)printf("edgeuse selected=x%x\n", es_eu);
		sedraw = 1;
		return;
	case ECMD_NMG_BACK:
		if( !es_eu )  {
			(void)printf("nmg_ed: no edge selected yet\n");
			return;
		}
		NMG_CK_EDGEUSE(es_eu);
		es_eu = RT_LIST_PPREV_CIRC(edgeuse, es_eu);
		(void)printf("edgeuse selected=x%x\n", es_eu);
		sedraw = 1;
		return;
	case ECMD_NMG_RADIAL:
		if( !es_eu )  {
			(void)printf("nmg_ed: no edge selected yet\n");
			return;
		}
		NMG_CK_EDGEUSE(es_eu);
		es_eu = es_eu->eumate_p->radial_p;
		(void)printf("edgeuse selected=x%x\n", es_eu);
		sedraw = 1;
		return;

	}
	/* For example, this will set es_edflag = ECMD_NMG_EPICK */
	es_edflag = arg;
	sedraw = 1;
}

/*
 *  Keypoint in model space is established in "pt".
 *  If "str" is set, then that point is used, else default
 *  for this solid is selected and set.
 *  "str" may be a constant string, in either upper or lower case,
 *  or it may be something complex like "(3,4)" for an ARS or spline
 *  to select a particular vertex or control point.
 *
 *  XXX Perhaps this should be done via solid-specific parse tables,
 *  so that solids could be pretty-printed & structprint/structparse
 *  processed as well?
 */
void
get_solid_keypoint( pt, strp, ip, mat )
point_t		pt;
char		**strp;
struct rt_db_internal	*ip;
mat_t		mat;
{
	char	*cp = *strp;
	point_t	mpt;

	RT_CK_DB_INTERNAL( ip );

	switch( ip->idb_type )  {
	case ID_ELL:
		{
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)ip->idb_ptr;
			RT_ELL_CK_MAGIC(ell);

			if( strcmp( cp, "V" ) == 0 )  {
				VMOVE( mpt, ell->v );
				*strp = "V";
				break;
			}
			if( strcmp( cp, "A" ) == 0 )  {
				VMOVE( mpt, ell->a );
				*strp = "A";
				break;
			}
			if( strcmp( cp, "B" ) == 0 )  {
				VMOVE( mpt, ell->b );
				*strp = "B";
				break;
			}
			if( strcmp( cp, "C" ) == 0 )  {
				VMOVE( mpt, ell->c );
				*strp = "C";
				break;
			}
			/* Default */
			VMOVE( mpt, ell->v );
			*strp = "V";
			break;
		}
	case ID_TOR:
		{
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)ip->idb_ptr;
			RT_TOR_CK_MAGIC(tor);

			if( strcmp( cp, "V" ) == 0 )  {
				VMOVE( mpt, tor->v );
				*strp = "V";
				break;
			}
			/* Default */
			VMOVE( mpt, tor->v );
			*strp = "V";
			break;
		}
	case ID_TGC:
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)ip->idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( strcmp( cp, "V" ) == 0 )  {
				VMOVE( mpt, tgc->v );
				*strp = "V";
				break;
			}
			if( strcmp( cp, "H" ) == 0 )  {
				VMOVE( mpt, tgc->h );
				*strp = "H";
				break;
			}
			if( strcmp( cp, "A" ) == 0 )  {
				VMOVE( mpt, tgc->a );
				*strp = "A";
				break;
			}
			if( strcmp( cp, "B" ) == 0 )  {
				VMOVE( mpt, tgc->b );
				*strp = "B";
				break;
			}
			if( strcmp( cp, "C" ) == 0 )  {
				VMOVE( mpt, tgc->c );
				*strp = "C";
				break;
			}
			if( strcmp( cp, "D" ) == 0 )  {
				VMOVE( mpt, tgc->d );
				*strp = "D";
				break;
			}
			/* Default */
			VMOVE( mpt, tgc->v );
			*strp = "V";
			break;
		}
	case ID_BSPLINE:
		{
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			register fastf_t	*fp;
			static char		buf[128];

			RT_NURB_CK_MAGIC(sip);
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			fp = &RT_NURB_GET_CONTROL_POINT( surf, spl_ui, spl_vi );
			VMOVE( mpt, fp );
			sprintf(buf, "Surf %d, index %d,%d",
				spl_surfno, spl_ui, spl_vi );
			*strp = buf;
			break;
		}
	case ID_GRIP:
		{
			struct rt_grip_internal *gip =
				(struct rt_grip_internal *)ip->idb_ptr;
			RT_GRIP_CK_MAGIC(gip);
			VMOVE( mpt, gip->center);
			*strp = "C";
			break;
		}
	case ID_NMG:
		{
			register struct model *m =
				(struct model *) es_int.idb_ptr;
			NMG_CK_MODEL(m);
			/* XXX Fall through, for now */
		}
	default:
		VSETALL( mpt, 0 );
		*strp = "(origin)";
		break;
	}
	MAT4X3PNT( pt, mat, mpt );
}

/* 	CALC_PLANES()
 *		calculate the plane (face) equations for an arb
 *		in solidrec pointed at by sp
 * XXX replaced by rt_arb_calc_planes()
 */
void
calc_planes( sp, type )
struct solidrec *sp;
int type;
{
	struct solidrec temprec;
	register int i, p1, p2, p3;

	/* find the plane equations */
	/* point notation - use temprec record */
	VMOVE( &temprec.s_values[0], &sp->s_values[0] );
	for(i=3; i<=21; i+=3) {
		VADD2( &temprec.s_values[i], &sp->s_values[i], &sp->s_values[0] );
	}
	type -= 4;	/* ARB4 at location 0, ARB5 at 1, etc */
	for(i=0; i<6; i++) {
		if(arb_faces[type][i*4] == -1)
			break;	/* faces are done */
		p1 = arb_faces[type][i*4];
		p2 = arb_faces[type][i*4+1];
		p3 = arb_faces[type][i*4+2];
		if(planeqn(i, p1, p2, p3, &temprec)) {
			(void)printf("No eqn for face %d%d%d%d\n",
				p1+1,p2+1,p3+1,arb_faces[type][i*4+3]+1);
			return;
		}
	}
}

/*
 *			I N I T _ S E D I T
 *
 *  First time in for this solid, set things up.
 *  If all goes well, change state to ST_S_EDIT.
 *  Solid editing is completed only via sedit_accept() / sedit_reject().
 */
void
init_sedit()
{
	register int i, type, p1, p2, p3;
	int			id;

	/*
	 * Check for a processed region or other illegal solid.
	 */
	if( illump->s_Eflag )  {
		(void)printf(
"Unable to Solid_Edit a processed region;  select a primitive instead\n");
		return;
	}

	/* Read solid description.  Save copy of original data */
	RT_INIT_EXTERNAL(&es_ext);
	RT_INIT_DB_INTERNAL(&es_int);
	if( db_get_external( &es_ext, illump->s_path[illump->s_last], dbip ) < 0 )
		READ_ERR_return;

	id = rt_id_solid( &es_ext );
	if( rt_functab[id].ft_import( &es_int, &es_ext, rt_identity ) < 0 )  {
		rt_log("init_sedit(%s):  solid import failure\n",
			illump->s_path[illump->s_last]->d_namep );
	    	if( es_int.idb_ptr )  rt_functab[id].ft_ifree( &es_int );
		db_free_external( &es_ext );
		return;				/* FAIL */
	}
	RT_CK_DB_INTERNAL( &es_int );

	es_menu = 0;
	new_way = 0;
	bcopy( (char *)es_ext.ext_buf, (char *)&es_rec, sizeof(es_rec) );

	if( es_rec.u_id == ID_SOLID )  {
		struct solidrec temprec;	/* copy of solid to determine type */

		temprec = es_rec.s;		/* struct copy */
		VMOVE( es_keypoint, es_rec.s.s_values );
		es_keyfixed = 0;

		if( (type = es_rec.s.s_cgtype) < 0 )
			type *= -1;
		if(type == BOX || type == RPP)
			type = ARB8;
		if(type == RAW) {
			/* rearrange vectors to correspond to the
			 *  	"standard" ARB6
			 */
			register struct solidrec *trp = &temprec;
			VMOVE(&trp->s_values[3], &es_rec.s.s_values[9]);
			VMOVE(&trp->s_values[6], &es_rec.s.s_values[21]);
			VMOVE(&trp->s_values[9], &es_rec.s.s_values[12]);
			VMOVE(&trp->s_values[12], &es_rec.s.s_values[3]);
			VMOVE(&trp->s_values[15], &es_rec.s.s_values[6]);
			VMOVE(&trp->s_values[18], &es_rec.s.s_values[18]);
			VMOVE(&trp->s_values[21], &es_rec.s.s_values[15]);
			es_rec.s = *trp;	/* struct copy */
			type = ARB6;
		}
		es_rec.s.s_cgtype = type;

		if( es_rec.s.s_type == GENARB8 ) {
			/* find the comgeom arb type */
			if( (type = type_arb( &es_rec )) == 0 ) {
				(void)printf("%s: BAD ARB\n",es_rec.s.s_name);
				return;
			}

			temprec = es_rec.s;
			es_rec.s.s_cgtype = type;
			es_type = type;	/* !!! Needed for facedef.c */

			/* find the plane equations */
			calc_planes( &es_rec.s, type );
		}
	}
#if 1
	/* Experimental, but working. */
	switch( id )  {
	case ID_ELL:
	case ID_EHY:
	case ID_EPA:
	case ID_RPC:
	case ID_RHC:
	case ID_TGC:
	case ID_TOR:
	case ID_ETO:
	case ID_BSPLINE:
	case ID_NMG:
	case ID_GRIP:
		rt_log("Experimental:  new_way=1\n");
		new_way = 1;

	}
	switch( id )  {
	case ID_BSPLINE:
		{
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			RT_NURB_CK_MAGIC(sip);
			spl_surfno = sip->nsrf/2;
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			spl_ui = surf->s_size[1]/2;
			spl_vi = surf->s_size[0]/2;
		}
		break;
	}
#endif

	/* Save aggregate path matrix */
	pathHmat( illump, es_mat, illump->s_last-1 );

	/* get the inverse matrix */
	mat_inv( es_invmat, es_mat );

	/* Establish initial keypoint */
	es_keytag = "";
	get_solid_keypoint( es_keypoint, &es_keytag, &es_int, es_mat );

	es_eu = (struct edgeuse *)NULL;

	sedit_menu();		/* put up menu header */

	/* Finally, enter solid edit state */
	dmp->dmr_light( LIGHT_ON, BE_ACCEPT );
	dmp->dmr_light( LIGHT_ON, BE_REJECT );
	dmp->dmr_light( LIGHT_OFF, BE_S_ILLUMINATE );

	(void)chg_state( ST_S_PICK, ST_S_EDIT, "Keyboard illuminate");
	chg_l2menu(ST_S_EDIT);
	es_edflag = IDLE;
	sedraw = 1;

	button( BE_S_EDIT );	/* Drop into edit menu right away */
}

/*
 *			R E P L O T _ E D I T I N G _ S O L I D
 *
 *  All solid edit routines call this subroutine after
 *  making a change to es_rec or es_mat.
 */
void
replot_editing_solid()
{
	int			id;
	struct rt_external	ext;
	struct rt_db_internal	intern;
	struct rt_db_internal	*ip;
	struct directory	*dp;

	dp = illump->s_path[illump->s_last];

	if( new_way )  {
		ip = &es_int;
	} else {
		/* Fake up an external representation */
		RT_INIT_EXTERNAL( &ext );
		ext.ext_buf = (genptr_t)&es_rec;
		ext.ext_nbytes = sizeof(union record);

		if( (id = rt_id_solid( &ext )) == ID_NULL )  {
			(void)printf("replot_editing_solid() unable to identify type of solid %s\n",
				dp->d_namep );
			return;
		}

	    	RT_INIT_DB_INTERNAL(&intern);
		if( rt_functab[id].ft_import( &intern, &ext, rt_identity ) < 0 )  {
			rt_log("%s:  solid import failure\n",
				dp->d_namep );
		    	if( intern.idb_ptr )  rt_functab[id].ft_ifree( &intern );
		    	return;			/* ERROR */
		}
		ip = &intern;
	}
	RT_CK_DB_INTERNAL( ip );

	(void)replot_modified_solid( illump, ip, es_mat );

	if( !new_way )  {
	    	if( intern.idb_ptr )  rt_functab[id].ft_ifree( &intern );
	}
}

/*
 *			T R A N S F O R M _ E D I T I N G _ S O L I D
 *
 */
void
transform_editing_solid(os, mat, is, free)
struct rt_db_internal	*os;		/* output solid */
mat_t			mat;
struct rt_db_internal	*is;		/* input solid */
int			free;
{
	struct directory	*dp;

	RT_CK_DB_INTERNAL( is );
	if( rt_functab[is->idb_type].ft_xform( os, mat, is, free ) < 0 )
		rt_bomb("transform_editing_solid");
#if 0
/*
 * this is the old way.  rt_db_xform_internal was transfered and
 * modified into rt_generic_xform() in librt/table.c
 * rt_generic_xform is normally called via rt_functab[id].ft_xform()
 */
	dp = illump->s_path[illump->s_last];
	if( rt_db_xform_internal( os, mat, is, free, dp->d_namep ) < 0 )
		rt_bomb("transform_editing_solid");		/* FAIL */
#endif
}

/*
 *			S E D I T _ M E N U
 *
 *
 *  Put up menu header
 */
void
sedit_menu()  {

	menuflag = 0;		/* No menu item selected yet */

	menu_array[MENU_L1] = MENU_NULL;
	chg_l2menu(ST_S_EDIT);
                                                                      
	switch( es_int.idb_type ) {

	case ID_ARB8:
		menu_array[MENU_L1] = cntrl_menu;
		break;
	case ID_TGC:
		menu_array[MENU_L1] = tgc_menu;
		break;
	case ID_TOR:
		menu_array[MENU_L1] = tor_menu;
		break;
	case ID_ELL:
		menu_array[MENU_L1] = ell_menu;
		break;
	case ID_ARS:
		menu_array[MENU_L1] = ars_menu;
		break;
	case ID_BSPLINE:
		menu_array[MENU_L1] = spline_menu;
		break;
	case ID_RPC:
		menu_array[MENU_L1] = rpc_menu;
		break;
	case ID_RHC:
		menu_array[MENU_L1] = rhc_menu;
		break;
	case ID_EPA:
		menu_array[MENU_L1] = epa_menu;
		break;
	case ID_EHY:
		menu_array[MENU_L1] = ehy_menu;
		break;
	case ID_ETO:
		menu_array[MENU_L1] = eto_menu;
		break;
	case ID_NMG:
		menu_array[MENU_L1] = nmg_menu;
		break;
	}
	es_edflag = IDLE;	/* Drop out of previous edit mode */
	es_menu = 0;
}

/* 			C A L C _ P N T S (  )
 * XXX replaced by rt_arb_calc_points() in facedef.c
 *
 * Takes the array es_peqn[] and intersects the planes to find the vertices
 * of a GENARB8.  The vertices are stored in the solid record 'old_srec' which
 * is of type 'type'.  If intersect fails, the points (in vector notation) of
 * 'old_srec' are used to clean up the array es_peqn[] for anyone else. The
 * vertices are put in 'old_srec' in vector notation.  This is an analog to
 * calc_planes().
 */
void
calc_pnts( old_srec, type )
struct solidrec *old_srec;
int type;
{
	struct solidrec temp_srec;
	short int i;

	/* find new points for entire solid */
	for(i=0; i<8; i++){
		/* use temp_srec until we know intersect doesn't fail */
		if( intersect(type,i*3,i,&temp_srec) ){
			(void)printf("Intersection of planes fails\n");
			/* clean up array es_peqn for anyone else */
			calc_planes( old_srec, type );
			return;				/* failure */
		}
	}

	/* back to vector notation */
	VMOVE( &old_srec->s_values[0], &temp_srec.s_values[0] );
	for(i=3; i<=21; i+=3){
		VSUB2(	&old_srec->s_values[i],
			&temp_srec.s_values[i],
			&temp_srec.s_values[0]  );
	}
	return;						/* success */
}

/*
 * 			S E D I T
 * 
 * A great deal of magic takes place here, to accomplish solid editing.
 *
 *  Called from mged main loop after any event handlers:
 *		if( sedraw > 0 )  sedit();
 *  to process any residual events that the event handlers were too
 *  lazy to handle themselves.
 *
 *  A lot of processing is deferred to here, so that the "p" command
 *  can operate on an equal footing to mouse events.
 */
void
sedit()
{
	register dbfloat_t *op;
	struct rt_arb_internal *arb;
	fastf_t	*eqp;
	static vect_t work;
	register int i;
	static int pnt5;		/* ECMD_ARB_SETUP_ROTFACE, special arb7 case */
	static int j;
	static float la, lb, lc, ld;	/* TGC: length of vectors */

	sedraw = 0;

	switch( es_edflag ) {

	case IDLE:
		/* do nothing */
		break;

	case ECMD_ARB_MAIN_MENU:
		/* put up control (main) menu for GENARB8s */
		menuflag = 0;
		es_edflag = IDLE;
		menu_array[MENU_L1] = cntrl_menu;
		break;

	case ECMD_ARB_SPECIFIC_MENU:
		/* put up specific arb edit menus */
		menuflag = 0;
		es_edflag = IDLE;
		switch( es_menu ){
			case MENU_ARB_MV_EDGE:  
				menu_array[MENU_L1] = which_menu[es_type-4];
				break;
			case MENU_ARB_MV_FACE:
				menu_array[MENU_L1] = which_menu[es_type+1];
				break;
			case MENU_ARB_ROT_FACE:
				menu_array[MENU_L1] = which_menu[es_type+6];
				break;
			default:
				(void)printf("Bad menu item.\n");
				return;
		}
		break;

	case ECMD_ARB_MOVE_FACE:
		/* move face through definite point */
		if(inpara) {
			arb = (struct rt_arb_internal *)es_int.idb_ptr;
			/* apply es_invmat to convert to real model space */
			MAT4X3PNT(work,es_invmat,es_para);
			/* change D of planar equation */
			es_peqn[es_menu][3]=VDOT(&es_peqn[es_menu][0], work);
			/* find new vertices, put in record in vector notation */
			(void)rt_arb_calc_points( arb , es_type , es_peqn , &mged_tol );
			new_way = 1;
		}
		break;

	case ECMD_ARB_SETUP_ROTFACE:
		new_way = 1;
		arb = (struct rt_arb_internal *)es_int.idb_ptr;

		/* check if point 5 is in the face */
		pnt5 = 0;
		for(i=0; i<4; i++)  {
			if( arb_vertices[es_type-4][es_menu*4+i]==5 )
				pnt5=1;
		}
		
		/* special case for arb7 */
		if( es_type == ARB7  && pnt5 ){
				(void)printf("\nFixed vertex is point 5.\n");
				fixv = 5;
		}
		else{
			/* find fixed vertex for ECMD_ARB_ROTATE_FACE */
			fixv=0;
			do  {
				int	type,loc,valid;
				char	line[128];
				
				type = es_type - 4;
				(void)printf("\nEnter fixed vertex number( ");
				loc = es_menu*4;
				for(i=0; i<4; i++){
					if( arb_vertices[type][loc+i] )
						printf("%d ",
						    arb_vertices[type][loc+i]);
				}
				printf(") [%d]: ",arb_vertices[type][loc]);

				(void)fgets( line, sizeof(line), stdin );
				line[strlen(line)-1] = '\0';		/* remove newline */
				if( feof(stdin) )  quit();
				if( line[0] == '\0' )
					fixv = arb_vertices[type][loc]; 	/* default */
				else
					fixv = atoi( line );
				
				/* check whether nimble fingers entered valid vertex */
				valid = 0;
				for(j=0; j<4; j++)  {
					if( fixv==arb_vertices[type][loc+j] )
						valid=1;
				}
				if( !valid )
					fixv=0;
			} while( fixv <= 0 || fixv > es_type );
		}
		
		pr_prompt();
		fixv--;
		es_edflag = ECMD_ARB_ROTATE_FACE;
		mat_idn( acc_rot_sol );
		dmaflag = 1;	/* draw arrow, etc */
		break;

	case ECMD_ARB_ROTATE_FACE:
		/* rotate a GENARB8 defining plane through a fixed vertex */

		arb = (struct rt_arb_internal *)es_int.idb_ptr;
		new_way = 1;

		if(inpara) {
			static mat_t invsolr;
			static vect_t tempvec;
			static float rota, fb;

			/*
			 * Keyboard parameters in degrees.
			 * First, cancel any existing rotations,
			 * then perform new rotation
			 */
			mat_inv( invsolr, acc_rot_sol );
			eqp = &es_peqn[es_menu][0];	/* es_menu==plane of interest */
			VMOVE( work, eqp );
			MAT4X3VEC( eqp, invsolr, work );

			if( inpara == 3 ){
				/* 3 params:  absolute X,Y,Z rotations */
				/* Build completely new rotation change */
				mat_idn( modelchanges );
				buildHrot( modelchanges,
					es_para[0] * degtorad,
					es_para[1] * degtorad,
					es_para[2] * degtorad );
				mat_copy(acc_rot_sol, modelchanges);

				/* Apply new rotation to face */
				eqp = &es_peqn[es_menu][0];
				VMOVE( work, eqp );
				MAT4X3VEC( eqp, modelchanges, work );
			}
			else if( inpara == 2 ){
				/* 2 parameters:  rot,fb were given */
				rota= es_para[0] * degtorad;
				fb  = es_para[1] * degtorad;
	
				/* calculate normal vector (length=1) from rot,fb */
				es_peqn[es_menu][0] = cos(fb) * cos(rota);
				es_peqn[es_menu][1] = cos(fb) * sin(rota);
				es_peqn[es_menu][2] = sin(fb);
			}
			else{
				(void)printf("Must be < rot fb | xdeg ydeg zdeg >\n");
				return;
			}

			/* point notation of fixed vertex */
			VMOVE( tempvec, arb->pt[fixv] );

			/* set D of planar equation to anchor at fixed vertex */
			/* es_menu == plane of interest */
			es_peqn[es_menu][3]=VDOT(eqp,tempvec);	

			/*  Clear out solid rotation */
			mat_idn( modelchanges );

		}  else  {
			/* Apply incremental changes */
			static vect_t tempvec;

			eqp = &es_peqn[es_menu][0];
			VMOVE( work, eqp );
			MAT4X3VEC( eqp, incr_change, work );

			/* point notation of fixed vertex */
			VMOVE( tempvec, arb->pt[fixv] );

			/* set D of planar equation to anchor at fixed vertex */
			/* es_menu == plane of interest */
			es_peqn[es_menu][3]=VDOT(eqp,tempvec);	
		}

		(void)rt_arb_calc_points( arb , es_type , es_peqn , &mged_tol );
		mat_idn( incr_change );

		/* no need to calc_planes again */
		replot_editing_solid();

		inpara = 0;
		return;

	case SSCALE:
		/* scale the solid uniformly about it's vertex point */
		if(inpara) {
			/* accumulate the scale factor */
			es_scale = es_para[0] / acc_sc_sol;
			acc_sc_sol = es_para[0];

		}
		if( new_way )  {
			mat_t	scalemat;
			mat_scale_about_pt( scalemat, es_keypoint, es_scale );
			transform_editing_solid(&es_int, scalemat, &es_int, 1);
		} else {
			for(i=3; i<=21; i+=3) { 
				op = &es_rec.s.s_values[i];
				VSCALE(op,op,es_scale);
			}
		}
		/* reset solid scale factor */
		es_scale = 1.0;
		break;

	case STRANS:
		/* translate solid  */
		if(inpara) {
			/* Keyboard parameter.
			 * Apply inverse of es_mat to these
			 * model coordinates first, because sedit_mouse()
			 * has already applied es_mat to them.
			 * XXX this does not make sense.
			 */
			MAT4X3PNT( work, es_invmat, es_para );
			if( new_way )  {
				vect_t	delta;
				mat_t	xlatemat;

				/* Need vector from current vertex/keypoint
				 * to desired new location.
				 */
				VSUB2( delta, es_para, es_keypoint );
				mat_idn( xlatemat );
				MAT_DELTAS_VEC( xlatemat, delta );
				transform_editing_solid(&es_int, xlatemat, &es_int, 1);
			} else {
				VMOVE(es_rec.s.s_values, work);
			}
		}
		break;
	case ECMD_VTRANS:
		/* translate a vertex */
		if( es_mvalid )  {
			/* Mouse parameter:  new position in model space */
			VMOVE( es_para, es_mparam );
			inpara = 1;
		}
		if(inpara) {
			/* Keyboard parameter:  new position in model space.
			/* XXX for now, splines only here */
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			register fastf_t	*fp;

			RT_NURB_CK_MAGIC(sip);
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			fp = &RT_NURB_GET_CONTROL_POINT( surf, spl_ui, spl_vi );
			VMOVE( fp, es_para );
		}
		break;

	case ECMD_TGC_MV_H:
		/*
		 * Move end of H of tgc, keeping plates perpendicular
		 * to H vector.
		 */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			if( inpara ) {
				/* apply es_invmat to convert to real model coordinates */
				MAT4X3PNT( work, es_invmat, es_para );
				VSUB2(tgc->h, work, tgc->v);
			}

			/* check for zero H vector */
			if( MAGNITUDE( tgc->h ) <= SQRT_SMALL_FASTF ) {
				(void)printf("Zero H vector not allowed, resetting to +Z\n");
				VSET(tgc->h, 0, 0, 1 );
				break;
			}

			/* have new height vector --  redefine rest of tgc */
			la = MAGNITUDE( tgc->a );
			lb = MAGNITUDE( tgc->b );
			lc = MAGNITUDE( tgc->c );
			ld = MAGNITUDE( tgc->d );

			/* find 2 perpendicular vectors normal to H for new A,B */
			mat_vec_perp( tgc->b, tgc->h );
			VCROSS(tgc->a, tgc->b, tgc->h);
			VUNITIZE(tgc->a);
			VUNITIZE(tgc->b);

			/* Create new C,D from unit length A,B, with previous len */
			VSCALE(tgc->c, tgc->a, lc);
			VSCALE(tgc->d, tgc->b, ld);

			/* Restore original vector lengths to A,B */
			VSCALE(tgc->a, tgc->a, la);
			VSCALE(tgc->b, tgc->b, lb);
		} else {
		if( inpara ) {
			/* apply es_invmat to convert to real model coordinates */
			MAT4X3PNT( work, es_invmat, es_para );
			VSUB2(&es_rec.s.s_tgc_H, work, &es_rec.s.s_tgc_V);
		}

		/* check for zero H vector */
		if( MAGNITUDE( &es_rec.s.s_tgc_H ) == 0.0 ) {
			(void)printf("Zero H vector not allowed, resetting to +Z\n");
			VSET( &es_rec.s.s_tgc_H, 0, 0, 1 );
			break;
		}

		/* have new height vector --  redefine rest of tgc */
		la = MAGNITUDE( &es_rec.s.s_tgc_A );
		lb = MAGNITUDE( &es_rec.s.s_tgc_B );
		lc = MAGNITUDE( &es_rec.s.s_tgc_C );
		ld = MAGNITUDE( &es_rec.s.s_tgc_D );

		/* find 2 perpendicular vectors normal to H for new A,B */
		j=0;
		for(i=0; i<3; i++) {
			work[i] = 0.0;
			if( fabs(es_rec.s.s_values[i+3]) < 
			    fabs(es_rec.s.s_values[j+3]) )
				j = i;
		}
		work[j] = 1.0;
		VCROSS(&es_rec.s.s_tgc_B, work, &es_rec.s.s_tgc_H);
		VCROSS(&es_rec.s.s_tgc_A, &es_rec.s.s_tgc_B, &es_rec.s.s_tgc_H);
		VUNITIZE(&es_rec.s.s_tgc_A);
		VUNITIZE(&es_rec.s.s_tgc_B);

		/* Create new C,D from unit length A,B, with previous len */
		VSCALE(&es_rec.s.s_tgc_C, &es_rec.s.s_tgc_A, lc);
		VSCALE(&es_rec.s.s_tgc_D, &es_rec.s.s_tgc_B, ld);

		/* Restore original vector lengths to A,B */
		VSCALE(&es_rec.s.s_tgc_A, &es_rec.s.s_tgc_A, la);
		VSCALE(&es_rec.s.s_tgc_B, &es_rec.s.s_tgc_B, lb);
		}
		break;

	case ECMD_TGC_MV_HH:
		/* Move end of H of tgc - leave ends alone */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			if( inpara ) {
				/* apply es_invmat to convert to real model coordinates */
				MAT4X3PNT( work, es_invmat, es_para );
				VSUB2(tgc->h, work, tgc->v);
			}

			/* check for zero H vector */
			if( MAGNITUDE( tgc->h ) <= SQRT_SMALL_FASTF ) {
				(void)printf("Zero H vector not allowed, resetting to +Z\n");
				VSET(tgc->h, 0, 0, 1 );
				break;
			}
		} else {
		if( inpara ) {
			/* apply es_invmat to convert to real model coordinates */
			MAT4X3PNT( work, es_invmat, es_para );
			VSUB2(&es_rec.s.s_tgc_H, work, &es_rec.s.s_tgc_V);
		}

		/* check for zero H vector */
		if( MAGNITUDE( &es_rec.s.s_tgc_H ) == 0.0 ) {
			(void)printf("Zero H vector not allowed, resetting to +Z\n");
			VSET( &es_rec.s.s_tgc_H, 0, 0, 1 );
			break;
		}
		}
		break;

	case PSCALE:
		pscale();
		break;

	case PTARB:	/* move an ARB point */
	case EARB:   /* edit an ARB edge */
		if( inpara ) { 
			/* apply es_invmat to convert to real model space */
			MAT4X3PNT( work, es_invmat, es_para );
			editarb( work );
		}
		break;

	case SROT:
		/* rot solid about vertex */
		if(inpara) {
			static mat_t invsolr;
			/*
			 * Keyboard parameters:  absolute x,y,z rotations,
			 * in degrees.  First, cancel any existing rotations,
			 * then perform new rotation
			 */
			mat_inv( invsolr, acc_rot_sol );

			/* Build completely new rotation change */
			mat_idn( modelchanges );
			buildHrot( modelchanges,
				es_para[0] * degtorad,
				es_para[1] * degtorad,
				es_para[2] * degtorad );
			/* Borrow incr_change matrix here */
			mat_mul( incr_change, modelchanges, invsolr );
			mat_copy(acc_rot_sol, modelchanges);

			/* Apply new rotation to solid */
			/*  Clear out solid rotation */
			mat_idn( modelchanges );
		}  else  {
			/* Apply incremental changes already in incr_change */
		}
		/* Apply changes to solid */
		if( new_way )  {
			mat_t	mat;
			/* xlate keypoint to origin, rotate, then put back. */
			mat_xform_about_pt( mat, incr_change, es_keypoint );
			transform_editing_solid(&es_int, mat, &es_int, 1);
		} else {
			for(i=1; i<8; i++) {
				op = &es_rec.s.s_values[i*3];
				VMOVE( work, op );
				MAT4X3VEC( op, incr_change, work );
			}
		}
		mat_idn( incr_change );
		break;

	case ECMD_TGC_ROT_H:
		/* rotate height vector */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			MAT4X3VEC(work, incr_change, tgc->h);
			VMOVE(tgc->h, work);
		} else {
			MAT4X3VEC(work, incr_change, &es_rec.s.s_tgc_H);
			VMOVE(&es_rec.s.s_tgc_H, work);
		}
		mat_idn( incr_change );
		break;

	case ECMD_TGC_ROT_AB:
		/* rotate surfaces AxB and CxD (tgc) */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			MAT4X3VEC(work, incr_change, tgc->a);
			VMOVE(tgc->a, work);
			MAT4X3VEC(work, incr_change, tgc->b);
			VMOVE(tgc->b, work);
			MAT4X3VEC(work, incr_change, tgc->c);
			VMOVE(tgc->c, work);
			MAT4X3VEC(work, incr_change, tgc->d);
			VMOVE(tgc->d, work);
		} else {
			for(i=2; i<6; i++) {
				op = &es_rec.s.s_values[i*3];
				MAT4X3VEC( work, incr_change, op );
				VMOVE( op, work );
			}
		}
		mat_idn( incr_change );
		break;

	case ECMD_ETO_ROT_C:
		/* rotate ellipse semi-major axis vector */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;

			RT_ETO_CK_MAGIC(eto);
			MAT4X3VEC(work, incr_change, eto->eto_C);
			VMOVE(eto->eto_C, work);
		}
		mat_idn( incr_change );
		break;

	case ECMD_NMG_EPICK:
	case ECMD_NMG_EMOVE:
		/* XXX Nothing to do here (yet), all done in mouse routine. */
		break;

	default:
		(void)printf("sedit():  unknown edflag = %d.\n", es_edflag );
	}

	/* must re-calculate the face plane equations for arbs */
	if( es_int.idb_type == GENARB8 )
		(void)rt_arb_calc_planes( es_peqn , arb , es_type , &mged_tol );

	/* If the keypoint changed location, find about it here */
	if( new_way )  {
		if (! es_keyfixed) {
			get_solid_keypoint( es_keypoint, &es_keytag, &es_int, es_mat );
		}
	}

	replot_editing_solid();

	inpara = 0;
	es_mvalid = 0;
	return;
}

/*
 *			S E D I T _ M O U S E
 *
 *  Mouse (pen) press in graphics area while doing Solid Edit.
 *  mousevec [X] and [Y] are in the range -1.0...+1.0, corresponding
 *  to viewspace.
 *
 *  In order to allow the "p" command to do the same things that
 *  a mouse event can, the preferred strategy is to store the value
 *  corresponding to what the "p" command would give in es_mparam,
 *  set es_mvalid=1, set sedraw=1, and return, allowing sedit()
 *  to actually do the work.
 */
void
sedit_mouse( mousevec )
CONST vect_t	mousevec;
{
	vect_t	pos_view;	 	/* Unrotated view space pos */
	vect_t	pos_model;		/* Rotated screen space pos */
	vect_t	tr_temp;		/* temp translation vector */
	vect_t	temp;
	struct rt_arb_internal *arb;

	if( es_edflag <= 0 )  return;
	switch( es_edflag )  {

	case SSCALE:
	case PSCALE:
		/* use mouse to get a scale factor */
		es_scale = 1.0 + 0.25 * ((fastf_t)
			(mousevec[Y] > 0 ? mousevec[Y] : -mousevec[Y]));
		if ( mousevec[Y] <= 0 )
			es_scale = 1.0 / es_scale;

		/* accumulate scale factor */
		acc_sc_sol *= es_scale;

		sedraw = 1;
		return;
	case STRANS:
		/* 
		 * Use mouse to change solid's location.
		 * Project solid's keypoint into view space,
		 * replace X,Y (but NOT Z) components, and
		 * project result back to model space.
		 * Then move keypoint there.
		 */
		if( new_way )  {
			point_t	pt;
			vect_t	delta;
			mat_t	xlatemat;

			MAT4X3PNT( temp, es_mat, es_keypoint );
			MAT4X3PNT( pos_view, model2view, temp );
			pos_view[X] = mousevec[X];
			pos_view[Y] = mousevec[Y];
			MAT4X3PNT( temp, view2model, pos_view );
			MAT4X3PNT( pt, es_invmat, temp );

			/* Need vector from current vertex/keypoint
			 * to desired new location.
			 */
			VSUB2( delta, es_keypoint, pt );
			mat_idn( xlatemat );
			MAT_DELTAS_VEC_NEG( xlatemat, delta );
			transform_editing_solid(&es_int, xlatemat, &es_int, 1);
		} else {
			/* XXX this makes bad assumptions about format of es_rec !! */
			MAT4X3PNT( temp, es_mat, es_rec.s.s_values );
			MAT4X3PNT( pos_view, model2view, temp );
			pos_view[X] = mousevec[X];
			pos_view[Y] = mousevec[Y];
			MAT4X3PNT( temp, view2model, pos_view );
			MAT4X3PNT( es_rec.s.s_values, es_invmat, temp );
		}
		sedraw = 1;
		return;
	case ECMD_VTRANS:
		/* 
		 * Use mouse to change a vertex location.
		 * Project vertex (in solid keypoint) into view space,
		 * replace X,Y (but NOT Z) components, and
		 * project result back to model space.
		 * Leave desired location in es_mparam.
		 */
		MAT4X3PNT( temp, es_mat, es_keypoint );
		MAT4X3PNT( pos_view, model2view, temp );
		pos_view[X] = mousevec[X];
		pos_view[Y] = mousevec[Y];
		MAT4X3PNT( temp, view2model, pos_view );
		MAT4X3PNT( es_mparam, es_invmat, temp );
		es_mvalid = 1;	/* es_mparam is valid */
		/* Leave the rest to code in sedit() */
		sedraw = 1;
		return;
	case ECMD_TGC_MV_H:
	case ECMD_TGC_MV_HH:
		/* Use mouse to change location of point V+H */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			VADD2( temp, tgc->v, tgc->h );
			MAT4X3PNT(pos_model, es_mat, temp);
			MAT4X3PNT( pos_view, model2view, pos_model );
			pos_view[X] = mousevec[X];
			pos_view[Y] = mousevec[Y];
			/* Do NOT change pos_view[Z] ! */
			MAT4X3PNT( temp, view2model, pos_view );
			MAT4X3PNT( tr_temp, es_invmat, temp );
			VSUB2( tgc->h, tr_temp, tgc->v );
		} else {
			VADD2( temp, &es_rec.s.s_tgc_V, &es_rec.s.s_tgc_H );
			MAT4X3PNT(pos_model, es_mat, temp);
			MAT4X3PNT( pos_view, model2view, pos_model );
			pos_view[X] = mousevec[X];
			pos_view[Y] = mousevec[Y];
			/* Do NOT change pos_view[Z] ! */
			MAT4X3PNT( temp, view2model, pos_view );
			MAT4X3PNT( tr_temp, es_invmat, temp );
			VSUB2( &es_rec.s.s_tgc_H, tr_temp, &es_rec.s.s_tgc_V );
		}
		sedraw = 1;
		return;
	case PTARB:
		/* move an arb point to indicated point */
		/* point is located at es_values[es_menu*3] */
		VADD2(temp, es_rec.s.s_values, &es_rec.s.s_values[es_menu*3]);
		MAT4X3PNT(pos_model, es_mat, temp);
		MAT4X3PNT(pos_view, model2view, pos_model);
		pos_view[X] = mousevec[X];
		pos_view[Y] = mousevec[Y];
		MAT4X3PNT(temp, view2model, pos_view);
		MAT4X3PNT(pos_model, es_invmat, temp);
		editarb( pos_model );
		sedraw = 1;
		return;
	case EARB:
		/* move arb edge, through indicated point */
		MAT4X3PNT( temp, view2model, mousevec );
		/* apply inverse of es_mat */
		MAT4X3PNT( pos_model, es_invmat, temp );
		editarb( pos_model );
		sedraw = 1;
		return;
	case ECMD_ARB_MOVE_FACE:
		/* move arb face, through  indicated  point */
		MAT4X3PNT( temp, view2model, mousevec );
		/* apply inverse of es_mat */
		MAT4X3PNT( pos_model, es_invmat, temp );
		/* change D of planar equation */
		es_peqn[es_menu][3]=VDOT(&es_peqn[es_menu][0], pos_model);
		/* calculate new vertices, put in record as vectors */
		calc_pnts( &es_rec.s, es_rec.s.s_cgtype );
		sedraw = 1;
		return;

	case ECMD_NMG_EPICK:
		/* XXX Should just leave desired location in es_mparam for sedit() */
		{
			struct model	*m = 
				(struct model *)es_int.idb_ptr;
			struct edge	*e;
			NMG_CK_MODEL(m);
			if( (e = nmg_find_e_nearest_pt2( &m->magic, mousevec,
			    model2view, &mged_tol )) == (struct edge *)NULL )  {
				(void)printf("ECMD_NMG_EPICK: unable to find an edge\n");
				return;
			}
			es_eu = e->eu_p;
			NMG_CK_EDGEUSE(es_eu);
			(void)printf("edgeuse selected=x%x\n", es_eu);
			sedraw = 1;
		}
		break;

	/* XXX Should just leave desired location in es_mparam for sedit() */
	case ECMD_NMG_EMOVE:
		/* move edge, through indicated point */
		MAT4X3PNT( temp, view2model, mousevec );
		/* apply inverse of es_mat */
		MAT4X3PNT( pos_model, es_invmat, temp );
		if( nmg_move_edge_thru_pt( es_eu, pos_model, &mged_tol ) < 0 ) {
			VPRINT("Unable to hit", pos_model);
		}
		sedraw = 1;
		return;

	default:
		(void)printf("mouse press undefined in this solid edit mode\n");
		break;
	}

	/* XXX I would prefer to see an explicit call to the guts of sedit()
	 * XXX here, rather than littering the place with global variables
	 * XXX for later interpretation.
	 */
}

/*
 *  Object Edit
 */
void
objedit_mouse( mousevec )
CONST vect_t	mousevec;
{
	fastf_t			scale;
	vect_t	pos_view;	 	/* Unrotated view space pos */
	vect_t	pos_model;	/* Rotated screen space pos */
	vect_t	tr_temp;		/* temp translation vector */
	vect_t	temp;

	mat_idn( incr_change );
	scale = 1;
	if( movedir & SARROW )  {
		/* scaling option is in effect */
		scale = 1.0 + (fastf_t)(mousevec[Y]>0 ?
			mousevec[Y] : -mousevec[Y]);
		if ( mousevec[Y] <= 0 )
			scale = 1.0 / scale;

		/* switch depending on scaling option selected */
		switch( edobj ) {

			case BE_O_SCALE:
				/* global scaling */
				incr_change[15] = 1.0 / scale;
			break;

			case BE_O_XSCALE:
				/* local scaling ... X-axis */
				incr_change[0] = scale;
				/* accumulate the scale factor */
				acc_sc[0] *= scale;
			break;

			case BE_O_YSCALE:
				/* local scaling ... Y-axis */
				incr_change[5] = scale;
				/* accumulate the scale factor */
				acc_sc[1] *= scale;
			break;

			case BE_O_ZSCALE:
				/* local scaling ... Z-axis */
				incr_change[10] = scale;
				/* accumulate the scale factor */
				acc_sc[2] *= scale;
			break;
		}

		/* Have scaling take place with respect to keypoint,
		 * NOT the view center.
		 */
		MAT4X3PNT(temp, es_mat, es_keypoint);
		MAT4X3PNT(pos_model, modelchanges, temp);
		wrt_point(modelchanges, incr_change, modelchanges, pos_model);
	}  else if( movedir & (RARROW|UARROW) )  {
		mat_t oldchanges;	/* temporary matrix */

		/* Vector from object keypoint to cursor */
		MAT4X3PNT( temp, es_mat, es_keypoint );
		MAT4X3PNT( pos_view, model2objview, temp );
		if( movedir & RARROW )
			pos_view[X] = mousevec[X];
		if( movedir & UARROW )
			pos_view[Y] = mousevec[Y];

		MAT4X3PNT( pos_model, view2model, pos_view );/* NOT objview */
		MAT4X3PNT( tr_temp, modelchanges, temp );
		VSUB2( tr_temp, pos_model, tr_temp );
		MAT_DELTAS(incr_change,
			tr_temp[X], tr_temp[Y], tr_temp[Z]);
		mat_copy( oldchanges, modelchanges );
		mat_mul( modelchanges, incr_change, oldchanges );
	}  else  {
		(void)printf("No object edit mode selected;  mouse press ignored\n");
		return;
	}
	mat_idn( incr_change );
	new_mats();
}

/*
 *			V L S _ S O L I D
 */
void
vls_solid( vp, ip, mat )
register struct rt_vls		*vp;
CONST struct rt_db_internal	*ip;
CONST mat_t			mat;
{
	struct rt_external	ext;
	struct rt_db_internal	intern;
	struct solidrec		sol;
	mat_t			ident;
	int			id;

	RT_VLS_CHECK(vp);
	RT_CK_DB_INTERNAL(ip);

	id = ip->idb_type;
	transform_editing_solid( &intern, mat, ip, 0 );

	if( rt_functab[id].ft_describe( vp, &intern, 1 /*verbose*/,
	    base2local ) < 0 )
		printf("vls_solid: describe error\n");
	rt_functab[id].ft_ifree( &intern );
}

/*
 *  			P S C A L E
 *  
 *  Partial scaling of a solid.
 */
void
pscale()
{
	register dbfloat_t *op;
	static fastf_t ma,mb;
	static fastf_t mr1,mr2;

	switch( es_menu ) {

	case MENU_TGC_SCALE_H:	/* scale height vector */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->h);
			}
			VSCALE(tgc->h, tgc->h, es_scale);
		} else {
			op = &es_rec.s.s_tgc_H;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_TOR_R1:
		/* scale radius 1 of TOR */
		if( new_way )  {
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)es_int.idb_ptr;
			fastf_t	newrad;
			RT_TOR_CK_MAGIC(tor);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = tor->r_a * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			if( tor->r_h <= newrad )
				tor->r_a = newrad;
		} else {
			mr2 = MAGNITUDE(&es_rec.s.s_tor_H);
			op = &es_rec.s.s_tor_B;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);

			op = &es_rec.s.s_tor_A;
			VSCALE(op, op, es_scale);
			mr1 = MAGNITUDE(op);
			if( mr1 < mr2 ) {
				VSCALE(op, op, (mr2+0.01)/mr1);
				op = &es_rec.s.s_tor_B;
				VSCALE(op, op, (mr2+0.01)/mr1);
				mr1 = MAGNITUDE(op);
			}
torcom:
			ma = mr1 - mr2;
			op = &es_rec.s.s_tor_C;
			mb = MAGNITUDE(op);
			VSCALE(op, op, ma/mb);

			op = &es_rec.s.s_tor_D;
			mb = MAGNITUDE(op);
			VSCALE(op, op, ma/mb);

			ma = mr1 + mr2;
			op = &es_rec.s.s_tor_E;
			mb = MAGNITUDE(op);
			VSCALE(op, op, ma/mb);

			op = &es_rec.s.s_tor_F;
			mb = MAGNITUDE(op);
			VSCALE(op, op, ma/mb);
		}
		break;

	case MENU_TOR_R2:
		/* scale radius 2 of TOR */
		if( new_way )  {
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)es_int.idb_ptr;
			fastf_t	newrad;
			RT_TOR_CK_MAGIC(tor);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = tor->r_h * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			if( newrad <= tor->r_a )
				tor->r_h = newrad;
		} else {
			op = &es_rec.s.s_values[3];
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
			mr2 = MAGNITUDE(op);
			mr1 = MAGNITUDE(&es_rec.s.s_values[6]);
			if(mr1 < mr2) {
				VSCALE(op, op, (mr1-0.01)/mr2);
				mr2 = MAGNITUDE(op);
			}
			goto torcom;
		}
		break;

	case MENU_ETO_R:
		/* scale radius 1 (r) of ETO */
		/* new_way only */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	ch, cv, dh, newrad;
			vect_t	Nu;

			RT_ETO_CK_MAGIC(eto);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = eto->eto_r * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			VMOVE(Nu, eto->eto_N);
			VUNITIZE(Nu);
			/* get horiz and vert components of C and Rd */
			cv = VDOT( eto->eto_C, Nu );
			ch = sqrt( VDOT( eto->eto_C, eto->eto_C ) - cv * cv );
			/* angle between C and Nu */
			dh = eto->eto_rd * cv / MAGNITUDE(eto->eto_C);
			/* make sure revolved ellipse doesn't overlap itself */
			if (ch <= newrad && dh <= newrad)
				eto->eto_r = newrad;
		}
		break;

	case MENU_ETO_RD:
		/* scale Rd, ellipse semi-minor axis length, of ETO */
		/* new_way only */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	dh, newrad, work;
			vect_t	Nu;

			RT_ETO_CK_MAGIC(eto);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = eto->eto_rd * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			work = MAGNITUDE(eto->eto_C);
				if (newrad <= work) {
				VMOVE(Nu, eto->eto_N);
				VUNITIZE(Nu);
				dh = newrad * VDOT( eto->eto_C, Nu ) / work;
				/* make sure revolved ellipse doesn't overlap itself */
				if (dh <= eto->eto_r)
					eto->eto_rd = newrad;
			}
		}
		break;

	case MENU_ETO_SCALE_C:
		/* scale vector C */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	ch, cv;
			vect_t	Nu, Work;

			RT_ETO_CK_MAGIC(eto);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(eto->eto_C);
			}
			if (es_scale * MAGNITUDE(eto->eto_C) >= eto->eto_rd) {
				VMOVE(Nu, eto->eto_N);
				VUNITIZE(Nu);
				VSCALE(Work, eto->eto_C, es_scale);
				/* get horiz and vert comps of C and Rd */
				cv = VDOT( Work, Nu );
				ch = sqrt( VDOT( Work, Work ) - cv * cv );
				if (ch <= eto->eto_r)
					VMOVE(eto->eto_C, Work);
			}
		}
		break;

	case MENU_RPC_B:
		/* scale vector B */
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;
			RT_RPC_CK_MAGIC(rpc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rpc->rpc_B);
			}
			VSCALE(rpc->rpc_B, rpc->rpc_B, es_scale);
		}
		break;

	case MENU_RPC_H:
		/* scale vector H */
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;

			RT_RPC_CK_MAGIC(rpc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rpc->rpc_H);
			}
			VSCALE(rpc->rpc_H, rpc->rpc_H, es_scale);
		}
		break;

	case MENU_RPC_R:
		/* scale rectangular half-width of RPC */
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_RPC_CK_MAGIC(rpc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / rpc->rpc_r;
			}
			rpc->rpc_r *= es_scale;
		}
		break;

	case MENU_RHC_B:
		/* scale vector B */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			RT_RHC_CK_MAGIC(rhc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rhc->rhc_B);
			}
			VSCALE(rhc->rhc_B, rhc->rhc_B, es_scale);
		}
		break;

	case MENU_RHC_H:
		/* scale vector H */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			RT_RHC_CK_MAGIC(rhc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rhc->rhc_H);
			}
			VSCALE(rhc->rhc_H, rhc->rhc_H, es_scale);
		}
		break;

	case MENU_RHC_R:
		/* scale rectangular half-width of RHC */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_RHC_CK_MAGIC(rhc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / rhc->rhc_r;
			}
			rhc->rhc_r *= es_scale;
		}
		break;

	case MENU_RHC_C:
		/* scale rectangular half-width of RHC */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_RHC_CK_MAGIC(rhc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / rhc->rhc_c;
			}
			rhc->rhc_c *= es_scale;
		}
		break;

	case MENU_EPA_H:
		/* scale height vector H */
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;

			RT_EPA_CK_MAGIC(epa);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(epa->epa_H);
			}
			VSCALE(epa->epa_H, epa->epa_H, es_scale);
		}
		break;

	case MENU_EPA_R1:
		/* scale semimajor axis of EPA */
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_EPA_CK_MAGIC(epa);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / epa->epa_r1;
			}
			if (epa->epa_r1 * es_scale >= epa->epa_r2)
				epa->epa_r1 *= es_scale;
		}
		break;

	case MENU_EPA_R2:
		/* scale semiminor axis of EPA */
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_EPA_CK_MAGIC(epa);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / epa->epa_r2;
			}
			if (epa->epa_r2 * es_scale <= epa->epa_r1)
				epa->epa_r2 *= es_scale;
		}
		break;

	case MENU_EHY_H:
		/* scale height vector H */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(ehy->ehy_H);
			}
			VSCALE(ehy->ehy_H, ehy->ehy_H, es_scale);
		}
		break;

	case MENU_EHY_R1:
		/* scale semimajor axis of EHY */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / ehy->ehy_r1;
			}
			if (ehy->ehy_r1 * es_scale >= ehy->ehy_r2)
				ehy->ehy_r1 *= es_scale;
		}
		break;

	case MENU_EHY_R2:
		/* scale semiminor axis of EHY */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / ehy->ehy_r2;
			}
			if (ehy->ehy_r2 * es_scale <= ehy->ehy_r1)
				ehy->ehy_r2 *= es_scale;
		}
		break;

	case MENU_EHY_C:
		/* scale distance between apex of EHY & asymptotic cone */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;
			fastf_t	newrad;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / ehy->ehy_c;
			}
			ehy->ehy_c *= es_scale;
		}
		break;

	case MENU_TGC_SCALE_A:
		/* scale vector A */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->a);
			}
			VSCALE(tgc->a, tgc->a, es_scale);
		} else {
			op = &es_rec.s.s_tgc_A;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_TGC_SCALE_B:
		/* scale vector B */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->b);
			}
			VSCALE(tgc->b, tgc->b, es_scale);
		} else {
			op = &es_rec.s.s_tgc_B;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_ELL_SCALE_A:
		/* scale vector A */
		if( new_way )  {
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->a);
			}
			VSCALE( ell->a, ell->a, es_scale );
		} else {
			op = &es_rec.s.s_ell_A;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_ELL_SCALE_B:
		/* scale vector B */
		if( new_way )  {
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->b);
			}
			VSCALE( ell->b, ell->b, es_scale );
		} else {
			op = &es_rec.s.s_ell_B;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_ELL_SCALE_C:
		/* scale vector C */
		if( new_way )  {
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->c);
			}
			VSCALE( ell->c, ell->c, es_scale );
		} else {
			op = &es_rec.s.s_ell_C;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_TGC_SCALE_C:
		/* TGC: scale ratio "c" */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->c);
			}
			VSCALE(tgc->c, tgc->c, es_scale);
		} else {
			op = &es_rec.s.s_tgc_C;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_TGC_SCALE_D:   /* scale  d for tgc */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->d);
			}
			VSCALE(tgc->d, tgc->d, es_scale);
		} else {
			op = &es_rec.s.s_tgc_D;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
		}
		break;

	case MENU_TGC_SCALE_AB:
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->a);
			}
			VSCALE(tgc->a, tgc->a, es_scale);
			ma = MAGNITUDE( tgc->a );
			mb = MAGNITUDE( tgc->b );
			VSCALE(tgc->b, tgc->b, ma/mb);
		} else {
			op = &es_rec.s.s_tgc_A;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
			ma = MAGNITUDE( op );
			op = &es_rec.s.s_tgc_B;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
		}
		break;

	case MENU_TGC_SCALE_CD:	/* scale C and D of tgc */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->c);
			}
			VSCALE(tgc->c, tgc->c, es_scale);
			ma = MAGNITUDE( tgc->c );
			mb = MAGNITUDE( tgc->d );
			VSCALE(tgc->d, tgc->d, ma/mb);
		} else {
			op = &es_rec.s.s_tgc_C;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
			ma = MAGNITUDE( op );
			op = &es_rec.s.s_tgc_D;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
		}
		break;

	case MENU_TGC_SCALE_ABCD: 		/* scale A,B,C, and D of tgc */
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->a);
			}
			VSCALE(tgc->a, tgc->a, es_scale);
			ma = MAGNITUDE( tgc->a );
			mb = MAGNITUDE( tgc->b );
			VSCALE(tgc->b, tgc->b, ma/mb);
			mb = MAGNITUDE( tgc->c );
			VSCALE(tgc->c, tgc->c, ma/mb);
			mb = MAGNITUDE( tgc->d );
			VSCALE(tgc->d, tgc->d, ma/mb);
		} else {
			op = &es_rec.s.s_tgc_A;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
			ma = MAGNITUDE( op );
			op = &es_rec.s.s_tgc_B;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
			op = &es_rec.s.s_tgc_C;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
			op = &es_rec.s.s_tgc_D;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
		}
		break;

	case MENU_ELL_SCALE_ABC:	/* set A,B, and C length the same */
		if( new_way )  {
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->a);
			}
			VSCALE( ell->a, ell->a, es_scale );
			ma = MAGNITUDE( ell->a );
			mb = MAGNITUDE( ell->b );
			VSCALE(ell->b, ell->b, ma/mb);
			mb = MAGNITUDE( ell->c );
			VSCALE(ell->c, ell->c, ma/mb);
		} else {
			op = &es_rec.s.s_ell_A;
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(op);
			}
			VSCALE(op, op, es_scale);
			ma = MAGNITUDE( op );
			op = &es_rec.s.s_ell_B;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
			op = &es_rec.s.s_ell_C;
			mb = MAGNITUDE( op );
			VSCALE(op, op, ma/mb);
		}
		break;

	}
}

/*
 *			I N I T _ O B J E D I T
 *
 */
void
init_objedit()
{
	register int		i;
	register int		type;
	int			id;

	/* for safety sake */
	es_menu = 0;
	es_edflag = -1;
	mat_idn(es_mat);

	/*
	 * Check for a processed region 
	 */
	if( illump->s_Eflag )  {

		/* Have a processed (E'd) region - NO key solid.
		 * 	Use the 'center' as the key
		 */
		VMOVE(es_keypoint, illump->s_center);

		/* The s_center takes the es_mat into account already */
	}

	/* Not an evaluated region - just a regular path ending in a solid */
	if( db_get_external( &es_ext, illump->s_path[illump->s_last], dbip ) < 0 )  {
		(void)printf("init_objedit(%s): db_get_external failure\n",
			illump->s_path[illump->s_last]->d_namep );
		button(BE_REJECT);
		return;
	}

	id = rt_id_solid( &es_ext );
	if( rt_functab[id].ft_import( &es_int, &es_ext, rt_identity ) < 0 )  {
		rt_log("init_objedit(%s):  solid import failure\n",
			illump->s_path[illump->s_last]->d_namep );
	    	if( es_int.idb_ptr )  rt_functab[id].ft_ifree( &es_int );
		db_free_external( &es_ext );
		return;				/* FAIL */
	}
	RT_CK_DB_INTERNAL( &es_int );

	/* XXX hack:  get first granule into es_rec (ugh) */
	bcopy( (char *)es_ext.ext_buf, (char *)&es_rec, sizeof(es_rec) );

	/* Find the keypoint for editing */
	id = rt_id_solid( &es_ext );
	switch( id )  {
	case ID_NULL:
		(void)printf("init_objedit(%s): bad database record\n",
			illump->s_path[illump->s_last]->d_namep );
		button(BE_REJECT);
		db_free_external( &es_ext );
		return;

	case ID_ARS_A:
		{
			register union record *rec =
				(union record *)es_ext.ext_buf;

			/* XXX should import the ARS! */

			/* only interested in vertex */
			VMOVE(es_keypoint, rec[1].b.b_values);
			es_rec.s.s_type = ARS;		/* XXX wrong */
			es_rec.s.s_cgtype = ARS;
		}
		break;

	case ID_TOR:
	case ID_TGC:
	case ID_ELL:
	case ID_ARB8:
	case ID_HALF:
	case ID_RPC:
	case ID_RHC:
	case ID_EHY:
	case ID_EPA:
	case ID_ETO:
		/* All folks with u_id == (DB_)ID_SOLID */
		if( es_rec.s.s_cgtype < 0 )
			es_rec.s.s_cgtype *= -1;

		if( es_rec.s.s_type == GENARB8 ) {
			/* find the comgeom arb type */
			if( (type = type_arb( &es_rec )) == 0 ) {
				(void)printf("%s: BAD ARB\n",es_rec.s.s_name);
				return;
			}
			es_rec.s.s_cgtype = type;
		}
		VMOVE( es_keypoint, es_rec.s.s_values );
		break;

	case ID_EBM:
		/* Use model origin as key point */
		VSETALL(es_keypoint, 0 );
		break;

	default:
		VMOVE(es_keypoint, illump->s_center);
		printf("init_objedit() using %g,%g,%g as keypoint\n",
			V3ARGS(es_keypoint) );
	}

	/* Save aggregate path matrix */
	pathHmat( illump, es_mat, illump->s_last-1 );

	/* get the inverse matrix */
	mat_inv( es_invmat, es_mat );

	/* XXX Zap out es_rec, nobody should look there any further */
	bzero( (char *)&es_rec, sizeof(es_rec) );
}

void
oedit_accept()
{
	register struct solid *sp;
	/* matrices used to accept editing done from a depth
	 *	>= 2 from the top of the illuminated path
	 */
	mat_t topm;	/* accum matrix from pathpos 0 to i-2 */
	mat_t inv_topm;	/* inverse */
	mat_t deltam;	/* final "changes":  deltam = (inv_topm)(modelchanges)(topm) */
	mat_t tempm;

	switch( ipathpos )  {
	case 0:
		moveHobj( illump->s_path[ipathpos], modelchanges );
		break;
	case 1:
		moveHinstance(
			illump->s_path[ipathpos-1],
			illump->s_path[ipathpos],
			modelchanges
		);
		break;
	default:
		mat_idn( topm );
		mat_idn( inv_topm );
		mat_idn( deltam );
		mat_idn( tempm );

		pathHmat( illump, topm, ipathpos-2 );

		mat_inv( inv_topm, topm );

		mat_mul( tempm, modelchanges, topm );
		mat_mul( deltam, inv_topm, tempm );

		moveHinstance(
			illump->s_path[ipathpos-1],
			illump->s_path[ipathpos],
			deltam
		);
		break;
	}

	/*
	 *  Redraw all solids affected by this edit.
	 *  Regenerate a new control list which does not
	 *  include the solids about to be replaced,
	 *  so we can safely fiddle the displaylist.
	 */
	modelchanges[15] = 1000000000;	/* => small ratio */
	dmaflag=1;
	refresh();

	/* Now, recompute new chunks of displaylist */
	FOR_ALL_SOLIDS( sp )  {
		if( sp->s_iflag == DOWN )
			continue;
		(void)replot_original_solid( sp );
		sp->s_iflag = DOWN;
	}
	mat_idn( modelchanges );

    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

void
oedit_reject()
{
    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

/* 			F _ E Q N ( )
 * Gets the A,B,C of a  planar equation from the command line and puts the
 * result into the array es_peqn[] at the position pointed to by the variable
 * 'es_menu' which is the plane being redefined. This function is only callable
 * when in solid edit and rotating the face of a GENARB8.
 */
int
f_eqn(argc, argv)
int	argc;
char	*argv[];
{
	short int i;
	vect_t tempvec;

	if( state != ST_S_EDIT ){
		(void)printf("Eqn: must be in solid edit\n");
		return CMD_BAD;
	}
	else if( es_rec.s.s_type != GENARB8 ){
		(void)printf("Eqn: type must be GENARB8\n");
		return CMD_BAD;
	}
	else if( es_edflag != ECMD_ARB_ROTATE_FACE ){
		(void)printf("Eqn: must be rotating a face\n");
		return CMD_BAD;
	}

	/* get the A,B,C from the command line */
	for(i=0; i<3; i++)
		es_peqn[es_menu][i]= atof(argv[i+1]);
	VUNITIZE( &es_peqn[es_menu][0] );

	/* set D of planar equation to anchor at fixed vertex */
	if( fixv ){				/* not the solid vertex */
		VADD2( tempvec, &es_rec.s.s_values[fixv*3], &es_rec.s.s_values[0] );
	}
	else{
		VMOVE( tempvec, &es_rec.s.s_values[0] );
	}
	es_peqn[es_menu][3]=VDOT( &es_peqn[es_menu][0], tempvec );
	
	calc_pnts( &es_rec.s, es_rec.s.s_cgtype );

	/* draw the new version of the solid */
	replot_editing_solid();

	/* update display information */
	dmaflag = 1;

	return CMD_OK;
}

/* Hooks from buttons.c */

void
sedit_accept()
{
	struct directory	*dp;
	int	id;

	if( not_state( ST_S_EDIT, "Solid edit accept" ) )  return;

	/* write editing changes out to disc */
	dp = illump->s_path[illump->s_last];
	if( !new_way )  {
		db_put( dbip, dp, &es_rec, 0, 1 );
	} else {
		/* Scale change on export is 1.0 -- no change */
		if( rt_functab[es_int.idb_type].ft_export( &es_ext, &es_int, 1.0 ) < 0 )  {
			rt_log("sedit_accept(%s):  solid export failure\n", dp->d_namep);
		    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
			db_free_external( &es_ext );
			return;				/* FAIL */
		}
	    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );

		if( db_put_external( &es_ext, dp, dbip ) < 0 )  {
			db_free_external( &es_ext );
			ERROR_RECOVERY_SUGGESTION;
			WRITE_ERR_return;
		}
	}

	es_edflag = -1;
	menuflag = 0;
	movedir = 0;
	new_way = 0;

    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

void
sedit_reject()
{
	if( not_state( ST_S_EDIT, "Solid edit reject" ) )  return;

	/* Restore the original solid */
	replot_original_solid( illump );

	menuflag = 0;
	movedir = 0;
	es_edflag = -1;
	new_way = 0;

    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

/* Input parameter editing changes from keyboard */
/* Format: p dx [dy dz]		*/
int
f_param( argc, argv )
int	argc;
char	**argv;
{
	register int i;

	if( es_edflag <= 0 )  {
		(void)printf("A solid editor option not selected\n");
		return CMD_BAD;
	}
	if( es_edflag == ECMD_TGC_ROT_H
		|| es_edflag == ECMD_TGC_ROT_AB
		|| es_edflag == ECMD_ETO_ROT_C ) {
		(void)printf("\"p\" command not defined for this option\n");
		return CMD_BAD;
	}

	inpara = 0;
	sedraw++;
	for( i = 1; i < argc && i <= 3 ; i++ )  {
		es_para[ inpara++ ] = atof( argv[i] );
	}

	if( es_edflag == PSCALE || es_edflag == SSCALE )  {
		if(es_para[0] <= 0.0) {
			(void)printf("ERROR: SCALE FACTOR <= 0\n");
			inpara = 0;
			sedraw = 0;
			return CMD_BAD;
		}
	}

	/* check if need to convert input values to the base unit */
	switch( es_edflag ) {

		case STRANS:
		case ECMD_VTRANS:
		case PSCALE:
		case EARB:
		case ECMD_ARB_MOVE_FACE:
		case ECMD_TGC_MV_H:
		case ECMD_TGC_MV_HH:
		case PTARB:
			/* must convert to base units */
			es_para[0] *= local2base;
			es_para[1] *= local2base;
			es_para[2] *= local2base;
			/* fall through */
		default:
			return CMD_OK;
	}

	/* XXX I would prefer to see an explicit call to the guts of sedit()
	 * XXX here, rather than littering the place with global variables
	 * XXX for later interpretation.
	 */
}

/*
 *  Returns -
 *	1	solid edit claimes the rotate event
 *	0	rotate event can be used some other way.
 */
int
sedit_rotate( xangle, yangle, zangle )
double	xangle, yangle, zangle;
{
	mat_t	tempp;

	if( es_edflag != ECMD_TGC_ROT_H &&
	    es_edflag != ECMD_TGC_ROT_AB &&
	    es_edflag != SROT &&
	    es_edflag != ECMD_ETO_ROT_C &&
	    es_edflag != ECMD_ARB_ROTATE_FACE)
		return 0;

	mat_idn( incr_change );
	buildHrot( incr_change, xangle, yangle, zangle );

	/* accumulate the translations */
	mat_mul(tempp, incr_change, acc_rot_sol);
	mat_copy(acc_rot_sol, tempp);

	/* sedit() will use incr_change or acc_rot_sol ?? */
	sedit();	/* change es_rec only, NOW */

	return 1;
}

/*
 *  Returns -
 *	1	object edit claimes the rotate event
 *	0	rotate event can be used some other way.
 */
int
objedit_rotate( xangle, yangle, zangle )
double	xangle, yangle, zangle;
{
	mat_t	tempp;
	vect_t	point;

	if( movedir != ROTARROW )  return 0;

	mat_idn( incr_change );
	buildHrot( incr_change, xangle, yangle, zangle );

	/* accumulate change matrix - do it wrt a point NOT view center */
	mat_mul(tempp, modelchanges, es_mat);
	MAT4X3PNT(point, tempp, es_keypoint);
	wrt_point(modelchanges, incr_change, modelchanges, point);

	new_mats();

	return 1;
}

/*
 *			L A B E L _ E D I T E D _ S O L I D
 *
 *  Put labels on the vertices of the currently edited solid.
 *  XXX This really should use import/export interface!!!  Or be part of it.
 */
label_edited_solid( pl, max_pl, xform, ip )
struct rt_point_labels	pl[];
int			max_pl;
CONST mat_t		xform;
struct rt_db_internal	*ip;
{
	register int	i;
	union record	temp_rec;	/* copy of es_rec record */
	point_t		work;
	point_t		pos_view;
	int		npl = 0;

	RT_CK_DB_INTERNAL( ip );

	switch( ip->idb_type )  {

#define	POINT_LABEL( _pt, _char )	{ \
	VMOVE( pl[npl].pt, _pt ); \
	pl[npl].str[0] = _char; \
	pl[npl++].str[1] = '\0'; }

#define	POINT_LABEL_STR( _pt, _str )	{ \
	VMOVE( pl[npl].pt, _pt ); \
	strncpy( pl[npl++].str, _str, sizeof(pl[0].str)-1 ); }

	case ID_ARB8:
		if( new_way )
		{
			struct rt_arb_internal *arb=
				(struct rt_arb_internal *)es_int.idb_ptr;
			RT_ARB_CK_MAGIC( arb );
			switch( es_type )
			{
				case ARB8:
					for( i=0 ; i<8 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					break;
				case ARB7:
					for( i=0 ; i<7 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					break;
				case ARB6:
					for( i=0 ; i<5 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					MAT4X3PNT( pos_view, xform, arb->pt[6] );
					POINT_LABEL( pos_view, '6' );
					break;
				case ARB5:
					for( i=0 ; i<5 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					break;
				case ARB4:
					for( i=0 ; i<3 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					MAT4X3PNT( pos_view, xform, arb->pt[4] );
					POINT_LABEL( pos_view, '4' );
					break;
			}
		}
		else
		{
		MAT4X3PNT( pos_view, xform, es_rec.s.s_values );
		POINT_LABEL( pos_view, '1' );
		temp_rec.s = es_rec.s;
		if(es_type == ARB4) {
			VMOVE(&temp_rec.s.s_values[9], &temp_rec.s.s_values[12]);
		}
		if(es_type == ARB6) {
			VMOVE(&temp_rec.s.s_values[15], &temp_rec.s.s_values[18]);
		}
		for(i=1; i<es_type; i++) {
			VADD2( work, es_rec.s.s_values, &temp_rec.s.s_values[i*3] );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, i + '1' );
		}
		}
		break;
	case ID_TGC:
		if( new_way )  {
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			MAT4X3PNT( pos_view, xform, tgc->v );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, tgc->v, tgc->a );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VADD2( work, tgc->v, tgc->b );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD3( work, tgc->v, tgc->h, tgc->c );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'C' );

			VADD3( work, tgc->v, tgc->h, tgc->d );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'D' );
		} else {
		MAT4X3PNT( pos_view, xform, &es_rec.s.s_tgc_V );
		POINT_LABEL( pos_view, 'V' );

		VADD2( work, &es_rec.s.s_tgc_V, &es_rec.s.s_tgc_A );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'A' );

		VADD2( work, &es_rec.s.s_tgc_V, &es_rec.s.s_tgc_B );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'B' );

		VADD3( work, &es_rec.s.s_tgc_V, &es_rec.s.s_tgc_H, &es_rec.s.s_tgc_C );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'C' );

		VADD3( work, &es_rec.s.s_tgc_V, &es_rec.s.s_tgc_H, &es_rec.s.s_tgc_D );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'D' );
		}
		break;

	case ID_ELL:
		if( new_way )  {
			point_t	work;
			point_t	pos_view;
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);

			MAT4X3PNT( pos_view, xform, ell->v );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, ell->v, ell->a );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VADD2( work, ell->v, ell->b );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD2( work, ell->v, ell->c );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'C' );
		} else {
		MAT4X3PNT( pos_view, xform, &es_rec.s.s_ell_V );
		POINT_LABEL( pos_view, 'V' );

		VADD2( work, &es_rec.s.s_ell_V, &es_rec.s.s_ell_A );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'A' );

		VADD2( work, &es_rec.s.s_ell_V, &es_rec.s.s_ell_B );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'B' );

		VADD2( work, &es_rec.s.s_ell_V, &es_rec.s.s_ell_C );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'C' );
		}
		break;

	case ID_TOR:
		if( new_way )  {
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)es_int.idb_ptr;
			fastf_t	r3, r4;
			vect_t	adir;
			RT_TOR_CK_MAGIC(tor);

			mat_vec_ortho( adir, tor->h );

			MAT4X3PNT( pos_view, xform, tor->v );
			POINT_LABEL( pos_view, 'V' );

			r3 = tor->r_a - tor->r_h;
			VJOIN1( work, tor->v, r3, adir );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'I' );

			r4 = tor->r_a + tor->r_h;
			VJOIN1( work, tor->v, r4, adir );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'O' );

			VJOIN1( work, tor->v, tor->r_a, adir );
			VADD2( work, work, tor->h );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );
		} else {
		MAT4X3PNT( pos_view, xform, &es_rec.s.s_tor_V );
		POINT_LABEL( pos_view, 'V' );

		VADD2( work, &es_rec.s.s_tor_V, &es_rec.s.s_tor_C );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'I' );

		VADD2( work, &es_rec.s.s_tor_V, &es_rec.s.s_tor_E );
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'O' );

		VADD3( work, &es_rec.s.s_tor_V, &es_rec.s.s_tor_A, &es_rec.s.s_tor_H);
		MAT4X3PNT(pos_view, xform, work);
		POINT_LABEL( pos_view, 'H' );
		}
		break;

	case ID_RPC:
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;
			vect_t	Ru;

			RT_RPC_CK_MAGIC(rpc);
			MAT4X3PNT( pos_view, xform, rpc->rpc_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, rpc->rpc_V, rpc->rpc_B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD2( work, rpc->rpc_V, rpc->rpc_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VCROSS( Ru, rpc->rpc_B, rpc->rpc_H );
			VUNITIZE( Ru );
			VSCALE( Ru, Ru, rpc->rpc_r );
			VADD2( work, rpc->rpc_V, Ru );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'r' );
		}
		break;

	case ID_RHC:
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			vect_t	Ru;

			RT_RHC_CK_MAGIC(rhc);
			MAT4X3PNT( pos_view, xform, rhc->rhc_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, rhc->rhc_V, rhc->rhc_B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD2( work, rhc->rhc_V, rhc->rhc_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VCROSS( Ru, rhc->rhc_B, rhc->rhc_H );
			VUNITIZE( Ru );
			VSCALE( Ru, Ru, rhc->rhc_r );
			VADD2( work, rhc->rhc_V, Ru );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'r' );

			VMOVE( work, rhc->rhc_B );
			VUNITIZE( work );
			VSCALE( work, work,
				MAGNITUDE(rhc->rhc_B) + rhc->rhc_c );
			VADD2( work, work, rhc->rhc_V );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'c' );
		}
		break;

	case ID_EPA:
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;
			vect_t	A, B;

			RT_EPA_CK_MAGIC(epa);
			MAT4X3PNT( pos_view, xform, epa->epa_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, epa->epa_V, epa->epa_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VSCALE( A, epa->epa_Au, epa->epa_r1 );
			VADD2( work, epa->epa_V, A );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VCROSS( B, epa->epa_Au, epa->epa_H );
			VUNITIZE( B );
			VSCALE( B, B, epa->epa_r2 );
			VADD2( work, epa->epa_V, B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );
		}
		break;

	case ID_EHY:
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;
			vect_t	A, B;

			RT_EHY_CK_MAGIC(ehy);
			MAT4X3PNT( pos_view, xform, ehy->ehy_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, ehy->ehy_V, ehy->ehy_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VSCALE( A, ehy->ehy_Au, ehy->ehy_r1 );
			VADD2( work, ehy->ehy_V, A );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VCROSS( B, ehy->ehy_Au, ehy->ehy_H );
			VUNITIZE( B );
			VSCALE( B, B, ehy->ehy_r2 );
			VADD2( work, ehy->ehy_V, B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VMOVE( work, ehy->ehy_H );
			VUNITIZE( work );
			VSCALE( work, work,
				MAGNITUDE(ehy->ehy_H) + ehy->ehy_c );
			VADD2( work, ehy->ehy_V, work );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'c' );
		}
		break;

	case ID_ETO:
		/* new_way only */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	ch, cv, dh, dv, cmag, phi;
			vect_t	Au, Nu;

			RT_ETO_CK_MAGIC(eto);

			MAT4X3PNT( pos_view, xform, eto->eto_V );
			POINT_LABEL( pos_view, 'V' );

			VMOVE(Nu, eto->eto_N);
			VUNITIZE(Nu);
			vec_ortho( Au, Nu );
			VUNITIZE(Au);

			cmag = MAGNITUDE(eto->eto_C);
			/* get horizontal and vertical components of C and Rd */
			cv = VDOT( eto->eto_C, Nu );
			ch = sqrt( cmag*cmag - cv*cv );
			/* angle between C and Nu */
			phi = acos( cv / cmag );
			dv = -eto->eto_rd * sin(phi);
			dh = eto->eto_rd * cos(phi);

			VJOIN2(work, eto->eto_V, eto->eto_r+ch, Au, cv, Nu);
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'C' );

			VJOIN2(work, eto->eto_V, eto->eto_r+dh, Au, dv, Nu);
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'D' );

			VJOIN1(work, eto->eto_V, eto->eto_r, Au);
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'r' );
		}
		break;

	case ID_ARS:
		MAT4X3PNT(pos_view, xform, es_rec.s.s_values);
		POINT_LABEL( pos_view, 'V' );
		break;

	case ID_BSPLINE:
		/* New way only */
		{
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			register fastf_t	*fp;

			RT_NURB_CK_MAGIC(sip);
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			fp = &RT_NURB_GET_CONTROL_POINT( surf, spl_ui, spl_vi );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL( pos_view, 'V' );

			fp = &RT_NURB_GET_CONTROL_POINT( surf, 0, 0 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " 0,0" );
			fp = &RT_NURB_GET_CONTROL_POINT( surf, 0, surf->s_size[1]-1 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " 0,u" );
			fp = &RT_NURB_GET_CONTROL_POINT( surf, surf->s_size[0]-1, 0 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " v,0" );
			fp = &RT_NURB_GET_CONTROL_POINT( surf, surf->s_size[0]-1, surf->s_size[1]-1 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " u,v" );
		}
		break;
	case ID_NMG:
		/* New way only */
		{
			register struct model *m =
				(struct model *) es_int.idb_ptr;
			NMG_CK_MODEL(m);

			if( es_eu )  {
				point_t	cent;
				NMG_CK_EDGEUSE(es_eu);
				VADD2SCALE( cent,
					es_eu->vu_p->v_p->vg_p->coord,
					es_eu->eumate_p->vu_p->v_p->vg_p->coord,
					0.5 );
				MAT4X3PNT(pos_view, xform, cent);
				POINT_LABEL_STR( pos_view, " eu" );
			}
		}
		break;
	}

	pl[npl].str[0] = '\0';	/* Mark ending */
}

/* -------------------------------- */
/*
 *			R T _ A R B _ C A L C _ P L A N E S
 *
 *	Calculate the plane (face) equations for an arb
 *	output previously went to es_peqn[i].
 *
 *  Returns -
 *	-1	Failure
 *	 0	OK
 */
int
rt_arb_calc_planes( planes, arb, type, tol )
plane_t			planes[6];
struct rt_arb_internal	*arb;
int			type;
struct rt_tol		*tol;
{
	register int i, p1, p2, p3;

	type -= 4;	/* ARB4 at location 0, ARB5 at 1, etc */

	for(i=0; i<6; i++) {
		if(arb_faces[type][i*4] == -1)
			break;	/* faces are done */
		p1 = arb_faces[type][i*4];
		p2 = arb_faces[type][i*4+1];
		p3 = arb_faces[type][i*4+2];

		if( rt_mk_plane_3pts( planes[i],
		    arb->pt[p1], arb->pt[p2], arb->pt[p3], tol ) < 0 )  {
			rt_log("rt_arb_calc_planes: No eqn for face %d%d%d%d\n",
				p1+1, p2+1, p3+1,
				arb_faces[type][i*4+3]+1);
			return -1;
		}
	}
	return 0;
}

/* -------------------------------- */
void
sedit_vpick( v_pos )
point_t	v_pos;
{
	point_t	m_pos;
	int	surfno, u, v;

	MAT4X3PNT( m_pos, objview2model, v_pos );

	if( nurb_closest2d( &surfno, &u, &v,
	    (struct rt_nurb_internal *)es_int.idb_ptr,
	    m_pos, model2objview ) >= 0 )  {
		spl_surfno = surfno;
		spl_ui = u;
		spl_vi = v;
		get_solid_keypoint( es_keypoint, &es_keytag, &es_int, es_mat );
	}
	chg_state( ST_S_VPICK, ST_S_EDIT, "Vertex Pick Complete");
	dmaflag = 1;
}

#define DIST2D(P0, P1)	sqrt(	((P1)[X] - (P0)[X])*((P1)[X] - (P0)[X]) + \
				((P1)[Y] - (P0)[Y])*((P1)[Y] - (P0)[Y]) )

#define DIST3D(P0, P1)	sqrt(	((P1)[X] - (P0)[X])*((P1)[X] - (P0)[X]) + \
				((P1)[Y] - (P0)[Y])*((P1)[Y] - (P0)[Y]) + \
				((P1)[Z] - (P0)[Z])*((P1)[Z] - (P0)[Z]) )

/*
 *	C L O S E S T 3 D
 *
 *	Given a vlist pointer (vhead) to point coordinates and a reference
 *	point (ref_pt), pass back in "closest_pt" the coordinates of the
 *	point nearest the reference point in 3 space.
 *
 */
int
rt_vl_closest3d(vhead, ref_pt, closest_pt)
struct rt_list	*vhead;
point_t		ref_pt, closest_pt;
{
	fastf_t		dist, cur_dist;
	pointp_t	c_pt;
	struct rt_vlist	*cur_vp;
	
	if (vhead == RT_LIST_NULL || RT_LIST_IS_EMPTY(vhead))
		return(1);	/* fail */

	/* initialize smallest distance using 1st point in list */
	cur_vp = RT_LIST_FIRST(rt_vlist, vhead);
	dist = DIST3D(ref_pt, cur_vp->pt[0]);
	c_pt = cur_vp->pt[0];

	for (RT_LIST_FOR(cur_vp, rt_vlist, vhead)) {
		register int	i;
		register int	nused = cur_vp->nused;
		register point_t *cur_pt = cur_vp->pt;
		
		for (i = 0; i < nused; i++) {
			cur_dist = DIST3D(ref_pt, cur_pt[i]);
			if (cur_dist < dist) {
				dist = cur_dist;
				c_pt = cur_pt[i];
			}
		}
	}
	VMOVE(closest_pt, c_pt);
	return(0);	/* success */
}

/*
 *	C L O S E S T 2 D
 *
 *	Given a pointer (vhead) to vlist point coordinates, a reference
 *	point (ref_pt), and a transformation matrix (mat), pass back in
 *	"closest_pt" the original, untransformed 3 space coordinates of
 *	the point nearest the reference point after all points have been
 *	transformed into 2 space projection plane coordinates.
 */
int
rt_vl_closest2d(vhead, ref_pt, mat, closest_pt)
struct rt_list	*vhead;
point_t		ref_pt, closest_pt;
mat_t		mat;
{
	fastf_t		dist, cur_dist;
	point_t		cur_pt2d, ref_pt2d;
	pointp_t	c_pt;
	struct rt_vlist	*cur_vp;
	
	if (vhead == RT_LIST_NULL || RT_LIST_IS_EMPTY(vhead))
		return(1);	/* fail */

	/* transform reference point to 2d */
	MAT4X3PNT(ref_pt2d, mat, ref_pt);

	/* initialize smallest distance using 1st point in list */
	cur_vp = RT_LIST_FIRST(rt_vlist, vhead);
	MAT4X3PNT(cur_pt2d, mat, cur_vp->pt[0]);
	dist = DIST2D(ref_pt2d, cur_pt2d);
	c_pt = cur_vp->pt[0];

	for (RT_LIST_FOR(cur_vp, rt_vlist, vhead)) {
		register int	i;
		register int	nused = cur_vp->nused;
		register point_t *cur_pt = cur_vp->pt;
		
		for (i = 0; i < nused; i++) {
			MAT4X3PNT(cur_pt2d, mat, cur_pt[i]);
			cur_dist = DIST2D(ref_pt2d, cur_pt2d);
			if (cur_dist < dist) {
				dist = cur_dist;
				c_pt = cur_pt[i];
			}
		}
	}
	VMOVE(closest_pt, c_pt);
	return(0);	/* success */
}

/*
 *				N U R B _ C L O S E S T 3 D
 *
 *	Given a vlist pointer (vhead) to point coordinates and a reference
 *	point (ref_pt), pass back in "closest_pt" the coordinates of the
 *	point nearest the reference point in 3 space.
 *
 */
int
nurb_closest3d(surface, uval, vval, spl, ref_pt )
int				*surface;
int				*uval;
int				*vval;
CONST struct rt_nurb_internal	*spl;
CONST point_t			ref_pt;
{
	struct snurb	*srf;
	fastf_t		*mesh;
	fastf_t		d;
	fastf_t		c_dist;		/* closest dist so far */
	int		c_surfno;
	int		c_u, c_v;
	int		u, v;
	int		i;

	RT_NURB_CK_MAGIC(spl);

	c_dist = INFINITY;
	c_surfno = c_u = c_v = -1;

	for( i = 0; i < spl->nsrf; i++ )  {
		int	advance;

		srf = spl->srfs[i];
		NMG_CK_SNURB(srf);
		mesh = srf->ctl_points;
		advance = RT_NURB_EXTRACT_COORDS(srf->pt_type);

		for( v = 0; v < srf->s_size[0]; v++ )  {
			for( u = 0; u < srf->s_size[1]; u++ )  {
				/* XXX 4-tuples? */
				d = DIST3D(ref_pt, mesh);
				if (d < c_dist)  {
					c_dist = d;
					c_surfno = i;
					c_u = u;
					c_v = v;
				}
				mesh += advance;
			}
		}
	}
	if( c_surfno < 0 )  return  -1;		/* FAIL */
	*surface = c_surfno;
	*uval = c_u;
	*vval = c_v;

	return(0);				/* success */
}

/*
 *				N U R B _ C L O S E S T 2 D
 *
 *	Given a pointer (vhead) to vlist point coordinates, a reference
 *	point (ref_pt), and a transformation matrix (mat), pass back in
 *	"closest_pt" the original, untransformed 3 space coordinates of
 *	the point nearest the reference point after all points have been
 *	transformed into 2 space projection plane coordinates.
 */
int
nurb_closest2d(surface, uval, vval, spl, ref_pt, mat )
int				*surface;
int				*uval;
int				*vval;
CONST struct rt_nurb_internal	*spl;
CONST point_t			ref_pt;
CONST mat_t			mat;
{
	struct snurb	*srf;
	point_t		ref_2d;
	fastf_t		*mesh;
	fastf_t		d;
	fastf_t		c_dist;		/* closest dist so far */
	int		c_surfno;
	int		c_u, c_v;
	int		u, v;
	int		i;

	RT_NURB_CK_MAGIC(spl);

	c_dist = INFINITY;
	c_surfno = c_u = c_v = -1;

	/* transform reference point to 2d */
	MAT4X3PNT(ref_2d, mat, ref_pt);

	for( i = 0; i < spl->nsrf; i++ )  {
		int	advance;

		srf = spl->srfs[i];
		NMG_CK_SNURB(srf);
		mesh = srf->ctl_points;
		advance = RT_NURB_EXTRACT_COORDS(srf->pt_type);

		for( v = 0; v < srf->s_size[0]; v++ )  {
			for( u = 0; u < srf->s_size[1]; u++ )  {
				point_t	cur;
				/* XXX 4-tuples? */
				MAT4X3PNT( cur, mat, mesh );
				d = DIST2D(ref_2d, cur);
				if (d < c_dist)  {
					c_dist = d;
					c_surfno = i;
					c_u = u;
					c_v = v;
				}
				mesh += advance;
			}
		}
	}
	if( c_surfno < 0 )  return  -1;		/* FAIL */
	*surface = c_surfno;
	*uval = c_u;
	*vval = c_v;

	return(0);				/* success */
}


int
f_keypoint (argc, argv)
int	argc;
char	**argv;
{
	if ((state != ST_S_EDIT) && (state != ST_O_EDIT))
	{
	    state_err("keypoint assignment");
	    return CMD_BAD;
	}

	switch (--argc)
	{
	    case 0:
		printf("%s (%g, %g, %g)\n", es_keytag, V3ARGS(es_keypoint));
		break;
	    case 3:
		VSET(es_keypoint,
		    atof( argv[1] ) * local2base,
		    atof( argv[2] ) * local2base,
		    atof( argv[3] ) * local2base);
		es_keytag = "user-specified";
		es_keyfixed = 1;
		break;
	    case 1:
		if (strcmp(argv[1], "reset") == 0)
		{
		    es_keytag = "";
		    es_keyfixed = 0;
		    get_solid_keypoint(es_keypoint, &es_keytag,
					&es_int, es_mat);
		    break;
		}
	    default:
		(void) printf("Usage: 'keypoint [<x y z> | reset]'\n");
		return CMD_BAD;
	}

	dmaflag = 1;
	return CMD_OK;
}
