/*
 *			V L I S T . C
 *
 *  Routines for the import and export of vlist chains as:
 *	Network independent binary,
 *	BRL-extended UNIX-Plot files.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1992 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "rtstring.h"
#include "raytrace.h"
#include "externs.h"

/*
 *			R T _ V L I S T _ E X P O R T
 *
 *  Convert a vlist chain into a blob of network-independent binary,
 *  for transmission to another machine.
 *  The result is stored in a vls string, so that both the address
 *  and length are available conveniently.
 */
void
rt_vlist_export( vls, hp, name )
struct rt_vls	*vls;
struct rt_list	*hp;
CONST char	*name;
{
	register struct rt_vlist	*vp;
	int		nelem;
	int		namelen;
	int		nbytes;
	unsigned char	*buf;
	unsigned char	*bp;

	RT_VLS_CHECK(vls);

	/* Count number of element in the vlist */
	nelem = 0;
	for( RT_LIST_FOR( vp, rt_vlist, hp ) )  {
		nelem += vp->nused;
	}

	/* Build output buffer for binary transmission
	 * nelem[4], String[n+1], cmds[nelem*1], pts[3*nelem*8]
	 */
	namelen = strlen(name)+1;
	nbytes = namelen + 4 + nelem * (1+3*8) + 2;

	rt_vls_setlen( vls, nbytes );
	buf = (unsigned char *)rt_vls_addr(vls);
	bp = rt_plong( buf, nelem );
	strncpy( bp, name, namelen );
	bp += namelen;

	/* Output cmds, as bytes */
	for( RT_LIST_FOR( vp, rt_vlist, hp ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		for( i = 0; i < nused; i++ )  {
			*bp++ = *cmd++;
		}
	}

	/* Output points, as three 8-byte doubles */
	for( RT_LIST_FOR( vp, rt_vlist, hp ) )  {
		register int	i;
		register int	nused = vp->nused;
		register point_t *pt = vp->pt;
		for( i = 0; i < nused; i++,pt++ )  {
			htond( bp, (char *)pt, 3 );
			bp += 3*8;
		}
	}
}

/*
 *			R T _ V L I S T _ I M P O R T
 *
 *  Convert a blob of network-independent binary prepared by vls_vlist()
 *  and received from another machine, into a vlist chain.
 */
void
rt_vlist_import( hp, namevls, buf )
struct rt_list	*hp;
struct rt_vls	*namevls;
CONST unsigned char	*buf;
{
	register struct rt_vlist	*vp;
	CONST register unsigned char	*bp;
	CONST unsigned char		*pp;		/* point pointer */
	int		nelem;
	int		namelen;
	int		i;
	point_t		point;

	RT_VLS_CHECK(namevls);

	nelem = rt_glong( buf );
	bp = buf+4;

	namelen = strlen(bp)+1;
	rt_vls_strncpy( namevls, (char *)bp, namelen );
	bp += namelen;

	pp = bp + nelem*1;

	for( i=0; i < nelem; i++ )  {
		int	cmd;

		cmd = *bp++;
		ntohd( (char *)point, pp, 3 );
		pp += 3*8;
		/* This macro might be expanded inline, for performance */
		RT_ADD_VLIST( hp, point, cmd );
	}
}

/*
 *			R T _ V L I S T _ C L E A N U P
 *
 *  The macro RT_FREE_VLIST() simply appends to the list &rt_g.rtg_vlfree.
 *  Now, give those structures back to rt_free().
 */
void
rt_vlist_cleanup()
{
	register struct rt_vlist	*vp;

	while( RT_LIST_WHILE( vp, rt_vlist, &rt_g.rtg_vlfree ) )  {
		RT_CK_VLIST( vp );
		RT_LIST_DEQUEUE( &(vp->l) );
		rt_free( (char *)vp, "rt_vlist" );
	}
}

/*
 *			R T _ V L I S T _ T O _ U P L O T
 *
 *  Output a vlist as an extended 3-D floating point UNIX-Plot file.
 *  You provide the file.
 *  Uses libplot3 routines to create the UNIX-Plot output.
 */
void
rt_vlist_to_uplot( fp, vhead )
FILE		*fp;
struct rt_list	*vhead;
{
	register struct rt_vlist	*vp;

	for( RT_LIST_FOR( vp, rt_vlist, vhead ) )  {
		register int		i;
		register int		nused = vp->nused;
		register CONST int	*cmd = vp->cmd;
		register point_t	 *pt = vp->pt;

		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			switch( *cmd )  {
			case RT_VLIST_POLY_START:
				break;
			case RT_VLIST_POLY_MOVE:
			case RT_VLIST_LINE_MOVE:
				pdv_3move( fp, *pt );
				break;
			case RT_VLIST_POLY_DRAW:
			case RT_VLIST_POLY_END:
			case RT_VLIST_LINE_DRAW:
				pdv_3cont( fp, *pt );
				break;
			default:
				rt_log("rt_vlist_to_uplot: unknown vlist cmd x%x\n",
					*cmd );
			}
		}
	}
}


#define	TBAD	0	/* no such command */
#define TNONE	1	/* no arguments */
#define TSHORT	2	/* Vax 16-bit short */
#define	TIEEE	3	/* IEEE 64-bit floating */
#define	TCHAR	4	/* unsigned chars */
#define	TSTRING	5	/* linefeed terminated string */

struct uplot {
	int	targ;		/* type of args */
	int	narg;		/* number or args */
	char	desc[14];	/* description */
};
static CONST struct uplot rt_uplot_error = { 0, 0, "error" };
static CONST struct uplot rt_uplot_letters[] = {
/*A*/	{ 0, 0, 0 },
/*B*/	{ 0, 0, 0 },
/*C*/	{ TCHAR, 3, "color" },
/*D*/	{ 0, 0, 0 },
/*E*/	{ 0, 0, 0 },
/*F*/	{ TNONE, 0, "flush" },
/*G*/	{ 0, 0, 0 },
/*H*/	{ 0, 0, 0 },
/*I*/	{ 0, 0, 0 },
/*J*/	{ 0, 0, 0 },
/*K*/	{ 0, 0, 0 },
/*L*/	{ TSHORT, 6, "3line" },
/*M*/	{ TSHORT, 3, "3move" },
/*N*/	{ TSHORT, 3, "3cont" },
/*O*/	{ TIEEE, 3, "d_3move" },
/*P*/	{ TSHORT, 3, "3point" },
/*Q*/	{ TIEEE, 3, "d_3cont" },
/*R*/	{ 0, 0, 0 },
/*S*/	{ TSHORT, 6, "3space" },
/*T*/	{ 0, 0, 0 },
/*U*/	{ 0, 0, 0 },
/*V*/	{ TIEEE, 6, "d_3line" },
/*W*/	{ TIEEE, 6, "d_3space" },
/*X*/	{ TIEEE, 3, "d_3point" },
/*Y*/	{ 0, 0, 0 },
/*Z*/	{ 0, 0, 0 },
/*[*/	{ 0, 0, 0 },
/*\*/	{ 0, 0, 0 },
/*]*/	{ 0, 0, 0 },
/*^*/	{ 0, 0, 0 },
/*_*/	{ 0, 0, 0 },
/*`*/	{ 0, 0, 0 },
/*a*/	{ TSHORT, 6, "arc" },
/*b*/	{ 0, 0, 0 },
/*c*/	{ TSHORT, 3, "circle" },
/*d*/	{ 0, 0, 0 },
/*e*/	{ TNONE, 0, "erase" },
/*f*/	{ TSTRING, 1, "linmod" },
/*g*/	{ 0, 0, 0 },
/*h*/	{ 0, 0, 0 },
/*i*/	{ TIEEE, 3, "d_circle" },
/*j*/	{ 0, 0, 0 },
/*k*/	{ 0, 0, 0 },
/*l*/	{ TSHORT, 4, "line" },
/*m*/	{ TSHORT, 2, "move" },
/*n*/	{ TSHORT, 2, "cont" },
/*o*/	{ TIEEE, 2, "d_move" },
/*p*/	{ TSHORT, 2, "point" },
/*q*/	{ TIEEE, 2, "d_cont" },
/*r*/	{ TIEEE, 6, "d_arc" },
/*s*/	{ TSHORT, 4, "space" },
/*t*/	{ TSTRING, 1, "label" },
/*u*/	{ 0, 0, 0 },
/*v*/	{ TIEEE, 4, "d_line" },
/*w*/	{ TIEEE, 4, "d_space" },
/*x*/	{ TIEEE, 2, "d_point" },
/*y*/	{ 0, 0, 0 },
/*z*/	{ 0, 0, 0 }
};

/*
 *	getshort		Read VAX-order 16-bit number
 */
static int
getshort(fp)
FILE	*fp;
{
	register long	v, w;

	v = getc(fp);
	v |= (getc(fp)<<8);	/* order is important! */

	/* worry about sign extension - sigh */
	if( v <= 0x7FFF )  return(v);
	w = -1;
	w &= ~0x7FFF;
	return( w | v );
}

/*
 *			R T _ U P L O T _ T O _ V L I S T
 *
 *  Read a BRL-style 3-D UNIX-plot file into a vector list.
 *  For now, discard color information, only extract vectors.
 *  This might be more naturally located in mged/plot.c
 */
int
rt_uplot_to_vlist( vbp, fp, char_size )
struct rt_vlblock	*vbp;
register FILE		*fp;
double			char_size;
{
	register struct rt_list	*vhead;
	register int	c;
	mat_t	mat;
	CONST struct uplot	*up;
	char	carg[256];
	fastf_t	arg[6];
	char	inbuf[8];
	vect_t	a,b;
	point_t	last_pos;
	int	cc;
	int	i;
	int	j;

	vhead = rt_vlblock_find( vbp, 0xFF, 0xFF, 0x00 );	/* Yellow */

	while( (c = getc(fp)) != EOF ) {
		/* look it up */
		if( c < 'A' || c > 'z' ) {
			up = &rt_uplot_error;
		} else {
			up = &rt_uplot_letters[ c - 'A' ];
		}

		if( up->targ == TBAD ) {
			fprintf( stderr, "Bad command '%c' (0x%02x)\n", c, c );
			return(-1);
		}

		if( up->narg > 0 )  {
			for( i = 0; i < up->narg; i++ ) {
			switch( up->targ ){
				case TSHORT:
					arg[i] = getshort(fp);
					break;
				case TIEEE:
					fread( inbuf, 8, 1, fp );
					ntohd( &arg[i], inbuf, 1 );
					break;
				case TSTRING:
					j = 0;
					while( (cc = getc(fp)) != '\n'
					    && cc != EOF )
						carg[j++] = cc;
					carg[j] = '\0';
					break;
				case TCHAR:
					carg[i] = getc(fp);
					arg[i] = 0;
					break;
				case TNONE:
				default:
					arg[i] = 0;	/* ? */
					break;
				}
			}
		}

		switch( c ) {
		case 's':
		case 'w':
		case 'S':
		case 'W':
			/* Space commands, do nothing. */
			break;
		case 'm':
		case 'o':
			/* 2-D move */
			arg[Z] = 0;
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_MOVE );
			break;
		case 'M':
		case 'O':
			/* 3-D move */
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_MOVE );
			break;
		case 'n':
		case 'q':
			/* 2-D draw */
			arg[Z] = 0;
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_DRAW );
			break;
		case 'N':
		case 'Q':
			/* 3-D draw */
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_DRAW );
			break;
		case 'l':
		case 'v':
			/* 2-D line */
			VSET( a, arg[0], arg[1], 0.0 );
			VSET( b, arg[2], arg[3], 0.0 );
			RT_ADD_VLIST( vhead, a, RT_VLIST_LINE_MOVE );
			RT_ADD_VLIST( vhead, b, RT_VLIST_LINE_DRAW );
			break;
		case 'L':
		case 'V':
			/* 3-D line */
			VSET( a, arg[0], arg[1], arg[2] );
			VSET( b, arg[3], arg[4], arg[5] );
			RT_ADD_VLIST( vhead, a, RT_VLIST_LINE_MOVE );
			RT_ADD_VLIST( vhead, b, RT_VLIST_LINE_DRAW );
			break;
		case 'p':
		case 'x':
			/* 2-D point */
			arg[Z] = 0;
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_MOVE );
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_DRAW );
			break;
		case 'P':
		case 'X':
			/* 3-D point */
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_MOVE );
			RT_ADD_VLIST( vhead, arg, RT_VLIST_LINE_DRAW );
			break;
		case 'C':
			/* Color */
			vhead = rt_vlblock_find( vbp,
				carg[0], carg[1], carg[2] );
			break;
		case 't':
			/* Text string */
			mat_idn(mat);
			if( RT_LIST_NON_EMPTY( vhead ) )  {
				struct rt_vlist *vlp;
				/* Use coordinates of last op */
				vlp = RT_LIST_LAST( rt_vlist, vhead );
				VMOVE( last_pos, vlp->pt[vlp->nused-1] );
			} else {
				VSETALL( last_pos, 0 );
			}
			rt_vlist_3string( vhead, carg, last_pos, mat, char_size );
			break;
		}
	}
	return(0);
}

/*
 *			R T _ L A B E L _ V L I S T _ V E R T S
 *
 *  Used by MGED's "labelvert" command.
 */
void
rt_label_vlist_verts( vbp, src, mat, sz )
struct rt_vlblock	*vbp;
struct rt_list		*src;
mat_t			mat;
double			sz;
{
	struct rt_vlist	*vp;
	struct rt_list	*vhead;
	char		label[256];
	fastf_t		scale;

	vhead = rt_vlblock_find( vbp, 255, 255, 255 );	/* white */

	for( RT_LIST_FOR( vp, rt_vlist, src ) )  {
		register int	i;
		register int	nused = vp->nused;
		register int	*cmd = vp->cmd;
		register point_t *pt = vp->pt;
		for( i = 0; i < nused; i++,cmd++,pt++ )  {
			/* XXX Skip polygon markers? */
			sprintf( label, " %g, %g, %g",
				V3ARGS(*pt) );
			rt_vlist_3string( vhead, label, pt, mat, sz );
		}
	}
}
