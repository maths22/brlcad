/*
 *			G _ E L L . C
 *
 *  Purpose -
 *	Intersect a ray with a Generalized Ellipsoid
 *
 *  Authors -
 *	Edwin O. Davisson	(Analysis)
 *	Michael John Muuss	(Programming)
 *	Peter F. Stiller	(Curvature Analysis)
 *	Phillip Dykstra		(RPPs, Curvature)
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
static char RCSell[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "nmg.h"
#include "./debug.h"

extern int rt_sph_prep();

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
 *  N' = W'
 *  
 *  The plane transforms back to the tangent plane at W, and this
 *  new plane (on the ellipsoid) has a normal vector of N, viz:
 *  
 *  N = inverse[ transpose( inverse[ S o R ] ) ] ( N' )
 *
 *  because if H is perpendicular to plane Q, and matrix M maps from
 *  Q to Q', then inverse[ transpose(M) ] (H) is perpendicular to Q'.
 *  Here, H and Q are in "prime space" with the unit sphere.
 *  [Somehow, the notation here is backwards].
 *  So, the mapping matrix M = inverse( S o R ), because
 *  S o R maps from normal space to the unit sphere.
 *
 *  N = inverse[ transpose( inverse[ S o R ] ) ] ( N' )
 *    = inverse[ transpose(invR o invS) ] ( N' )
 *    = inverse[ transpose(invS) o transpose(invR) ] ( N' )
 *    = inverse[ inverse(S) o R ] ( N' )
 *    = invR o S ( N' )
 *
 *    = invR o S ( W' )
 *    = invR( S( S( R( W - V ) ) ) )
 *
 *  because inverse(R) = transpose(R), so R = transpose( invR ),
 *  and S = transpose( S ).
 *
 *  Note that the normal vector N produced above will not have unit length.
 */

struct ell_specific {
	vect_t	ell_V;		/* Vector to center of ellipsoid */
	vect_t	ell_Au;		/* unit-length A vector */
	vect_t	ell_Bu;
	vect_t	ell_Cu;
	vect_t	ell_invsq;	/* [ 1/(|A|**2), 1/(|B|**2), 1/(|C|**2) ] */
	mat_t	ell_SoR;	/* Scale(Rot(vect)) */
	mat_t	ell_invRSSR;	/* invRot(Scale(Scale(Rot(vect)))) */
};

/* Should be in a header file to share betwee g_ell.c and g_sph.c */
struct ell_internal  {
	point_t	v;
	vect_t	a;
	vect_t	b;
	vect_t	c;
};

/*
 *			R T _ E L L _ I M P O R T
 *
 *  Import an ellipsoid/sphere from the database format to
 *  the internal structure.
 *  Apply modeling transformations as well.
 */
int
rt_ell_import( eip, rp, mat )
struct ell_internal	*eip;
union record		*rp;
register mat_t		mat;
{
	LOCAL fastf_t	vec[3*4];

	/* Check record type */
	if( rp->u_id != ID_SOLID )  {
		rt_log("rt_ell_import: defective record\n");
		return(-1);
	}

	/* Convert from database to internal format */
	rt_fastf_float( vec, rp->s.s_values, 4 );

	/* Apply modeling transformations */
	MAT4X3PNT( eip->v, mat, &vec[0*3] );
	MAT4X3VEC( eip->a, mat, &vec[1*3] );
	MAT4X3VEC( eip->b, mat, &vec[2*3] );
	MAT4X3VEC( eip->c, mat, &vec[3*3] );

	return(0);		/* OK */
}

/*
 *  			R T _ E L L _ P R E P
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
 *  	stp->st_specific for use by rt_ell_shot().
 */
int
rt_ell_prep( stp, rec, rtip )
struct soltab		*stp;
union record		*rec;
struct rt_i		*rtip;
{
	register struct ell_specific *ell;
	LOCAL fastf_t	magsq_a, magsq_b, magsq_c;
	LOCAL mat_t	R;
	LOCAL mat_t	Rinv;
	LOCAL mat_t	SS;
	LOCAL mat_t	mtemp;
	LOCAL vect_t	Au, Bu, Cu;	/* A,B,C with unit length */
	LOCAL vect_t	w1, w2, P;	/* used for bounding RPP */
	LOCAL fastf_t	f;
	struct ell_internal ei;
	int		i;

	/*
	 *  For a fast way out, hand this solid off to the SPH routine.
	 *  If it takes it, then there is nothing to do, otherwise
	 *  the solid is an ELL.
	 */
	if( rt_sph_prep( stp, rec, rtip ) == 0 )
		return(0);		/* OK */

	if( rec == (union record *)0 )  {
		rec = db_getmrec( rtip->rti_dbip, stp->st_dp );
		i = rt_ell_import( &ei, rec, stp->st_pathmat );
		rt_free( (char *)rec, "ell record" );
	} else {
		i = rt_ell_import( &ei, rec, stp->st_pathmat );
	}
	if( i < 0 )  {
		rt_log("rt_ell_setup(%s): db import failure\n", stp->st_name);
		return(-1);		/* BAD */
	}

	/* Validate that |A| > 0, |B| > 0, |C| > 0 */
	magsq_a = MAGSQ( ei.a );
	magsq_b = MAGSQ( ei.b );
	magsq_c = MAGSQ( ei.c );
	if( magsq_a < 0.005 || magsq_b < 0.005 || magsq_c < 0.005 ) {
		rt_log("ell(%s):  zero length A, B, or C vector\n",
			stp->st_name );
		return(1);		/* BAD */
	}

	/* Create unit length versions of A,B,C */
	f = 1.0/sqrt(magsq_a);
	VSCALE( Au, ei.a, f );
	f = 1.0/sqrt(magsq_b);
	VSCALE( Bu, ei.b, f );
	f = 1.0/sqrt(magsq_c);
	VSCALE( Cu, ei.c, f );

	/* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
	f = VDOT( Au, Bu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  A not perpendicular to B, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( Bu, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  B not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( Au, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  A not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}

	/* Solid is OK, compute constant terms now */
	GETSTRUCT( ell, ell_specific );
	stp->st_specific = (genptr_t)ell;

	VMOVE( ell->ell_V, ei.v );

	VSET( ell->ell_invsq, 1.0/magsq_a, 1.0/magsq_b, 1.0/magsq_c );
	VMOVE( ell->ell_Au, Au );
	VMOVE( ell->ell_Bu, Bu );
	VMOVE( ell->ell_Cu, Cu );

	mat_idn( ell->ell_SoR );
	mat_idn( R );

	/* Compute R and Rinv matrices */
	VMOVE( &R[0], Au );
	VMOVE( &R[4], Bu );
	VMOVE( &R[8], Cu );
	mat_trn( Rinv, R );			/* inv of rot mat is trn */

	/* Compute SoS (Affine transformation) */
	mat_idn( SS );
	SS[ 0] = ell->ell_invsq[0];
	SS[ 5] = ell->ell_invsq[1];
	SS[10] = ell->ell_invsq[2];

	/* Compute invRSSR */
	mat_mul( mtemp, SS, R );
	mat_mul( ell->ell_invRSSR, Rinv, mtemp );

	/* Compute SoR */
	VSCALE( &ell->ell_SoR[0], ei.a, ell->ell_invsq[0] );
	VSCALE( &ell->ell_SoR[4], ei.b, ell->ell_invsq[1] );
	VSCALE( &ell->ell_SoR[8], ei.c, ell->ell_invsq[2] );

	/* Compute bounding sphere */
	VMOVE( stp->st_center, ei.v );
	f = magsq_a;
	if( magsq_b > f )
		f = magsq_b;
	if( magsq_c > f )
		f = magsq_c;
	stp->st_aradius = stp->st_bradius = sqrt(f);

	/* Compute bounding RPP */
	VSET( w1, magsq_a, magsq_b, magsq_c );

	/* X */
	VSET( P, 1.0, 0, 0 );		/* bounding plane normal */
	MAT3X3VEC( w2, R, P );		/* map plane to local coord syst */
	VELMUL( w2, w2, w2 );		/* square each term */
	f = VDOT( w1, w2 );
	f = f / sqrt(f);
	stp->st_min[X] = ell->ell_V[X] - f;	/* V.P +/- f */
	stp->st_max[X] = ell->ell_V[X] + f;

	/* Y */
	VSET( P, 0, 1.0, 0 );		/* bounding plane normal */
	MAT3X3VEC( w2, R, P );		/* map plane to local coord syst */
	VELMUL( w2, w2, w2 );		/* square each term */
	f = VDOT( w1, w2 );
	f = f / sqrt(f);
	stp->st_min[Y] = ell->ell_V[Y] - f;	/* V.P +/- f */
	stp->st_max[Y] = ell->ell_V[Y] + f;

	/* Z */
	VSET( P, 0, 0, 1.0 );		/* bounding plane normal */
	MAT3X3VEC( w2, R, P );		/* map plane to local coord syst */
	VELMUL( w2, w2, w2 );		/* square each term */
	f = VDOT( w1, w2 );
	f = f / sqrt(f);
	stp->st_min[Z] = ell->ell_V[Z] - f;	/* V.P +/- f */
	stp->st_max[Z] = ell->ell_V[Z] + f;

	return(0);			/* OK */
}

void
rt_ell_print( stp )
register struct soltab *stp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;

	VPRINT("V", ell->ell_V);
	mat_print("S o R", ell->ell_SoR );
	mat_print("invRSSR", ell->ell_invRSSR );
}

/*
 *  			R T _ E L L _ S H O T
 *  
 *  Intersect a ray with an ellipsoid, where all constant terms have
 *  been precomputed by rt_ell_prep().  If an intersection occurs,
 *  a struct seg will be acquired and filled in.
 *  
 *  Returns -
 *  	0	MISS
 *  	segp	HIT
 */
struct seg *
rt_ell_shot( stp, rp, ap )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	register struct seg *segp;
	LOCAL vect_t	dprime;		/* D' */
	LOCAL vect_t	pprime;		/* P' */
	LOCAL vect_t	xlated;		/* translated vector */
	LOCAL fastf_t	dp, dd;		/* D' dot P', D' dot D' */
	LOCAL fastf_t	k1, k2;		/* distance constants of solution */
	FAST fastf_t	root;		/* root of radical */

	/* out, Mat, vect */
	MAT4X3VEC( dprime, ell->ell_SoR, rp->r_dir );
	VSUB2( xlated, rp->r_pt, ell->ell_V );
	MAT4X3VEC( pprime, ell->ell_SoR, xlated );

	dp = VDOT( dprime, pprime );
	dd = VDOT( dprime, dprime );

	if( (root = dp*dp - dd * (VDOT(pprime,pprime)-1.0)) < 0 )
		return(SEG_NULL);		/* No hit */
	root = sqrt(root);

	GET_SEG(segp, ap->a_resource);
	segp->seg_stp = stp;
	if( (k1=(-dp+root)/dd) <= (k2=(-dp-root)/dd) )  {
		/* k1 is entry, k2 is exit */
		segp->seg_in.hit_dist = k1;
		segp->seg_out.hit_dist = k2;
	} else {
		/* k2 is entry, k1 is exit */
		segp->seg_in.hit_dist = k2;
		segp->seg_out.hit_dist = k1;
	}
	return(segp);			/* HIT */
}


#define SEG_MISS(SEG)		(SEG).seg_stp=(struct soltab *) 0;	

/*
 *			R T _ E L L _ V S H O T
 *
 *  This is the Becker vector version.
 */
void
rt_ell_vshot( stp, rp, segp, n, resp)
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		  	    n; /* Number of ray/object pairs */
struct resource         *resp; /* pointer to a list of free segs */
{
	register int    i;
	register struct ell_specific *ell;
	LOCAL vect_t	dprime;		/* D' */
	LOCAL vect_t	pprime;		/* P' */
	LOCAL vect_t	xlated;		/* translated vector */
	LOCAL fastf_t	dp, dd;		/* D' dot P', D' dot D' */
	LOCAL fastf_t	k1, k2;		/* distance constants of solution */
	FAST fastf_t	root;		/* root of radical */

	/* for each ray/ellipse pair */
#	include "noalias.h"
	for(i = 0; i < n; i++){
#if !CRAY /* XXX currently prevents vectorization on cray */
	 	if (stp[i] == 0) continue; /* stp[i] == 0 signals skip ray */
#endif
		ell = (struct ell_specific *)stp[i]->st_specific;

		MAT4X3VEC( dprime, ell->ell_SoR, rp[i]->r_dir );
		VSUB2( xlated, rp[i]->r_pt, ell->ell_V );
		MAT4X3VEC( pprime, ell->ell_SoR, xlated );

		dp = VDOT( dprime, pprime );
		dd = VDOT( dprime, dprime );

		if( (root = dp*dp - dd * (VDOT(pprime,pprime)-1.0)) < 0 ) {
			SEG_MISS(segp[i]);		/* No hit */
		}
	        else {
			root = sqrt(root);

			segp[i].seg_next = SEG_NULL;
			segp[i].seg_stp = stp[i];

			if( (k1=(-dp+root)/dd) <= (k2=(-dp-root)/dd) )  {
				/* k1 is entry, k2 is exit */
				segp[i].seg_in.hit_dist = k1;
				segp[i].seg_out.hit_dist = k2;
			} else {
				/* k2 is entry, k1 is exit */
				segp[i].seg_in.hit_dist = k2;
				segp[i].seg_out.hit_dist = k1;
			}
		}
	}
}

/*
 *  			R T _ E L L _ N O R M
 *  
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_ell_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	LOCAL vect_t xlated;
	LOCAL fastf_t scale;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
	VSUB2( xlated, hitp->hit_point, ell->ell_V );
	MAT4X3VEC( hitp->hit_normal, ell->ell_invRSSR, xlated );
	scale = 1.0 / MAGNITUDE( hitp->hit_normal );
	VSCALE( hitp->hit_normal, hitp->hit_normal, scale );

	/* tuck away this scale for the curvature routine */
	hitp->hit_vpriv[X] = scale;
}

/*
 *			R T _ E L L _ C U R V E
 *
 *  Return the curvature of the ellipsoid.
 */
void
rt_ell_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	vect_t	u, v;			/* basis vectors (with normal) */
	vect_t	vec1, vec2;		/* eigen vectors */
	vect_t	tmp;
	fastf_t	a, b, c, scale;

	/*
	 * choose a tangent plane coordinate system
	 *  (u, v, normal) form a right-handed triple
	 */
	vec_ortho( u, hitp->hit_normal );
	VCROSS( v, hitp->hit_normal, u );

	/* get the saved away scale factor */
	scale = - hitp->hit_vpriv[X];

	/* find the second fundamental form */
	MAT4X3VEC( tmp, ell->ell_invRSSR, u );
	a = VDOT(u, tmp) * scale;
	b = VDOT(v, tmp) * scale;
	MAT4X3VEC( tmp, ell->ell_invRSSR, v );
	c = VDOT(v, tmp) * scale;

	eigen2x2( &cvp->crv_c1, &cvp->crv_c2, vec1, vec2, a, b, c );
	VCOMB2( cvp->crv_pdir, vec1[X], u, vec1[Y], v );
	VUNITIZE( cvp->crv_pdir );
}

/*
 *  			R T _ E L L _ U V
 *  
 *  For a hit on the surface of an ELL, return the (u,v) coordinates
 *  of the hit point, 0 <= u,v <= 1.
 *  u = azimuth
 *  v = elevation
 */
void
rt_ell_uv( ap, stp, hitp, uvp )
struct application *ap;
struct soltab *stp;
register struct hit *hitp;
register struct uvcoord *uvp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;
	LOCAL vect_t work;
	LOCAL vect_t pprime;
	LOCAL fastf_t r;

	/* hit_point is on surface;  project back to unit sphere,
	 * creating a vector from vertex to hit point which always
	 * has length=1.0
	 */
	VSUB2( work, hitp->hit_point, ell->ell_V );
	MAT4X3VEC( pprime, ell->ell_SoR, work );
	/* Assert that pprime has unit length */

	/* U is azimuth, atan() range: -pi to +pi */
	uvp->uv_u = mat_atan2( pprime[Y], pprime[X] ) * rt_inv2pi;
	if( uvp->uv_u < 0 )
		uvp->uv_u += 1.0;
	/*
	 *  V is elevation, atan() range: -pi/2 to +pi/2,
	 *  because sqrt() ensures that X parameter is always >0
	 */
	uvp->uv_v = mat_atan2( pprime[Z],
		sqrt( pprime[X] * pprime[X] + pprime[Y] * pprime[Y]) ) *
		rt_invpi + 0.5;

	/* approximation: r / (circumference, 2 * pi * aradius) */
	r = ap->a_rbeam + ap->a_diverge * hitp->hit_dist;
	uvp->uv_du = uvp->uv_dv =
		rt_inv2pi * r / stp->st_aradius;
}

/*
 *			R T _ E L L _ F R E E
 */
void
rt_ell_free( stp )
register struct soltab *stp;
{
	register struct ell_specific *ell =
		(struct ell_specific *)stp->st_specific;

	rt_free( (char *)ell, "ell_specific" );
}

int
rt_ell_class()
{
	return(0);
}

/*
 *			E L L _ 1 6 P T S
 *
 * Also used by the TGC code
 */
#define ELLOUT(n)	ov+(n-1)*3
void
ell_16pts( ov, V, A, B )
register fastf_t *ov;
register fastf_t *V;
fastf_t *A, *B;
{
	static fastf_t c, d, e, f,g,h;

	e = h = .92388;			/* cos(22.5) */
	c = d = .707107;		/* cos(45) */
	g = f = .382683;		/* cos(67.5) */

	/*
	 * For angle theta, compute surface point as
	 *
	 *	V  +  cos(theta) * A  + sin(theta) * B
	 *
	 * note that sin(theta) is cos(90-theta).
	 */

	VADD2( ELLOUT(1), V, A );
	VJOIN2( ELLOUT(2), V, e, A, f, B );
	VJOIN2( ELLOUT(3), V, c, A, d, B );
	VJOIN2( ELLOUT(4), V, g, A, h, B );
	VADD2( ELLOUT(5), V, B );
	VJOIN2( ELLOUT(6), V, -g, A, h, B );
	VJOIN2( ELLOUT(7), V, -c, A, d, B );
	VJOIN2( ELLOUT(8), V, -e, A, f, B );
	VSUB2( ELLOUT(9), V, A );
	VJOIN2( ELLOUT(10), V, -e, A, -f, B );
	VJOIN2( ELLOUT(11), V, -c, A, -d, B );
	VJOIN2( ELLOUT(12), V, -g, A, -h, B );
	VSUB2( ELLOUT(13), V, B );
	VJOIN2( ELLOUT(14), V, g, A, -h, B );
	VJOIN2( ELLOUT(15), V, c, A, -d, B );
	VJOIN2( ELLOUT(16), V, e, A, -f, B );
}

/*
 *			R T _ E L L _ P L O T
 */
int
rt_ell_plot( rp, mat, vhead, dp, abs_tol, rel_tol, norm_tol )
union record		*rp;
register mat_t		mat;
struct vlhead		*vhead;
struct directory	*dp;
double			abs_tol;
double			rel_tol;
double			norm_tol;
{
	register int		i;
	struct ell_internal	ei;
	fastf_t top[16*3];
	fastf_t middle[16*3];
	fastf_t bottom[16*3];
	fastf_t	points[3*8];

	if( rt_ell_import( &ei, rp, mat ) < 0 )  {
		rt_log("rt_ell_plot(%s): db import failure\n", dp->d_namep);
		return(-1);
	}

	ell_16pts( top, ei.v, ei.a, ei.b );
	ell_16pts( bottom, ei.v, ei.b, ei.c );
	ell_16pts( middle, ei.v, ei.a, ei.c );

	ADD_VL( vhead, &top[15*ELEMENTS_PER_VECT], 0 );
	for( i=0; i<16; i++ )  {
		ADD_VL( vhead, &top[i*ELEMENTS_PER_VECT], 1 );
	}

	ADD_VL( vhead, &bottom[15*ELEMENTS_PER_VECT], 0 );
	for( i=0; i<16; i++ )  {
		ADD_VL( vhead, &bottom[i*ELEMENTS_PER_VECT], 1 );
	}

	ADD_VL( vhead, &middle[15*ELEMENTS_PER_VECT], 0 );
	for( i=0; i<16; i++ )  {
		ADD_VL( vhead, &middle[i*ELEMENTS_PER_VECT], 1 );
	}
	return(0);
}

static point_t	octa_verts[6] = {
	{ 1, 0, 0 },	/* XPLUS */
	{-1, 0, 0 },	/* XMINUS */
	{ 0, 1, 0 },	/* YPLUS */
	{ 0,-1, 0 },	/* YMINUS */
	{ 0, 0, 1 },	/* ZPLUS */
	{ 0, 0,-1 }	/* ZMINUS */
};
#define XPLUS 0
#define XMIN  1
#define YPLUS 2
#define YMIN  3
#define ZPLUS 4
#define ZMIN  5

/* Vertices of a unit octahedron */
/* These need to be organized properly to give reasonable normals */
static struct usvert {
	int	a;
	int	b;
	int	c;
} octahedron[8] = {
    { XPLUS, ZPLUS, YPLUS },
    { YPLUS, ZPLUS, XMIN  },
    { XMIN , ZPLUS, YMIN  },
    { YMIN , ZPLUS, XPLUS },
    { XPLUS, YPLUS, ZMIN  },
    { YPLUS, XMIN , ZMIN  },
    { XMIN , YMIN , ZMIN  },
    { YMIN , XPLUS, ZMIN  }
};

struct ell_state {
	struct shell	*s;
	struct ell_internal	ei;
	mat_t		invRinvS;
	mat_t		invRoS;
	fastf_t		hunt_tol;
	fastf_t		hunt_tol_sq;
	fastf_t		theta_tol;
};

struct vert_strip {
	int		nverts_per_strip;
	int		nverts;
	struct vertex	**vp;
	int		nfaces_per_strip;
	int		nfaces;
	struct faceuse	**fu;
};

/*
 *			R T _ E L L _ T E S S
 *
 *  Tessellate an ellipsoid.
 *
 *  The strategy is based upon the approach of Jon Leech 3/24/89,
 *  from program "sphere", which generates a polygon mesh
 *  approximating a sphere by
 *  recursive subdivision. First approximation is an octahedron;
 *  each level of refinement increases the number of polygons by
 *  a factor of 4.
 *  Level 3 (128 polygons) is a good tradeoff if gouraud
 *  shading is used to render the database.
 *
 *  At the start, points ABC lie on surface of the unit sphere.
 *  Pick DEF as the midpoints of the three edges of ABC.
 *  Normalize the new points to lie on surface of the unit sphere.
 *
 *	  1
 *	  B
 *	 /\
 *    3 /  \ 4
 *    D/____\E
 *    /\    /\
 *   /	\  /  \
 *  /____\/____\
 * A      F     C
 * 0      5     2
 *
 *  Returns -
 *	-1	failure
 *	 0	OK.  *r points to nmgregion that holds this tessellation.
 */
int
rt_ell_tess( r, m, rp, mat, dp, abs_tol, rel_tol, norm_tol )
struct nmgregion	**r;
struct model		*m;
register union record	*rp;
register mat_t		mat;
struct directory	*dp;
double			abs_tol;
double			rel_tol;
double			norm_tol;
{
	LOCAL mat_t	R;
	LOCAL mat_t	S;
	LOCAL mat_t	invR;
	LOCAL mat_t	invS;
	LOCAL vect_t	Au, Bu, Cu;	/* A,B,C with unit length */
	LOCAL fastf_t	Alen, Blen, Clen;
	LOCAL fastf_t	invAlen, invBlen, invClen;
	LOCAL fastf_t	magsq_a, magsq_b, magsq_c;
	LOCAL fastf_t	f;
	struct ell_state	state;
	register int		i;
	fastf_t		radius;
	int		nsegs;
	int		nstrips;
	struct vert_strip	*strips;
	int		j;
	struct vertex		**vertp[4];
	int	faceno;
	int	stripno;
	int	boff;		/* base offset */
	int	toff;		/* top offset */
	int	blim;		/* base subscript limit */
	int	tlim;		/* top subscrpit limit */

	if( rt_ell_import( &state.ei, rp, mat ) < 0 )  {
		rt_log("rt_ell_tess(%s): import failure\n", dp->d_namep);
		return(-1);
	}

	/* Validate that |A| > 0, |B| > 0, |C| > 0 */
	magsq_a = MAGSQ( state.ei.a );
	magsq_b = MAGSQ( state.ei.b );
	magsq_c = MAGSQ( state.ei.c );
	if( magsq_a < 0.005 || magsq_b < 0.005 || magsq_c < 0.005 ) {
		rt_log("rt_ell_tess(%s):  zero length A, B, or C vector\n",
			dp->d_namep );
		return(-2);		/* BAD */
	}

	/* Create unit length versions of A,B,C */
	invAlen = 1.0/(Alen = sqrt(magsq_a));
	VSCALE( Au, state.ei.a, invAlen );
	invBlen = 1.0/(Blen = sqrt(magsq_b));
	VSCALE( Bu, state.ei.b, invBlen );
	invClen = 1.0/(Clen = sqrt(magsq_c));
	VSCALE( Cu, state.ei.c, invClen );
rt_log("ell radii A=%g, B=%g, C=%g\n", Alen, Blen, Clen);
VPRINT("Au", Au);
VPRINT("Bu", Bu);
VPRINT("Cu", Cu);

	/* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
	f = VDOT( Au, Bu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  A not perpendicular to B, f=%f\n",dp->d_namep, f);
		return(-3);		/* BAD */
	}
	f = VDOT( Bu, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  B not perpendicular to C, f=%f\n",dp->d_namep, f);
		return(-3);		/* BAD */
	}
	f = VDOT( Au, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("ell(%s):  A not perpendicular to C, f=%f\n",dp->d_namep, f);
		return(-3);		/* BAD */
	}

	{
		vect_t	axb;
		VCROSS( axb, Au, Bu );
		f = VDOT( axb, Cu );
rt_log("AxB dot C = %g\n", f);
		if( f < 0 )  {
			VREVERSE( Cu, Cu );
			VREVERSE( state.ei.c, state.ei.c );
		}
	}

	/* Compute R and Rinv matrices */
	mat_idn( R );
	VMOVE( &R[0], Au );
	VMOVE( &R[4], Bu );
	VMOVE( &R[8], Cu );
	mat_trn( invR, R );			/* inv of rot mat is trn */

	/* Compute S and invS matrices */
	/* invS is just 1/diagonal elements */
	mat_idn( S );
	S[ 0] = invAlen;
	S[ 5] = invBlen;
	S[10] = invClen;
	mat_inv( invS, S );

	/* invRinvS, for converting points from unit sphere to model */
	mat_mul( state.invRinvS, invR, invS );
#if 0
mat_print("R", R);
mat_print("invR", invR);
mat_print("S", S);
mat_print("invS", invS);
mat_print("invRinvS", state.invRinvS);
#endif

	/* invRoS, for converting normals from unit sphere to model */
	mat_mul( state.invRoS, invR, S );

	/* Compute radius of bounding sphere */
	radius = Alen;
	if( Blen > radius )
		radius = Blen;
	if( Clen > radius )
		radius = Clen;

	/*
	 *  Establish tolerances
	 */
	if( rel_tol <= 0.0 || rel_tol >= 1.0 )  {
		rel_tol = 0.0;		/* none */
	} else {
		/* Convert rel to absolute by scaling by diameter */
		rel_tol *= 2*radius;
	}
	if( abs_tol <= 0.0 )  {
		if( rel_tol <= 0.0 )  {
			/* No tolerance given, use a default */
			abs_tol = 2 * 0.10 * radius;	/* 10% */
		} else {
			/* Use absolute-ized relative tolerance */
			abs_tol = rel_tol;
		}
	} else {
		/* Absolute tolerance was given, pick smaller */
		if( rel_tol > 0.0 && rel_tol < abs_tol )
			abs_tol = rel_tol;
	}
	/*
	 *  Converte distance tolerance into a maximum permissible
	 *  angle tolerance.  'radius' is largest radius.
	 */
	
	state.theta_tol = 2 * acos( 1.0 - abs_tol / radius );
rt_log("ell abs_tol=%g, state.theta_tol=%g\n", abs_tol, state.theta_tol);

	/* To ensure normal tolerance, remain below this angle */
	if( norm_tol > 0.0 && norm_tol < state.theta_tol )  {
		state.theta_tol = norm_tol;
		abs_tol = radius * ( 1 - cos( state.theta_tol * 0.5 ) );
rt_log("norm_tol abs_tol=%g, state.theta_tol=%g\n", abs_tol, state.theta_tol);
	}

	state.hunt_tol = abs_tol * 0.01;
	state.hunt_tol_sq = state.hunt_tol * state.hunt_tol;
rt_log("hunt_tol = %g, hunt_tol_sq=%g\n", state.hunt_tol, state.hunt_tol_sq);

	*r = nmg_mrsv( m );	/* Make region, empty shell, vertex */
	state.s = m->r_p->s_p;

	/* Find the number of segments to divide 90 degrees worth into */
	nsegs = rt_halfpi / state.theta_tol + 0.999;
	if( nsegs < 2 )  nsegs = 2;

	/*  Find total number of strips of vertices that will be needed.
	 *  nsegs for each hemisphere, plus the equator.
	 */
	nstrips = 2 * nsegs + 1;
	strips = (struct vert_strip *)rt_calloc( nstrips,
		sizeof(struct vert_strip), "strips[]" );

	/* North pole */
	strips[0].nverts = 1;
	strips[0].nverts_per_strip = 0;
	strips[0].nfaces = 0;
	strips[0].nfaces_per_strip = 0;
	strips[0].fu = (struct faceuse **)0;
	/* South pole */
	strips[nstrips-1].nverts = 1;
	strips[nstrips-1].nverts_per_strip = 0;
	strips[nstrips-1].nfaces = 0;
	strips[nstrips-1].nfaces_per_strip = 0;
	strips[nstrips-1].fu = (struct faceuse **)0;
	/* equator */
	strips[nsegs].nverts = nsegs * 4;
	strips[nsegs].nverts_per_strip = nsegs;
	strips[nsegs].nfaces_per_strip = 2 * nsegs - 1;
	strips[nsegs].nfaces = (2 * nsegs - 1)*4;

	for( i=1; i<nsegs; i++ )  {
		strips[i].nverts_per_strip =
			strips[nstrips-1-i].nverts_per_strip = i;
		strips[i].nverts =
			strips[nstrips-1-i].nverts = i * 4;
		strips[i].nfaces_per_strip =
			strips[nstrips-1-i].nfaces_per_strip = 2 * i - 1;
		strips[i].nfaces =
			strips[nstrips-1-i].nfaces = (2 * i - 1)*4;
	}
	/* All strips have vertices */
	for( i=0; i<nstrips; i++ )  {
		strips[i].vp = (struct vertex **)rt_calloc( strips[i].nverts,
			sizeof(struct vertex *), "strip vertex[]" );
	}
	/* All strips have faces, except for the poles */
	for( i=1; i < nstrips-1; i++ )  {
		strips[i].fu = (struct faceuse **)rt_calloc( strips[i].nfaces,
			sizeof(struct faceuse *), "strip faceuse[]" );
	}

	/* First, build the triangular mesh topology */
	/* Do the top. "toff" in i-1 is UP, towards +B */
	for( i = 1; i <= nsegs; i++ )  {
		faceno = 0;
		tlim = strips[i-1].nverts;
		blim = strips[i].nverts;
		for( stripno=0; stripno<4; stripno++ )  {
			toff = stripno * strips[i-1].nverts_per_strip;
			boff = stripno * strips[i].nverts_per_strip;

			/* Connect this quarter strip */
			for( j = 0; j < strips[i].nverts_per_strip; j++ )  {

				/* "Right-side-up" triangle */
				vertp[0] = &(strips[i].vp[j+boff]);
				vertp[1] = &(strips[i].vp[(j+1+boff)%blim]);
				vertp[2] = &(strips[i-1].vp[(j+toff)%tlim]);
				if( (strips[i].fu[faceno++] = nmg_cmface(state.s, vertp, 3 )) == 0 )  {
					rt_log("rt_ell_tess() nmg_cmface failure\n");
					goto fail;
				}
				if( j+1 >= strips[i].nverts_per_strip )  break;

				/* Follow with interior "Up-side-down" triangle */
				vertp[0] = &(strips[i].vp[(j+1+boff)%blim]);
				vertp[1] = &(strips[i-1].vp[(j+1+toff)%tlim]);
				vertp[2] = &(strips[i-1].vp[(j+toff)%tlim]);
				if( (strips[i].fu[faceno++] = nmg_cmface(state.s, vertp, 3 )) == 0 )  {
					rt_log("rt_ell_tess() nmg_cmface failure\n");
					goto fail;
				}
			}
		}
	}
	/* Do the bottom.  Everything is upside down. "toff" in i+1 is DOWN */
	for( i = nsegs; i < nstrips; i++ )  {
		faceno = 0;
		tlim = strips[i+1].nverts;
		blim = strips[i].nverts;
		for( stripno=0; stripno<4; stripno++ )  {
			toff = stripno * strips[i+1].nverts_per_strip;
			boff = stripno * strips[i].nverts_per_strip;

			/* Connect this quarter strip */
			for( j = 0; j < strips[i].nverts_per_strip; j++ )  {

				/* "Right-side-up" triangle */
				vertp[0] = &(strips[i].vp[j+boff]);
				vertp[1] = &(strips[i+1].vp[(j+toff)%tlim]);
				vertp[2] = &(strips[i].vp[(j+1+boff)%blim]);
				if( (strips[i].fu[faceno++] = nmg_cmface(state.s, vertp, 3 )) == 0 )  {
					rt_log("rt_ell_tess() nmg_cmface failure\n");
					goto fail;
				}
				if( j+1 >= strips[i].nverts_per_strip )  break;

				/* Follow with interior "Up-side-down" triangle */
				vertp[0] = &(strips[i].vp[(j+1+boff)%blim]);
				vertp[1] = &(strips[i+1].vp[(j+toff)%tlim]);
				vertp[2] = &(strips[i+1].vp[(j+1+toff)%tlim]);
				if( (strips[i].fu[faceno++] = nmg_cmface(state.s, vertp, 3 )) == 0 )  {
					rt_log("rt_ell_tess() nmg_cmface failure\n");
					goto fail;
				}
			}
		}
	}

	/*  Compute the geometry of each vertex.
	 *  Start with the location in the unit sphere, and project back.
	 *  i=0 is "straight up" along +B.
	 */
	for( i=0; i < nstrips; i++ )  {
		double	alpha;		/* decline down from B to A */
		double	beta;		/* angle around equator (azimuth) */
		fastf_t		cos_alpha, sin_alpha;
		fastf_t		cos_beta, sin_beta;

		alpha = (((double)i) / (nstrips-1));
		cos_alpha = cos(alpha*rt_pi);
		sin_alpha = sin(alpha*rt_pi);
		for( j=0; j < strips[i].nverts; j++ )  {
			point_t		sphere_pt;
			point_t		model_pt;

			beta = ((double)j) / strips[i].nverts;
			cos_beta = cos(beta*rt_twopi);
			sin_beta = sin(beta*rt_twopi);
			VSET( sphere_pt,
				cos_beta * sin_alpha,
				cos_alpha,
				sin_beta * sin_alpha );
			/* Convert from ideal sphere coordinates */
			MAT4X3PNT( model_pt, state.invRinvS, sphere_pt );
			VADD2( model_pt, model_pt, state.ei.v );
			/* Associate vertex geometry */
			if( strips[i].vp[j] )
				nmg_vertex_gv( strips[i].vp[j], model_pt );
		}
	}

	/* Associate face geometry.  Poles have no faces "above" them. */
	for( i=1; i < nstrips-1; i++ )  {
		for( j=0; j < strips[i].nfaces; j++ )  {
			if( strips[i].fu[j] )
				rt_mk_nmg_planeeqn( strips[i].fu[j] );
		}
	}

	/* Compute "geometry" for region and shell */
	nmg_region_a( *r );

	/* Release memory */
	/* All strips have vertices */
	for( i=0; i<nstrips; i++ )  {
		rt_free( (char *)strips[i].vp, "strip vertex[]" );
	}
	/* All strips have faces, except for the poles */
	for( i=1; i < nstrips-1; i++ )  {
		rt_free( (char *)strips[i].fu, "strip faceuse[]" );
	}
	rt_free( (char *)strips, "strips[]" );
	return(0);
fail:
	/* Release memory */
	/* All strips have vertices */
	for( i=0; i<nstrips; i++ )  {
		rt_free( (char *)strips[i].vp, "strip vertex[]" );
	}
	/* All strips have faces, except for the poles */
	for( i=1; i < nstrips-1; i++ )  {
		rt_free( (char *)strips[i].fu, "strip faceuse[]" );
	}
	rt_free( (char *)strips, "strips[]" );
	return(-1);
}
