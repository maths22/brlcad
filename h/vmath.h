/*
 *			V M A T H . H
 *
 *  This header file defines many commonly used 3D vector math macros.
 *
 *  Note that while many people in the computer graphics field use
 *  post-multiplication with row vectors (ie, vector * matrix * matrix ...)
 *  the BRL CAD system uses the more traditional representation of 
 *  column vectors (ie, ... matrix * matrix * vector).  (The matrices
 *  in these two representations are the transposes of each other). Therefore,
 *  when transforming a vector by a matrix, pre-multiplication is used, ie:
 *
 *		view_vec = model2view_mat * model_vec
 *
 *  Furthermore, additional transformations are multiplied on the left, ie:
 *
 *		vec'  =  T1 * vec
 *		vec'' =  T2 * T1 * vec  =  T2 * vec'
 *
 *  The most notable implication of this is the location of the
 *  "delta" (translation) values in the matrix, ie:
 *
 *        x'     ( R0   R1   R2   Dx )      x
 *        y' =   ( R4   R5   R6   Dy )   *  y
 *        z'     ( R8   R9   R10  Dz )      z
 *        w'     (  0    0    0   1/s)      w
 *
 *  Note -
 *	vect_t objects are 3-tuples
 *	hvect_t objects are 4-tuples
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Distribution Status -
 *	This file is public domain, distribution unlimited.
 *
 *  $Header$
 */

#ifndef VMATH_H
#define VMATH_H seen

/*
 * Types for matrixes and vectors.
 */
typedef	fastf_t	mat_t[4*4];
typedef	fastf_t	*matp_t;

#define ELEMENTS_PER_VECT	3	/* # of fastf_t's per vect_t */
#define ELEMENTS_PER_PT         3
#define HVECT_LEN		4	/* # of fastf_t's per hvect_t */
#define HPT_LEN			4

typedef	fastf_t	vect_t[ELEMENTS_PER_VECT];
typedef	fastf_t	*vectp_t;

typedef fastf_t	point_t[ELEMENTS_PER_PT];
typedef fastf_t	*pointp_t;

typedef fastf_t hvect_t[HVECT_LEN];
typedef fastf_t hpoint_t[HPT_LEN];

/* Element names in homogeneous vector (4-tuple) */
#define	X	0
#define	Y	1
#define Z	2
#define H	3

/* Locations of deltas in 4x4 Homogenous Transform matrix */
#define MDX	3
#define MDY	7
#define MDZ	11
#define MAT_DELTAS(m,x,y,z)	(m)[MDX] = x; \
				(m)[MDY] = y; \
				(m)[MDZ] = z;

/* Set vector at `a' to have coordinates `b', `c', `d' */
#define VSET(a,b,c,d)	(a)[0] = (b);\
			(a)[1] = (c);\
			(a)[2] = (d)

/* Set all elements of vector to same scalar value */
#define VSETALL(a,s)	(a)[0] = (a)[1] = (a)[2] = (s);

/* Transfer vector at `b' to vector at `a' */
#define VMOVE(a,b)	(a)[0] = (b)[0];\
			(a)[1] = (b)[1];\
			(a)[2] = (b)[2]

/* Reverse the direction of b and store it in a */
#define VREVERSE(a,b)	(a)[0] = -(b)[0]; \
			(a)[1] = -(b)[1]; \
			(a)[2] = -(b)[2];

/* Add vectors at `b' and `c', store result at `a' */
#define VADD2(a,b,c)	(a)[0] = (b)[0] + (c)[0];\
			(a)[1] = (b)[1] + (c)[1];\
			(a)[2] = (b)[2] + (c)[2]

/* Subtract vector at `c' from vector at `b', store result at `a' */
#define VSUB2(a,b,c)	(a)[0] = (b)[0] - (c)[0];\
			(a)[1] = (b)[1] - (c)[1];\
			(a)[2] = (b)[2] - (c)[2]

/* Vectors:  A = B - C - D */
#define VSUB3(a,b,c,d)	(a)[0] = (b)[0] - (c)[0] - (d)[0];\
			(a)[1] = (b)[1] - (c)[1] - (d)[1];\
			(a)[2] = (b)[2] - (c)[2] - (d)[2]

/* Add 3 vectors at `b', `c', and `d', store result at `a' */
#define VADD3(a,b,c,d)	(a)[0] = (b)[0] + (c)[0] + (d)[0];\
			(a)[1] = (b)[1] + (c)[1] + (d)[1];\
			(a)[2] = (b)[2] + (c)[2] + (d)[2]

/* Add 4 vectors at `b', `c', `d', and `e', store result at `a' */
#define VADD4(a,b,c,d,e) (a)[0] = (b)[0] + (c)[0] + (d)[0] + (e)[0];\
			(a)[1] = (b)[1] + (c)[1] + (d)[1] + (e)[1];\
			(a)[2] = (b)[2] + (c)[2] + (d)[2] + (e)[2]

/* Scale vector at `b' by scalar `c', store result at `a' */
#define VSCALE(a,b,c)	(a)[0] = (b)[0] * (c);\
			(a)[1] = (b)[1] * (c);\
			(a)[2] = (b)[2] * (c)

/* Normalize vector 'a' to be a unit vector */
#define VUNITIZE(a)	{FAST double f; f = MAGNITUDE(a); \
			if( f < 1.0e-10 ) f = 0.0; else f = 1.0/f; \
			(a)[0] *= f; (a)[1] *= f; (a)[2] *= f; }

/* Combine together several vectors, scaled by a scalar */
#define VCOMB3(o, a,b, c,d, e,f)	{\
	(o)[X] = (a) * (b)[X] + (c) * (d)[X] + (e) * (f)[X];\
	(o)[Y] = (a) * (b)[Y] + (c) * (d)[Y] + (e) * (f)[Y];\
	(o)[Z] = (a) * (b)[Z] + (c) * (d)[Z] + (e) * (f)[Z];}

#define VCOMB2(o, a,b, c,d)	{\
	(o)[X] = (a) * (b)[X] + (c) * (d)[X];\
	(o)[Y] = (a) * (b)[Y] + (c) * (d)[Y];\
	(o)[Z] = (a) * (b)[Z] + (c) * (d)[Z];}

/* Compose vector at `a' of:
 *	Vector at `b' plus
 *	scalar `c' times vector at `d' plus
 *	scalar `e' times vector at `f'
 */
#define VJOIN2(a,b,c,d,e,f)	\
	(a)[0] = (b)[0] + (c) * (d)[0] + (e) * (f)[0];\
	(a)[1] = (b)[1] + (c) * (d)[1] + (e) * (f)[1];\
	(a)[2] = (b)[2] + (c) * (d)[2] + (e) * (f)[2]

#define VJOIN1(a,b,c,d) \
	(a)[0] = (b)[0] + (c) * (d)[0];\
	(a)[1] = (b)[1] + (c) * (d)[1];\
	(a)[2] = (b)[2] + (c) * (d)[2];

/* Return scalar magnitude squared of vector at `a' */
#define MAGSQ(a)	( (a)[0]*(a)[0] + (a)[1]*(a)[1] + (a)[2]*(a)[2] )

/* Return scalar magnitude of vector at `a' */
#define MAGNITUDE(a)	sqrt( MAGSQ( a ) )
extern double sqrt();

/* Store cross product of vectors at `b' and `c' in vector at `a' */
#define VCROSS(a,b,c)	(a)[0] = (b)[1] * (c)[2] - (b)[2] * (c)[1];\
			(a)[1] = (b)[2] * (c)[0] - (b)[0] * (c)[2];\
			(a)[2] = (b)[0] * (c)[1] - (b)[1] * (c)[0]

/* Compute dot product of vectors at `a' and `b' */
#define VDOT(a,b)	( (a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] )

/* Print vector name and components on stdout */
#define VPRINT(a,b)	(void)fprintf(stderr,"%s (%g, %g, %g)\n", a, (b)[0], (b)[1], (b)[2])
#define HPRINT(a,b)	(void)fprintf(stderr,"%s (%g, %g, %g, %g)\n", a, (b)[0], (b)[1], (b)[2], (b)[3])

/* Vector element multiplication.  Really: diagonal matrix X vect */
#define VELMUL(a,b,c) \
	(a)[0] = (b)[0] * (c)[0];\
	(a)[1] = (b)[1] * (c)[1];\
	(a)[2] = (b)[2] * (c)[2];

/* Apply the 3x3 part of a mat_t to a 3-tuple. */
#define MAT3X3VEC(o,mat,vec) \
	(o)[0] = (mat)[0]*(vec)[0]+(mat)[1]*(vec)[1] + (mat)[ 2]*(vec)[2]; \
	(o)[1] = (mat)[4]*(vec)[0]+(mat)[5]*(vec)[1] + (mat)[ 6]*(vec)[2]; \
	(o)[2] = (mat)[8]*(vec)[0]+(mat)[9]*(vec)[1] + (mat)[10]*(vec)[2];

/* Multiply a 3-tuple by the 3x3 part of a mat_t. */
#define VEC3X3MAT(o,i,m) \
	(o)[X] = (i)[X]*(m)[0] + (i)[Y]*(m)[4] + (i)[Z]*(m)[8]; \
	(o)[Y] = (i)[X]*(m)[1] + (i)[Y]*(m)[5] + (i)[Z]*(m)[9]; \
	(o)[Z] = (i)[X]*(m)[2] + (i)[Y]*(m)[6] + (i)[Z]*(m)[10];

/* Apply the 3x3 part of a mat_t to a 2-tuple (Z part=0). */
#define MAT3X2VEC(o,mat,vec) \
	(o)[0] = (mat)[0]*(vec)[0] + (mat)[1]*(vec)[1]; \
	(o)[1] = (mat)[4]*(vec)[0] + (mat)[5]*(vec)[1]; \
	(o)[2] = (mat)[8]*(vec)[0] + (mat)[9]*(vec)[1];

/* Multiply a 2-tuple (Z=0) by the 3x3 part of a mat_t. */
#define VEC2X3MAT(o,i,m) \
	(o)[X] = (i)[X]*(m)[0] + (i)[Y]*(m)[4]; \
	(o)[Y] = (i)[X]*(m)[1] + (i)[Y]*(m)[5]; \
	(o)[Z] = (i)[X]*(m)[2] + (i)[Y]*(m)[6];

/* Apply a 4x4 matrix to a 3-tuple which is a absolue Point in space */
#define MAT4X3PNT(o,m,i) \
	{ FAST fastf_t f; \
	f = 1.0/((m)[12]*(i)[X] + (m)[13]*(i)[Y] + (m)[14]*(i)[Z] + (m)[15]);\
	(o)[X]=((m)[0]*(i)[X] + (m)[1]*(i)[Y] + (m)[ 2]*(i)[Z] + (m)[3]) * f;\
	(o)[Y]=((m)[4]*(i)[X] + (m)[5]*(i)[Y] + (m)[ 6]*(i)[Z] + (m)[7]) * f;\
	(o)[Z]=((m)[8]*(i)[X] + (m)[9]*(i)[Y] + (m)[10]*(i)[Z] + (m)[11])* f;}

/* Multiply an Absolute 3-Point by a full 4x4 matrix. */
#define PNT3X4MAT(o,i,m) \
	{ FAST fastf_t f; \
	f = 1.0/((i)[X]*(m)[3] + (i)[Y]*(m)[7] + (i)[Z]*(m)[11] + (m)[15]);\
	(o)[X]=((i)[X]*(m)[0] + (i)[Y]*(m)[4] + (i)[Z]*(m)[8] + (m)[12]) * f;\
	(o)[Y]=((i)[X]*(m)[1] + (i)[Y]*(m)[5] + (i)[Z]*(m)[9] + (m)[13]) * f;\
	(o)[Z]=((i)[X]*(m)[2] + (i)[Y]*(m)[6] + (i)[Z]*(m)[10] + (m)[14])* f;}

/* Multiply an Absolute hvect_t 4-Point by a full 4x4 matrix. */
#define MAT4X4PNT(o,m,i) \
	(o)[X]=(m)[ 0]*(i)[X] + (m)[ 1]*(i)[Y] + (m)[ 2]*(i)[Z] + (m)[ 3]*(i)[H];\
	(o)[Y]=(m)[ 4]*(i)[X] + (m)[ 5]*(i)[Y] + (m)[ 6]*(i)[Z] + (m)[ 7]*(i)[H];\
	(o)[Z]=(m)[ 8]*(i)[X] + (m)[ 9]*(i)[Y] + (m)[10]*(i)[Z] + (m)[11]*(i)[H];\
	(o)[H]=(m)[12]*(i)[X] + (m)[13]*(i)[Y] + (m)[14]*(i)[Z] + (m)[15]*(i)[H];

/* Apply a 4x4 matrix to a 3-tuple which is a relative Vector in space */
#define MAT4X3VEC(o,m,i) \
	{ FAST fastf_t f;	f = 1.0/((m)[15]);\
	(o)[X] = ((m)[0]*(i)[X] + (m)[1]*(i)[Y] + (m)[ 2]*(i)[Z]) * f; \
	(o)[Y] = ((m)[4]*(i)[X] + (m)[5]*(i)[Y] + (m)[ 6]*(i)[Z]) * f; \
	(o)[Z] = ((m)[8]*(i)[X] + (m)[9]*(i)[Y] + (m)[10]*(i)[Z]) * f; }

/* Multiply a Relative 3-Vector by most of a 4x4 matrix */
#define VEC3X4MAT(o,i,m) \
	{ FAST fastf_t f; 	f = 1.0/((m)[15]); \
	(o)[X] = ((i)[X]*(m)[0] + (i)[Y]*(m)[4] + (i)[Z]*(m)[8]) * f; \
	(o)[Y] = ((i)[X]*(m)[1] + (i)[Y]*(m)[5] + (i)[Z]*(m)[9]) * f; \
	(o)[Z] = ((i)[X]*(m)[2] + (i)[Y]*(m)[6] + (i)[Z]*(m)[10]) * f; }

/* Multiply a Relative 2-Vector by most of a 4x4 matrix */
#define VEC2X4MAT(o,i,m) \
	{ FAST fastf_t f; 	f = 1.0/((m)[15]); \
	(o)[X] = ((i)[X]*(m)[0] + (i)[Y]*(m)[4]) * f; \
	(o)[Y] = ((i)[X]*(m)[1] + (i)[Y]*(m)[5]) * f; \
	(o)[Z] = ((i)[X]*(m)[2] + (i)[Y]*(m)[6]) * f; }

/* Compare two vectors for EXACT equality.  Use carefully. */
#define VEQUAL(a,b)	((a)[X]==(b)[X] && (a)[Y]==(b)[Y] && (a)[Z]==(b)[Z])

/* Macros to update min and max X,Y,Z values to contain a point */
#define V_MIN(r,s)	if( (r) > (s) ) r = (s)
#define V_MAX(r,s)	if( (r) < (s) ) r = (s)
#define VMIN(r,s)	{ V_MIN(r[X],s[X]); V_MIN(r[Y],s[Y]); V_MIN(r[Z],s[Z]); }
#define VMAX(r,s)	{ V_MAX(r[X],s[X]); V_MAX(r[Y],s[Y]); V_MAX(r[Z],s[Z]); }
#define VMINMAX( min, max, pt )	{ VMIN( min, pt ); VMAX( max, pt ); }

/* Divide out homogeneous parameter from hvect_t, creating vect_t */
#define HDIVIDE(a,b)  \
	(a)[X] = (b)[X] / (b)[H];\
	(a)[Y] = (b)[Y] / (b)[H];\
	(a)[Z] = (b)[Z] / (b)[H];

#endif VMATH_H
