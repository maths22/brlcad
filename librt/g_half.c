/*
 *  			H A L F . C
 *  
 *  Function -
 *  	Intersect a ray with a Halfspace
 *
 *
 *  A HALFSPACE is defined by an outward pointing normal vector,
 *  and the distance from the origin to the plane, which is defined
 *  by N and d.
 *
 *  With outward pointing normal vectors,
 *  the ray enters the half-space defined by a plane when D dot N < 0,
 *  is parallel to the plane when D dot N = 0, and exits otherwise.
 *
 *  The inside of the halfspace bounded by the plane
 *  consists of all points P such that
 *	VDOT(P,N) - N[3] <= 0
 *
 *  See the remarks in h/vmath.h for more details.
 *  
 *  Authors -
 *	Michael John Muuss
 *	Dave Becker		(Vectorization)
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
#include "db.h"
#include "./debug.h"

#if defined(alliant) && !defined(__STDC__)
extern double   modf();
#endif

struct half_specific  {
	plane_t	half_eqn;		/* Plane equation, outward normal */
	vect_t	half_Xbase;		/* "X" basis direction */
	vect_t	half_Ybase;		/* "Y" basis direction */
};
#define HALF_NULL	((struct half_specific *)0)

/*
 *  			H L F _ P R E P
 */
int
hlf_prep( stp, rec, rtip )
struct soltab	*stp;
union record	*rec;
struct rt_i	*rtip;
{
	register struct half_specific *halfp;

	/*
	 * Process a HALFSPACE, which is represented as a 
	 * normal vector, and a distance.
	 */
	GETSTRUCT( halfp, half_specific );
	stp->st_specific = (genptr_t)halfp;

	if( hlf_import( halfp->half_eqn, rec, stp->st_pathmat ) < 0 )  {
		rt_log("hlf_prep(%s): db import failure\n", stp->st_name);
		rt_free( (char *)halfp, "half_specific" );
		return(1);	/* BAD */
	}

	VSCALE( stp->st_center, halfp->half_eqn, halfp->half_eqn[3] );

	/* X and Y basis for uv map */
	vec_perp( halfp->half_Xbase, stp->st_center );
	VCROSS( halfp->half_Ybase, halfp->half_Xbase, halfp->half_eqn );
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
void
hlf_print( stp )
register struct soltab *stp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;

	if( halfp == HALF_NULL )  {
		rt_log("half(%s):  no data?\n", stp->st_name);
		return;
	}
	VPRINT( "Normal", halfp->half_eqn );
	rt_log( "d = %f\n", halfp->half_eqn[3] );
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
hlf_shot( stp, rp, ap )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;
	LOCAL fastf_t	in, out;	/* ray in/out distances */

	in = -INFINITY;
	out = INFINITY;

	{
		FAST fastf_t	slant_factor;	/* Direction dot Normal */
		FAST fastf_t	norm_dist;

		norm_dist = VDOT( halfp->half_eqn, rp->r_pt ) - halfp->half_eqn[3];
		if( (slant_factor = -VDOT( halfp->half_eqn, rp->r_dir )) < -1.0e-10 )  {
			/* exit point, when dir.N < 0.  out = min(out,s) */
			out = norm_dist/slant_factor;
		} else if ( slant_factor > 1.0e-10 )  {
			/* entry point, when dir.N > 0.  in = max(in,s) */
			in = norm_dist/slant_factor;
		}  else  {
			/* ray is parallel to plane when dir.N == 0.
			 * If it is outside the solid, stop now */
			if( norm_dist > 0.0 )
				return( SEG_NULL );	/* MISS */
		}
	}
	if( rt_g.debug & DEBUG_ARB8 )
		rt_log("half: in=%f, out=%f\n", in, out);

	{
		register struct seg *segp;

		GET_SEG( segp, ap->a_resource );
		segp->seg_stp = stp;
		segp->seg_in.hit_dist = in;
		segp->seg_out.hit_dist = out;
		return(segp);			/* HIT */
	}
	/* NOTREACHED */
}

#define SEG_MISS(SEG)		(SEG).seg_stp=(struct soltab *) 0;	
/*
 *			H L F _ V S H O T
 *
 *  This is the Becker vector version
 */
void
hlf_vshot( stp, rp, segp, n, resp)
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		  	    n; /* Number of ray/object pairs */
struct resource         *resp; /* pointer to a list of free segs */
{
	register int    i;
	register struct half_specific *halfp;
	LOCAL fastf_t	in, out;	/* ray in/out distances */
	FAST fastf_t	slant_factor;	/* Direction dot Normal */
	FAST fastf_t	norm_dist;

	/* for each ray/halfspace pair */
#	include "noalias.h"
	for(i = 0; i < n; i++){
		if (stp[i] == 0) continue; /* indicates "skip this pair" */

		halfp = (struct half_specific *)stp[i]->st_specific;

		in = -INFINITY;
		out = INFINITY;

		norm_dist = VDOT(halfp->half_eqn, rp[i]->r_pt) - halfp->half_eqn[3];

		if((slant_factor = -VDOT(halfp->half_eqn, rp[i]->r_dir)) <
								-1.0e-10) {
			/* exit point, when dir.N < 0.  out = min(out,s) */
			out = norm_dist/slant_factor;
		} else if ( slant_factor > 1.0e-10 )  {
			/* entry point, when dir.N > 0.  in = max(in,s) */
			in = norm_dist/slant_factor;
		}  else  {
			/* ray is parallel to plane when dir.N == 0.
			 * If it is outside the solid, stop now */
			if( norm_dist > 0.0 ) {
				SEG_MISS(segp[i]);		/* No hit */
			        continue;
			}
		}

		/* HIT */
		segp[i].seg_next = SEG_NULL;
		segp[i].seg_stp = stp[i];
		segp[i].seg_in.hit_dist = in;
		segp[i].seg_out.hit_dist = out;
	}
}

/*
 *  			H L F _ N O R M
 *
 *  Given ONE ray distance, return the normal and entry/exit point.
 *  The normal is already filled in.
 */
void
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
	 * it is, it has value half_eqn.
	 */
	VMOVE( hitp->hit_normal, halfp->half_eqn );

	/* We are expected to compute hit_point here.  May be infinite. */
	f = hitp->hit_dist;
	if( f <= -INFINITY || f >= INFINITY )  {
		rt_log("hlf_norm:  dist=INFINITY, pt=?\n");
		VSETALL( hitp->hit_point, INFINITY );
	} else {
		VJOIN1( hitp->hit_point, rp->r_pt, f, rp->r_dir );
	}
}

/*
 *			H L F _ C U R V E
 *
 *  Return the "curvature" of the halfspace.
 *  Pick a principle direction orthogonal to normal, and 
 *  indicate no curvature.
 */
void
hlf_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;

	vec_ortho( cvp->crv_pdir, halfp->half_eqn );
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
void
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

/*
 *			H L F _ F R E E
 */
void
hlf_free( stp )
struct soltab *stp;
{
	register struct half_specific *halfp =
		(struct half_specific *)stp->st_specific;

	rt_free( (char *)halfp, "half_specific");
}

int
hlf_class()
{
	return(0);
}

/*
 *			H L F _ P L O T
 *
 *  The representation of a halfspace is an OUTWARD pointing
 *  normal vector, and the distance of the plane from the origin.
 *
 *  Drawing a halfspace is difficult when using a finite display.
 *  Drawing the boundary plane is hard enough.
 *  We just make a cross in the plane, with the outward normal
 *  drawn shorter.
 */
void
hlf_plot( rp, mat, vhead, dp )
union record	*rp;
mat_t		mat;
struct vlhead	*vhead;
struct directory *dp;
{
	plane_t	eqn;
	vect_t cent;		/* some point on the plane */
	vect_t xbase, ybase;	/* perpendiculars to normal */
	vect_t x1, x2;
	vect_t y1, y2;
	vect_t tip;

	if( hlf_import( eqn, rp, mat ) < 0 )  {
		rt_log("hlf_plot(%s): db import failure\n", dp->d_namep);
		return;
	}

	/* Invent a "center" point on the plane -- point closets to origin */
	VSCALE( cent, eqn, eqn[3] );

	/* The use of "x" and "y" here is not related to the axis */
	vec_perp( xbase, cent );
	VCROSS( ybase, xbase, eqn );

	/* Arrange for the cross to be 2 meters across */
	VUNITIZE( xbase );
	VUNITIZE( ybase);
	VSCALE( xbase, xbase, 1000 );
	VSCALE( ybase, ybase, 1000 );

	VADD2( x1, cent, xbase );
	VSUB2( x2, cent, xbase );
	VADD2( y1, cent, ybase );
	VSUB2( y2, cent, ybase );

	ADD_VL( vhead, x1, 0 );		/* the cross */
	ADD_VL( vhead, x2, 1 );
	ADD_VL( vhead, y1, 0 );
	ADD_VL( vhead, y2, 1 );
	ADD_VL( vhead, x2, 1 );		/* the box */
	ADD_VL( vhead, y1, 1 );
	ADD_VL( vhead, x1, 1 );
	ADD_VL( vhead, y2, 1 );

	VSCALE( tip, eqn, 500 );
	VADD2( tip, cent, tip );
	ADD_VL( vhead, cent, 0 );
	ADD_VL( vhead, tip, 1 );
}

/*
 *			H A L F _ I M P O R T
 *
 *  Returns -
 *	-1	failure
 *	 0	success
 */
int
hlf_import( eqn, rp, mat )
plane_t		eqn;
union record	*rp;
mat_t		mat;
{
	point_t		orig_pt;
	point_t		pt;
	fastf_t		orig_eqn[3*2];
	register double	f,t;

	if( rp->u_id != ID_SOLID )  {
		rt_log("hlf_import: defective record, id=x%x\n", rp->u_id);
		return(-1);
	}

	rt_fastf_float( orig_eqn, rp->s.s_values, 2 );	/* 2 floats too many */

	/* Pick a point on the original halfspace */
	VSCALE( orig_pt, orig_eqn, orig_eqn[1*ELEMENTS_PER_VECT] );

	/* Transform the point, and the normal */
	MAT4X3VEC( eqn, mat, orig_eqn );
	MAT4X3PNT( pt, mat, orig_pt );

	/*
	 *  The transformed normal is all that is required.
	 *  The new distance is found from the transformed point on the plane.
	 */
	eqn[3] = VDOT( pt, eqn );

	/* Verify that normal has unit length */
	f = MAGNITUDE( eqn );
	if( f < SMALL )  {
		rt_log("hlf_import:  bad normal, len=%g\n", f );
		return(-1);		/* BAD */
	}
	t = f - 1.0;
	if( !NEAR_ZERO( t, 0.001 ) )  {
		/* Restore normal to unit length */
		f = 1/f;
		VSCALE( eqn, eqn, f );
		eqn[3] *= f;
	}
	return(0);			/* OK */
}
