/*
 *  			H A L F . C
 *  
 *  Function -
 *  	Intersect a ray with a Halfspace
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
static char RCShalf[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./debug.h"


/*
 *			Ray/HALF Intersection
 *
 *  A HALFSPACE is defined by an outward pointing normal vector,
 *  and the distance from the origin to the plane, which is defined
 *  by N and d.
 *
 *  With outward pointing normal vectors,
 *  note that the ray enters the half-space defined by a plane when D cdot N <
 *  0, is parallel to the plane when D cdot N = 0, and exits otherwise.
 */

struct half_specific  {
	fastf_t	half_d;			/* dist from origin along N */
	vect_t	half_N;			/* Unit-length Normal (outward) */
	vect_t	half_Xbase;		/* "X" basis direction */
	vect_t	half_Ybase;		/* "Y" basis direction */
};
#define HALF_NULL	((struct half_specific *)0)

/*
 *			R T _ M A K E _ P E R P
 *
 *  Given a vector, create another vector which is perpendicular to it,
 *  but may not have unit length.
 */
void
rt_make_perp( new, old )
vect_t new, old;
{
	register int i;
	LOCAL vect_t another;	/* Another vector, different */

	i = X;
	if( fabs(old[Y])<fabs(old[i]) )  i=Y;
	if( fabs(old[Z])<fabs(old[i]) )  i=Z;
	VSETALL( another, 0 );
	another[i] = 1.0;
	if( old[X] == 0 && old[Y] == 0 && old[Z] == 0 )  {
		VMOVE( new, another );
	} else {
		VCROSS( new, another, old );
	}
}

/*
 *			R T _ O R T H O V E C
 *
 *  Given a vector, create another vector which is perpendicular to it,
 *  and with unit length.  This algorithm taken from Gift's arvec.f;
 *  a faster algorithm may be possible.
 */
rt_orthovec( out, in )
register fastf_t *out, *in;
{
	register int j, k;
	FAST fastf_t	f;

	if( NEAR_ZERO(in[X], 0.0001) && NEAR_ZERO(in[Y], 0.0001) &&
	    NEAR_ZERO(in[Z], 0.0001) )  {
		VSETALL( out, 0 );
		VPRINT("rt_orthovec: zero-length input", in);
		return;
	}

	/* Find component closest to zero */
	f = fabs(in[X]);
	j = Y;
	k = Z;
	if( fabs(in[Y]) < f )  {
		f = fabs(in[Y]);
		j = Z;
		k = X;
	}
	if( fabs(in[Z]) < f )  {
		j = X;
		k = Y;
	}
	f = hypot( in[j], in[k] );
	if( NEAR_ZERO( f, SMALL ) ) {
		VPRINT("rt_orthovec: zero hypot on", in);
		VSETALL( out, 0 );
		return;
	}
	f = 1.0/f;
	VSET( out, 0, -in[k]*f, in[j]*f );
}

/*
 *  			H L F _ P R E P
 */
hlf_prep( vec, stp, mat )
fastf_t *vec;
struct soltab *stp;
matp_t mat;
{
	register struct half_specific *halfp;
	FAST fastf_t f;

	/*
	 * Process a HALFSPACE, which is represented as a 
	 * normal vector, and a distance.
	 */
	GETSTRUCT( halfp, half_specific );
	stp->st_specific = (int *)halfp;

	VMOVE( halfp->half_N, &vec[0] );
	f = MAGSQ( halfp->half_N );
	if( f < 0.001 )  {
		rt_log("half(%s):  bad normal\n", stp->st_name );
		return(1);	/* BAD */
	}
	f -= 1.0;
	if( !NEAR_ZERO( f, 0.001 ) )  {
		rt_log("half(%s):  normal not unit length\n", stp->st_name );
		VUNITIZE( halfp->half_N );
	}
	halfp->half_d = vec[1*ELEMENTS_PER_VECT];
	VSCALE( stp->st_center, halfp->half_N, halfp->half_d );

	/* X and Y basis for uv map */
	rt_make_perp( halfp->half_Xbase, stp->st_center );
	VCROSS( halfp->half_Ybase, halfp->half_Xbase, halfp->half_N );
	VUNITIZE( halfp->half_Xbase );
	VUNITIZE( halfp->half_Ybase );

	/* No bounding sphere or bounding RPP is possible */
	VSETALL( stp->st_min, -INFINITY);
	VSETALL( stp->st_max,  INFINITY);

	stp->st_aradius = INFINITY;
	stp->st_bradius = INFINITY;
	return(0);		/* OK */
}

/*
 *  			H L F _ P R I N T
 */
hlf_print( stp )
register struct soltab *stp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;
	register int i;

	if( halfp == HALF_NULL )  {
		rt_log("half(%s):  no data?\n", stp->st_name);
		return;
	}
	VPRINT( "Normal", halfp->half_N );
	rt_log( "d = %f\n", halfp->half_d );
	VPRINT( "Xbase", halfp->half_Xbase );
	VPRINT( "Ybase", halfp->half_Ybase );
}

/*
 *			H L F _ S H O T
 *  
 * Function -
 *	Shoot a ray at a HALFSPACE
 *
 * Algorithm -
 * 	The intersection distance is computed.
 *  
 * Returns -
 *	0	MISS
 *  	segp	HIT
 */
struct seg *
hlf_shot( stp, rp, res )
struct soltab		*stp;
register struct xray	*rp;
struct resource		*res;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;
	LOCAL fastf_t	in, out;	/* ray in/out distances */

	in = -INFINITY;
	out = INFINITY;

	{
		FAST fastf_t	dn;		/* Direction dot Normal */
		FAST fastf_t	dxbdn;

		dxbdn = VDOT( halfp->half_N, rp->r_pt ) - halfp->half_d;
		if( (dn = -VDOT( halfp->half_N, rp->r_dir )) < -1.0e-10 )  {
			/* exit point, when dir.N < 0.  out = min(out,s) */
			out = dxbdn/dn;
		} else if ( dn > 1.0e-10 )  {
			/* entry point, when dir.N > 0.  in = max(in,s) */
			in = dxbdn/dn;
		}  else  {
			/* ray is parallel to plane when dir.N == 0.
			 * If it is outside the solid, stop now */
			if( dxbdn > 0.0 )
				return( SEG_NULL );	/* MISS */
		}
	}
	if( rt_g.debug & DEBUG_ARB8 )
		rt_log("half: in=%f, out=%f\n", in, out);

	{
		register struct seg *segp;

		GET_SEG( segp, res );
		segp->seg_stp = stp;
		segp->seg_in.hit_dist = in;
		segp->seg_out.hit_dist = out;
		return(segp);			/* HIT */
	}
	/* NOTREACHED */
}

/*
 *  			H L F _ N O R M
 *
 *  Given ONE ray distance, return the normal and entry/exit point.
 *  The normal is already filled in.
 */
hlf_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;
	FAST fastf_t f;

	/*
	 * At most one normal is really defined, but whichever one
	 * it is, it has value half_N.
	 */
	VMOVE( hitp->hit_normal, halfp->half_N );

	/* We are expected to compute hit_point here.  May be infinite. */
	f = hitp->hit_dist;
	if( f <= -INFINITY || f >= INFINITY )  {
		VJOIN1( hitp->hit_point, rp->r_pt, f, rp->r_dir );
	} else {
		rt_log("hlf_norm:  dist=INFINITY, pt=?\n");
		VSETALL( hitp->hit_point, INFINITY );
	}
}

/*
 *			H L F _ C U R V E
 *
 *  Return the "curvature" of the halfspace.
 *  Pick a principle direction orthogonal to normal, and 
 *  indicate no curvature.
 */
hlf_curve( cvp, hitp, stp, rp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
struct xray *rp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;

	rt_orthovec( cvp->crv_pdir, halfp->half_N );
	cvp->crv_c1 = cvp->crv_c2 = 0;
}

/*
 *  			H L F _ U V
 *  
 *  For a hit on a face of an HALF, return the (u,v) coordinates
 *  of the hit point.  0 <= u,v <= 1.
 *  u extends along the Xbase direction
 *  v extends along the "Ybase" direction
 *  Note that a "toroidal" map is established, varying each from
 *  0 up to 1 and then back down to 0 again.
 */
hlf_uv( ap, stp, hitp, uvp )
struct application *ap;
struct soltab *stp;
register struct hit *hitp;
register struct uvcoord *uvp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;
	LOCAL vect_t P_A;
	FAST fastf_t f;
	auto double ival;

	f = hitp->hit_dist;
	if( f <= -INFINITY || f >= INFINITY )  {
		rt_log("hlf_uv:  infinite dist\n");
		rt_pr_hit( "hlf_uv", hitp );
		uvp->uv_u = uvp->uv_v = 0;
		uvp->uv_du = uvp->uv_dv = 0;
		return;
	}
	VSUB2( P_A, hitp->hit_point, stp->st_center );

	f = VDOT( P_A, halfp->half_Xbase )/10000;
	if( f <= -INFINITY || f >= INFINITY )  {
		rt_log("hlf_uv:  bad X vdot\n");
		VPRINT("Xbase", halfp->half_Xbase);
		rt_pr_hit( "hlf_uv", hitp );
		VPRINT("st_center", stp->st_center );
		f = 0;
	}
	if( f < 0 )  f = -f;
	f = modf( f, &ival );
	if( f < 0.5 )
		uvp->uv_u = 2 * f;		/* 0..1 */
	else
		uvp->uv_u = 2 * (1 - f);	/* 1..0 */

	f = VDOT( P_A, halfp->half_Ybase )/10000;
	if( f <= -INFINITY || f >= INFINITY )  {
		rt_log("hlf_uv:  bad Y vdot\n");
		VPRINT("Xbase", halfp->half_Ybase);
		rt_pr_hit( "hlf_uv", hitp );
		VPRINT("st_center", stp->st_center );
		f = 0;
	}
	if( f < 0 )  f = -f;
	f = modf( f, &ival );
	if( f < 0.5 )
		uvp->uv_v = 2 * f;		/* 0..1 */
	else
		uvp->uv_v = 2 * (1 - f);	/* 1..0 */

	if( uvp->uv_u < 0 || uvp->uv_v < 0 )  {
		if( rt_g.debug )
			rt_log("half_uv: bad uv=%f,%f\n", uvp->uv_u, uvp->uv_v);
		/* Fix it up */
		if( uvp->uv_u < 0 )  uvp->uv_u = (-uvp->uv_u);
		if( uvp->uv_v < 0 )  uvp->uv_v = (-uvp->uv_v);
	}
	
	uvp->uv_du = uvp->uv_dv =
		(ap->a_rbeam + ap->a_diverge * hitp->hit_dist) / (10000/2);
	if( uvp->uv_du < 0 || uvp->uv_dv < 0 )  {
		rt_pr_hit( "hlf_uv", hitp );
		uvp->uv_du = uvp->uv_dv = 0;
	}
}
