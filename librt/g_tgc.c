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
 *	Mike Muuss		(Optimization)
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
static char RCStgc[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"
#include "./complex.h"
#include "./polyno.h"

static void	tgc_rotate(), tgc_shear();
static void	tgc_scale();
void rt_pt_sort();

struct  tgc_specific {
	vect_t	tgc_V;		/*  Vector to center of base of TGC	*/
	fastf_t	tgc_sH;		/*  magnitude of sheared H vector	*/
	fastf_t	tgc_A;		/*  magnitude of A vector		*/
	fastf_t	tgc_B;		/*  magnitude of B vector		*/
	fastf_t	tgc_C;		/*  magnitude of C vector		*/
	fastf_t	tgc_D;		/*  magnitude of D vector		*/
	fastf_t	tgc_CA_H;	/*  (C-A)/H  X_of_Z */
	fastf_t tgc_DB_H;	/*  (D-B)/H  Y_of_Z */
	fastf_t	tgc_AAdCC;	/*  (|A|**2)/(|C|**2) */
	fastf_t	tgc_BBdDD;	/*  (|B|**2)/(|D|**2) */
	vect_t	tgc_N;		/*  normal at 'top' of cone		*/
	mat_t	tgc_ScShR;	/*  Scale( Shear( Rot( vect )))		*/
	mat_t	tgc_invRtShSc;	/*  invRot( trnShear( Scale( vect )))	*/
	char	tgc_AD_CB;	/*  boolean:  A*D == C*B  */
};


#define VLARGE		1000000.0
#define	ALPHA(x,y,c,d)	( (x)*(x)*(c) + (y)*(y)*(d) )


/*
 *			T G C _ P R E P
 *
 *  Given the parameters (in vector form) of a truncated general cone,
 *  compute the constant terms and a transformation matrix needed for
 *  solving the intersection of a ray with the cone.
 *
 *  Also compute the return transformation for normals in the transformed
 *  space to the original space.  This NOT the inverse of the transformation
 *  matrix (if you really want to know why, talk to Ed Davisson).
 */
tgc_prep( vec, stp, mat, rtip )
register fastf_t	*vec;
struct soltab		*stp;
matp_t			mat;
struct rt_i		*rtip;
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

	/*
	 *  For a fast way out, hand this solid off to the REC routine.
	 *  If it takes it, then there is nothing to do, otherwise
	 *  the solid is a TGC.
	 */
	if( rec_prep( vec, stp, mat ) == 0 )
		return(0);		/* OK */

#define TGC_V	&vec[0*ELEMENTS_PER_VECT]
#define TGC_H	&vec[1*ELEMENTS_PER_VECT]
#define TGC_A	&vec[2*ELEMENTS_PER_VECT]
#define TGC_B	&vec[3*ELEMENTS_PER_VECT]
#define TGC_C	&vec[4*ELEMENTS_PER_VECT]
#define TGC_D	&vec[5*ELEMENTS_PER_VECT]

	/* Apply rotation to Hv, A,B,C,D */
	MAT4X3VEC( Hv, mat, TGC_H );
	MAT4X3VEC( A, mat, TGC_A );
	MAT4X3VEC( B, mat, TGC_B );
	MAT4X3VEC( C, mat, TGC_C );
	MAT4X3VEC( D, mat, TGC_D );

	/* Validate that |H| > 0, compute |A| |B| |C| |D|		*/
	mag_h = sqrt( magsq_h = MAGSQ( Hv ) );
	mag_a = sqrt( magsq_a = MAGSQ( A ) );
	mag_b = sqrt( magsq_b = MAGSQ( B ) );
	mag_c = sqrt( magsq_c = MAGSQ( C ) );
	mag_d = sqrt( magsq_d = MAGSQ( D ) );

	if( NEAR_ZERO( magsq_h, 0.0001 ) ) {
		rt_log("tgc(%s):  zero length H vector\n", stp->st_name );
		return(1);		/* BAD */
	}

	/* Ascertain whether H lies in A-B plane 			*/
	VCROSS( work, A, B );
	f = VDOT( Hv, work )/ ( mag_a*mag_b*mag_h );
	if ( NEAR_ZERO(f, 0.0001) ) {
		rt_log("tgc(%s):  H lies in A-B plane\n",stp->st_name);
		return(1);		/* BAD */
	}

	/* Validate that figure is not two-dimensional			*/
	if ( NEAR_ZERO( magsq_a, 0.0001 ) && NEAR_ZERO( magsq_c, 0.0001 ) ) {
		rt_log("tgc(%s):  vectors A, C zero length\n", stp->st_name );
		return (1);
	}
	if ( NEAR_ZERO( magsq_b, 0.0001 ) && NEAR_ZERO( magsq_d, 0.0001 ) ) {
		rt_log("tgc(%s):  vectors B, D zero length\n", stp->st_name );
		return (1);
	}

	/* Validate that both ends are not degenerate */
	if( mag_a * mag_b <= 0.001 && mag_c*mag_d <= 0.001 )  {
		rt_log("tgc(%s):  Both ends degenerate\n", stp->st_name);
		return(1);		/* BAD */
	}

	if( mag_a * mag_b > 0.001 )  {
		/* Validate that A.B == 0 */
		f = VDOT( A, B ) / (mag_a * mag_b);
		if( ! NEAR_ZERO(f, 0.0001) ) {
			rt_log("tgc(%s):  A not perpendicular to B\n",stp->st_name);
			return(1);		/* BAD */
		}
	}
	if( mag_c * mag_d > 0.001 )  {
		/* Validate that C.D == 0 */
		f = VDOT( C, D ) / (mag_c * mag_d);
		if( ! NEAR_ZERO(f, 0.0001) ) {
			rt_log("tgc(%s):  C not perpendicular to D\n",stp->st_name);
			return(1);		/* BAD */
		}
	}

	if( mag_a * mag_c > 0.001 )  {
		/* Validate that  A || C */
		f = 1.0 - VDOT( A, C ) / (mag_a * mag_c);
		if( ! NEAR_ZERO(f, 0.0001) ) {
			rt_log("tgc(%s):  A not parallel to C\n",stp->st_name);
			return(1);		/* BAD */
		}
	}

	if( mag_b * mag_d > 0.001 )  {
		/* Validate that  B || D, for parallel planes	*/
		f = 1.0 - VDOT( B, D ) / (mag_b * mag_d);
		if( ! NEAR_ZERO(f, 0.0001) ) {
			rt_log("tgc(%s):  B not parallel to D\n",stp->st_name);
			return(1);		/* BAD */
		}
	}

	/* solid is OK, compute constant terms, etc. */
	GETSTRUCT( tgc, tgc_specific );
	stp->st_specific = (int *)tgc;

	/* Apply full 4X4mat to V */
	{
		register fastf_t *p = TGC_V;
		MAT4X3PNT( tgc->tgc_V, mat, p );
	}

	tgc->tgc_A = mag_a;
	tgc->tgc_B = mag_b;
	tgc->tgc_C = mag_c;
	tgc->tgc_D = mag_d;

	/* Part of computing ALPHA() */
	if( NEAR_ZERO(magsq_c, 0.0001) )
		tgc->tgc_AAdCC = VLARGE;
	else
		tgc->tgc_AAdCC = magsq_a / magsq_c;
	if( NEAR_ZERO(magsq_d, 0.0001) )
		tgc->tgc_BBdDD = VLARGE;
	else
		tgc->tgc_BBdDD = magsq_b / magsq_d;

	/*  If the eccentricities of the two ellipses are the same,
	 *  then the cone equation reduces to a much simpler quadratic
	 *  form.  Otherwise it is a (gah!) quartic equation.
	 */
	f = rt_reldiff( (tgc->tgc_A*tgc->tgc_D), (tgc->tgc_C*tgc->tgc_B) );
	tgc->tgc_AD_CB = (f < 0.0001);		/* A*D == C*B */
	tgc_rotate( A, B, Hv, Rot, iRot, tgc );
	MAT4X3VEC( nH, Rot, Hv );
	tgc->tgc_sH = nH[Z];

	tgc->tgc_CA_H = tgc->tgc_C/tgc->tgc_A - 1.0;
	tgc->tgc_DB_H = tgc->tgc_D/tgc->tgc_B - 1.0;
	if( NEAR_ZERO( tgc->tgc_CA_H, 0.0001 ) )
		tgc->tgc_CA_H = 0.0;
	if( NEAR_ZERO( tgc->tgc_DB_H, 0.0001 ) )
		tgc->tgc_DB_H = 0.0;

	/*
	 *	Added iShr parameter to tgc_shear().
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

	/* Compute bounding sphere and RPP */
	{
		LOCAL fastf_t dx, dy, dz;	/* For bounding sphere */
		LOCAL vect_t temp;

		/* There are 8 corners to the bounding RPP */
		/* This may not be minimal, but does fully contain the TGC */
		VADD2( temp, tgc->tgc_V, A );
		VADD2( work, temp, B );
#define TGC_MM(v)	VMINMAX( stp->st_min, stp->st_max, v );
		TGC_MM( work );	/* V + A + B */
		VSUB2( work, temp, B );
		TGC_MM( work );	/* V + A - B */

		VSUB2( temp, tgc->tgc_V, A );
		VADD2( work, temp, B );
		TGC_MM( work );	/* V - A + B */
		VSUB2( work, temp, B );
		TGC_MM( work );	/* V - A - B */

		VADD3( temp, tgc->tgc_V, Hv, C );
		VADD2( work, temp, D );
		TGC_MM( work );	/* V + H + C + D */
		VSUB2( work, temp, D );
		TGC_MM( work );	/* V + H + C - D */

		VADD2( temp, tgc->tgc_V, Hv );
		VSUB2( temp, temp, C );
		VADD2( work, temp, D );
		TGC_MM( work );	/* V + H - C + D */
		VSUB2( work, temp, D );
		TGC_MM( work );	/* V + H - C - D */

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
	VMOVE( tgc->tgc_N, uC );

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

/*
 *  			T G C _ P R I N T
 */
tgc_print( stp )
register struct soltab	*stp;
{
	register struct tgc_specific	*tgc =
		(struct tgc_specific *)stp->st_specific;

	VPRINT( "V", tgc->tgc_V );
	rt_log( "mag sheared H = %f\n", tgc->tgc_sH );
	rt_log( "mag A = %f\n", tgc->tgc_A );
	rt_log( "mag B = %f\n", tgc->tgc_B );
	rt_log( "mag C = %f\n", tgc->tgc_C );
	rt_log( "mag D = %f\n", tgc->tgc_D );
	VPRINT( "Top normal", tgc->tgc_N );

	mat_print( "Sc o Sh o R", tgc->tgc_ScShR );
	mat_print( "invR o trnSh o Sc", tgc->tgc_invRtShSc );

	if( tgc->tgc_AD_CB )  {
		rt_log( "A*D == C*B.  Equal eccentricities gives quadratic equation.\n");
	} else {
		rt_log( "A*D != C*B.  Quartic equation.\n");
	}
	rt_log( "(C-A)/H = %f\n", tgc->tgc_CA_H );
	rt_log( "(D-B)/H = %f\n", tgc->tgc_DB_H );
	rt_log( "(|A|**2)/(|C|**2) = %f\n", tgc->tgc_AAdCC );
	rt_log( "(|B|**2)/(|D|**2) = %f\n", tgc->tgc_BBdDD );
}

/* To be clean, hit_private (a char *), is set to one of these */
static char tgc_compute[4];

/*
 *			T G C _ S H O T
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
tgc_shot( stp, rp, ap )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
{
	register struct tgc_specific	*tgc =
		(struct tgc_specific *)stp->st_specific;
	register struct seg	*segp;
	LOCAL vect_t		pprime;
	LOCAL vect_t		dprime;
	LOCAL vect_t		work;
	LOCAL fastf_t		k[4], pt[2];
	LOCAL fastf_t		t, b, zval, dir;
	LOCAL fastf_t		t_scale, alf1, alf2;
	LOCAL int		npts;
	LOCAL int		intersect;
	LOCAL vect_t		cor_pprime;	/* corrected P prime */
	LOCAL fastf_t		cor_proj;	/* corrected projected dist */
	LOCAL int		i;
	LOCAL poly		C;	/*  final equation	*/
	LOCAL poly		Xsqr, Ysqr;
	LOCAL poly		R, Rsqr;
	LOCAL poly		sum;

	/* find rotated point and direction */
	MAT4X3VEC( dprime, tgc->tgc_ScShR, rp->r_dir );

	/*
	 *  A vector of unit length in model space (r_dir) changes length in
	 *  the special unit-tgc space.  This scale factor will restore
	 *  proper length after hit points are found.
	 */
	t_scale = 1/MAGNITUDE( dprime );
	VSCALE( dprime, dprime, t_scale );	/* VUNITIZE( dprime ); */

	if( NEAR_ZERO( dprime[Z], 0.0001 ) )
		dprime[Z] = 0.0;	/* prevent rootfinder heartburn */

	VSUB2( work, rp->r_pt, tgc->tgc_V );
	MAT4X3VEC( pprime, tgc->tgc_ScShR, work );

	/* Translating ray origin along direction of ray to closest
	 * pt. to origin of solids coordinate system, new ray origin
	 * is 'cor_pprime'.
	 */
	cor_proj = VDOT( pprime, dprime );
	VSCALE( cor_pprime, dprime, cor_proj );
	VSUB2( cor_pprime, pprime, cor_pprime );

	/*
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
	 *
	 *  Express each variable (X, Y, and Z) as a linear equation
	 *  in 'k', eg, (dprime[X] * k) + cor_pprime[X], and
	 *  substitute into the cone equation.
	 */
	Xsqr.dgr = 2;
	Xsqr.cf[0] = dprime[X] * dprime[X];
	Xsqr.cf[1] = 2.0 * dprime[X] * cor_pprime[X];
	Xsqr.cf[2] = cor_pprime[X] * cor_pprime[X];

	Ysqr.dgr = 2;
	Ysqr.cf[0] = dprime[Y] * dprime[Y];
	Ysqr.cf[1] = 2.0 * dprime[Y] * cor_pprime[Y];
	Ysqr.cf[2] = cor_pprime[Y] * cor_pprime[Y];

	R.dgr = 1;
	R.cf[0] = dprime[Z] * tgc->tgc_CA_H;
	/* A vector is unitized (tgc->tgc_A == 1.0) */
	R.cf[1] = (cor_pprime[Z] * tgc->tgc_CA_H) + 1.0;
	(void) polyMul( &R, &R, &Rsqr );

	/*
	 *  If the eccentricities of the two ellipses are the same,
	 *  then the cone equation reduces to a much simpler quadratic
	 *  form.  Otherwise it is a (gah!) quartic equation.
	 */
	if ( tgc->tgc_AD_CB ){
		FAST fastf_t roots;

		(void) polyAdd( &Xsqr, &Ysqr, &sum );
		(void) polySub( &sum, &Rsqr, &C );

		/* Find the real roots the easy way.  C.dgr==2 */
		if( (roots = C.cf[1]*C.cf[1] - 4 * C.cf[0] * C.cf[2]) < 0 ) {
			npts = 0;	/* no real roots */
		} else {
			roots = sqrt(roots);
			k[0] = (roots - C.cf[1]) * 0.5 / C.cf[0];
			k[1] = (roots + C.cf[1]) * (-0.5) / C.cf[0];
			npts = 2;
		}
	} else {
		LOCAL poly	Q, Qsqr;
		LOCAL poly	T1, T2, T3;
		LOCAL complex	val[MAXP];	/* roots of final equation */
		register int	l;
		register int nroots;

		Q.dgr = 1;
		Q.cf[0] = dprime[Z] * tgc->tgc_DB_H;
		/* B vector is unitized (tgc->tgc_B == 1.0) */
		Q.cf[1] = (cor_pprime[Z] * tgc->tgc_DB_H) + 1.0;
		(void) polyMul( &Q, &Q, &Qsqr );

		(void) polyMul( &Qsqr, &Xsqr, &T1 );
		(void) polyMul( &Rsqr, &Ysqr, &T2 );
		(void) polyMul( &Rsqr, &Qsqr, &T3 );
		(void) polyAdd( &T1, &T2, &sum );
		(void) polySub( &sum, &T3, &C );

		/*  The equation is 4th order, so we expect 0 to 4 roots */
		nroots = polyRoots( &C , val );

		/*  Only real roots indicate an intersection in real space.
		 *
		 *  Look at each root returned; if the imaginary part is zero
		 *  or sufficiently close, then use the real part as one value
		 *  of 't' for the intersections
		 */
		for ( l=0, npts=0; l < nroots; l++ ){
			if ( NEAR_ZERO( val[l].im, 0.0001 ) )
				k[npts++] = val[l].re;
		}
		/* Here, 'npts' is number of points being returned */
		if ( npts != 0 && npts != 2 && npts != 4 ){
			rt_log("tgc:  reduced %d to %d roots\n",nroots,npts);
			rt_pr_roots( nroots, val );
		}
	}

	/*
	 * Reverse above translation by adding distance to all 'k' values.
	 */
	for( i = 0; i < npts; ++i )
		k[i] -= cor_proj;

	if ( npts != 0 && npts != 2 && npts != 4 ){
		rt_log("tgc(%s):  %d intersects != {0,2,4}\n",
			stp->st_name, npts );
		return( SEG_NULL );			/* No hit	*/
	}

	/* Most distant to least distant	*/
	rt_pt_sort( k, npts );

	/* Now, k[0] > k[npts-1] */

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
	for ( i=0; i < npts; i++ ){
		zval = k[i]*dprime[Z] + pprime[Z];
		/* Height vector is unitized (tgc->tgc_sH == 1.0) */
		if ( zval < 1.0 && zval > 0.0 ){
			if ( ++intersect == 2 )  {
				pt[IN] = k[i];
			}  else
				pt[OUT] = k[i];
		}
	}
	if ( intersect == 2 ){
		/*  If two between-plane intersections exist, they are
		 *  the hit points for the ray.
		 */
		GET_SEG( segp, ap->a_resource );
		segp->seg_stp = stp;

		segp->seg_in.hit_dist = pt[IN] * t_scale;
		segp->seg_in.hit_private = &tgc_compute[0];	/* compute N */
		VJOIN1( segp->seg_in.hit_vpriv, pprime, pt[IN], dprime );

		segp->seg_out.hit_dist = pt[OUT] * t_scale;
		segp->seg_out.hit_private = &tgc_compute[0];	/* compute N */
		VJOIN1( segp->seg_out.hit_vpriv, pprime, pt[OUT], dprime );

		return( segp );
	}
	if ( intersect == 1 )  {
		char *nflag;	/* tgc_compute[1] = normal, [2] = reverse normal */
		/*
		 *  If only one between-plane intersection exists (pt[OUT]),
		 *  then the other intersection must be on
		 *  one of the planar surfaces (pt[IN]).
		 *
		 *  Find which surface it lies on by calculating the 
		 *  X and Y values of the line as it intersects each
		 *  plane (in the standard coordinate system), and test
		 *  whether this lies within the governing ellipse.
		 */
		if( dprime[Z] == 0.0 )  {
			rt_log("tgc: dprime[Z] = 0!\n" );
			return(SEG_NULL);
		}
		b = ( -pprime[Z] )/dprime[Z];
		/*  Height vector is unitized (tgc->tgc_sH == 1.0) */
		t = ( 1.0 - pprime[Z] )/dprime[Z];

		VJOIN1( work, pprime, b, dprime );
		/* A and B vectors are unitized (tgc->tgc_A == _B == 1.0) */
		/* alf1 = ALPHA(work[X], work[Y], 1.0, 1.0 ) */
		alf1 = work[X]*work[X] + work[Y]*work[Y];

		VJOIN1( work, pprime, t, dprime );
		/* Must scale C and D vectors */
		alf2 = ALPHA(work[X], work[Y], tgc->tgc_AAdCC,tgc->tgc_BBdDD);

		if ( alf1 <= 1.0 ){
			pt[IN] = b;
			nflag = &tgc_compute[2]; /* copy reverse normal */
		} else if ( alf2 <= 1.0 ){
			pt[IN] = t;
			nflag = &tgc_compute[1];	/* copy normal */
		} else {
			/* intersection apparently invalid  */
			rt_log("tgc(%s):  only 1 intersect\n", stp->st_name);
			return( SEG_NULL );
		}

		GET_SEG( segp, ap->a_resource );
		segp->seg_stp = stp;
		/* pt[OUT] on skin, pt[IN] on end */
		if ( pt[OUT] >= pt[IN] )  {
			segp->seg_in.hit_dist = pt[IN] * t_scale;
			segp->seg_in.hit_private = nflag;

			segp->seg_out.hit_dist = pt[OUT] * t_scale;
			segp->seg_out.hit_private = &tgc_compute[0];	/* compute N */
			/* transform-space vector needed for normal */
			VJOIN1( segp->seg_out.hit_vpriv, pprime, pt[OUT], dprime );
		} else {
			segp->seg_in.hit_dist = pt[OUT] * t_scale;
			/* transform-space vector needed for normal */
			segp->seg_in.hit_private = &tgc_compute[0];	/* compute N */
			VJOIN1( segp->seg_in.hit_vpriv, pprime, pt[OUT], dprime );

			segp->seg_out.hit_dist = pt[IN] * t_scale;
			segp->seg_out.hit_private = nflag;
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
	if( dprime[Z] == 0.0 )
		return(SEG_NULL);

	dir = VDOT( tgc->tgc_N, rp->r_dir );	/* direc */
	if ( NEAR_ZERO( dir, 0.0001 ) )
		return( SEG_NULL );

	b = ( -pprime[Z] )/dprime[Z];
	/* Height vector is unitized (tgc->tgc_sH == 1.0) */
	t = ( 1.0 - pprime[Z] )/dprime[Z];

	VJOIN1( work, pprime, b, dprime );
	/* A and B vectors are unitized (tgc->tgc_A == _B == 1.0) */
	/* alpf = ALPHA(work[0], work[1], 1.0, 1.0 ) */
	alf1 = work[X]*work[X] + work[Y]*work[Y];

	VJOIN1( work, pprime, t, dprime );
	/* Must scale C and D vectors. */
	alf2 = ALPHA(work[X], work[Y], tgc->tgc_AAdCC,tgc->tgc_BBdDD);

	/*  It should not be possible for one planar intersection
	 *  to be outside its ellipse while the other is inside ...
	 *  but I wouldn't take any chances.
	 */
	if ( alf1 > 1.0 || alf2 > 1.0 )
		return( SEG_NULL );

	GET_SEG( segp, ap->a_resource );
	segp->seg_stp = stp;

	/*  Use the dot product (found earlier) of the plane
	 *  normal with the direction vector to determine the
	 *  orientation of the intersections.
	 */
	if ( dir > 0.0 ){
		segp->seg_in.hit_dist = b * t_scale;
		segp->seg_in.hit_private = &tgc_compute[2];	/* reverse normal */

		segp->seg_out.hit_dist = t * t_scale;
		segp->seg_out.hit_private = &tgc_compute[1];	/* normal */
	} else {
		segp->seg_in.hit_dist = t * t_scale;
		segp->seg_in.hit_private = &tgc_compute[1];	/* normal */

		segp->seg_out.hit_dist = b * t_scale;
		segp->seg_out.hit_private = &tgc_compute[2];	/* reverse normal */
	}
	return( segp );
}


/*
 *			R T _ P T _ S O R T
 *
 *  Sorts the values in t[] in descending order.
 */
void
rt_pt_sort( t, npts )
register fastf_t t[];
{
	FAST fastf_t	u;
	register short	lim, n;

	for( lim = npts-1; lim > 0; lim-- )  {
		for( n = 0; n < lim; n++ )  {
			if( (u=t[n]) < t[n+1] )  {
				/* bubble larger towards [0] */
				t[n] = t[n+1];
				t[n+1] = u;
			}
		}
	}
}


/*
 *			T G C _ N O R M
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
tgc_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct tgc_specific	*tgc =
		(struct tgc_specific *)stp->st_specific;
	FAST fastf_t	Q;
	FAST fastf_t	R;
	LOCAL vect_t	stdnorm;

	/* Hit point */
	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );

	/* Hits on the end plates are easy */
	switch( hitp->hit_private-tgc_compute )  {
	case 1:
		VMOVE( hitp->hit_normal, tgc->tgc_N );
		break;
	case 2:
		VREVERSE( hitp->hit_normal, tgc->tgc_N );
		break;
	case 0:
		/* Compute normal, given hit point on standard (unit) cone */
		R = 1 + tgc->tgc_CA_H * hitp->hit_vpriv[Z];
		Q = 1 + tgc->tgc_DB_H * hitp->hit_vpriv[Z];
		stdnorm[X] = 2 * hitp->hit_vpriv[X] * Q * Q;
		stdnorm[Y] = 2 * hitp->hit_vpriv[Y] * R * R;
		stdnorm[Z] = 2 * ( Q * tgc->tgc_DB_H *
				(hitp->hit_vpriv[X]*hitp->hit_vpriv[X] - R*R)
			     + R * tgc->tgc_CA_H *
				(hitp->hit_vpriv[Y]*hitp->hit_vpriv[Y] - Q*Q) );
		MAT4X3VEC( hitp->hit_normal, tgc->tgc_invRtShSc, stdnorm );
		VUNITIZE( hitp->hit_normal );
		break;
	default:
		rt_log("tgc_norm: bad flag x%x\n", (int)hitp->hit_private);
		break;
	}
}

tgc_uv()
{
}

/*
 *			T G C _ F R E E
 */
tgc_free( stp )
struct soltab *stp;
{
	register struct tgc_specific	*tgc =
		(struct tgc_specific *)stp->st_specific;

	rt_free( (char *)tgc, "tgc_specific");
}

tgc_class()
{
}

tgc_plot()
{
}

tgc_curve( cvp, hitp, stp, rp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
struct xray *rp;
{
	static int count = 0;
	register struct tgc_specific *tgc =
		(struct tgc_specific *)stp->st_specific;

	if( count++ == 0 )
		rt_log("WARNING: tgc_curve is not fully implemented\n");

	switch( hitp->hit_private-tgc_compute )  {
	case 0:
		rt_orthovec( cvp->crv_pdir, hitp->hit_normal );	/* XXX - random guess */
		cvp->crv_c1 = 0;
		cvp->crv_c2 = 0;			/* XXX - to do */

		if( VDOT( hitp->hit_normal, rp->r_dir ) > 0 )  {
			/* ray strikes surface from inside; make curv negative */
			cvp->crv_c2 = - cvp->crv_c2;
		}
		break;
	case 1:
	case 2:
		rt_orthovec( cvp->crv_pdir, hitp->hit_normal );
		cvp->crv_c1 = cvp->crv_c2 = 0;
		break;
	default:
		rt_log("tgc_curve: bad flag x%x\n", (int)hitp->hit_private);
		break;
	}
}
