/*
 *			E L L . C
 *
 *  Purpose -
 *	Intersect a ray with a Generalized Ellipsoid
 *
 *  Authors -
 *	Edwin O. Davisson	(Analysis)
 *	Michael John Muuss	(Programming)
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
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/db.h"
#include "raytrace.h"
#include "debug.h"

/*
 *  Algorithm:
 *  
 *  Given V, A, B, and C, there is a set of points on this ellipsoid
 *  
 *  { (x,y,z) | (x,y,z) is on ellipsoid defined by V, A, B, C }
 *  
 *  Through a series of Affine Transformations, this set will be
 *  transformed into a set of points on a unit sphere at the origin
 *  
 *  { (x',y',z') | (x',y',z') is on Sphere at origin }
 *  
 *  The transformation from X to X' is accomplished by:
 *  
 *  X' = S(R( X - V ))
 *  
 *  where R(X) =  ( A/(|A|) )
 *  		 (  B/(|B|)  ) . X
 *  		  ( C/(|C|) )
 *  
 *  and S(X) =	 (  1/|A|   0     0   )
 *  		(    0    1/|B|   0    ) . X
 *  		 (   0      0   1/|C| )
 *  
 *  To find the intersection of a line with the ellipsoid, consider
 *  the parametric line L:
 *  
 *  	L : { P(n) | P + t(n) . D }
 *  
 *  Call W the actual point of intersection between L and the ellipsoid.
 *  Let W' be the point of intersection between L' and the unit sphere.
 *  
 *  	L' : { P'(n) | P' + t(n) . D' }
 *  
 *  W = invR( invS( W' ) ) + V
 *  
 *  Where W' = k D' + P'.
 *  
 *  Let dp = D' dot P'
 *  Let dd = D' dot D'
 *  Let pp = P' dot P'
 *  
 *  and k = [ -dp +/- sqrt( dp*dp - dd * (pp - 1) ) ] / dd
 *  which is constant.
 *  
 *  Now, D' = S( R( D ) )
 *  and  P' = S( R( P - V ) )
 *  
 *  Substituting,
 *  
 *  W = V + invR( invS[ k *( S( R( D ) ) ) + S( R( P - V ) ) ] )
 *    = V + invR( ( k * R( D ) ) + R( P - V ) )
 *    = V + k * D + P - V
 *    = k * D + P
 *  
 *  Note that ``k'' is constant, and is the same in the formulations
 *  for both W and W'.
 *  
 *  NORMALS.  Given the point W on the ellipsoid, what is the vector
 *  normal to the tangent plane at that point?
 *  
 *  Map W onto the unit sphere, ie:  W' = S( R( W - V ) ).
 *  
 *  Plane on unit sphere at W' has a normal vector of the same value(!).
 *  
 *  The plane transforms back to the tangent plane at W, and this
 *  new plane (on the ellipsoid) has a normal vector of N, viz:
 *  
 *  N = inverse[ transpose(invR o invS) ] ( W' )
 *    = inverse[ transpose(invS) o transpose(invR) ] ( W' )
 *    = inverse[ inverse(S) o R ] ( W' )
 *    = invR o S ( W' )
 *    = invR( S( S( R( W - V ) ) ) )
 *
 *  Note that the normal vector produced above will not have unit length.
 */

struct ell_specific {
	vect_t	ell_V;		/* Vector to center of ellipsoid */
	mat_t	ell_SoR;	/* Scale(Rot(vect)) */
	mat_t	ell_invRSSR;	/* invRot(Scale(Scale(Rot(vect)))) */
};

#undef EPSILON
#define EPSILON	0.005		/* More appropriate for NEAR_ZERO here */

/*
 *  			E L L _ P R E P
 *  
 *  Given a pointer to a GED database record, and a transformation matrix,
 *  determine if this is a valid ellipsoid, and if so, precompute various
 *  terms of the formula.
 *  
 *  Returns -
 *  	0	ELL is OK
 *  	!0	Error in description
 *  
 *  Implicit return -
 *  	A struct ell_specific is created, and it's address is stored in
 *  	stp->st_specific for use by ell_shot().
 */
ell_prep( vec, stp, mat )
register fastf_t *vec;
struct soltab *stp;
matp_t mat;			/* Homogenous 4x4, with translation, [15]=1 */
{
	register struct ell_specific *ell;
	LOCAL fastf_t	magsq_a, magsq_b, magsq_c;
	LOCAL mat_t	R;
	LOCAL mat_t	Rinv;
	LOCAL mat_t	SS;
	LOCAL mat_t	mtemp;
	LOCAL vect_t	A, B, C;
	LOCAL vect_t	invsq;	/* [ 1/(|A|**2), 1/(|B|**2), 1/(|C|**2) ] */
	LOCAL vect_t	work;
	LOCAL vect_t	vbc;	/* used for bounding RPP */
	LOCAL fastf_t	f;

#define SP_V	&vec[0*ELEMENTS_PER_VECT]
#define SP_A	&vec[1*ELEMENTS_PER_VECT]
#define SP_B	&vec[2*ELEMENTS_PER_VECT]
#define SP_C	&vec[3*ELEMENTS_PER_VECT]

	/*
	 * Apply rotation only to A,B,C
	 */
	MAT4X3VEC( A, mat, SP_A );
	MAT4X3VEC( B, mat, SP_B );
	MAT4X3VEC( C, mat, SP_C );

	/* Validate that |A| > 0, |B| > 0, |C| > 0 */
	magsq_a = MAGSQ( A );
	magsq_b = MAGSQ( B );
	magsq_c = MAGSQ( C );
	if( NEAR_ZERO(magsq_a) || NEAR_ZERO(magsq_b) || NEAR_ZERO(magsq_c) ) {
		fprintf(stderr,"ell(%s):  zero length A, B, or C vector\n",
			stp->st_name );
		return(1);		/* BAD */
	}

	/* Validate that A.B == 0, B.C == 0, A.C == 0 */
	f = VDOT( A, B );
	if( ! NEAR_ZERO(f) )  {
		fprintf(stderr,"ell(%s):  A not perpendicular to B, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( B, C );
	if( ! NEAR_ZERO(f) )  {
		fprintf(stderr,"ell(%s):  B not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( A, C );
	if( ! NEAR_ZERO(f) )  {
		fprintf(stderr,"ell(%s):  A not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}

	/* Solid is OK, compute constant terms now */
	GETSTRUCT( ell, ell_specific );
	stp->st_specific = (int *)ell;

	/* Apply full 4x4mat to V */
	MAT4X3PNT( ell->ell_V, mat, SP_V );

	VSET( invsq, 1.0/magsq_a, 1.0/magsq_b, 1.0/magsq_c );

	mat_idn( ell->ell_SoR );
	mat_idn( R );

	/* Compute R and Rinv matrices */
	f = 1.0/sqrt(magsq_a);
	VSCALE( &R[0], A, f );
	f = 1.0/sqrt(magsq_b);
	VSCALE( &R[4], B, f );
	f = 1.0/sqrt(magsq_c);
	VSCALE( &R[8], C, f );
	mat_trn( Rinv, R );			/* inv of rot mat is trn */

	/* Compute SoS (Affine transformation) */
	mat_idn( SS );
	SS[ 0] = invsq[0];
	SS[ 5] = invsq[1];
	SS[10] = invsq[2];

	/* Compute invRSSR */
	mat_mul( mtemp, SS, R );
	mat_mul( ell->ell_invRSSR, Rinv, mtemp );

	/* Compute SoR */
	VSCALE( &ell->ell_SoR[0], A, invsq[0] );
	VSCALE( &ell->ell_SoR[4], B, invsq[1] );
	VSCALE( &ell->ell_SoR[8], C, invsq[2] );

	/* Compute bounding sphere */
	VMOVE( stp->st_center, ell->ell_V );
	f = magsq_a;
	if( magsq_b > f )
		f = magsq_b;
	if( magsq_c > f )
		f = magsq_c;
	stp->st_radsq = f;

	/* Compute bounding RPP */
#define MINMAX(a,b,c)	{ FAST fastf_t ftemp;\
			if( (ftemp = (c)) < (a) )  a = ftemp;\
			if( ftemp > (b) )  b = ftemp; }

#define MM(v)	MINMAX( stp->st_min[X], stp->st_max[X], v[X] ); \
		MINMAX( stp->st_min[Y], stp->st_max[Y], v[Y] ); \
		MINMAX( stp->st_min[Z], stp->st_max[Z], v[Z] )

	/* There are 8 corners to the enclosing RPP;  find max and min */
	VADD3( vbc, ell->ell_V, B, C );
	VADD2( work, vbc, A ); MM( work );	/* V + A + B + C */
	VSUB2( work, vbc, A ); MM( work );	/* V - A + B + C */

	VSUB2( vbc, ell->ell_V, B );
	VADD2( vbc, vbc, C );
	VADD2( work, vbc, A ); MM( work );	/* V + A - B + C */
	VSUB2( work, vbc, A ); MM( work );	/* V - A - B + C */
	
	VSUB2( vbc, ell->ell_V, C );
	VADD2( vbc, vbc, B );
	VADD2( work, vbc, A ); MM( work );	/* V + A + B - C */
	VSUB2( work, vbc, A ); MM( work );	/* V - A + B - C */

	VSUB3( vbc, ell->ell_V, B, C );
	VADD2( work, vbc, A ); MM( work );	/* V + A - B - C */
	VSUB2( work, vbc, A ); MM( work );	/* V - A - B - C */

	return(0);			/* OK */
}

ell_print( stp )
register struct soltab *stp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;

	VPRINT("V", ell->ell_V);
	mat_print("S o R", ell->ell_SoR );
	mat_print("invRSSR", ell->ell_invRSSR );
}

/*
 *  			E L L _ S H O T
 *  
 *  Intersect a ray with an ellipsoid, where all constant terms have
 *  been precomputed by ell_prep().  If an intersection occurs,
 *  a struct seg will be acquired and filled in.
 *  
 *  Returns -
 *  	0	MISS
 *  	segp	HIT
 */
struct seg *
ell_shot( stp, rp )
struct soltab *stp;
register struct xray *rp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	register struct seg *segp;
	LOCAL vect_t	dprime;		/* D' */
	LOCAL vect_t	pprime;		/* P' */
	LOCAL fastf_t	dp, dd, pp;	/* D' dot P', D' dot D', P' dot P' */
	LOCAL fastf_t	root;		/* root of radical */
	LOCAL fastf_t	k1, k2;		/* distance constants of solution */
	LOCAL vect_t	xlated;		/* translated vector */

	/* out, Mat, vect */
	MAT4X3VEC( dprime, ell->ell_SoR, rp->r_dir );
	VSUB2( xlated, rp->r_pt, ell->ell_V );
	MAT4X3VEC( pprime, ell->ell_SoR, xlated );

	dp = VDOT( dprime, pprime );
	dd = VDOT( dprime, dprime );
	pp = VDOT( pprime, pprime );

	root = dp*dp - dd * (pp-1.0);
	if( root < 0 )
		return(SEG_NULL);		/* No hit */
	root = sqrt(root);

	k1 = (-dp + root) / dd;
	k2 = (-dp - root) / dd;

	if( k1 > k2 )  {
		FAST fastf_t f;	/*  XXX  */
		f = k1;
		k1 = k2;
		k2 = f;
	}
	/*
	 * Now, k1 is entry point, and k2 is exit point
	 */
	GET_SEG(segp);
	segp->seg_stp = stp;

	/* ASSERT that MAGNITUDE(rp->r_dir) == 1 */
	segp->seg_in.hit_dist = k1;
	segp->seg_out.hit_dist = k2;

	/*
	 *  For each point, if intersect comes "after" start of ray,
	 *  compute exact intersection point, and surface normal.
	 */
	segp->seg_flag = 0;
	if( k1 >= 0 )  {
		FAST fastf_t f;		/* XXX */
		segp->seg_flag |= SEG_IN;

		/* Intersection point */
		VJOIN1( segp->seg_in.hit_point, rp->r_pt, k1, rp->r_dir );

		/* Normal at that point, pointing out */
		VSUB2( xlated, segp->seg_in.hit_point, ell->ell_V );
		MAT4X3VEC( segp->seg_in.hit_normal, ell->ell_invRSSR, xlated );
		f = 1.0 / MAGNITUDE(segp->seg_in.hit_normal );
		VSCALE( segp->seg_in.hit_normal, segp->seg_in.hit_normal, f);
	}
	if( k2 >= 0 )  {
		FAST fastf_t f;		/* XXX */
		segp->seg_flag |= SEG_OUT;

		VJOIN1( segp->seg_out.hit_point, rp->r_pt, k2, rp->r_dir );

		VSUB2( xlated, segp->seg_out.hit_point, ell->ell_V );
		MAT4X3VEC( segp->seg_out.hit_normal,ell->ell_invRSSR, xlated );
		f = 1.0 / MAGNITUDE( segp->seg_out.hit_normal );
		VSCALE(segp->seg_out.hit_normal, segp->seg_out.hit_normal, f);
	}
	return(segp);			/* HIT */
}
