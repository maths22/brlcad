/*
 *			M A T . C
 *
 * 4 x 4 Matrix manipulation functions..............
 *
 *	mat_atan2()			Wrapper for library atan2()
 *	mat_zero( &m )			Fill matrix m with zeros
 *	mat_idn( &m )			Fill matrix m with identity matrix
 *	mat_copy( &o, &i )		Copy matrix i to matrix o
 *	mat_mul( &o, &i1, &i2 )		Multiply i1 by i2 and store in o
 *	matXvec( &ov, &m, &iv )		Multiply m by vector iv, store in ov
 *	mat_inv( &om, &im )		Invert matrix im, store result in om
 *	mat_print( &title, &m )		Print matrix (with title) on stdout.
 *	mat_trn( &o, &i )		Transpose matrix i into matrix o
 *	mat_ae( &o, azimuth, elev)	Make rot matrix from azimuth+elevation
 *	mat_angles( &o, alpha, beta, gama )	Make rot matrix from angles
 *	eigen2x2()			Eigen values and vectors
 *	mat_lookat			Make rot mat:  xform from D to -Z
 *	mat_fromto			Make rot mat:  xform from A to B
 *
 *
 * Matrix array elements have the following positions in the matrix:
 *
 *				|  0  1  2  3 |		| 0 |
 *	  [ 0 1 2 3 ]		|  4  5  6  7 |		| 1 |
 *				|  8  9 10 11 |		| 2 |
 *				| 12 13 14 15 |		| 3 |
 *
 *
 *     preVector (vect_t)	 Matrix (mat_t)    postVector (vect_t)
 *
 *  Authors -
 *	Robert S. Miles
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
static char RCSmat[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"

double mat_degtorad = 0.0174532925199433;


/*
 *			M A T _ A T A N 2
 *
 *  A wrapper for the system atan2().  On the Silicon Graphics,
 *  and perhaps on others, x==0 incorrectly returns infinity.
 */
double
mat_atan2(y,x)
double	y,x;
{
	if( x > -1.0e-20 && x < 1.0e-20 )  {
		/* X is equal to zero, check Y */
		if( y < -1.0e-20 )  return( -3.14159265358979323/2 );
		if( y >  1.0e-20 )  return(  3.14159265358979323/2 );
		return(0.0);
	}
	return( atan2( y, x ) );
}

/*
 *			M A T _ Z E R O
 *
 * Fill in the matrix "m" with zeros.
 */
void
mat_zero( m )
register matp_t m;
{
	register int i = 0;

	/* Clear everything */
	for(; i<16; i++)
		*m++ = 0.0;
}


/*
 *			M A T _ I D N
 *
 * Fill in the matrix "m" with an identity matrix.
 */
void
mat_idn( m )
register matp_t m;
{
	/* Clear everything first */
	mat_zero( m );

	/* Set ones in the diagonal */
	m[0] = m[5] = m[10] = m[15] = 1.0;
}


/*
 *			M A T _ C O P Y
 *
 * Copy the matrix "im" into the matrix "om".
 */
void
mat_copy( om, im )
register matp_t om;
register matp_t im;
{
	register int i = 0;

	/* Copy all elements */
	for(; i<16; i++)
		*om++ = *im++;
}


/*
 *			M A T _ M U L
 *
 * Multiply matrix "a" by "b" and store the result in "o".
 * NOTE:  This is different from multiplying "b" by "a"
 * (most of the time!)
 * NOTE: "o" must not be the same as either of the inputs.
 */
void
mat_mul( o, a, b )
register matp_t o;
register matp_t a;
register matp_t b;
{
	o[ 0] = a[ 0]*b[ 0] + a[ 1]*b[ 4] + a[ 2]*b[ 8] + a[ 3]*b[12];
	o[ 1] = a[ 0]*b[ 1] + a[ 1]*b[ 5] + a[ 2]*b[ 9] + a[ 3]*b[13];
	o[ 2] = a[ 0]*b[ 2] + a[ 1]*b[ 6] + a[ 2]*b[10] + a[ 3]*b[14];
	o[ 3] = a[ 0]*b[ 3] + a[ 1]*b[ 7] + a[ 2]*b[11] + a[ 3]*b[15];

	o[ 4] = a[ 4]*b[ 0] + a[ 5]*b[ 4] + a[ 6]*b[ 8] + a[ 7]*b[12];
	o[ 5] = a[ 4]*b[ 1] + a[ 5]*b[ 5] + a[ 6]*b[ 9] + a[ 7]*b[13];
	o[ 6] = a[ 4]*b[ 2] + a[ 5]*b[ 6] + a[ 6]*b[10] + a[ 7]*b[14];
	o[ 7] = a[ 4]*b[ 3] + a[ 5]*b[ 7] + a[ 6]*b[11] + a[ 7]*b[15];

	o[ 8] = a[ 8]*b[ 0] + a[ 9]*b[ 4] + a[10]*b[ 8] + a[11]*b[12];
	o[ 9] = a[ 8]*b[ 1] + a[ 9]*b[ 5] + a[10]*b[ 9] + a[11]*b[13];
	o[10] = a[ 8]*b[ 2] + a[ 9]*b[ 6] + a[10]*b[10] + a[11]*b[14];
	o[11] = a[ 8]*b[ 3] + a[ 9]*b[ 7] + a[10]*b[11] + a[11]*b[15];

	o[12] = a[12]*b[ 0] + a[13]*b[ 4] + a[14]*b[ 8] + a[15]*b[12];
	o[13] = a[12]*b[ 1] + a[13]*b[ 5] + a[14]*b[ 9] + a[15]*b[13];
	o[14] = a[12]*b[ 2] + a[13]*b[ 6] + a[14]*b[10] + a[15]*b[14];
	o[15] = a[12]*b[ 3] + a[13]*b[ 7] + a[14]*b[11] + a[15]*b[15];
}


/*
 *			M A T X V E C
 *
 * Multiply the matrix "im" by the vector "iv" and store the result
 * in the vector "ov".  Note this is post-multiply.
 */
void
matXvec(ov, im, iv)
register vectp_t ov;
register matp_t im;
register vectp_t iv;
{
	register int eo = 0;		/* Position in output vector */
	register int em = 0;		/* Position in input matrix */
	register int ei;		/* Position in input vector */

	/* For each element in the output array... */
	for(; eo<4; eo++) {

		ov[eo] = 0;		/* Start with zero in output */

		for(ei=0; ei<4; ei++)
			ov[eo] += im[em++] * iv[ei];
	}
}


/*
 *			M A T _ I N V
 *
 * The matrix pointed at by "im" is inverted and stored in the area
 * pointed at by "om".
 */
/* 
 * Invert a 4-by-4 matrix using Algorithm 120 from ACM.
 * This is a modified Gauss-Jordan alogorithm
 * Note:  Inversion is done in place, with 3 work vectors
 */
void
mat_inv( output, input )
matp_t input;
register matp_t output;
{
	register int i, j;			/* Indices */
	LOCAL int k;				/* Indices */
	LOCAL int	z[4];			/* Temporary */
	LOCAL fastf_t	b[4];			/* Temporary */
	LOCAL fastf_t	c[4];			/* Temporary */

	mat_copy( output, input );	/* Duplicate */

	/* Initialization */
	for( j = 0; j < 4; j++ )
		z[j] = j;

	/* Main Loop */
	for( i = 0; i < 4; i++ )  {
		FAST fastf_t y;				/* local temporary */

		k = i;
		y = output[i*4+i];
		for( j = i+1; j < 4; j++ )  {
			FAST fastf_t w;			/* local temporary */

			w = output[i*4+j];
			if( fabs(w) > fabs(y) )  {
				k = j;
				y = w;
			}
		}

		if( fabs(y) < 0.000001 )  {
			rt_log("mat_inv:  error!\n");
			abort();
			return;		/* ERROR */
		}
		y = 1.0 / y;

		for( j = 0; j < 4; j++ )  {
			FAST fastf_t temp;		/* Local */

			c[j] = output[j*4+k];
			output[j*4+k] = output[j*4+i];
			output[j*4+i] = - c[j] * y;
			temp = output[i*4+j] * y;
			b[j] = temp;
			output[i*4+j] = temp;
		}

		output[i*4+i] = y;
		j = z[i];
		z[i] = z[k];
		z[k] = j;
		for( k = 0; k < 4; k++ )  {
			if( k == i )  continue;
			for( j = 0; j < 4; j++ )  {
				if( j == i )  continue;
				output[k*4+j] = output[k*4+j] - b[j] * c[k];
			}
		}
	}

	/*  Second Loop */
	for( i = 0; i < 4; i++ )  {
		while( (k = z[i]) != i )  {
			LOCAL int p;			/* Local temp */

			for( j = 0; j < 4; j++ )  {
				FAST fastf_t w;		/* Local temp */

				w = output[i*4+j];
				output[i*4+j] = output[k*4+j];
				output[k*4+j] = w;
			}
			p = z[i];
			z[i] = z[k];
			z[k] = p;
		}
	}
}

/*
 *			V T O H _ M O V E
 *
 * Takes a pointer to a [x,y,z] vector, and a pointer
 * to space for a homogeneous vector [x,y,z,w],
 * and builds [x,y,z,1].
 */
void
vtoh_move( h, v )
register vectp_t h, v;
{
	*h++ = *v++;
	*h++ = *v++;
	*h++ = *v;
	*h++ = 1.0;
}

/*
 *			H T O V _ M O V E
 *
 * Takes a pointer to [x,y,z,w], and converts it to
 * an ordinary vector [x/w, y/w, z/w].
 * Optimization for the case of w==1 is performed.
 */
void
htov_move( v, h )
register vectp_t v, h;
{
	FAST fastf_t inv;

	if( h[3] == 1.0 )  {
		*v++ = *h++;
		*v++ = *h++;
		*v   = *h;
	}  else  {
		if( h[3] == 0.0 )  {
			rt_log("htov_move: divide by %f!\n", h[3]);
			return;
		}
		inv = 1.0 / h[3];
		*v++ = *h++ * inv;
		*v++ = *h++ * inv;
		*v   = *h   * inv;
	}
}

/*
 *			M A T _ P R I N T
 */
void
mat_print( title, m )
char *title;
mat_t m;
{
	register int i;

	rt_log("MATRIX %s:\n  ", title);
	for(i=0; i<16; i++)  {
		rt_log(" %8.3f", m[i]);
		if( (i&3) == 3 ) rt_log("\n  ");
	}
}

/*
 *			M A T _ T R N
 */
void
mat_trn( om, im )
register matp_t om;
register matp_t im;
{
	*om++ = im[0];
	*om++ = im[4];
	*om++ = im[8];
	*om++ = im[12];

	*om++ = im[1];
	*om++ = im[5];
	*om++ = im[9];
	*om++ = im[13];

	*om++ = im[2];
	*om++ = im[6];
	*om++ = im[10];
	*om++ = im[14];

	*om++ = im[3];
	*om++ = im[7];
	*om++ = im[11];
	*om++ = im[15];
}

/*
 *			M A T _ A E
 *
 *  Compute a 4x4 rotation matrix given Azimuth and Elevation.
 *  
 *  Azimuth is +X, Elevation is +Z, both in degrees.
 *
 *  Formula due to Doug Gwyn, BRL.
 */
void
mat_ae( m, azimuth, elev )
register matp_t m;
double azimuth;
double elev;
{
	LOCAL double sin_az, sin_el;
	LOCAL double cos_az, cos_el;

	azimuth *= mat_degtorad;
	elev *= mat_degtorad;

	sin_az = sin(azimuth);
	cos_az = cos(azimuth);
	sin_el = sin(elev);
	cos_el = cos(elev);

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

/*
 *			M A T _ A N G L E S
 *
 * This routine builds a Homogeneous rotation matrix, given
 * alpha, beta, and gamma as angles of rotation, in degrees.
 *
 * Alpha is angle of rotation about the X axis, and is done third.
 * Beta is angle of rotation about the Y axis, and is done second.
 * Gamma is angle of rotation about Z axis, and is done first.
 */
void
mat_angles( mat, alpha, beta, ggamma )
register matp_t mat;
double alpha, beta, ggamma;
{
	LOCAL double calpha, cbeta, cgamma;
	LOCAL double salpha, sbeta, sgamma;

	if( alpha == 0.0 && beta == 0.0 && ggamma == 0.0 )  {
		mat_idn( mat );
		return;
	}

	alpha *= mat_degtorad;
	beta *= mat_degtorad;
	ggamma *= mat_degtorad;

	calpha = cos( alpha );
	cbeta = cos( beta );
	cgamma = cos( ggamma );

	salpha = sin( alpha );
	sbeta = sin( beta );
	sgamma = sin( ggamma );

	mat[0] = cbeta * cgamma;
	mat[1] = -cbeta * sgamma;
	mat[2] = -sbeta;
	mat[3] = 0.0;

	mat[4] = -salpha * sbeta * cgamma + calpha * sgamma;
	mat[5] = salpha * sbeta * sgamma + calpha * cgamma;
	mat[6] = -salpha * cbeta;
	mat[7] = 0.0;

	mat[8] = calpha * sbeta * cgamma + salpha * sgamma;
	mat[9] = -calpha * sbeta * sgamma + salpha * cgamma;
	mat[10] = calpha * cbeta;
	mat[11] = 0.0;
	mat[12] = mat[13] = mat[14] = 0.0;
	mat[15] = 1.0;
}

/*
 *  Find the eigenvalues and eigenvectors of a
 *  symmetric 2x2 matrix.
 *	( a b )
 *	( b c )
 *
 *  The eigenvalue with the smallest absolute value is
 *  returned in val1, with its eigenvector in vec1.
 */
void
eigen2x2( val1, val2, vec1, vec2, a, b, c )
fastf_t	*val1, *val2;
vect_t	vec1, vec2;
fastf_t	a, b, c;
{
	fastf_t	d, root;
	fastf_t	v1, v2;

	d = 0.5 * (c - a);

	/* Check for diagonal matrix */
	if( NEAR_ZERO(b, 1.0e-10) ) {
		/* smaller mag first */
		if( fabs(c) < fabs(a) ) {
			*val1 = c;
			VSET( vec1, 0.0, 1.0, 0.0 );
			*val2 = a;
			VSET( vec2, -1.0, 0.0, 0.0 );
		} else {
			*val1 = a;
			VSET( vec1, 1.0, 0.0, 0.0 );
			*val2 = c;
			VSET( vec2, 0.0, 1.0, 0.0 );
		}
		return;
	}

	root = sqrt( d*d + b*b );
	v1 = 0.5 * (c + a) - root;
	v2 = 0.5 * (c + a) + root;

	/* smaller mag first */
	if( fabs(v1) < fabs(v2) ) {
		*val1 = v1;
		*val2 = v2;
		VSET( vec1, b, d - root, 0.0 );
	} else {
		*val1 = v2;
		*val2 = v1;
		VSET( vec1, root - d, b, 0.0 );
	}
	VUNITIZE( vec1 );
	VSET( vec2, -vec1[Y], vec1[X], 0.0 );	/* vec1 X vec2 = +Z */
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
 *
 *  MAT4X3VEC( to, m, from ) is the identity that is created.
 */
void
mat_fromto( m, from, to )
mat_t	m;
vect_t	from;
vect_t	to;
{
	double	az, el;
	LOCAL double sin_az, sin_el;
	LOCAL double cos_az, cos_el;

	az = mat_atan2( to[Y], to[X] ) - mat_atan2( from[Y], from[X] );
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

/*
 *			M A T _ L O O K A T
 *
 *  Given a direction vector D, product a matrix suitable for use
 *  as a "model2view" matrix that transforms the vector D
 *  into the -Z ("view") axis.
 *
 *  Note that due to the special property of mat_fromto()
 *  that prevents "twist" on the vector by orienting on the X-Y
 *  plane, we must first find the transformation that maps
 *  D into the +X axis, and then rotate to the -Z axis.
 */
void
mat_lookat( rot, dir )
mat_t rot;
vect_t dir;
{
	mat_t	second;
	mat_t	first;
	vect_t	x;

	/* Rotate from Dir to +X */
	VSET( x, 1, 0, 0 );
	mat_fromto( first, dir, x );

	/* Rotate so that +X is now -Z axis */
	mat_angles( second, -90.0, 0.0, 90.0 );
	mat_mul( rot, second, first );
}
