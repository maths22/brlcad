/*
 *			A R S . C
 *
 *  Function -
 *	Intersect a ray with an ARS (Arbitrary faceted solid)
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
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSars[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/db.h"
#include "../h/raytrace.h"
#include "debug.h"
#include "rtdir.h"
#include "plane.h"

/* Describe algorithm here */

#define ARSMINMAX(a,b,c)	{ FAST fastf_t ftemp;\
			if( (ftemp = (c)) < (a) )  a = ftemp;\
			if( ftemp > (b) )  b = ftemp; }

extern fastf_t *rd_curve();

/*
 *			A R S _ P R E P
 *  
 *  This routine is used to prepare a list of planar faces for
 *  being shot at by the ars routines.
 *
 * Process an ARS, which is represented as a vector
 * from the origin to the first point, and many vectors
 * from the first point to the remaining points.
 *  
 *  This routine is unusual in that it has to read additional
 *  database records to obtain all the necessary information.
 */
ars_prep( vecxx, stp, mat, sp )
fastf_t *vecxx;
struct soltab *stp;
matp_t mat;
struct solidrec *sp;
{
	LOCAL struct ars_rec *ap;	/* ARS A record pointer */
	LOCAL fastf_t dx, dy, dz;	/* For finding the bounding spheres */
	register int	i, j;
	register fastf_t **curves;	/* array of curve base addresses */
	LOCAL int pts_per_curve;
	LOCAL int ncurves;
	LOCAL vect_t base_vect;
	LOCAL struct tri_specific *plp;
	LOCAL int pts;			/* return from arsface() */
	LOCAL fastf_t f;

	ap = (struct ars_rec *) sp;	/* PUN for record.anything */
	ncurves = ap->a_m;
	pts_per_curve = ap->a_n;

	/*
	 * Read all the curves into memory, and store their pointers
	 */
	i = (ncurves+1) * sizeof(fastf_t **);
	curves = (fastf_t **)malloc( i );
	for( i=0; i < ncurves; i++ )  {
		curves[i] = rd_curve( pts_per_curve );
	}

	/*
	 * Convert from vector to point notation IN PLACE
	 * by rotating vectors and adding base vector.
	 * Observe special treatment for base vector.
	 */
	for( i = 0; i < ncurves; i++ )  {
		register fastf_t *v;

		v = curves[i];
		for( j = 0; j < pts_per_curve; j++ )  {
			LOCAL vect_t	homog;

			if( i==0 && j == 0 )  {
				VMOVE( homog, v );
				MAT4X3PNT( base_vect, mat, homog );
				VMOVE( v, base_vect );
			}  else  {
				MAT4X3VEC( homog, mat, v );
				VADD2( v, base_vect, homog );
			}
			v += ELEMENTS_PER_VECT;
		}
		VMOVE( v, curves[i] );		/* replicate first point */
	}

	/*
	 * Compute bounding sphere.
	 * Find min and max of the point co-ordinates.
	 */
	stp->st_max[X] = stp->st_max[Y] = stp->st_max[Z] = -INFINITY;
	stp->st_min[X] = stp->st_min[Y] = stp->st_min[Z] =  INFINITY;

	for( i = 0; i < ncurves; i++ )  {
		register fastf_t *v;

		v = curves[i];
		for( j = 0; j < pts_per_curve; j++ )  {
			ARSMINMAX( stp->st_min[X], stp->st_max[X], *v++ );
			ARSMINMAX( stp->st_min[Y], stp->st_max[Y], *v++ );
			ARSMINMAX( stp->st_min[Z], stp->st_max[Z], *v++ );
			v++;		/* depends on ELEMENTS_PER_VECT */
		}
	}
	VSET( stp->st_center,
		(stp->st_max[X] + stp->st_min[X])/2,
		(stp->st_max[Y] + stp->st_min[Y])/2,
		(stp->st_max[Z] + stp->st_min[Z])/2 );

	dx = (stp->st_max[X] - stp->st_min[X])/2;
	f = dx;
	dy = (stp->st_max[Y] - stp->st_min[Y])/2;
	if( dy > f )  f = dy;
	dz = (stp->st_max[Z] - stp->st_min[Z])/2;
	if( dz > f )  f = dz;
	stp->st_aradius = f;
	stp->st_bradius = sqrt(dx*dx + dy*dy + dz*dz);
	stp->st_specific = (int *) 0;

	/*
	 *  Compute planar faces
	 *  Will examine curves[i][pts_per_curve], provided by rd_curve.
	 */
	for( i = 0; i < ncurves-1; i++ )  {
		register fastf_t *v1, *v2;

		v1 = curves[i];
		v2 = curves[i+1];
		for( j = 0; j < pts_per_curve;
		    j++, v1 += ELEMENTS_PER_VECT, v2 += ELEMENTS_PER_VECT )  {
		    	/* carefully make faces, w/inward pointing normals */
			arsface( stp,
				&v1[0],
				&v2[ELEMENTS_PER_VECT],
				&v1[ELEMENTS_PER_VECT] );
			arsface( stp,
				&v2[0],
				&v2[ELEMENTS_PER_VECT],
				&v1[0] );
		}
	}

	/*
	 *  Free storage for faces
	 */
	for( i = 0; i < ncurves; i++ )  {
		free( (char *)curves[i] );
	}
	free( (char *)curves );

	return(0);		/* OK */
}

/*
 *			R D _ C U R V E
 *  
 *  rd_curve() reads a set of ARS B records and returns a pointer
 *  to a malloc()'ed memory area of fastf_t's to hold the curve.
 */
fastf_t *
rd_curve(npts)
int npts;
{
	LOCAL int bytes;
	LOCAL int lim;
	LOCAL fastf_t *base;
	register fastf_t *fp;		/* pointer to temp vector */
	register int i;
	LOCAL union record r;

	/* Leave room for first point to be repeated */
	bytes = (npts+1) * sizeof(fastf_t) * ELEMENTS_PER_VECT;
	if( (fp = (fastf_t *)malloc(bytes)) == (fastf_t *) 0 )
		rt_bomb("ars.c/rd_curve():  malloc error");
	base = fp;

	while( npts > 0 )  {
		if( read( rt_i.fd, (char *) &r, sizeof(r) ) != sizeof(r) )
			rt_bomb("ars.c/rd_curve():  read error");

		if( r.b.b_id != ID_ARS_B )  {
			rt_log("ars.c/rd_curve():  non-ARS_B record!\n");
			break;
		}
		lim = (npts>8) ? 8 : npts;
		for( i=0; i<lim; i++ )  {
			VMOVE( fp, &r.b.b_values[i*3] );	/* cvt, too */
			fp += ELEMENTS_PER_VECT;
			npts--;
		}
	}
	return( base );
}

/*
 *			A R S F A C E
 *
 *  This function is called with pointers to 3 points,
 *  and is used to prepare ARS faces.
 *  ap, bp, cp point to vect_t points.
 *
 * Return -
 *	0	if the 3 points didn't form a plane (eg, colinear, etc).
 *	#pts	(3) if a valid plane resulted.
 */
arsface( stp, ap, bp, cp )
struct soltab *stp;
pointp_t ap, bp, cp;
{
	register struct tri_specific *trip;
	vect_t work;
	LOCAL fastf_t m1, m2, m3, m4;

	GETSTRUCT( trip, tri_specific );
	VMOVE( trip->tri_A, ap );
	VSUB2( trip->tri_BA, bp, ap );
	VSUB2( trip->tri_CA, cp, ap );
	VCROSS( trip->tri_wn, trip->tri_BA, trip->tri_CA );

	/* Check to see if this plane is a line or pnt */
	m1 = MAGNITUDE( trip->tri_BA );
	m2 = MAGNITUDE( trip->tri_CA );
	VSUB2( work, bp, cp );
	m3 = MAGNITUDE( work );
	m4 = MAGNITUDE( trip->tri_wn );
	if( NEAR_ZERO(m1,0.0001) || NEAR_ZERO(m2,0.0001) ||
	    NEAR_ZERO(m3,0.0001) || NEAR_ZERO(m4,0.0001) )  {
		free( (char *)trip);
		if( rt_g.debug & DEBUG_ARB8 )
			(void)rt_log("ars(%s): degenerate facet\n", stp->st_name);
		return(0);			/* BAD */
	}		

	/*  wn is a GIFT-style
	 *  normal, needing to point inwards, whereas N is an outward
	 *  pointing normal.
	 *  There is no chance of "fixing" the normals, as bumps in the
	 *  sides of the ARS can point in any direction.
	 */
	VREVERSE( trip->tri_N, trip->tri_wn );
	VUNITIZE( trip->tri_N );

	/* Add this face onto the linked list for this solid */
	trip->tri_forw = (struct tri_specific *)stp->st_specific;
	stp->st_specific = (int *)trip;
	return(3);				/* OK */
}

/*
 *  			A R S _ P R I N T
 */
ars_print( stp )
register struct soltab *stp;
{
	register struct tri_specific *trip =
		(struct tri_specific *)stp->st_specific;
	register int i;

	if( trip == (struct tri_specific *)0 )  {
		rt_log("ars(%s):  no faces\n", stp->st_name);
		return;
	}
	do {
		VPRINT( "A", trip->tri_A );
		VPRINT( "B-A", trip->tri_BA );
		VPRINT( "C-A", trip->tri_CA );
		VPRINT( "BA x CA", trip->tri_wn );
		VPRINT( "Normal", trip->tri_N );
		rt_log("\n");
	} while( trip = trip->tri_forw );
}

/*
 *			A R S _ S H O T
 *  
 * Function -
 *	Shoot a ray at an ARS.
 *  
 * Returns -
 *	0	MISS
 *  	segp	HIT
 */
struct seg *
ars_shot( stp, rp )
struct soltab *stp;
register struct xray *rp;
{
	register struct tri_specific *trip =
		(struct tri_specific *)stp->st_specific;
#define MAXHITS 12		/* # surfaces hit, must be even */
	LOCAL struct hit hits[MAXHITS];
	register struct hit *hp;
	LOCAL int	nhits;

	nhits = 0;
	hp = &hits[0];

	/* consider each face */
	for( ; trip; trip = trip->tri_forw )  {
		FAST fastf_t	dn;		/* Direction dot Normal */
		LOCAL fastf_t	abs_dn;
		FAST fastf_t	k;
		LOCAL fastf_t	alpha, beta;
		LOCAL fastf_t	ds;
		LOCAL vect_t	wxb;		/* vertex - ray_start */
		LOCAL vect_t	xp;		/* wxb cross ray_dir */

		/*
		 *  Ray Direction dot N.  (N is outward-pointing normal)
		 */
		dn = VDOT( trip->tri_wn, rp->r_dir );
		if( rt_g.debug & DEBUG_ARB8 )
			rt_log("Face N.Dir=%f\n", dn );
		/*
		 *  If ray lies directly along the face, drop this face.
		 */
		abs_dn = dn >= 0.0 ? dn : (-dn);
		if( abs_dn <= 0.0001 )
			continue;
		VSUB2( wxb, trip->tri_A, rp->r_pt );
		VCROSS( xp, wxb, rp->r_dir );

		/* Check for exceeding along the one side */
		alpha = VDOT( trip->tri_CA, xp );
		if( dn < 0.0 )  alpha = -alpha;
		if( alpha < 0.0 || alpha > abs_dn )
			continue;

		/* Check for exceeding along the other side */
		beta = VDOT( trip->tri_BA, xp );
		if( dn > 0.0 )  beta = -beta;
		if( beta < 0.0 || beta > abs_dn )
			continue;
		if( alpha+beta > abs_dn )
			continue;
		ds = VDOT( wxb, trip->tri_wn );
		k = ds / dn;		/* shot distance */

		/* For hits other than the first one, might check
		 *  to see it this is approx. equal to previous one */

		/*  If dn < 0, we should be entering the solid.
		 *  However, we just assume in/out sorting later will work.
		 *  Really should mark and check this!
		 */
		VJOIN1( hp->hit_point, rp->r_pt, k, rp->r_dir );

		/* HIT is within planar face */
		hp->hit_dist = k;
		VMOVE( hp->hit_normal, trip->tri_N );
		if(rt_g.debug&DEBUG_ARB8) rt_log("ars: hit dist=%f, dn=%f, k=%f\n", hp->hit_dist, dn, k );
		if( nhits++ >= MAXHITS )  {
			rt_log("ars(%s): too many hits\n", stp->st_name);
			break;
		}
		hp++;
	}
	if( nhits == 0 )
		return(SEG_NULL);		/* MISS */

	/* Sort hits, Near to Far */
	HitSort( hits, nhits );

	if( nhits&1 )  {
		register int i;
		/*
		 * If this condition exists, it is almost certainly due to
		 * the dn==0 check above.  Thus, we will make the last
		 * surface infinitely thin and just replicate the entry
		 * point as the exit point.  This at least makes the
		 * presence of this solid known.  There may be something
		 * better we can do.
		 */
		hits[nhits] = hits[nhits-1];	/* struct copy */
		VREVERSE( hits[nhits].hit_normal, hits[nhits-1].hit_normal );
		hits[nhits].hit_dist *= 1.0001;
		rt_log("ERROR: ars(%s): %d hits, false exit\n",
			stp->st_name, nhits);
		nhits++;
		for(i=0; i < nhits; i++ )
			rt_log("%f, ", hits[i].hit_dist );
		rt_log("\n");
	}

	/* nhits is even, build segments */
	{
		register struct seg *segp;			/* XXX */
		segp = SEG_NULL;
		while( nhits > 0 )  {
			register struct seg *newseg;		/* XXX */
			GET_SEG(newseg);
			newseg->seg_next = segp;
			segp = newseg;
			segp->seg_stp = stp;
			segp->seg_in = hits[nhits-2];	/* struct copy */
			segp->seg_out = hits[nhits-1];	/* struct copy */
			nhits -= 2;
		}
		return(segp);			/* HIT */
	}
	/* NOTREACHED */
}

HitSort( h, nh )
register struct hit h[];
register int nh;
{
	register int i, j;
	LOCAL struct hit temp;

	for( i=0; i < nh-1; i++ )  {
		for( j=i+1; j < nh; j++ )  {
			if( h[i].hit_dist <= h[j].hit_dist )
				continue;
			temp = h[j];		/* struct copy */
			h[j] = h[i];		/* struct copy */
			h[i] = temp;		/* struct copy */
		}
	}
}

ars_norm()
{
}
ars_uv()
{
}
