/*
 *			K U R T . C
 * 
 *  Program to generate polygons from a multi-valued function.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright 
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "db.h"
#include "vmath.h"
#include "raytrace.h"

#include "./complex.h"
#include "./polyno.h"

mat_t	identity;
double degtorad = 0.0174532925199433;

struct val {
	double	v_z[3];
	double	v_x;
	double	v_y;
	int	v_n;
} val[20][20];

main(argc, argv)
char	**argv;
{
	vect_t	norm;
	char	rgb[3];
	int	ix, iy;
	double	x, y;
	double	size;
	double	base;
	int	quant;
	char	name[64];
	vect_t	pos, aim;
	char	white[3];

	mk_id( stdout, "Kurt's multi-valued function");

	/* Create the detail cells */
	size = 10;	/* mm */
	quant = 18;
	base = -size*(quant/2);
	for( ix=quant-1; ix>=0; ix-- )  {
		x = base + ix*size;
		for( iy=quant-1; iy>=0; iy-- )  {
			y = base + iy*size;
			do_cell( &val[ix][iy], x, y );
		}
	}
	/* Draw cells */
	mk_polysolid( stdout, "kurt" );
	for( ix=quant-2; ix>=0; ix-- )  {
		for( iy=quant-2; iy>=0; iy-- )  {
			draw_rect( &val[ix][iy],
				   &val[ix+1][iy], 
				   &val[ix][iy+1], 
				   &val[ix+1][iy+1] );
		}
	}

#ifdef never
	/* Create some light */
	white[0] = white[1] = white[2] = 255;
	base = size*(quant/2+1);
	VSET( aim, 0, 0, 0 );
	VSET( pos, base, base, size );
	do_light( "l1", pos, aim, 1, 100.0, white );
	VSET( pos, -base, base, size );
	do_light( "l2", pos, aim, 1, 100.0, white );
	VSET( pos, -base, -base, size );
	do_light( "l3", pos, aim, 1, 100.0, white );
	VSET( pos, base, -base, size );
	do_light( "l4", pos, aim, 1, 100.0, white );

	/* Build the overall combination */
	mk_comb( stdout, "clut", quant*quant+1+4, 0 );
	mk_memb( stdout, UNION, "plane.r", identity );
	for( ix=quant-1; ix>=0; ix-- )  {
		for( iy=quant-1; iy>=0; iy-- )  {
			sprintf( name, "x%dy%d", ix, iy );
			mk_memb( stdout, UNION, name, identity );
		}
	}
	mk_memb( stdout, UNION, "l1", identity );
	mk_memb( stdout, UNION, "l2", identity );
	mk_memb( stdout, UNION, "l3", identity );
	mk_memb( stdout, UNION, "l4", identity );
#endif
}

do_cell( vp, xc, yc )
struct val	*vp;
double	xc, yc;		/* center coordinates, z=0+ */
{
	LOCAL poly	polynom;
	LOCAL complex	roots[4];	/* roots of final equation */
	fastf_t		k[4];
	int		i, l;
	int		nroots;
	int		lim;

	polynom.dgr = 3;
	polynom.cf[0] = 1;
	polynom.cf[1] = 0;
	polynom.cf[2] = yc;
	polynom.cf[3] = xc;
	vp->v_n = 0;
	vp->v_x = xc;
	vp->v_y = yc;
	nroots = polyRoots( &polynom, roots );
	if( nroots < 0 || (nroots & 1) == 0 )  {
		fprintf(stderr,"%d roots?\n", nroots);
		return;
	}
	for ( l=0; l < nroots; l++ ){
		if ( NEAR_ZERO( roots[l].im, 0.0001 ) )
			vp->v_z[vp->v_n++] = roots[l].re;
	}
	/* Sort in increasing Z */
	for( lim = nroots-1; lim > 0; lim-- )  {
		for( l=0; l < lim; l++ )  {
			register double t;
			if( (t=vp->v_z[l]) > vp->v_z[l+1] )  {
				vp->v_z[l] = vp->v_z[l+1];
				vp->v_z[l+1] = t;
			}
		}
	}
}

draw_rect( a, b, c, d )
struct val *a, *b, *c, *d;
{
	register int lvl;
	register int i;
	point_t pt[4];
	fastf_t	verts[5][3];
	fastf_t	norms[5][3];
	static struct val v[3];

	/* Find max # of points at any of the 4 vertices */
	lvl = a->v_n;
	if( b->v_n > lvl )  lvl = b->v_n;
	if( c->v_n > lvl )  lvl = c->v_n;
	if( d->v_n > lvl )  lvl = d->v_n;

	if( a->vn == lvl && b->vn == lvl && 
	    c->vn == lvl && d->vn == lvl )  {
		for( i=0; i<lvl; i++ )  {
			VSET( verts[0], a->v_x, a->v_y, a->v_z[i] );
			VSET( verts[1], b->v_x, b->v_y, b->v_z[i] );
			VSET( verts[2], c->v_x, c->v_y, c->v_z[i] );
			/* even # faces point up, odd#s point down */
			pnorms( norms, verts, i&1, 3 );
			mk_facet( stdout, 3, verts, norms );
		}
	    	return;
	}
	/* Harder case:  handle different depths on corners */
}

/*
 *  Find the single outward pointing normal for a facet.
 *  Assumes all points are coplanar (they better be!).
 */
pnorms( norms, verts, down, npts )
fastf_t	norms[5][3];
fastf_t	verts[5][3];
int	down;		/* 0 = norms up, 1 = norms down */
int	npts;
{
	register int i;
	vect_t	ab, ac;
	vect_t	n;
	vect_t	out;		/* hopefully points outwards */

	VSUB2( ab, verts[1], verts[0] );
	VSUB2( ac, verts[2], verts[0] );
	VCROSS( n, ab, ac );
	VUNITIZE( n );

	/* If normal points opposite to flag, flip it */
	if( down )  {
		if( n[Z] > 0 )  {
			VREVERSE( n, n );
		}
	} else {
		if( n[Z] < 0 )  {
			VREVERSE( n, n );
		}
	}

	/* Use same normal for all vertices (flat shading) */
	for( i=0; i<npts; i++ )  {
		VMOVE( norms[i], n );
	}
}

do_light( name, pos, dir_at, da_flag, r, rgb )
char	*name;
point_t	pos;
vect_t	dir_at;		/* direction or aim point */
int	da_flag;	/* 0 = direction, !0 = aim point */
double	r;		/* radius of light */
char	*rgb;
{
	char	nbuf[64];
	mat_t	rot;
	mat_t	xlate;
	mat_t	both;
	vect_t	begin;
	vect_t	trial;
	vect_t	from;
	vect_t	dir;

	if( da_flag )  {
		VSUB2( dir, dir_at, pos );
		VUNITIZE( dir );
	} else {
		VMOVE( dir, dir_at );
	}

	sprintf( nbuf, "%s.s", name );
	mk_sph( stdout, nbuf, 0.0, 0.0, 0.0, r );

	/*
	 * Need to rotate from 0,0,-1 to vect "dir",
	 * then xlate to final position.
	 */
	VSET( from, 0, 0, -1 );
	mat_fromto( rot, from, dir );
	mat_idn( xlate );
	MAT_DELTAS( xlate, pos[X], pos[Y], pos[Z] );
	mat_mul( both, xlate, rot );

	mk_mcomb( stdout, name, 1, 1, "light", "shadows=1", 1, rgb );
	mk_memb( stdout, UNION, nbuf, both );
}

/* wrapper for atan2.  On SGI (and perhaps others), x==0 returns infinity */
double
xatan2(y,x)
double	y,x;
{
	if( x > -1.0e-20 && x < 1.0e-20 )  return(0.0);
	return( atan2( y, x ) );
}
/*
 *			M A T _ F R O M T O
 *
 *  Given two vectors, compute a rotation matrix that will transform
 *  space by the angle between the two.  Since there are many
 *  candidate matricies, the method used here is to convert the vectors
 *  to azimuth/elevation form (azimuth is +X, elevation is +Z),
 *  take the difference, and form the rotation matrix.
 *  See mat_ae for that algorithm.
 *
 *  The input 'from' and 'to' vectors must be unit length.
 */
mat_fromto( m, from, to )
mat_t	m;
vect_t	from;
vect_t	to;
{
	double	az, el;
	LOCAL double sin_az, sin_el;
	LOCAL double cos_az, cos_el;

	az = xatan2( to[Y], to[X] ) - xatan2( from[Y], from[X] );
	el = asin( to[Z] ) - asin( from[Z] );

	sin_az = sin(az);
	cos_az = cos(az);
	sin_el = sin(el);
	cos_el = cos(el);

	m[0] = cos_el * cos_az;
	m[1] = -sin_az;
	m[2] = -sin_el * cos_az;
	m[3] = 0;

	m[4] = cos_el * sin_az;
	m[5] = cos_az;
	m[6] = -sin_el * sin_az;
	m[7] = 0;

	m[8] = sin_el;
	m[9] = 0;
	m[10] = cos_el;
	m[11] = 0;

	m[12] = m[13] = m[14] = 0;
	m[15] = 1.0;
}
