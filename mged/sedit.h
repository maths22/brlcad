/*
 *  			S E D I T . H
 *  
 *  This header file contains the esolid structure definition,
 *  which holds all the information necessary for solid editing.
 *  
 *  Author -
 *	Kieth A Applin
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
 *
 *  $Header$
 */

/* data for solid editing */
union record es_rec;		/* current solid record */
union record es_orig;		/* original solid record */
#define es_name	es_rec.s.s_name
#define es_type	es_rec.s.s_cgtype	/* COMGEOM solid type */
#define es_gentype es_rec.s.s_type	/* GED general solid type */

int     es_edflag;		/* type of editing for this solid */
#define IDLE	0
#define STRANS	1
#define SSCALE	2
#define SROT	3
#define MENU	4
#define PSCALE	5
#define PROT	6
#define EARB	7
#define	MOVEH	8
#define MOVEHH	9
#define PTARB	10
float	es_scale;		/* scale factor */
float 	es_para[3];		/* keyboard input parameter changes */
float	es_peqn[7][4];		/* ARBs defining plane equations */
float	es_m[3];		/* edge(line) slope */
int	es_menu;		/* item selected from menu */
#define MENUH		1
#define MENUR1		2
#define MENUR2		3
#define MENUA		4
#define MENUB		5
#define MENUC		6
#define MENURH		7
#define MENURAB 	8
#define	MENUMH		9
#define MENUMHH		10
#define MENUP1		11
#define MENUP2		12
#define MENUAB		13
#define MENUCD		14
#define MENUABCD	15
#define MENUABC		16
mat_t	es_mat;			/* accumulated matrix of path */ 
mat_t 	es_invmat;		/* inverse of es_mat   KAA */
int	es_nlines;		/* # lines in printed display */
#define ES_LINELEN	128		/* len of display text lines */
char	es_display[ES_LINELEN*10];/* buffer for lines of display */

extern int arb_faces[5][24];	/* from edarb.c */
extern int arb_planes[5][24];	/* from edarb.c */
