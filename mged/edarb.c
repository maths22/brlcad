/*
 *			E D A R B . C
 *
 * Functions -
 *	editarb		edit ARB edge (and move points)
 *	planeqn		finds plane equation given 3 points
 *	intersect	finds intersection point of three planes
 *	mv_edge		moves an arb edge
 *	f_extrude	"extrude" command -- project an ARB face
 *	f_arbdef	define ARB8 using rot fb angles to define face
 *	f_mirface	mirror an ARB face
 *	f_permute	permute ARB8 vertex labels
 *
 *  Author -
 *	Keith A. Applin
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

#include <stdio.h>
#include <math.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "./sedit.h"
#include "raytrace.h"
#include "./ged.h"
#include "externs.h"
#include "./solid.h"
#include "./dm.h"

void	ext4to6();

/* face definitions for each arb type */
int arb_faces[5][24] = {
	{0,1,2,3, 0,1,4,5, 1,2,4,5, 0,2,4,5, -1,-1,-1,-1, -1,-1,-1,-1},	/* ARB4 */
	{0,1,2,3, 4,0,1,5, 4,1,2,5, 4,2,3,5, 4,3,0,5, -1,-1,-1,-1},	/* ARB5 */
	{0,1,2,3, 1,2,4,6, 0,4,6,3, 4,1,0,5, 6,2,3,7, -1,-1,-1,-1},	/* ARB6 */
	{0,1,2,3, 4,5,6,7, 0,3,4,7, 1,2,6,5, 0,1,5,4, 3,2,6,4},		/* ARB7 */
	{0,1,2,3, 4,5,6,7, 0,4,7,3, 1,2,6,5, 0,1,5,4, 3,2,6,7},		/* ARB8 */
};

/*
 *  			E D I T A R B
 *  
 *  An ARB edge is moved by finding the direction of
 *  the line containing the edge and the 2 "bounding"
 *  planes.  The new edge is found by intersecting the
 *  new line location with the bounding planes.  The
 *  two "new" planes thus defined are calculated and the
 *  affected points are calculated by intersecting planes.
 *  This keeps ALL faces planar.
 *
 */

/*  The storage for the "specific" ARB types is :
 *
 *	ARB4	0 1 2 0 3 3 3 3
 *	ARB5	0 1 2 3 4 4 4 4
 *	ARB6	0 1 2 3 4 4 5 5
 *	ARB7	0 1 2 3 4 5 6 4
 *	ARB8	0 1 2 3 4 5 6 7
 */

/* Another summary of how the vertices of ARBs are stored:
 *
 * Vertices:	1	2	3	4	5	6	7	8
 * Location----------------------------------------------------------------
 *	ARB8	0	1	2	3	4	5	6	7
 *	ARB7	0	1	2	3	4,7	5	6
 *	ARB6	0	1	2	3	4,5	6,7
 * 	ARB5	0	1	2	3	4,5,6,7
 *	ARB4	0,3	1	2	4,5,6,7
 */

/* The following arb editing arrays generally contain the following:
 *
 *	location 	comments
 *------------------------------------------------------------------------
 *	0,1		edge end points
 * 	2,3		bounding planes 1 and 2
 *	4, 5,6,7	plane 1 to recalculate, using next 3 points
 *	8, 9,10,11	plane 2 to recalculate, using next 3 points
 *	12, 13,14,15	plane 3 to recalculate, using next 3 points
 *	16,17		points (vertices) to recalculate
 *
 *
 * Each line is repeated for each edge (or point) to move
*/

/* edit array for arb8's */
static short earb8[12][18] = {
	{0,1, 2,3, 0,0,1,2, 4,0,1,4, -1,0,0,0, 3,5},	/* edge 12 */
	{1,2, 4,5, 0,0,1,2, 3,1,2,5, -1,0,0,0, 3,6},	/* edge 23 */
	{2,3, 3,2, 0,0,2,3, 5,2,3,6, -1,0,0,0, 1,7},	/* edge 34 */
	{0,3, 4,5, 0,0,1,3, 2,0,3,4, -1,0,0,0, 2,7},	/* edge 41 */
	{0,4, 0,1, 2,0,4,3, 4,0,1,4, -1,0,0,0, 7,5},	/* edge 15 */
	{1,5, 0,1, 4,0,1,5, 3,1,2,5, -1,0,0,0, 4,6},	/* edge 26 */
	{4,5, 2,3, 4,0,5,4, 1,4,5,6, -1,0,0,0, 1,7},	/* edge 56 */
	{5,6, 4,5, 3,1,5,6, 1,4,5,6, -1,0,0,0, 2,7},	/* edge 67 */
	{6,7, 3,2, 5,2,7,6, 1,4,6,7, -1,0,0,0, 3,4},	/* edge 78 */
	{4,7, 4,5, 2,0,7,4, 1,4,5,7, -1,0,0,0, 3,6},	/* edge 58 */
	{2,6, 0,1, 3,1,2,6, 5,2,3,6, -1,0,0,0, 5,7},	/* edge 37 */
	{3,7, 0,1, 2,0,3,7, 5,2,3,7, -1,0,0,0, 4,6},	/* edge 48 */
};

/* edit array for arb7's */
static short earb7[12][18] = {
	{0,1, 2,3, 0,0,1,2, 4,0,1,4, -1,0,0,0, 3,5},	/* edge 12 */
	{1,2, 4,5, 0,0,1,2, 3,1,2,5, -1,0,0,0, 3,6},	/* edge 23 */
	{2,3, 3,2, 0,0,2,3, 5,2,3,6, -1,0,0,0, 1,4},	/* edge 34 */
	{0,3, 4,5, 0,0,1,3, 2,0,3,4, -1,0,0,0, 2,-1},	/* edge 41 */
	{0,4, 0,5, 4,0,5,4, 2,0,3,4, 1,4,5,6, 1,-1},	/* edge 15 */
	{1,5, 0,1, 4,0,1,5, 3,1,2,5, -1,0,0,0, 4,6},	/* edge 26 */
	{4,5, 5,3, 2,0,3,4, 4,0,5,4, 1,4,5,6, 1,-1},	/* edge 56 */
	{5,6, 4,5, 3,1,6,5, 1,4,5,6, -1,0,0,0, 2, -1},	/* edge 67 */
	{2,6, 0,1, 5,2,3,6, 3,1,2,6, -1,0,0,0, 4,5},	/* edge 37 */
	{4,6, 4,3, 2,0,3,4, 5,3,4,6, 1,4,5,6, 2,-1},	/* edge 57 */
	{3,4, 0,1, 4,0,1,4, 2,0,3,4, 5,2,3,4, 5,6},	/* edge 45 */
	{-1,-1, -1,-1, 5,2,3,4, 4,0,1,4, 8,2,1,-1, 6,5},	/* point 5 */
};

/* edit array for arb6's */
static short earb6[10][18] = {
	{0,1, 2,1, 3,0,1,4, 0,0,1,2, -1,0,0,0, 3,-1},	/* edge 12 */
	{1,2, 3,4, 1,1,2,5, 0,0,1,2, -1,0,0,0, 3,4},	/* edge 23 */
	{2,3, 1,2, 4,2,3,5, 0,0,2,3, -1,0,0,0, 1,-1},	/* edge 34 */
	{0,3, 3,4, 2,0,3,5, 0,0,1,3, -1,0,0,0, 4,2},	/* edge 14 */
	{0,4, 0,1, 3,0,1,4, 2,0,3,4, -1,0,0,0, 6,-1},	/* edge 15 */
	{1,4, 0,2, 3,0,1,4, 1,1,2,4, -1,0,0,0, 6,-1},	/* edge 25 */
	{2,6, 0,2, 4,6,2,3, 1,1,2,6, -1,0,0,0, 4,-1},	/* edge 36 */
	{3,6, 0,1, 4,6,2,3, 2,0,3,6, -1,0,0,0, 4,-1},	/* edge 46 */
	{-1,-1, -1,-1, 2,0,3,4, 1,1,2,4, 3,0,1,4, 6,-1},/* point 5 */
	{-1,-1, -1,-1, 2,0,3,6, 1,1,2,6, 4,2,3,6, 4,-1},/* point 6 */
};

/* edit array for arb5's */
static short earb5[9][18] = {
	{0,1, 4,2, 0,0,1,2, 1,0,1,4, -1,0,0,0, 3,-1},	/* edge 12 */
	{1,2, 1,3, 0,0,1,2, 2,1,2,4, -1,0,0,0, 3,-1},	/* edge 23 */
	{2,3, 2,4, 0,0,2,3, 3,2,3,4, -1,0,0,0, 1,-1},	/* edge 34 */
	{0,3, 1,3, 0,0,1,3, 4,0,3,4, -1,0,0,0, 2,-1},	/* edge 14 */
	{0,4, 0,2, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* edge 15 */
	{1,4, 0,3, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* edge 25 */
	{2,4, 0,4, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1}, 	/* edge 35 */
	{3,4, 0,1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* edge 45 */
	{-1,-1, -1,-1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* point 5 */
};

/* edit array for arb4's */
static short earb4[5][18] = {
	{-1,-1, -1,-1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* point 1 */
	{-1,-1, -1,-1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* point 2 */
	{-1,-1, -1,-1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* point 3 */
	{-1,-1, -1,-1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* dummy */
	{-1,-1, -1,-1, 9,0,0,0, 9,0,0,0, 9,0,0,0, -1,-1},	/* point 4 */
};


int
editarb( pos_model )
vect_t pos_model;
{
	static int pt1, pt2, bp1, bp2, newp, p1, p2, p3;
	short *edptr;		/* pointer to arb edit array */
	short *final;		/* location of points to redo */
	register dbfloat_t *op;
	static int i, *iptr;

	/* set the pointer */
	switch( es_type ) {

		case ARB4:
			edptr = &earb4[es_menu][0];
			final = &earb4[es_menu][16];
		break;

		case ARB5:
			edptr = &earb5[es_menu][0];
			final = &earb5[es_menu][16];
			if(es_edflag == PTARB) {
				edptr = &earb5[8][0];
				final = &earb5[8][16];
			}
		break;

		case ARB6:
			edptr = &earb6[es_menu][0];
			final = &earb6[es_menu][16];
			if(es_edflag == PTARB) {
				i = 9;
				if(es_menu == 4)
					i = 8;
				edptr = &earb6[i][0];
				final = &earb6[i][16];
			}
		break;

		case ARB7:
			edptr = &earb7[es_menu][0];
			final = &earb7[es_menu][16];
			if(es_edflag == PTARB) {
				edptr = &earb7[11][0];
				final = &earb7[11][16];
			}
		break;

		case ARB8:
			edptr = &earb8[es_menu][0];
			final = &earb8[es_menu][16];
		break;

		default:
			(void)printf("edarb: unknown ARB type\n");
		return(1);
	}

	/* convert to point notation (in place ----- DANGEROUS) */
	for(i=3; i<=21; i+=3) {
		op = &es_rec.s.s_values[i];
		VADD2( op, op, &es_rec.s.s_values[0] );
	}

	/* do the arb editing */

	if( es_edflag == PTARB ) {
		/* moving a point - not an edge */
		VMOVE(&es_rec.s.s_values[es_menu*3], &pos_model[0]);
		edptr += 4;
	} else if( es_edflag == EARB ) {
		vect_t	edge_dir;

		/* moving an edge */
		pt1 = *edptr++;
		pt2 = *edptr++;
		/* direction of this edge */
		if( newedge ) {
			/* edge direction comes from edgedir() in pos_model */
			VMOVE( edge_dir, pos_model );
			VMOVE(pos_model, &es_rec.s.s_values[pt1*3]);
			newedge = 0;
		} else {
			/* must calculate edge direction */
			VSUB2(edge_dir, &es_rec.s.s_values[3*pt2], &es_rec.s.s_values[3*pt1]);
		}
		if(MAGNITUDE(edge_dir) == 0.0) 
			goto err;
		/* bounding planes bp1,bp2 */
		bp1 = *edptr++;
		bp2 = *edptr++;

		/* move the edge */
/*
printf("moving edge: %d%d  bound planes: %d %d\n",pt1+1,pt2+1,bp1+1,bp2+1);
*/
		if( mv_edge(pos_model, bp1, bp2, pt1, pt2, edge_dir) )
			goto err;
	}

	/* editing is done - insure planar faces */
	/* redo plane eqns that changed */
	newp = *edptr++; 	/* plane to redo */
	if( newp == 9 ) {
		/* special flag --> redo all the planes */
		iptr = &arb_faces[es_type-4][0];
		for(i=0; i<6; i++) {
			p1 = *iptr++;
			p2 = *iptr++;
			p3 = *iptr++;
			iptr++;
/*
printf("REDO plane %d with points %d %d %d\n",i+1,p1+1,p2+1,p3+1);
*/
			if( planeqn(i, p1, p2, p3, &es_rec.s) )
				goto err;
			if( *iptr == -1 )
				break;		/* finished */
		}
	}
	if(newp >= 0 && newp < 6) {
		for(i=0; i<3; i++) {
			/* redo this plane (newp), use points p1,p2,p3 */
			p1 = *edptr++;
			p2 = *edptr++;
			p3 = *edptr++;
/*
printf("redo plane %d with points %d %d %d\n",newp+1,p1+1,p2+1,p3+1);
*/
			if( planeqn(newp, p1, p2, p3, &es_rec.s) )
				goto err;
			/* next plane */
			if( (newp = *edptr++) == -1 || newp == 8 )
				break;
		}
	}
	if(newp == 8) {
		/* special...redo next planes using pts defined in faces */
		for(i=0; i<3; i++) {
			if( (newp = *edptr++) == -1 )
				break;
			iptr = &arb_faces[es_type-4][4*newp];
			p1 = *iptr++;
			p2 = *iptr++;
			p3 = *iptr++;
/*
printf("REdo plane %d with points %d %d %d\n",newp+1,p1+1,p2+1,p3+1);
*/
			if( planeqn(newp, p1, p2, p3, &es_rec.s) )
				goto err;
		}
	}

	/* the changed planes are all redone
	 *	push necessary points back into the planes
	 */
	edptr = final;	/* point to the correct location */
	for(i=0; i<2; i++) {
		if( (p1 = *edptr++) == -1 )
			break;
		/* intersect proper planes to define vertex p1 */
/*
printf("intersect: type=%d   point = %d\n",es_type,p1+1);
*/
		if( intersect( es_type, p1*3, p1, &es_rec.s ) )
			goto err;
	}

	/* Special case for ARB7: move point 5 .... must
	 *	recalculate plane 2 = 456
	 */
	if(es_type == ARB7 && es_edflag == PTARB) {
/*
printf("redo plane 2 == 5,6,7 for ARB7\n");
*/
		if( planeqn(2, 4, 5, 6, &es_rec.s) )
			goto err;
	}

	/* carry along any like points */
	switch( es_type ) {
		case ARB8:
		break;

		case ARB7:
			VMOVE(&es_rec.s.s_values[21], &es_rec.s.s_values[12]);
		break;

		case ARB6:
			VMOVE(&es_rec.s.s_values[15], &es_rec.s.s_values[12]);
			VMOVE(&es_rec.s.s_values[21], &es_rec.s.s_values[18]);
		break;

		case ARB5:
			for(i=15; i<=21; i+=3) {
				VMOVE(&es_rec.s.s_values[i], &es_rec.s.s_values[12]);
			}
		break;

		case ARB4:
			VMOVE(&es_rec.s.s_values[9], &es_rec.s.s_values[0]);
			for(i=15; i<=21; i+=3) {
				VMOVE(&es_rec.s.s_values[i], &es_rec.s.s_values[12]);
			}
		break;
	}

	/* back to vector notation */
	for(i=3; i<=21; i+=3) {
		op = &es_rec.s.s_values[i];
		VSUB2( op, op, &es_rec.s.s_values[0] );
	}
	return(0);		/* OK */

err:
	/* Error handling */
	(void)printf("cannot move edge: %d%d\n", pt1+1,pt2+1);
	es_edflag = IDLE;

	/* back to vector notation */
	for(i=3; i<=21; i+=3) {
		op = &es_rec.s.s_values[i];
		VSUB2(op, op, &es_rec.s.s_values[0]);
	}
	return(1);		/* BAD */
}

/*   PLANEQN:
 *	finds equation of a plane defined by 3 points use1,use2,use3
 *	of solid record sp.  Equation is stored at "loc" of es_peqn
 *	array.
 *
 *  Returns -
 *	 0	success
 *	-1	failure
 */
int
planeqn(loc, use1, use2, use3, sp)
int loc, use1, use2, use3;
struct solidrec *sp;
{
	vect_t	a,b,c;
	struct rt_tol	tol;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	/* XXX This converts data types as well! */
	VMOVE( a, &sp->s_values[use1*3] );
	VMOVE( b, &sp->s_values[use2*3] );
	VMOVE( c, &sp->s_values[use3*3] );

	return( rt_mk_plane_3pts( es_peqn[loc], a, b, c, &tol ) );
}

/* planes to define ARB vertices */
CONST int rt_arb_planes[5][24] = {
	{0,1,3, 0,1,2, 0,2,3, 0,1,3, 1,2,3, 1,2,3, 1,2,3, 1,2,3},	/* ARB4 */
	{0,1,4, 0,1,2, 0,2,3, 0,3,4, 1,2,4, 1,2,4, 1,2,4, 1,2,4},	/* ARB5 */
	{0,2,3, 0,1,3, 0,1,4, 0,2,4, 1,2,3, 1,2,3, 1,2,4, 1,2,4},	/* ARB6 */
	{0,2,4, 0,3,4, 0,3,5, 0,2,5, 1,4,5, 1,3,4, 1,3,5, 1,2,4},	/* ARB7 */
	{0,2,4, 0,3,4, 0,3,5, 0,2,5, 1,2,4, 1,3,4, 1,3,5, 1,2,5},	/* ARB8 */
};

/*	INTERSECT:
 *	Finds intersection point of three planes.
 *		The planes are at es_planes[type][loc] and
 *		the result is stored at "pos" in solid struct sp.
 *
 *  XXX replaced by rt_arb_3face_intersect().
 *
 *  Returns -
 *	 0	success
 *	 1	failure
 */
int
intersect( type, loc, pos, sp )
int type, loc, pos;
struct solidrec *sp;
{
	vect_t	vec1;
	int	j;
	int	i1, i2, i3;

	j = type - 4;

	i1 = rt_arb_planes[j][loc];
	i2 = rt_arb_planes[j][loc+1];
	i3 = rt_arb_planes[j][loc+2];

	if( rt_mkpoint_3planes( vec1, es_peqn[i1], es_peqn[i2],
	    es_peqn[i3] ) < 0 )
		return(1);
	VMOVE( &sp->s_values[pos*3], vec1 );	/* XXX type conversion too */
	return( 0 );
}

/*
 *			R T _ A R B _ 3 F A C E _ I N T E R S E C T
 *
 *	Finds the intersection point of three faces of an ARB.
 *
 *  Returns -
 *	  0	success
 *	 -1	failure
 */
int
rt_arb_3face_intersect( point, planes, type, loc )
point_t			point;
plane_t			planes[6];
int			type;		/* 4..8 */
int			loc;
{
	int	j;
	int	i1, i2, i3;

	j = type - 4;

	i1 = rt_arb_planes[j][loc];
	i2 = rt_arb_planes[j][loc+1];
	i3 = rt_arb_planes[j][loc+2];

	return rt_mkpoint_3planes( point, planes[i1], planes[i2], planes[i3] );
}

/*  MV_EDGE:
 *	Moves an arb edge (end1,end2) with bounding
 *	planes bp1 and bp2 through point "thru".
 *	The edge has (non-unit) slope "dir".
 *	Note that the fact that the normals here point in rather than
 *	out makes no difference for computing the correct intercepts.
 *	After the intercepts are found, they should be checked against
 *	the other faces to make sure that they are always "inside".
 */
int
mv_edge(thru, bp1, bp2, end1, end2, dir)
vect_t thru;
int bp1, bp2, end1, end2;
vect_t	dir;
{
	dbfloat_t *op;
	fastf_t	t1, t2;
	struct rt_tol	tol;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	if( rt_isect_line3_plane( &t1, thru, dir, es_peqn[bp1], &tol ) < 0 ||
	    rt_isect_line3_plane( &t2, thru, dir, es_peqn[bp2], &tol ) < 0 )  {
		(void)printf("edge (direction) parallel to face normal\n");
		return( 1 );
	}

	op = &es_rec.s.s_values[end1*3];
	VJOIN1( op, thru, t1, dir );

	op = &es_rec.s.s_values[end2*3];
	VJOIN1( op, thru, t2, dir );

	return( 0 );
}

/* Extrude command - project an arb face */
/* Format: extrude face distance	*/
void
f_extrude( argc, argv )
int	argc;
char	**argv;
{
	register int i, j;
	static int face;
	static int pt[4];
	static int prod;
	static fastf_t dist;
	static struct solidrec lsolid;	/* local copy of solid */

	if( not_state( ST_S_EDIT, "Extrude" ) )
		return;

	if( es_rec.s.s_type != GENARB8 )  {
		(void)printf("Extrude: solid type must be ARB\n");
		return;
	}

	if(es_type != ARB8 && es_type != ARB6 && es_type != ARB4) {
		(void)printf("ARB%d: extrusion of faces not allowed\n",es_type);
		return;
	}

	face = atoi( argv[1] );

	/* get distance to project face */
	dist = atof( argv[2] );
	/* apply es_mat[15] to get to real model space */
	/* convert from the local unit (as input) to the base unit */
	dist = dist * es_mat[15] * local2base;

	/* convert to point notation in temporary buffer */
	VMOVE( &lsolid.s_values[0], &es_rec.s.s_values[0] );
	for( i = 3; i <= 21; i += 3 )  {  
		VADD2(&lsolid.s_values[i], &es_rec.s.s_values[i], &lsolid.s_values[0]);
	}

	if( (es_type == ARB6 || es_type == ARB4) && face < 1000 ) {
		/* 3 point face */
		pt[0] = face / 100;
		i = face - (pt[0]*100);
		pt[1] = i / 10;
		pt[2] = i - (pt[1]*10);
		pt[3] = 1;
	}
	else {
		pt[0] = face / 1000;
		i = face - (pt[0]*1000);
		pt[1] = i / 100;
		i = i - (pt[1]*100);
		pt[2] = i / 10;
		pt[3] = i - (pt[2]*10);
	}

	/* user can input face in any order - will use product of
	 * face points to distinguish faces:
	 *    product       face
	 *       24         1234 for ARB8
	 *     1680         5678 for ARB8
	 *      252         2367 for ARB8
	 *      160         1548 for ARB8
	 *      672         4378 for ARB8
	 *       60         1256 for ARB8
	 *	 10	    125 for ARB6
	 *	 72	    346 for ARB6
	 * --- special case to make ARB6 from ARB4
	 * ---   provides easy way to build ARB6's
	 *        6	    123 for ARB4
	 *	  8	    124 for ARB4
 	 *	 12	    134 for ARB4
	 *	 24	    234 for ARB4
	 */
	prod = 1;
	for( i = 0; i <= 3; i++ )  {
		prod *= pt[i];
		if(es_type == ARB6 && pt[i] == 6)
			pt[i]++;
		if(es_type == ARB4 && pt[i] == 4)
			pt[i]++;
		pt[i]--;
		if( pt[i] > 7 )  {
			(void)printf("bad face: %d\n",face);
			return;
		}
	}

	/* find plane containing this face */
	if( planeqn(6, pt[0], pt[1], pt[2], &lsolid) ) {
		(void)printf("face: %d is not a plane\n",face);
		return;
	}
	/* get normal vector of length == dist */
	for( i = 0; i < 3; i++ )
		es_peqn[6][i] *= dist;

	/* protrude the selected face */
	switch( prod )  {

	case 24:   /* protrude face 1234 */
		if(es_type == ARB6) {
			(void)printf("ARB6: extrusion of face %d not allowed\n",face);
			return;
		}
		if(es_type == ARB4)
			goto a4toa6;	/* extrude face 234 of ARB4 to make ARB6 */

		for( i = 0; i < 4; i++ )  {
			j = i + 4;
			VADD2( &lsolid.s_values[j*3],
				&lsolid.s_values[i*3],
				&es_peqn[6][0]);
		}
		break;

	case 6:		/* extrude ARB4 face 123 to make ARB6 */
	case 8:		/* extrude ARB4 face 124 to make ARB6 */
	case 12:	/* extrude ARB4 face 134 to Make ARB6 */
a4toa6:
		ext4to6(pt[0], pt[1], pt[2], &lsolid);
		es_rec.s.s_cgtype = ARB6;
		sedit_menu();
	break;

	case 1680:   /* protrude face 5678 */
		for( i = 0; i < 4; i++ )  {
			j = i + 4;
			VADD2( &lsolid.s_values[i*3],
				&lsolid.s_values[j*3],
				&es_peqn[6][0] );
		}
		break;

	case 60:   /* protrude face 1256 */
	case 10:   /* extrude face 125 of ARB6 */
		VADD2( &lsolid.s_values[9],
			&lsolid.s_values[0],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[6],
			&lsolid.s_values[3],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[21],
			&lsolid.s_values[12],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[18],
			&lsolid.s_values[15],
			&es_peqn[6][0] );
		break;

	case 672:   /* protrude face 4378 */
	case 72:	/* extrude face 346 of ARB6 */
		VADD2( &lsolid.s_values[0],
			&lsolid.s_values[9],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[3],
			&lsolid.s_values[6],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[15],
			&lsolid.s_values[18],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[12],
			&lsolid.s_values[21],
			&es_peqn[6][0] );
		break;

	case 252:   /* protrude face 2367 */
		VADD2( &lsolid.s_values[0],
			&lsolid.s_values[3],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[9],
			&lsolid.s_values[6],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[12],
			&lsolid.s_values[15],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[21],
			&lsolid.s_values[18],
			&es_peqn[6][0] );
		break;

	case 160:   /* protrude face 1548 */
		VADD2( &lsolid.s_values[3],
			&lsolid.s_values[0],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[15],
			&lsolid.s_values[12],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[6],
			&lsolid.s_values[9],
			&es_peqn[6][0] );
		VADD2( &lsolid.s_values[18],
			&lsolid.s_values[21],
			&es_peqn[6][0] );
		break;

	case 120:
	case 180:
		(void)printf("ARB6: extrusion of face %d not allowed\n",face);
		return;

	default:
		(void)printf("bad face: %d\n", face );
		return;
	}

	/* redo the plane equations */
	for(i=0; i<6; i++) {
		if(arb_faces[es_type-4][i*4] == -1)
			break;
		pt[0] = arb_faces[es_type-4][i*4];
		pt[1] = arb_faces[es_type-4][i*4+1];
		pt[2] = arb_faces[es_type-4][i*4+2];
		if(planeqn(i, pt[0], pt[1], pt[2], &lsolid)) {
			(void)printf("No equation for face %d%d%d%d\n",
				pt[0]+1,pt[1]+1,pt[2]+1,arb_faces[es_type-4][i*4+3]);
			return;
		}
	}

	/* Convert back to point&vector notation */
	VMOVE( &es_rec.s.s_values[0], &lsolid.s_values[0] );
	for( i = 3; i <= 21; i += 3 )  {  
		VSUB2( &es_rec.s.s_values[i], &lsolid.s_values[i], &lsolid.s_values[0]);
	}

	/* draw the updated solid */
	replot_editing_solid();
	dmaflag = 1;
}

/* define an arb8 using rot fb angles to define a face */
/* Format: a name rot fb	*/
void
f_arbdef( argc, argv )
int	argc;
char	**argv;
{
	register struct directory *dp;
	union record record;
	int i, j;
	fastf_t rota, fb;
	vect_t	norm;

	if( db_lookup( dbip,  argv[1] , LOOKUP_QUIET ) != DIR_NULL )  {
		aexists( argv[1] );
		return;
	}

	/* get rotation angle */
	rota = atof( argv[2] ) * degtorad;

	/* get fallback angle */
	fb = atof( argv[3] ) * degtorad;

	if( (dp = db_diradd( dbip,  argv[1], -1, 1, DIR_SOLID )) == DIR_NULL ||
	    db_alloc( dbip, dp, 1 ) < 0 )  {
	    	ALLOC_ERR_return;
	}
	NAMEMOVE( argv[1], record.s.s_name );
	record.s.s_id = ID_SOLID;
	record.s.s_type = GENARB8;
	record.s.s_cgtype = ARB8;

	/* put vertex of new solid at center of screen */
	record.s.s_values[0] = -toViewcenter[MDX];
	record.s.s_values[1] = -toViewcenter[MDY];
	record.s.s_values[2] = -toViewcenter[MDZ];

	/* calculate normal vector (length = 2) defined by rot,fb */
	norm[0] = cos(fb) * cos(rota) * -50.8;
	norm[1] = cos(fb) * sin(rota) * -50.8;
	norm[2] = sin(fb) * -50.8;

	for( i = 3; i < 24; i++ )
		record.s.s_values[i] = 0.0;

	/* find two perpendicular vectors which are perpendicular to norm */
	j = 0;
	for( i = 0; i < 3; i++ )  {
		if( fabs(norm[i]) < fabs(norm[j]) )
			j = i;
	}
	record.s.s_values[j+3] = 1.0;
	VCROSS( &record.s.s_values[9], &record.s.s_values[3], norm );
	VCROSS( &record.s.s_values[3], &record.s.s_values[9], norm );

	/* create new rpp 20x20x2 */
	/* the 20x20 faces are in rot,fb plane */
	VUNITIZE( &record.s.s_values[3] );
	VUNITIZE( &record.s.s_values[9] );
	VSCALE(&record.s.s_values[3], &record.s.s_values[3], 508.0);
	VSCALE(&record.s.s_values[9], &record.s.s_values[9], 508.0);
	VADD2( &record.s.s_values[6],
		&record.s.s_values[3],
		&record.s.s_values[9] );
	VMOVE( &record.s.s_values[12], norm );
	for( i = 3; i < 12; i += 3 )  {
		j = i + 12;
		VADD2( &record.s.s_values[j], &record.s.s_values[i], norm );
	}

	/* update dbip->dbi_fd and draw new arb8 */
	if( db_put( dbip, dp, &record, 0, 1 ) < 0 )  WRITE_ERR_return;
	if( no_memory )  {
		(void)printf(
			"ARB8 (%s) created but no memory left to draw it\n",
			argv[1] );
		return;
	}

	/* draw the "made" solid */
	f_edit( 2, argv );	/* depends on name being in argv[1] */
}

/* Mirface command - mirror an arb face */
/* Format: mirror face axis	*/
void
f_mirface( argc, argv )
int	argc;
char	**argv;
{
	register int i, j, k;
	static int face;
	static int pt[4];
	static int prod;
	static vect_t work;
	static struct solidrec lsolid;	/* local copy of solid */

	if( not_state( ST_S_EDIT, "Mirface" ) )
		return;

	if( es_rec.s.s_type != GENARB8 )  {
		(void)printf("Mirface: solid type must be ARB\n");
		return;
	}

	if(es_type != ARB8 && es_type != ARB6) {
		(void)printf("ARB%d: mirroring of faces not allowed\n",es_type);
		return;
	}
	face = atoi( argv[1] );
	if( face > 9999 || (face < 1000 && es_type != ARB6) ) {
		(void)printf("ERROR: %d bad face\n",face);
		return;
	}
	/* check which axis */
	k = -1;
	if( strcmp( argv[2], "x" ) == 0 )
		k = 0;
	if( strcmp( argv[2], "y" ) == 0 )
		k = 1;
	if( strcmp( argv[2], "z" ) == 0 )
		k = 2;
	if( k < 0 ) {
		(void)printf("axis must be x, y or z\n");
		return;
	}

	work[0] = work[1] = work[2] = 1.0;
	work[k] = -1.0;

	/* convert to point notation in temporary buffer */
	VMOVE( &lsolid.s_values[0], &es_rec.s.s_values[0] );
	for( i = 3; i <= 21; i += 3 )  {  
		VADD2(&lsolid.s_values[i], &es_rec.s.s_values[i], &lsolid.s_values[0]);
	}

	if(es_type == ARB6 && face < 1000) { 	/* 3 point face */
		pt[0] = face / 100;
		i = face - (pt[0]*100);
		pt[1] = i / 10;
		pt[2] = i - (pt[1]*10);
		pt[3] = 1;
	}
	else {
		pt[0] = face / 1000;
		i = face - (pt[0]*1000);
		pt[1] = i / 100;
		i = i - (pt[1]*100);
		pt[2] = i / 10;
		pt[3] = i - (pt[2]*10);
	}

	/* user can input face in any order - will use product of
	 * face points to distinguish faces:
	 *    product       face
	 *       24         1234 for ARB8
	 *     1680         5678 for ARB8
	 *      252         2367 for ARB8
	 *      160         1548 for ARB8
	 *      672         4378 for ARB8
	 *       60         1256 for ARB8
	 *	 10	    125 for ARB6
	 *	 72	    346 for ARB6
	 */
	prod = 1;
	for( i = 0; i <= 3; i++ )  {
		prod *= pt[i];
		pt[i]--;
		if( pt[i] > 7 )  {
			(void)printf("bad face: %d\n",face);
			return;
		}
	}

	/* mirror the selected face */
	switch( prod )  {

	case 24:   /* mirror face 1234 */
		if(es_type == ARB6) {
			(void)printf("ARB6: mirroring of face %d not allowed\n",face);
			return;
		}
		for( i = 0; i < 4; i++ )  {
			j = i + 4;
			VELMUL( &lsolid.s_values[j*3],
				&lsolid.s_values[i*3],
				work);
		}
		break;

	case 1680:   /* mirror face 5678 */
		for( i = 0; i < 4; i++ )  {
			j = i + 4;
			VELMUL( &lsolid.s_values[i*3],
				&lsolid.s_values[j*3],
				work );
		}
		break;

	case 60:   /* mirror face 1256 */
	case 10:	/* mirror face 125 of ARB6 */
		VELMUL( &lsolid.s_values[9],
			&lsolid.s_values[0],
			work );
		VELMUL( &lsolid.s_values[6],
			&lsolid.s_values[3],
			work );
		VELMUL( &lsolid.s_values[21],
			&lsolid.s_values[12],
			work );
		VELMUL( &lsolid.s_values[18],
			&lsolid.s_values[15],
			work );
		break;

	case 672:   /* mirror face 4378 */
	case 72:	/* mirror face 346 of ARB6 */
		VELMUL( &lsolid.s_values[0],
			&lsolid.s_values[9],
			work );
		VELMUL( &lsolid.s_values[3],
			&lsolid.s_values[6],
			work );
		VELMUL( &lsolid.s_values[15],
			&lsolid.s_values[18],
			work );
		VELMUL( &lsolid.s_values[12],
			&lsolid.s_values[21],
			work );
		break;

	case 252:   /* mirror face 2367 */
		VELMUL( &lsolid.s_values[0],
			&lsolid.s_values[3],
			work );
		VELMUL( &lsolid.s_values[9],
			&lsolid.s_values[6],
			work );
		VELMUL( &lsolid.s_values[12],
			&lsolid.s_values[15],
			work );
		VELMUL( &lsolid.s_values[21],
			&lsolid.s_values[18],
			work );
		break;

	case 160:   /* mirror face 1548 */
		VELMUL( &lsolid.s_values[3],
			&lsolid.s_values[0],
			work );
		VELMUL( &lsolid.s_values[15],
			&lsolid.s_values[12],
			work );
		VELMUL( &lsolid.s_values[6],
			&lsolid.s_values[9],
			work );
		VELMUL( &lsolid.s_values[18],
			&lsolid.s_values[21],
			work );
		break;

	case 120:
	case 180:
		(void)printf("ARB6: mirroring of face %d not allowed\n",face);
		return;

	default:
		(void)printf("bad face: %d\n", face );
		return;
	}

	/* redo the plane equations */
	for(i=0; i<6; i++) {
		if(arb_faces[es_type-4][i*4] == -1)
			break;
		pt[0] = arb_faces[es_type-4][i*4];
		pt[1] = arb_faces[es_type-4][i*4+1];
		pt[2] = arb_faces[es_type-4][i*4+2];
		if(planeqn(i, pt[0], pt[1], pt[2], &lsolid)) {
			(void)printf("No equation for face %d%d%d%d\n",
				pt[0]+1,pt[1]+1,pt[2]+1,arb_faces[es_type-4][i*4+3]);
			return;
		}
	}

	/* Convert back to point&vector notation */
	VMOVE( &es_rec.s.s_values[0], &lsolid.s_values[0] );
	for( i = 3; i <= 21; i += 3 )  {  
		VSUB2( &es_rec.s.s_values[i], &lsolid.s_values[i], &lsolid.s_values[0]);
	}

	/* draw the updated solid */
	replot_editing_solid();
	dmaflag = 1;
}

/* Edgedir command:  define the direction of an arb edge being moved
 *	Format:  edgedir deltax deltay deltaz
	     OR  edgedir rot fb
 */

void
f_edgedir( argc, argv )
int	argc;
char	**argv;
{
	register int i;
	vect_t slope;
	FAST fastf_t rot, fb;

	if( not_state( ST_S_EDIT, "Edgedir" ) )
		return;

	if( es_edflag != EARB ) {
		(void)printf("Not moving an ARB edge\n");
		return;
	}

	if( es_rec.s.s_type != GENARB8 ) {
		(void)printf("Edgedir: solid type must be an ARB\n");
		return;
	}

	/* set up slope -
	 *	if 2 values input assume rot, fb used
	 *	else assume delta_x, delta_y, delta_z
	 */
	if( argc == 3 ) {
		rot = atof( argv[1] ) * degtorad;
		fb = atof( argv[2] ) * degtorad;
		slope[0] = cos(fb) * cos(rot);
		slope[1] = cos(fb) * sin(rot);
		slope[2] = sin(fb);
	}
	else {
		for(i=0; i<3; i++) {
			/* put edge slope in slope[] array */
			slope[i] = atof( argv[i+1] );
		}
	}

	if(MAGNITUDE(slope) == 0) {
		(void)printf("BAD slope\n");
		return;
	}

	/* get it done */
	newedge = 1;
	editarb( slope );
	sedraw++;

}


/*	EXT4TO6():	extrudes face pt1 pt2 pt3 of an ARB4 "distance"
 *			to produce ARB6 using solid record "sp"
 */
void
ext4to6(pt1, pt2, pt3, sp)
int pt1, pt2, pt3;
register struct solidrec *sp;
{
	struct solidrec tmp;
	register int i;

	VMOVE(&tmp.s_values[0], &sp->s_values[pt1*3]);
	VMOVE(&tmp.s_values[3], &sp->s_values[pt2*3]);
	VMOVE(&tmp.s_values[12], &sp->s_values[pt3*3]);
	VMOVE(&tmp.s_values[15], &sp->s_values[pt3*3]);

	/* extrude "distance" to get remaining points */
	VADD2(&tmp.s_values[6], &tmp.s_values[3], &es_peqn[6][0]);
	VADD2(&tmp.s_values[9], &tmp.s_values[0], &es_peqn[6][0]);
	VADD2(&tmp.s_values[18], &tmp.s_values[12], &es_peqn[6][0]);
	VMOVE(&tmp.s_values[21], &tmp.s_values[18]);

	/* copy to the original record */
	for(i=0; i<=21; i+=3) {
		VMOVE(&sp->s_values[i], &tmp.s_values[i]);
	}
}

/* Permute command - permute the vertex labels of an ARB8
/* Format: permute jkl	*/
void
f_permute( argc, argv )

int	argc;
char	**argv;

{
    /*
     *	1) Why were all vars declared static?
     *	2) Recompute plane equations?
     */
    register int vertex, i, j, k;
    int triple;
    int	modulus;
    int	*p;
    struct solidrec lsolid;	/* local copy of solid */
    struct solidrec tsolid;	/* temporary copy of solid */
    static int perm_array[8][7] =
    {
	{12345678, 12654378, 14325876, 14852376, 15624873, 15842673, 0},
	{21436587, 21563487, 23416785, 23761485, 26513784, 26731584, 0},
	{32147658, 32674158, 34127856, 34872156, 37624851, 37842651, 0},
	{41238567, 41583267, 43218765, 43781265, 48513762, 48731562, 0},
	{51268437, 51486237, 56218734, 56781234, 58416732, 58761432, 0},
	{62157348, 62375148, 65127843, 65872143, 67325841, 67852341, 0},
	{73268415, 73486215, 76238514, 76583214, 78436512, 78563412, 0},
	{84157326, 84375126, 85147623, 85674123, 87345621, 87654321, 0}
    };

    if (not_state(ST_S_EDIT, "Permute"))
	return;
    if ((es_rec.s.s_type != GENARB8) || (es_type != ARB8))
    {
	(void) printf("Permute: solid type must be ARB8\n");
	return;
    }

    /*
     *	Find the encoded form of the specified permutation,
     *	if it exists
     */
    triple = atoi(argv[1]);
    if ((triple < 123) || (triple > 876))
    {
	(void) printf("ERROR: bad vertex triple: %d\n", triple);
	return;
    }
    vertex = triple / 100;
    for (p = perm_array[vertex - 1]; *p > 0; ++p)
    {
	if (*p / 100000 == triple)
	    break;
    }
    if (*p == 0)
    {
	(void) printf("ERROR: bad vertex triple: %d\n", triple);
	return;
    }

    /* Convert to point notation in temporary buffer */
    VMOVE(&lsolid.s_values[0], &es_rec.s.s_values[0]);
    for(i = 3; i <= 21; i += 3)
    {  
	VADD2(&lsolid.s_values[i],
		&es_rec.s.s_values[i], &lsolid.s_values[0]);
    }

#if 0
    for (i = 0; i <= 21; i += 3)
    {
	char	string[1024];

	sprintf(string, "vertex %d", i / 3 + 1);
	VPRINT(string, &lsolid.s_values[i]);
    }
#endif

    /*
     *	Collect the vertices in the specified order
     */
    modulus = 100000000;
    for (j = 0; j <= 21; j += 3)
    {
	k = ((*p % modulus) * 10) / modulus - 1;
	VMOVE(&tsolid.s_values[j], &lsolid.s_values[3 * k]);
	modulus /= 10;
    }
    /*
     *	Reinstall the permuted vertices back into the temporary buffer
     */
    for (j = 0; j <= 21; j += 3)
    {
	VMOVE(&lsolid.s_values[j], &tsolid.s_values[j]);
    }

    /* Convert back to point&vector notation */
    VMOVE(&es_rec.s.s_values[0], &lsolid.s_values[0]);
    for (i = 3; i <= 21; i += 3)
    {  
	VSUB2(&es_rec.s.s_values[i],
		&lsolid.s_values[i], &lsolid.s_values[0]);
    }

    /* draw the updated solid */
    replot_editing_solid();
    dmaflag = 1;
}
