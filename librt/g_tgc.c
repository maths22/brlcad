/*
 *			T G C . C
 *
 * Purpose -
 *	Intersect a ray with a Truncated General Cone.
 *
 * Method -
 *	TGC:  solve quartic equation of cone and line
 *
 * Authors -
 *	Edwin O. Davisson	(Analysis)
 *	Jeff Hanes		(Programming)
 *	Gary Moss		(Improvement)
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
#include "../h/db.h"
#include "vmath.h"
#include "raytrace.h"
#include "debug.h"

#include "./polyno.h"
#include "./complex.h"
#include <math.h>

HIDDEN int	stdCone();
HIDDEN void	tgcnormal();
static void	tgc_rotate(), tgc_shear(), tgc_sort();
static void	tgc_scale();
static void	tgc_minmax();

struct  tgc_specific {
	vect_t	tgc_V;		/*  Vector to center of base of TGC	*/
	fastf_t	tgc_sH;		/*  magnitude of sheared H vector	*/
	fastf_t	tgc_A;		/*  magnitude of A vector		*/
	fastf_t	tgc_B;		/*  magnitude of B vector		*/
	fastf_t	tgc_C;		/*  magnitude of C vector		*/
	fastf_t	tgc_D;		/*  magnitude of D vector		*/
	fastf_t	tgc_ABsq;	/*  (A/B)**2 or (C/D)**2  */
	fastf_t	tgc_CA_H;	/*  (C-A)/H  X_of_Z */
	fastf_t tgc_DB_H;	/*  (D-B)/H  Y_of_Z */
	vect_t	tgc_norm;	/*  normal at 'top' of cone		*/
	mat_t	tgc_ScShR;	/*  Scale( Shear( Rot( vect )))		*/
	mat_t	tgc_inv_ScShR;	/*  invRot( invShear( invScale( vect )))*/
	mat_t	tgc_invRtShSc;	/*  invRot( trnShear( Scale( vect )))	*/
	char	tgc_AD_CB;	/*  boolean:  A*D == C*B  */
};


#define VLARGE		1000000.0

#define	Alpha(x,y,c,d)     (( NEAR_ZERO(c) || NEAR_ZERO(d) ) ? VLARGE :\
			( (x)*(x)/((c)*(c)) + (y)*(y)/((d)*(d))) )


/*
 *		>>>  t g c _ p r e p ( )  <<<
 *
 *  Given the parameters (in vector form) of a truncated general cone,
 *  compute the constant terms and a transformation matrix needed for
 *  solving the intersection of a ray with the cone.
 *
 *  Also compute the return transformation for normals in the transformed
 *  space to the original space.  This NOT the inverse of the transformation
 *  matrix (if you really want to know why, talk to Ed Davisson).
 */
tgc_prep( vec, stp, mat )
register fastf_t *vec;
struct soltab *stp;
matp_t mat;			/* Homogenous 4x4, with translation, [15]=1 */
{
	register struct tgc_specific *tgc;
	LOCAL fastf_t	magsq_h, magsq_a, magsq_b, magsq_c, magsq_d;
	LOCAL fastf_t	mag_h, mag_a, mag_b, mag_c, mag_d;
	LOCAL mat_t	Rot, Shr, Scl;
	LOCAL mat_t	iRot, tShr, iShr, iScl;
	LOCAL mat_t	tmp;
	LOCAL vect_t	Hv, A, B, C, D;
	LOCAL vect_t	nH;
	LOCAL vect_t	work;
	FAST fastf_t	f;

#define SP_V	&vec[0*ELEMENTS_PER_VECT]
#define SP_H	&vec[1*ELEMENTS_PER_VECT]
#define SP_A	&vec[2*ELEMENTS_PER_VECT]
#define SP_B	&vec[3*ELEMENTS_PER_VECT]
#define SP_C	&vec[4*ELEMENTS_PER_VECT]
#define SP_D	&vec[5*ELEMENTS_PER_VECT]

	/* Apply rotation to Hv, A,B,C,D */
	MAT4X3VEC( Hv, mat, SP_H );
	MAT4X3VEC( A, mat, SP_A );
	MAT4X3VEC( B, mat, SP_B );
	MAT4X3VEC( C, mat, SP_C );
	MAT4X3VEC( D, mat, SP_D );

	/* Validate that |H| > 0, compute |A| |B| |C| |D|		*/
	mag_h = sqrt( magsq_h = MAGSQ( Hv ) );
	mag_a = sqrt( magsq_a = MAGSQ( A ) );
	mag_b = sqrt( magsq_b = MAGSQ( B ) );
	mag_c = sqrt( magsq_c = MAGSQ( C ) );
	mag_d = sqrt( magsq_d = MAGSQ( D ) );
	if( NEAR_ZERO( magsq_h ) ) {
		fprintf(stderr,"tgc(%s):  zero length H vector\n", stp->st_name );
		return(1);		/* BAD */
	}

	/* Ascertain whether H lies in A-B plane 			*/
	VCROSS( work, A, B );
	f = VDOT( Hv, work )/ ( mag_a*mag_b*mag_h );
	if ( NEAR_ZERO(f) ) {
		fprintf(stderr,"tgc(%s):  H lies in A-B plane\n",stp->st_name);
		return(1);		/* BAD */
	}

	/* Validate that figure is not two-dimensional			*/
	if ( NEAR_ZERO( magsq_a ) && NEAR_ZERO( magsq_c ) ) {
		fprintf(stderr,"tgc(%s):  vectors A, C zero length\n", stp->st_name );
		return (1);
	}
	if ( NEAR_ZERO( magsq_b ) && NEAR_ZERO( magsq_d ) ) {
		fprintf(stderr,"tgc(%s):  vectors B, D zero length\n", stp->st_name );
		return (1);
	}

	/* Validate that A.B == 0, C.D == 0				*/
	f = VDOT( A, B ) / (mag_a * mag_b);
	if( ! NEAR_ZERO(f) ) {
		fprintf(stderr,"tgc(%s):  A not perpendicular to B\n",stp->st_name);
		return(1);		/* BAD */
	}
	f = VDOT( C, D ) / (mag_c * mag_d);
	if( ! NEAR_ZERO(f) ) {
		fprintf(stderr,"tgc(%s):  C not perpendicular to D\n",stp->st_name);
		return(1);		/* BAD */
	}

	/* Validate that  A || C  and  B || D, for parallel planes	*/
	f = 1.0 - VDOT( A, C ) / (mag_a * mag_c);
	if( ! NEAR_ZERO(f) ) {
		fprintf(stderr,"tgc(%s):  A not parallel to C\n",stp->st_name);
		return(1);		/* BAD */
	}
	f = 1.0 - VDOT( B, D ) / (mag_b * mag_d);
	if( ! NEAR_ZERO(f) ) {
		fprintf(stderr,"tgc(%s):  B not parallel to D\n",stp->st_name);
		return(1);		/* BAD */
	}

	/* solid is OK, compute constant terms, etc. */
	GETSTRUCT( tgc, tgc_specific );
	stp->st_specific = (int *)tgc;

	/* Apply full 4X4mat to V */
	{
		register fastf_t *p = SP_V;
		MAT4X3PNT( tgc->tgc_V, mat, p );
	}

	tgc->tgc_A = mag_a;
	tgc->tgc_B = mag_b;
	tgc->tgc_C = mag_c;
	tgc->tgc_D = mag_d;

	/*  If the eccentricities of the two ellipses are the same,
	 *  then the cone equation reduces to a much simpler quadratic
	 *  form.  Otherwise it is a (gah!) quartic equation.
	 */
	f = reldiff( (tgc->tgc_A*tgc->tgc_D), (tgc->tgc_C*tgc->tgc_B) );
	tgc->tgc_AD_CB = (f < EPSILON);		/* A*D == C*B */
	if ( tgc->tgc_AD_CB )  {
		if ( !NEAR_ZERO( tgc->tgc_B ) )  {
			tgc->tgc_ABsq = tgc->tgc_A/tgc->tgc_B;
			tgc->tgc_ABsq *= tgc->tgc_ABsq;		/* (A/B)**2 */
		} else if( !NEAR_ZERO( tgc->tgc_D ) )  {
			tgc->tgc_ABsq = tgc->tgc_C/tgc->tgc_D;
			tgc->tgc_ABsq *= tgc->tgc_ABsq;		/* (A/B)**2 */
		} else {
			fprintf(stderr,"tgc: (A/B)**2 is enormous\n");
			tgc->tgc_ABsq = VLARGE;
		}
	} else
		tgc->tgc_ABsq = 0;			/* safety */

	tgc_rotate( A, B, Hv, Rot, iRot, tgc );
	MAT4X3VEC( nH, Rot, Hv );
	tgc->tgc_sH = nH[Z];

	tgc->tgc_CA_H = tgc->tgc_C/tgc->tgc_A - 1.0;
	tgc->tgc_DB_H = tgc->tgc_D/tgc->tgc_B - 1.0;

	/*
	 *	Added iShr parameter to tgc_shear().
	 *	Added a matrix to tgc_specific for inverse transformation
	 *		of std. solid intersection points (tgc_inv_ScShR)
	 *		which includes a the inverse of a scaling transform.
	 *	Changed inverse transformation of normal vectors of std.
	 *		solid intersection to include shear inverse
	 *		(tgc_invRtShSc).
	 *	Fold in scaling transformation into the transformation to std.
	 *		space from target space (tgc_ScShR).
	 */
	tgc_shear( nH, Z, Shr, tShr, iShr );
	tgc_scale( tgc->tgc_A, tgc->tgc_B, tgc->tgc_sH, Scl, iScl );
	mat_mul( tmp, Shr, Rot );
	mat_mul( tgc->tgc_ScShR, Scl, tmp );

	mat_mul( tmp, tShr, Scl );
	mat_mul( tgc->tgc_invRtShSc, iRot, tmp );

	mat_mul( tmp, iShr, iScl );
	mat_mul( tgc->tgc_inv_ScShR, iRot, tmp );

	/* Compute bounding sphere and RPP */
	{
		LOCAL fastf_t dx, dy, dz;	/* For bounding sphere */
		LOCAL vect_t temp;

		/* There are 8 corners to the bounding RPP */
		/* This may not be minimal, but does fully contain the TGC */
		VADD2( temp, tgc->tgc_V, A );
		VADD2( work, temp, B );
		tgc_minmax( stp, work );	/* V + A + B */
		VSUB2( work, temp, B );
		tgc_minmax( stp, work );	/* V + A - B */

		VSUB2( temp, tgc->tgc_V, A );
		VADD2( work, temp, B );
		tgc_minmax( stp, work );	/* V - A + B */
		VSUB2( work, temp, B );
		tgc_minmax( stp, work );	/* V - A - B */

		VADD3( temp, tgc->tgc_V, Hv, C );
		VADD2( work, temp, D );
		tgc_minmax( stp, work );	/* V + H + C + D */
		VSUB2( work, temp, D );
		tgc_minmax( stp, work );	/* V + H + C - D */

		VADD2( temp, tgc->tgc_V, Hv );
		VSUB2( temp, temp, C );
		VADD2( work, temp, D );
		tgc_minmax( stp, work );	/* V + H - C + D */
		VSUB2( work, temp, D );
		tgc_minmax( stp, work );	/* V + H - C - D */

		VSET( stp->st_center,
			(stp->st_max[X] + stp->st_min[X])/2,
			(stp->st_max[Y] + stp->st_min[Y])/2,
			(stp->st_max[Z] + stp->st_min[Z])/2 );

		dx = (stp->st_max[X] - stp->st_min[X])/2;
		dy = (stp->st_max[Y] - stp->st_min[Y])/2;
		dz = (stp->st_max[Z] - stp->st_min[Z])/2;
		stp->st_radsq = dx*dx + dy*dy + dz*dz;
	}
	return (0);
}


/*
 *		>>>  r o t a t e ( )  <<<
 *
 *  To rotate vectors  A  and  B  ( where  A  is perpendicular to  B )
 *  to the X and Y axes respectively, create a rotation matrix
 *
 *	    | A' |
 *	R = | B' |
 *	    | C' |
 *
 *  where  A',  B'  and  C'  are vectors such that
 *
 *	A' = A/|A|	B' = B/|B|	C' = C/|C|
 *
 *  where    C = H - ( H.A' )A' - ( H.B' )B'
 *
 *  The last operation ( Gram Schmidt method ) finds the component
 *  of the vector  H  perpendicular  A  and to  B.  This is, therefore
 *  the normal for the planar sections of the truncated cone.
 */
static void
tgc_rotate( A, B, Hv, Rot, Inv, tgc )
vect_t		A, B, Hv;
mat_t		Rot, Inv;
struct tgc_specific	*tgc;
{
	LOCAL vect_t	uA, uB, uC;	/*  unit vectors		*/
	LOCAL fastf_t	mag_ha,		/*  magnitude of H in the	*/
			mag_hb;		/*    A and B directions	*/

	/* copy A and B, then 'unitize' the results			*/
	VMOVE( uA, A );
	VUNITIZE( uA );
	VMOVE( uB, B );
	VUNITIZE( uB );

	/*  Find component of H in the A direction			*/
	mag_ha = VDOT( Hv, uA );
	/*  Find component of H in the B direction			*/
	mag_hb = VDOT( Hv, uB );

	/*  Subtract the A and B components of H to find the component
	 *  perpendicular to both, then 'unitize' the result.
	 */
	VJOIN2( uC, Hv, -mag_ha, uA, -mag_hb, uB );
	VUNITIZE( uC );
	VMOVE( tgc->tgc_norm, uC );

	mat_idn( Rot );
	mat_idn( Inv );

	Rot[0] = Inv[0] = uA[X];
	Rot[1] = Inv[4] = uA[Y];
	Rot[2] = Inv[8] = uA[Z];

	Rot[4] = Inv[1] = uB[X];
	Rot[5] = Inv[5] = uB[Y];
	Rot[6] = Inv[9] = uB[Z];

	Rot[8]  = Inv[2]  = uC[X];
	Rot[9]  = Inv[6]  = uC[Y];
	Rot[10] = Inv[10] = uC[Z];
}

/*
 *		>>>  s h e a r ( )  <<<
 *
 *  To shear the H vector to the Z axis, every point must be shifted
 *  in the X direction by  -(Hx/Hz)*z , and in the Y direction by
 *  -(Hy/Hz)*z .  This operation makes the equation for the standard
 *  cone much easier to work with.
 *
 *  NOTE:  This computes the TRANSPOSE of the shear matrix rather than
 *  the inverse.
 *
 * Begin changes GSM, EOD -- Added INVERSE (Inv) calculation.
 */
static void
tgc_shear( vect, axis, Shr, Trn, Inv )
vect_t	vect;
int	axis;
mat_t	Shr, Trn, Inv;
{
	mat_idn( Shr );
	mat_idn( Trn );
	mat_idn( Inv );

	if ( axis == X ){
		Inv[4] = -1.0 * (Shr[4] = Trn[1] = -vect[Y]/vect[X]);
		Inv[8] = -1.0 * (Shr[8] = Trn[2] = -vect[Z]/vect[X]);
	} else if ( axis == Y ){
		Inv[1] = -1.0 * (Shr[1] = Trn[4] = Inv[1] = -vect[X]/vect[Y]);
		Inv[9] = -1.0 * (Shr[9] = Trn[6] = Inv[9] = -vect[Z]/vect[Y]);
	} else if ( axis == Z ){
		Inv[2] = -1.0 * (Shr[2] = Trn[8] = -vect[X]/vect[Z]);
		Inv[6] = -1.0 * (Shr[6] = Trn[9] = -vect[Y]/vect[Z]);
	}
}

/*	s c a l e ( )
 */
static void
tgc_scale( a, b, h, Scl, Inv )
fastf_t	a, b, h;
mat_t	Scl, Inv;
{
	mat_idn( Scl );
	mat_idn( Inv );
	Scl[0]  /= a;
	Scl[5]  /= b;
	Scl[10] /= h;
	Inv[0]  = a;
	Inv[5]  = b;
	Inv[10] = h;
	return;
}

tgc_print( stp )
register struct soltab	*stp;
{
	register struct tgc_specific	*tgc =
		(struct tgc_specific *)stp->st_specific;

	VPRINT( "V", tgc->tgc_V );
	fprintf(stderr, "mag sheared H = %f\n", tgc->tgc_sH );
	fprintf(stderr, "mag A = %f\n", tgc->tgc_A );
	fprintf(stderr, "mag B = %f\n", tgc->tgc_B );
	fprintf(stderr, "mag C = %f\n", tgc->tgc_C );
	fprintf(stderr, "mag D = %f\n", tgc->tgc_D );
	VPRINT( "Top normal", tgc->tgc_norm );

	mat_print( "Sc o Sh o R", tgc->tgc_ScShR );
	mat_print( "invR o trnSh o Sc", tgc->tgc_invRtShSc );

	if( tgc->tgc_AD_CB )  {
		fprintf(stderr, "A*D == C*B.  Equal eccentricities gives quadratic equation.\n");
		fprintf(stderr, "(A/B)**2 = %f\n", tgc->tgc_ABsq );
	} else {
		fprintf(stderr, "A*D != C*B.  Quatric equation.\n");
	}
	fprintf(stderr, "(C-A)/H = %f\n", tgc->tgc_CA_H );
	fprintf(stderr, "(D-B)/H = %f\n", tgc->tgc_DB_H );
}

/*
 *		>>>  t g c _ s h o t ( )  <<<
 *
 *  Intersect a ray with a truncated general cone, where all constant
 *  terms have been computed by tgc_prep().
 *
 *  NOTE:  All lines in this function are represented parametrically
 *  by a point,  P( Px, Py, Pz ) and a unit direction vector,
 *  D = iDx + jDy + kDz.  Any point on a line can be expressed
 *  by one variable 't', where
 *
 *        X = Dx*t + Px,
 *        Y = Dy*t + Py,
 *        Z = Dz*t + Pz.
 *
 *  First, convert the line to the coordinate system of a "stan-
 *  dard" cone.  This is a cone whose base lies in the X-Y plane,
 *  and whose H (now H') vector is lined up with the Z axis.  
 *
 *  Then find the equation of that line and the standard cone
 *  as an equation in 't'.  Solve the equation using a general
 *  polynomial root finder.  Use those values of 't' to compute
 *  the points of intersection in the original coordinate system.
 */
struct seg *
tgc_shot( stp, rp )
struct soltab		*stp;
register struct xray	*rp;
{
	register struct tgc_specific	*tgc =
		(struct tgc_specific *)stp->st_specific;
	LOCAL struct seg	*segp;
	LOCAL vect_t		pprime,
				dprime,
				norm,
				work;
	LOCAL vect_t		more_work;

	LOCAL fastf_t		k[4], pt[2],
				t, b, zval, dir,
				alf1, alf2;
	LOCAL int		npts, n, intersect;
	LOCAL vect_t		cor_pprime;	/* corrected P prime */
	LOCAL fastf_t		cor_proj;	/* corrected projected dist */
	LOCAL int		i;

	/* find rotated point and direction				*/
	MAT4X3VEC( dprime, tgc->tgc_ScShR, rp->r_dir );
	VUNITIZE( dprime );

	VSUB2( work, rp->r_pt, tgc->tgc_V );
	MAT4X3VEC( pprime, tgc->tgc_ScShR, work );

	/* Translating ray origin along direction of ray to closest
	 * pt. to origin of solids coordinate system, new ray origin
	 * is 'cor_pprime'.
	 */
	cor_proj = VDOT( pprime, dprime );
	VSCALE( cor_pprime, dprime, cor_proj );
	VSUB2( cor_pprime, pprime, cor_pprime );

	npts = stdCone( cor_pprime, dprime, tgc, k );

	/*
	 * Reverse above translation by adding distance to all 'k' values.
	 */
	for( i = 0; i < npts; ++i )
		k[i] -= cor_proj;

	if ( npts != 0 && npts != 2 && npts != 4 ){
		fprintf(stderr,"tgc(%s):  %d intersects != {0,2,4}\n",
			stp->st_name, npts );
		return( SEG_NULL );			/* No hit	*/
	}

	/* Most distant to least distant	*/
	tgc_sort( k, npts );

	/* Now, t[0] > t[npts-1].  See if this is an easy out. */
	if( k[0] <= 0.0 )
		return(SEG_NULL);		/* No hit out front. */

	/* General Cone may have 4 intersections, but	*
	 * Truncated Cone may only have 2.		*/

#define OUT		0
#define	IN		1

	/*		Truncation Procedure
	 *
	 *  Determine whether any of the intersections found are
	 *  between the planes truncating the cone.
	 */
	intersect = 0;
	for ( n=0; n < npts; ++n ){
		zval = k[n]*dprime[Z] + pprime[Z];
		/* Height vector is unitized (tgc->tgc_sH == 1.0) */
		if ( zval < 1.0 && zval > 0.0 ){
			/* Why keep going after intersect == 2 ??? */
			if ( ++intersect == 2 )
				pt[IN] = k[n];
			else
				pt[OUT] = k[n];
		}
	}

	if ( intersect == 2 ){
		/*  If two between-plane intersections exist, they are
		 *  the hit points for the ray.
		 */
		GET_SEG( segp );
		segp->seg_stp = stp;

		/* IN hit point back to target coord. system.		*/
		VJOIN1( work, pprime, pt[IN], dprime );
		MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
		VADD2( segp->seg_in.hit_point, more_work, tgc->tgc_V );

		tgcnormal( segp->seg_in.hit_normal, work, tgc );

		/* Compute IN hit distance.				*/
		VSUB2( work, segp->seg_in.hit_point, rp->r_pt );
		segp->seg_in.hit_dist = VDOT( work, rp->r_dir );

		/* OUT hit point back to target coord. system.		*/
		VJOIN1( work, pprime, pt[OUT], dprime );
		MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
		VADD2( segp->seg_out.hit_point, more_work, tgc->tgc_V );

		tgcnormal( segp->seg_out.hit_normal, work, tgc );

		/* Compute OUT hit distance.				*/
		VSUB2( work, segp->seg_out.hit_point, rp->r_pt );
		segp->seg_out.hit_dist = VDOT( work, rp->r_dir );

		return( segp );
	}
	if ( intersect == 1 )  {
		/*  If only one such intersection exists, the other
		 *  intersection must be on one of the planar surfaces.
		 *
		 *  Find which surface it lies on by calculating the 
		 *  X and Y values of the line as it intersects each
		 *  plane (in the standard coordinate system), and test
		 *  whether this lies within the governing ellipse.
		 */
		b = ( -pprime[Z] )/dprime[Z];
		/*  Height vector is unitized (tgc->tgc_sH == 1.0) */
		t = ( 1.0 - pprime[Z] )/dprime[Z];

		VJOIN1( work, pprime, b, dprime );
		/* A and B vectors are unitized (tgc->tgc_A == _B == 1.0) */
		/* alf1 = Alpha(work[0], work[1], 1.0, 1.0 ) */
		alf1 = work[0]*work[0] + work[1]*work[1];

		VJOIN1( work, pprime, t, dprime );
		/* Must scale C and D vectors */
		alf2 = Alpha(work[0],work[1],tgc->tgc_C/tgc->tgc_A,tgc->tgc_D/tgc->tgc_B);

		if ( alf1 <= 1.0 ){
			pt[IN] = b;
			VREVERSE( norm, tgc->tgc_norm );
		} else if ( alf2 <= 1.0 ){
			pt[IN] = t;
			VMOVE( norm, tgc->tgc_norm );
		} else {
			/* intersection apparently invalid  */
			fprintf(stderr,"tgc(%s):  only 1 intersect\n", stp->st_name);
			return( SEG_NULL );
		}

		GET_SEG( segp );
		segp->seg_stp = stp;
		if ( pt[OUT] == Max( pt[OUT], pt[IN] ) ){
			/* IN hit point back to target coord. system.	*/
			VJOIN1( work, pprime, pt[IN], dprime );
			MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
			VADD2( segp->seg_in.hit_point, more_work, tgc->tgc_V );

			VMOVE( segp->seg_in.hit_normal, norm );

			/* Compute IN hit distance.			*/
			VSUB2( work, segp->seg_in.hit_point, rp->r_pt );
			segp->seg_in.hit_dist = VDOT( work, rp->r_dir );

			/* OUT hit point back to target coord. system.		*/
			VJOIN1( work, pprime, pt[OUT], dprime );
			MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
			VADD2( segp->seg_out.hit_point, more_work, tgc->tgc_V );

			tgcnormal( segp->seg_out.hit_normal, work, tgc );

			/* Compute OUT hit distance.				*/
			VSUB2( work, segp->seg_out.hit_point, rp->r_pt );
			segp->seg_out.hit_dist = VDOT( work, rp->r_dir );
		} else {
			/* IN hit point back to target coord. system.	*/
			VJOIN1( work, pprime, pt[OUT], dprime );
			MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
			VADD2( segp->seg_in.hit_point, more_work, tgc->tgc_V );

			tgcnormal( segp->seg_in.hit_normal, work, tgc );

			/* Compute IN hit distance.			*/
			VSUB2( work, segp->seg_in.hit_point, rp->r_pt );
			segp->seg_in.hit_dist = VDOT( work, rp->r_dir );

			/* OUT hit point back to target coord. system.	*/
			VJOIN1( work, pprime, pt[IN], dprime );
			MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
			VADD2( segp->seg_out.hit_point, more_work, tgc->tgc_V );

			VMOVE( segp->seg_out.hit_normal, norm );

			/* Compute OUT hit distance.			*/
			VSUB2( work, segp->seg_out.hit_point, rp->r_pt );
			segp->seg_out.hit_dist = VDOT( work, rp->r_dir );
		}
		return( segp );
	}

	/*  If all conic interections lie outside the plane,
	 *  then check to see whether there are two planar
	 *  intersections inside the governing ellipses.
	 *
	 *  But first, if the direction is parallel (or nearly
	 *  so) to the planes, it (obviously) won't intersect
	 *  either of them.
	 */
	dir = VDOT( tgc->tgc_norm, rp->r_dir );	/* direc */
	if ( NEAR_ZERO( dir ) )
		return( SEG_NULL );

	b = ( -pprime[Z] )/dprime[Z];
	/* Height vector is unitized (tgc->tgc_sH == 1.0) */
	t = ( 1.0 - pprime[Z] )/dprime[Z];

	VJOIN1( work, pprime, b, dprime );
	/* A and B vectors are unitized (tgc->tgc_A == _B == 1.0) */
	/* alpf = Alpha(work[0], work[1], 1.0, 1.0 ) */
	alf1 = work[0]*work[0] + work[1]*work[1];

	VJOIN1( work, pprime, t, dprime );
	/* Must scale C and D vectors. */
	alf2 = Alpha(work[0],work[1],tgc->tgc_C/tgc->tgc_A,tgc->tgc_D/tgc->tgc_B);

	/*  It should not be possible for one planar intersection
	 *  to be outside its ellipse while the other is inside ...
	 *  but I wouldn't take any chances.
	 */
	if ( alf1 > 1.0 || alf2 > 1.0 )
		return( SEG_NULL );

	GET_SEG( segp );
	segp->seg_stp = stp;

	/*  Use the dot product (found earlier) of the plane
	 *  normal with the direction vector to determine the
	 *  orientation of the intersections.
	 */
	if ( dir > 0.0 ){
		/* IN hit point back to target coord. system.		*/
		VJOIN1( work, pprime, b, dprime );
		MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
		VADD2( segp->seg_in.hit_point, more_work, tgc->tgc_V );

		VREVERSE( segp->seg_in.hit_normal, tgc->tgc_norm );

		/* Compute IN hit distance.				*/
		VSUB2( work, segp->seg_in.hit_point, rp->r_pt );
		segp->seg_in.hit_dist = VDOT( work, rp->r_dir );

		/* OUT hit point back to target coord. system.		*/
		VJOIN1( work, pprime, t, dprime );
		MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
		VADD2( segp->seg_out.hit_point, more_work, tgc->tgc_V );

		VMOVE( segp->seg_out.hit_normal, tgc->tgc_norm );

		/* Compute OUT hit distance.				*/
		VSUB2( work, segp->seg_out.hit_point, rp->r_pt );
		segp->seg_out.hit_dist = VDOT( work, rp->r_dir );
	} else {
		/* IN hit point back to target coord. system.		*/
		VJOIN1( work, pprime, t, dprime );
		MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
		VADD2( segp->seg_in.hit_point, more_work, tgc->tgc_V );

		VMOVE( segp->seg_in.hit_normal, tgc->tgc_norm );

		/* Compute IN hit distance.				*/
		VSUB2( work, segp->seg_in.hit_point, rp->r_pt );
		segp->seg_in.hit_dist = VDOT( work, rp->r_dir );

		/* OUT hit point back to target coord. system.		*/
		VJOIN1( work, pprime, b, dprime );
		MAT4X3VEC( more_work, tgc->tgc_inv_ScShR, work );
		VADD2( segp->seg_out.hit_point, more_work, tgc->tgc_V );

		VREVERSE( segp->seg_out.hit_normal, tgc->tgc_norm );

		/* Compute OUT hit distance.				*/
		VSUB2( work, segp->seg_out.hit_point, rp->r_pt );
		segp->seg_out.hit_dist = VDOT( work, rp->r_dir );
	}
	return( segp );
}


/*
 *		>>>  s t d C o n e ( )  <<<
 *
 *  Given a line and the parameters for a standard cone, finds
 *  the roots of the equation for that cone and line.
 *  Returns the number of real roots found.
 * 
 *  Given a line and the cone parameters, finds the equation
 *  of the cone in terms of the variable 't'.
 *
 *  The equation for the cone is:
 *
 *      X**2 * Q**2  +  Y**2 * R**2  -  R**2 * Q**2 = 0
 *
 *  where	R = a + ((c - a)/|H'|)*Z 
 *		Q = b + ((d - b)/|H'|)*Z
 *
 *  First, find X, Y, and Z in terms of 't' for this line, then
 *  substitute them into the equation above.
 */
HIDDEN int
stdCone( pprime, dprime, tgc, t )
vect_t		pprime, dprime;
struct tgc_specific	*tgc;
double		t[];
{
	LOCAL poly	C;	/*  final equation	*/
	LOCAL poly	Xsqr, Ysqr;
	LOCAL poly	Q, Qsqr;
	LOCAL poly	R, Rsqr;
	LOCAL poly	T1, T2, T3, sum;
	LOCAL complex	val[MAXP];	/* roots of final equation */
	register int	i, l, npts;

	/*  Express each variable (X, Y, and Z) as a linear equation
	 *  in 't', eg, (dprime[X] * t) + pprime[X], and
	 *  substitute into the cone equation.
	 */
	Xsqr.dgr = 2;
	Xsqr.cf[0] = dprime[X] * dprime[X];
	Xsqr.cf[1] = 2.0 * dprime[X] * pprime[X];
	Xsqr.cf[2] = pprime[X] * pprime[X];

	Ysqr.dgr = 2;
	Ysqr.cf[0] = dprime[Y] * dprime[Y];
	Ysqr.cf[1] = 2.0 * dprime[Y] * pprime[Y];
	Ysqr.cf[2] = pprime[Y] * pprime[Y];

	R.dgr = 1;
	R.cf[0] = dprime[Z] * tgc->tgc_CA_H;
	/* A vector is unitized (tgc->tgc_A == 1.0) */
	R.cf[1] = (pprime[Z] * tgc->tgc_CA_H) + 1.0;
	(void) polyMul( &R, &R, &Rsqr );

	/*  If the eccentricities of the two ellipses are the same,
	 *  then the cone equation reduces to a much simpler quadratic
	 *  form.  Otherwise it is a (gah!) quartic equation.
	 */
	if ( tgc->tgc_AD_CB ){
		(void) polyScal( &Ysqr, tgc->tgc_ABsq );
		(void) polyAdd( &Xsqr, &Ysqr, &sum );
		(void) polySub( &sum, &Rsqr, &C );
	} else {
		Q.dgr = 1;
		Q.cf[0] = dprime[Z] * tgc->tgc_DB_H;
		/* B vector is unitized (tgc->tgc_B == 1.0) */
		Q.cf[1] = (pprime[Z] * tgc->tgc_DB_H) + 1.0;
		(void) polyMul( &Q, &Q, &Qsqr );

		(void) polyMul( &Qsqr, &Xsqr, &T1 );
		(void) polyMul( &Rsqr, &Ysqr, &T2 );
		(void) polyMul( &Rsqr, &Qsqr, &T3 );
		(void) polyAdd( &T1, &T2, &sum );
		(void) polySub( &sum, &T3, &C );
	}
	if( C.dgr != 0 && C.dgr != 2 && C.dgr != 4 )  {
		fprintf(stderr, "stdCone:  equation is degree %d!\n", C.dgr);
	}

	/*  Since the equation must be either 2nd or 4th order, the root
	 *  finder should return 2 or 4 roots.  Any other result indicates
	 *  a problem, so return an error value.
	 *
	 *  Oops! The equations can take the form  a = 0, which obviously
	 *  has no solution, so 0 is a valid result.
	 */
	npts = polyRoots( &C , val );

	/*  Only real roots indicate an intersection in real space.
	 *
	 *  Look at each root returned; if the imaginary part is zero
	 *  or sufficiently close, then use the real part as one value
	 *  of 't' for the intersections
	 */
	for ( l=0, i=0; l < npts; l++ ){
		if ( NEAR_ZERO( val[l].im ) )
			t[i++] = val[l].re;
	}
	/* Here, 'i' is number of points being returned */
	if ( i != 0 && i != 2 && i != 4 ){
		fprintf(stderr,"stdCone:  reduced %d to %d roots\n",npts,i);
		pr_roots( npts, val );
	}
	return i;
}


/*
 *		>>>  P t S o r t ( )  <<<
 *
 *  Sorts the values of 't' in descending order.  The sort is
 *  simplified to deal with only 2 or 4 values.  Returns the
 *  address of the first 't' in the array.
 */
static void
tgc_sort( t, npts )
register double	t[];

{
	LOCAL fastf_t	u;
	register int	n;

#define XCH(a,b)	{ u=a; a=b; b=u; }
	if ( npts == 2 ){
		if ( t[0] < t[1] ){
			XCH( t[0], t[1] );
		}
		return;
	}

	for ( n=0; n < 2; ++n ){
		if ( t[n] < t[n+2] ){
			XCH( t[n], t[n+2] );
		}
	}
	for ( n=0; n < 3; ++n ){
		if ( t[n] < t[n+1] ){
			XCH( t[n], t[n+1] );
		}
	}
}


/*
 *		>>>  t g c n o r m a l ( )  <<<
 *
 *  Compute the normal to the cone, given a point on the STANDARD
 *  CONE centered at the origin of the X-Y plane.
 *
 *  The gradient of the cone at that point is the normal vector
 *  in the standard space.  This vector will need to be rotated
 *  back to the coordinate system of the original cone in order
 *  to be useful.  Then the rotated vector must be 'unitized.'
 *
 *  NOTE:  The rotation required is NOT the inverse of the of the
 *	   rotation to the standard cone.  If you really want to 
 *	   know why, talk to Ed Davisson or to me (only if you're
 *	   truly desperate would I advise this).
 *
 *  The equation for the standard cone is:
 *
 *	   X**2 * Q**2  +  Y**2 * R**2  -  R**2 * Q**2 = 0
 *
 *  where,
 *		R = a + ((c - a)/|H'|)*Z 
 *		Q = b + ((d - b)/|H'|)*Z
 *
 *  Therefore, the gradient of f(x,y,z) = 0 is:
 *
 *	df/dx = 2 * x * Q**2
 *	df/dy = 2 * y * R**2
 *	df/dz = x**2 * 2 * Q * dQ/dz + y**2 * 2 * R * dR/dz +
 *		R**2 * 2 * Q * dQ/dz + 2 * R * dR/dz * Q**2
 *
 *  where,
 *		dQ/dz = (c - a)/|H'|
 *		dR/dz = (d - b)/|H'|
 */
HIDDEN void
tgcnormal( norm, hit, tgc )
register vectp_t		norm, hit;
register struct tgc_specific	*tgc;
{
	LOCAL fastf_t	Q, parQ, R, parR;
	LOCAL vect_t	stdnorm;

	R      = 1.0 + tgc->tgc_CA_H * hit[Z];
	parR   = 2.0 * R * tgc->tgc_CA_H;

	Q      = 1.0 + tgc->tgc_DB_H * hit[Z];
	parQ   = 2.0 * Q * tgc->tgc_DB_H;

	stdnorm[X] = 2.0 * hit[X] * Q * Q;
	stdnorm[Y] = 2.0 * hit[Y] * R * R;
	stdnorm[Z] =  parQ * hit[X] * hit[X] + parR * hit[Y] * hit[Y]
			- R * R * parQ - parR * Q * Q;

	MAT4X3VEC( norm, tgc->tgc_invRtShSc, stdnorm );
	VUNITIZE( norm );
}

#define MINMAX(a,b,c)	{if( (ftemp = (c)) < (a) )  a = ftemp;\
			if( ftemp > (b) )  b = ftemp; }

static void
tgc_minmax(stp, v)
register struct soltab *stp;
vectp_t v;
{
	FAST fastf_t ftemp;

	MINMAX( stp->st_min[X], stp->st_max[X], v[X] );
	MINMAX( stp->st_min[Y], stp->st_max[Y], v[Y] );
	MINMAX( stp->st_min[Z], stp->st_max[Z], v[Z] );
}
