/*
 *			G _ E B M . C
 *
 *  Purpose -
 *	Intersect a ray with an Extruded Bitmap,
 *	where the bitmap is taken from a bw(5) file.
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
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSebm[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"
#include "./fixpt.h"

#define EBM_NAME_LEN 128
struct ebm_specific {
	char		ebm_file[EBM_NAME_LEN];
	unsigned char	*ebm_map;
	int		ebm_xdim;	/* X dimension */
	int		ebm_ydim;	/* Y dimension */
	double		ebm_tallness;	/* Z dimension */
	vect_t		ebm_xnorm;	/* local +X norm in model coords */
	vect_t		ebm_ynorm;
	vect_t		ebm_znorm;
	vect_t		ebm_cellsize;	/* ideal coords: size of each cell */
	mat_t		ebm_mat;	/* model to ideal space */
	vect_t		ebm_origin;	/* local coords of grid origin (0,0,0) for now */
	vect_t		ebm_large;	/* local coords of XYZ max */
};
#define EBM_NULL	((struct ebm_specific *)0)
#define EBM_O(m)	offsetof(struct ebm_specific, m)

struct structparse rt_ebm_parse[] = {
#if CRAY && !__STDC__
	"%s",	EBM_NAME_LEN, "file",	0,		FUNC_NULL,
#else
	"%s",	EBM_NAME_LEN, "file",	offsetofarray(struct ebm_specific, ebm_file), FUNC_NULL,
#endif
	"%d",	1, "w",		EBM_O(ebm_xdim),	FUNC_NULL,
	"%d",	1, "n",		EBM_O(ebm_ydim),	FUNC_NULL,
	"%f",	1, "d",		EBM_O(ebm_tallness),	FUNC_NULL,
	/* XXX might have option for ebm_origin */
	"",	0, (char *)0, 0,			FUNC_NULL
};

struct ebm_specific	*rt_ebm_import();
RT_EXTERN(int rt_ebm_dda,(struct xray *rp, struct soltab *stp,
	struct application *ap, struct seg *seghead));
RT_EXTERN(int rt_seg_planeclip,(struct seg *out_hd, struct seg *in_hd,
	vect_t out_norm, fastf_t in, fastf_t out,
	struct xray *rp, struct application *ap));

/*
 *  Codes to represent surface normals.
 *  In a bitmap, there are only 4 possible normals.
 *  With this code, reverse the sign to reverse the direction.
 *  As always, the normal is expected to point outwards.
 */
#define NORM_ZPOS	3
#define NORM_YPOS	2
#define NORM_XPOS	1
#define NORM_XNEG	(-1)
#define NORM_YNEG	(-2)
#define NORM_ZNEG	(-3)

/*
 *  Regular bit addressing is used:  (0..W-1, 0..N-1),
 *  but the bitmap is stored with two cells of zeros all around,
 *  so permissible subscripts run (-2..W+1, -2..N+1).
 *  This eliminates special-case code for the boundary conditions.
 */
#define	BIT_XWIDEN	2
#define	BIT_YWIDEN	2
#define BIT(xx,yy)	ebmp->ebm_map[((yy)+BIT_YWIDEN)*(ebmp->ebm_xdim + \
				BIT_XWIDEN*2)+(xx)+BIT_XWIDEN]

/*
 *			R T _ S E G _ P L A N E C L I P
 *
 *  Take a segment chain, in sorted order (ascending hit_dist),
 *  and clip to the range (in, out) along the normal "out_norm".
 *  For the particular ray "rp", find the parametric distances:
 *	kmin is the minimum permissible parameter, "in" units away
 *	kmax is the maximum permissible parameter, "out" units away
 *
 *  Returns -
 *	1	OK: trimmed segment chain, still in sorted order
 *	0	ERROR
 */
int
rt_seg_planeclip( out_hd, in_hd, out_norm, in, out, rp, ap )
struct seg	*out_hd;
struct seg	*in_hd;
vect_t		out_norm;
fastf_t		in, out;
struct xray	*rp;
struct application *ap;
{
	fastf_t		norm_dist_min, norm_dist_max;
	fastf_t		slant_factor;
	fastf_t		kmin, kmax;
	vect_t		in_norm;
	register struct seg	*curr;
	int		out_norm_code;
	int		count;

	norm_dist_min = in - VDOT( rp->r_pt, out_norm );
	slant_factor = VDOT( rp->r_dir, out_norm );	/* always abs < 1 */
	if( NEAR_ZERO( slant_factor, SQRT_SMALL_FASTF ) )  {
		if( norm_dist_min < 0.0 )  {
			rt_log("rt_seg_planeclip ERROR -- ray parallel to baseplane, outside \n");
			/* XXX Free segp chain */
			return(0);
		}
		kmin = -INFINITY;
	} else
		kmin =  norm_dist_min / slant_factor;

	VREVERSE( in_norm, out_norm );
	norm_dist_max = out - VDOT( rp->r_pt, in_norm );
	slant_factor = VDOT( rp->r_dir, in_norm );	/* always abs < 1 */
	if( NEAR_ZERO( slant_factor, SQRT_SMALL_FASTF ) )  {
		if( norm_dist_max < 0.0 )  {
			rt_log("rt_seg_planeclip ERROR -- ray parallel to baseplane, outside \n");
			/* XXX Free segp chain */
			return(0);
		}
		kmax = INFINITY;
	} else
		kmax =  norm_dist_max / slant_factor;

	if( kmin > kmax )  {
		/* If r_dir[Z] < 0, will need to swap min & max */
		slant_factor = kmax;
		kmax = kmin;
		kmin = slant_factor;
		out_norm_code = NORM_ZPOS;
	} else {
		out_norm_code = NORM_ZNEG;
	}
	if(rt_g.debug&DEBUG_EBM)rt_log("kmin=%g, kmax=%g, out_norm_code=%d\n", kmin, kmax, out_norm_code );

	count = 0;
	while( RT_LIST_LOOP( curr, seg, &(in_hd->l) ) )  {
		RT_LIST_DEQUEUE( &(curr->l) );
		if(rt_g.debug&DEBUG_EBM)rt_log(" rt_seg_planeclip seg( %g, %g )\n", curr->seg_in.hit_dist, curr->seg_out.hit_dist );
		if( curr->seg_out.hit_dist <= kmin )  {
			if(rt_g.debug&DEBUG_EBM)rt_log("seg_out %g <= kmin %g, freeing\n", curr->seg_out.hit_dist, kmin );
			RT_FREE_SEG(curr, ap->a_resource);
			continue;
		}
		if( curr->seg_in.hit_dist >= kmax )  {
			if(rt_g.debug&DEBUG_EBM)rt_log("seg_in  %g >= kmax %g, freeing\n", curr->seg_in.hit_dist, kmax );
			RT_FREE_SEG(curr, ap->a_resource);
			continue;
		}
		if( curr->seg_in.hit_dist <= kmin )  {
			if(rt_g.debug&DEBUG_EBM)rt_log("seg_in = kmin %g\n", kmin );
			curr->seg_in.hit_dist = kmin;
			curr->seg_in.hit_surfno = out_norm_code;
		}
		if( curr->seg_out.hit_dist >= kmax )  {
			if(rt_g.debug&DEBUG_EBM)rt_log("seg_out= kmax %g\n", kmax );
			curr->seg_out.hit_dist = kmax;
			curr->seg_out.hit_surfno = (-out_norm_code);
		}
		RT_LIST_INSERT( &(out_hd->l), &(curr->l) );
		count += 2;
	}
	return( count );
}

static int rt_ebm_normtab[3] = { NORM_XPOS, NORM_YPOS, NORM_ZPOS };


/*
 *			R T _ E B M _ D D A
 *
 *  Step through the 2-D array, in local coordinates ("ideal space").
 *
 *
 */
int
rt_ebm_dda( rp, stp, ap, seghead )
register struct xray	*rp;
struct soltab		*stp;
struct application	*ap;
struct seg		*seghead;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;
	vect_t	invdir;
	double	t0;	/* in point of cell */
	double	t1;	/* out point of cell */
	double	tmax;	/* out point of entire grid */
	vect_t	t;	/* next t value for XYZ cell plane intersect */
	vect_t	delta;	/* spacing of XYZ cell planes along ray */
	int	igrid[3];/* Grid cell coordinates of cell (integerized) */
	vect_t	P;	/* hit point */
	int	inside;	/* inside/outside a solid flag */
	int	in_index;
	int	out_index;
	int	j;

	/* Compute the inverse of the direction cosines */
	if( !NEAR_ZERO( rp->r_dir[X], SQRT_SMALL_FASTF ) )  {
		invdir[X]=1.0/rp->r_dir[X];
	} else {
		invdir[X] = INFINITY;
		rp->r_dir[X] = 0.0;
	}
	if( !NEAR_ZERO( rp->r_dir[Y], SQRT_SMALL_FASTF ) )  {
		invdir[Y]=1.0/rp->r_dir[Y];
	} else {
		invdir[Y] = INFINITY;
		rp->r_dir[Y] = 0.0;
	}
	if( !NEAR_ZERO( rp->r_dir[Z], SQRT_SMALL_FASTF ) )  {
		invdir[Z]=1.0/rp->r_dir[Z];
	} else {
		invdir[Z] = INFINITY;
		rp->r_dir[Z] = 0.0;
	}

	/* intersect ray with ideal grid rpp */
	VSETALL( P, 0 );
	if( ! rt_in_rpp(rp, invdir, P, ebmp->ebm_large ) )
		return(0);	/* MISS */
	VJOIN1( P, rp->r_pt, rp->r_min, rp->r_dir );	/* P is hit point */
if(rt_g.debug&DEBUG_EBM)VPRINT("ebm_origin", ebmp->ebm_origin);
if(rt_g.debug&DEBUG_EBM)VPRINT("r_pt", rp->r_pt);
if(rt_g.debug&DEBUG_EBM)VPRINT("P", P);
if(rt_g.debug&DEBUG_EBM)VPRINT("cellsize", ebmp->ebm_cellsize);
	t0 = rp->r_min;
	tmax = rp->r_max;
if(rt_g.debug&DEBUG_EBM)rt_log("[shoot: r_min=%g, r_max=%g]\n", rp->r_min, rp->r_max);

	/* find grid cell where ray first hits ideal space bounding RPP */
	igrid[X] = (P[X] - ebmp->ebm_origin[X]) / ebmp->ebm_cellsize[X];
	igrid[Y] = (P[Y] - ebmp->ebm_origin[Y]) / ebmp->ebm_cellsize[Y];
	if( igrid[X] < 0 )  {
		igrid[X] = 0;
	} else if( igrid[X] >= ebmp->ebm_xdim ) {
		igrid[X] = ebmp->ebm_xdim-1;
	}
	if( igrid[Y] < 0 )  {
		igrid[Y] = 0;
	} else if( igrid[Y] >= ebmp->ebm_ydim ) {
		igrid[Y] = ebmp->ebm_ydim-1;
	}
if(rt_g.debug&DEBUG_EBM)rt_log("g[X] = %d, g[Y] = %d\n", igrid[X], igrid[Y]);

	if( rp->r_dir[X] == 0.0 && rp->r_dir[Y] == 0.0 )  {
		register struct seg	*segp;

		/*  Ray is traveling exactly along Z axis.
		 *  Just check the one cell hit.
		 *  Depend on higher level to clip ray to Z extent.
		 */
if(rt_g.debug&DEBUG_EBM)rt_log("ray on local Z axis\n");
		if( BIT( igrid[X], igrid[Y] ) == 0 )
			return(0);	/* MISS */
		RT_GET_SEG(segp, ap->a_resource);
		segp->seg_stp = stp;
		segp->seg_in.hit_dist = 0;
		segp->seg_out.hit_dist = INFINITY;
		if( rp->r_dir[Z] < 0 )  {
			segp->seg_in.hit_surfno = NORM_ZPOS;
			segp->seg_out.hit_surfno = NORM_ZNEG;
		} else {
			segp->seg_in.hit_surfno = NORM_ZNEG;
			segp->seg_out.hit_surfno = NORM_ZPOS;
		}
		RT_LIST_INSERT( &(seghead->l), &(segp->l) );
		return(2);			/* HIT */
	}

	/* X setup */
	if( rp->r_dir[X] == 0.0 )  {
		t[X] = INFINITY;
		delta[X] = 0;
	} else {
		j = igrid[X];
		if( rp->r_dir[X] < 0 ) j++;
		t[X] = (ebmp->ebm_origin[X] + j*ebmp->ebm_cellsize[X] -
			rp->r_pt[X]) * invdir[X];
		delta[X] = ebmp->ebm_cellsize[X] * fabs(invdir[X]);
	}
	/* Y setup */
	if( rp->r_dir[Y] == 0.0 )  {
		t[Y] = INFINITY;
		delta[Y] = 0;
	} else {
		j = igrid[Y];
		if( rp->r_dir[Y] < 0 ) j++;
		t[Y] = (ebmp->ebm_origin[Y] + j*ebmp->ebm_cellsize[Y] -
			rp->r_pt[Y]) * invdir[Y];
		delta[Y] = ebmp->ebm_cellsize[Y] * fabs(invdir[Y]);
	}
#if 0
	/* Z setup */
	if( rp->r_dir[Z] == 0.0 )  {
		t[Z] = INFINITY;
	} else {
		/* Consider igrid[Z] to be either 0 or 1 */
		if( rp->r_dir[Z] < 0 )  {
			t[Z] = (ebmp->ebm_origin[Z] + ebmp->ebm_cellsize[Z] -
				rp->r_pt[Z]) * invdir[Z];
		} else {
			t[Z] = (ebmp->ebm_origin[Z] - rp->r_pt[Z]) * invdir[Z];
		}
	}
#endif

	/* The delta[] elements *must* be positive, as t must increase */
if(rt_g.debug&DEBUG_EBM)rt_log("t[X] = %g, delta[X] = %g\n", t[X], delta[X] );
if(rt_g.debug&DEBUG_EBM)rt_log("t[Y] = %g, delta[Y] = %g\n", t[Y], delta[Y] );

	/* Find face of entry into first cell -- max initial t value */
	if( t[X] >= t[Y] )  {
		in_index = X;
		t0 = t[X];
	} else {
		in_index = Y;
		t0 = t[Y];
	}
if(rt_g.debug&DEBUG_EBM)rt_log("Entry index is %s, t0=%g\n", in_index==X ? "X" : "Y", t0);

	/* Advance to next exits */
	t[X] += delta[X];
	t[Y] += delta[Y];

	/* Ensure that next exit is after first entrance */
	if( t[X] < t0 )  {
		rt_log("*** advancing t[X]\n");
		t[X] += delta[X];
	}
	if( t[Y] < t0 )  {
		rt_log("*** advancing t[Y]\n");
		t[Y] += delta[Y];
	}
if(rt_g.debug&DEBUG_EBM)rt_log("Exit t[X]=%g, t[Y]=%g\n", t[X], t[Y] );

	inside = 0;

	while( t0 < tmax ) {
		int	val;
		struct seg	*segp;

		/* find minimum exit t value */
		out_index = t[X] < t[Y] ? X : Y;

		t1 = t[out_index];

		/* Ray passes through cell igrid[XY] from t0 to t1 */
		val = BIT( igrid[X], igrid[Y] );
if(rt_g.debug&DEBUG_EBM)rt_log("igrid [%d %d] from %g to %g, val=%d\n",
			igrid[X], igrid[Y],
			t0, t1, val );
if(rt_g.debug&DEBUG_EBM)rt_log("Exit index is %s, t[X]=%g, t[Y]=%g\n",
			out_index==X ? "X" : "Y", t[X], t[Y] );

		if( t1 <= t0 )  rt_log("ERROR ebm t1=%g < t0=%g\n", t1, t0 );
		if( !inside )  {
			if( val > 0 )  {
				/* Handle the transition from vacuum to solid */
				/* Start of segment (entering a full voxel) */
				inside = 1;

				RT_GET_SEG(segp, ap->a_resource);
				segp->seg_stp = stp;
				segp->seg_in.hit_dist = t0;

				/* Compute entry normal */
				if( rp->r_dir[in_index] < 0 )  {
					/* Go left, entry norm goes right */
					segp->seg_in.hit_surfno =
						rt_ebm_normtab[in_index];
				}  else  {
					/* go right, entry norm goes left */
					segp->seg_in.hit_surfno =
						(-rt_ebm_normtab[in_index]);
				}
				RT_LIST_INSERT( &(seghead->l), &(segp->l) );

				if(rt_g.debug&DEBUG_EBM) rt_log("START t=%g, surfno=%d\n",
					t0, segp->seg_in.hit_surfno);
			} else {
				/* Do nothing, marching through void */
			}
		} else {
			register struct seg	*tail;
			if( val > 0 )  {
				/* Do nothing, marching through solid */
			} else {
				/* End of segment (now in an empty voxel) */
				/* Handle transition from solid to vacuum */
				inside = 0;

				tail = RT_LIST_LAST( seg, &(seghead->l) );
				tail->seg_out.hit_dist = t0;

				/* Compute exit normal */
				if( rp->r_dir[in_index] < 0 )  {
					/* Go left, exit normal goes left */
					tail->seg_out.hit_surfno =
						(-rt_ebm_normtab[in_index]);
				}  else  {
					/* go right, exit norm goes right */
					tail->seg_out.hit_surfno =
						rt_ebm_normtab[in_index];
				}
				if(rt_g.debug&DEBUG_EBM) rt_log("END t=%g, surfno=%d\n",
					t0, tail->seg_out.hit_surfno );
			}
		}

		/* Take next step */
		t0 = t1;
		in_index = out_index;
		t[out_index] += delta[out_index];
		if( rp->r_dir[out_index] > 0 ) {
			igrid[out_index]++;
		} else {
			igrid[out_index]--;
		}
	}

	if( inside )  {
		register struct seg	*tail;

		/* Close off the final segment */
		tail = RT_LIST_LAST( seg, &(seghead->l) );
		tail->seg_out.hit_dist = tmax;

		/* Compute exit normal.  Previous out_index is now in_index */
		if( rp->r_dir[in_index] < 0 )  {
			/* Go left, exit normal goes left */
			tail->seg_out.hit_surfno = (-rt_ebm_normtab[in_index]);
		}  else  {
			/* go right, exit norm goes right */
			tail->seg_out.hit_surfno = rt_ebm_normtab[in_index];
		}
		if(rt_g.debug&DEBUG_EBM) rt_log("closed END t=%g, surfno=%d\n",
			tmax, tail->seg_out.hit_surfno );
	}

	if( RT_LIST_IS_EMPTY( &(seghead->l) ) )
		return(0);
	return(2);
}

/*
 *			R T _ E B M _ I M P O R T
 */
HIDDEN struct ebm_specific *
rt_ebm_import( rp )
union record	*rp;
{
	register struct ebm_specific *ebmp;
	struct rt_vls	str;
	FILE	*fp;
	int	nbytes;
	register int	y;
	char	*cp;

	GETSTRUCT( ebmp, ebm_specific );

	rt_vls_init( &str );
	cp = rp->ss.ss_str;
	/* First word is name of solid type (eg, "ebm") -- skip over it */
	while( *cp && !isspace(*cp) )  cp++;
	/* Skip all white space */
	while( *cp && isspace(*cp) )  cp++;

	rt_vls_strcpy( &str, cp );
	rt_structparse( &str, rt_ebm_parse, (char *)ebmp );
	rt_vls_free( &str );

	/* Check for reasonable values */
	if( ebmp->ebm_file[0] == '\0' || ebmp->ebm_xdim < 1 ||
	    ebmp->ebm_ydim < 1 ||
	    ebmp->ebm_tallness <= 0.0 )  {
	    	rt_log("Unreasonable EBM parameters\n");
		rt_free( (char *)ebmp, "ebm_specific" );
		return( EBM_NULL );
	}

	/* Get bit map from .bw(5) file */
	nbytes = (ebmp->ebm_xdim+BIT_XWIDEN*2)*(ebmp->ebm_ydim+BIT_YWIDEN*2);
	ebmp->ebm_map = (unsigned char *)rt_malloc( nbytes, "ebm_import bitmap" );
#ifdef SYSV
	memset( ebmp->ebm_map, 0, nbytes );
#else
	bzero( ebmp->ebm_map, nbytes );
#endif
	if( (fp = fopen(ebmp->ebm_file, "r")) == NULL )  {
		perror(ebmp->ebm_file);
		rt_free( (char *)ebmp, "ebm_specific" );
		return( EBM_NULL );
	}

	/* Because of in-memory padding, read each scanline separately */
	for( y=0; y < ebmp->ebm_ydim; y++ )
		(void)fread( &BIT(0, y), ebmp->ebm_xdim, 1, fp );
	fclose(fp);
	return( ebmp );
}

/*
 *			R T _ R O T _ B O U N D _ R P P
 *
 *  Given an RPP that defines a bounding cube in a local coordinate
 *  system, transform that cube into another coordinate system,
 *  and then find the new (usually somewhat larger) bounding RPP.
 */
void
rt_rot_bound_rpp( omin, omax, mat, imin, imax )
vect_t	omin, omax;
mat_t	mat;
vect_t	imin, imax;
{
	point_t	local;		/* vertex point in local coordinates */
	point_t	model;		/* vertex point in model coordinates */

#define ROT_VERT( a, b, c )  \
	VSET( local, a[X], b[Y], c[Z] ); \
	MAT4X3PNT( model, mat, local ); \
	VMINMAX( omin, omax, model ) \

	ROT_VERT( imin, imin, imin );
	ROT_VERT( imin, imin, imax );
	ROT_VERT( imin, imax, imin );
	ROT_VERT( imin, imax, imax );
	ROT_VERT( imax, imin, imin );
	ROT_VERT( imax, imin, imax );
	ROT_VERT( imax, imax, imin );
	ROT_VERT( imax, imax, imax );
#undef ROT_VERT
}

/*
 *			R T _ E B M _ P R E P
 *
 *  Returns -
 *	0	OK
 *	!0	Failure
 *
 *  Implicit return -
 *	A struct ebm_specific is created, and it's address is stored
 *	in stp->st_specific for use by rt_ebm_shot().
 */
int
rt_ebm_prep( stp, rp, rtip )
struct soltab	*stp;
union record	*rp;
struct rt_i	*rtip;
{
	register struct ebm_specific *ebmp;
	vect_t	norm;
	vect_t	radvec;
	vect_t	diam;
	vect_t	small;

	if( (ebmp = rt_ebm_import( rp )) == EBM_NULL )
		return(-1);	/* ERROR */

	/* build Xform matrix from model(world) to ideal(local) space */
	mat_inv( ebmp->ebm_mat, stp->st_pathmat );

	/* Pre-compute the necessary normals */
	VSET( norm, 1, 0 , 0 );
	MAT4X3VEC( ebmp->ebm_xnorm, stp->st_pathmat, norm );
	VSET( norm, 0, 1, 0 );
	MAT4X3VEC( ebmp->ebm_ynorm, stp->st_pathmat, norm );
	VSET( norm, 0, 0, 1 );
	MAT4X3VEC( ebmp->ebm_znorm, stp->st_pathmat, norm );

	stp->st_specific = (genptr_t)ebmp;

	/* Find bounding RPP of rotated local RPP */
	VSETALL( small, 0 );
	VSET( ebmp->ebm_large, ebmp->ebm_xdim, ebmp->ebm_ydim, ebmp->ebm_tallness );
	rt_rot_bound_rpp( stp->st_min, stp->st_max, stp->st_pathmat,
		small, ebmp->ebm_large );

	/* for now, EBM origin in ideal coordinates is at origin */
	VSETALL( ebmp->ebm_origin, 0 );
	VADD2( ebmp->ebm_large, ebmp->ebm_large, ebmp->ebm_origin );

	/* for now, EBM cell size in ideal coordinates is one unit/cell */
	VSETALL( ebmp->ebm_cellsize, 1 );

	VSUB2( diam, stp->st_max, stp->st_min );
	VADD2SCALE( stp->st_center, stp->st_min, stp->st_max, 0.5 );
	VSCALE( radvec, diam, 0.5 );
	stp->st_aradius = stp->st_bradius = MAGNITUDE( radvec );

	return(0);		/* OK */
}

/*
 *			R T _ E B M _ P R I N T
 */
void
rt_ebm_print( stp )
register struct soltab	*stp;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;

	rt_log("ebm file = %s\n", ebmp->ebm_file );
	rt_log("dimensions = (%d, %d, %g)\n",
		ebmp->ebm_xdim, ebmp->ebm_ydim,
		ebmp->ebm_tallness );
	VPRINT("model cellsize", ebmp->ebm_cellsize);
	VPRINT("model grid origin", ebmp->ebm_origin);
}

/*
 *			R T _ E B M _ S H O T
 *
 *  Intersect a ray with an extruded bitmap.
 *  If intersection occurs, a pointer to a sorted linked list of
 *  "struct seg"s will be returned.
 *
 *  Returns -
 *	0	MISS
 *	>0	HIT
 */
int
rt_ebm_shot( stp, rp, ap, seghead )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
struct seg		*seghead;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;
	vect_t		norm;
	struct xray	ideal_ray;
	struct seg	myhead;
	int		i;

	RT_LIST_INIT( &(myhead.l) );

	/* Transform actual ray into ideal space at origin in X-Y plane */
	MAT4X3PNT( ideal_ray.r_pt, ebmp->ebm_mat, rp->r_pt );
	MAT4X3VEC( ideal_ray.r_dir, ebmp->ebm_mat, rp->r_dir );

#if 0
rt_log("%g %g %g %g %g %g\n",
ideal_ray.r_pt[X], ideal_ray.r_pt[Y], ideal_ray.r_pt[Z],
ideal_ray.r_dir[X], ideal_ray.r_dir[Y], ideal_ray.r_dir[Z] );
#endif
	if( rt_ebm_dda( &ideal_ray, stp, ap, &myhead ) <= 0 )
		return(0);

	VSET( norm, 0, 0, -1 );		/* letters grow in +z, which is "inside" the halfspace */
	i = rt_seg_planeclip( seghead, &myhead, norm, 0.0, ebmp->ebm_tallness,
		&ideal_ray, ap );
#if 0
	if( segp )  {
		vect_t	a, b;
		/* Plot where WE think the ray goes */
		VJOIN1( a, rp->r_pt, segp->seg_in.hit_dist, rp->r_dir );
		VJOIN1( b, rp->r_pt, segp->seg_out.hit_dist, rp->r_dir );
		pl_color( stdout, 0, 0, 255 );	/* B */
		pdv_3line( stdout, a, b );
	}
#endif
	return(i);
}

/*
 *			R T _ E B M _ N O R M
 *
 *  Given one ray distance, return the normal and
 *  entry/exit point.
 *  This is mostly a matter of translating the stored
 *  code into the proper normal.
 */
void
rt_ebm_norm( hitp, stp, rp )
register struct hit	*hitp;
struct soltab		*stp;
register struct xray	*rp;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );

	switch( hitp->hit_surfno )  {
	case NORM_XPOS:
		VMOVE( hitp->hit_normal, ebmp->ebm_xnorm );
		break;
	case NORM_XNEG:
		VREVERSE( hitp->hit_normal, ebmp->ebm_xnorm );
		break;

	case NORM_YPOS:
		VMOVE( hitp->hit_normal, ebmp->ebm_ynorm );
		break;
	case NORM_YNEG:
		VREVERSE( hitp->hit_normal, ebmp->ebm_ynorm );
		break;

	case NORM_ZPOS:
		VMOVE( hitp->hit_normal, ebmp->ebm_znorm );
		break;
	case NORM_ZNEG:
		VREVERSE( hitp->hit_normal, ebmp->ebm_znorm );
		break;

	default:
		rt_log("ebm_norm(%s): surfno=%d bad\n",
			stp->st_name, hitp->hit_surfno );
		VSETALL( hitp->hit_normal, 0 );
		break;
	}
}

/*
 *			R T _ E B M _ C U R V E
 *
 *  Everything has sharp edges.  This makes things easy.
 */
void
rt_ebm_curve( cvp, hitp, stp )
register struct curvature	*cvp;
register struct hit		*hitp;
struct soltab			*stp;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;

	vec_ortho( cvp->crv_pdir, hitp->hit_normal );
	cvp->crv_c1 = cvp->crv_c2 = 0;
}

/*
 *			R T _ E B M _ U V
 *
 *  Map the hit point in 2-D into the range 0..1
 *  untransformed X becomes U, and Y becomes V.
 */
void
rt_ebm_uv( ap, stp, hitp, uvp )
struct application	*ap;
struct soltab		*stp;
register struct hit	*hitp;
register struct uvcoord	*uvp;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;

	/* XXX uv should be xy in ideal space */
}

/*
 * 			R T _ E B M _ F R E E
 */
void
rt_ebm_free( stp )
struct soltab	*stp;
{
	register struct ebm_specific *ebmp =
		(struct ebm_specific *)stp->st_specific;

	rt_free( (char *)ebmp->ebm_map, "ebm_map" );
	rt_free( (char *)ebmp, "ebm_specific" );
}

int
rt_ebm_class()
{
	return(0);
}

/*
 *			R T _ E B M _ P L O T
 */
int
rt_ebm_plot( rp, matp, vhead, dp )
union record	*rp;
matp_t		matp;
struct vlhead	*vhead;
struct directory *dp;
{
	register struct ebm_specific *ebmp;
	register int	x,y;
	register int	following;
	register int	base;

	if( (ebmp = rt_ebm_import( rp )) == EBM_NULL )
		return(-1);

	/* Find vertical lines */
	base = 0;	/* lint */
	for( x=0; x <= ebmp->ebm_xdim; x++ )  {
		following = 0;
		for( y=0; y <= ebmp->ebm_ydim; y++ )  {
			if( following )  {
				if( (BIT( x-1, y )==0) != (BIT( x, y )==0) )
					continue;
				rt_ebm_plate( x, base, x, y, ebmp->ebm_tallness,
					matp, vhead );
				following = 0;
			} else {
				if( (BIT( x-1, y )==0) == (BIT( x, y )==0) )
					continue;
				following = 1;
				base = y;
			}
		}
	}

	/* Find horizontal lines */
	for( y=0; y <= ebmp->ebm_ydim; y++ )  {
		following = 0;
		for( x=0; x <= ebmp->ebm_xdim; x++ )  {
			if( following )  {
				if( (BIT( x, y-1 )==0) != (BIT( x, y )==0) )
					continue;
				rt_ebm_plate( base, y, x, y, ebmp->ebm_tallness,
					matp, vhead );
				following = 0;
			} else {
				if( (BIT( x, y-1 )==0) == (BIT( x, y )==0) )
					continue;
				following = 1;
				base = x;
			}
		}
	}
	rt_free( (char *)ebmp->ebm_map, "ebm_map" );
	rt_free( (char *)ebmp, "ebm_specific" );
	return(0);
}

/* either x1==x2, or y1==y2 */
rt_ebm_plate( x1, y1, x2, y2, t, mat, vhead )
int			x1, y1;
int			x2, y2;
double			t;
register matp_t		mat;
register struct vlhead	*vhead;
{
	LOCAL point_t	s, p;
	LOCAL point_t	srot, prot;

	VSET( s, x1, y1, 0.0 );
	MAT4X3PNT( srot, mat, s );
	ADD_VL( vhead, srot, 0 );

	VSET( p, x1, y1, t );
	MAT4X3PNT( prot, mat, p );
	ADD_VL( vhead, prot, 1 );

	VSET( p, x2, y2, t );
	MAT4X3PNT( prot, mat, p );
	ADD_VL( vhead, prot, 1 );

	p[Z] = 0;
	MAT4X3PNT( prot, mat, p );
	ADD_VL( vhead, prot, 1 );

	ADD_VL( vhead, srot, 1 );
}

/******** Test Driver *********/
#ifdef TEST_DRIVER

FILE			*plotfp;

struct soltab		Tsolid;
struct directory	Tdir;
struct application	Tappl;
struct ebm_specific	*bmsp;
struct resource		resource;

main( argc, argv )
int	argc;
char	**argv;
{
	point_t	pt1;
	point_t	pt2;
	register int	x, y;
	fastf_t		xx, yy;
	mat_t		mat;
	register struct ebm_specific	*ebmp;
	int		arg;
	FILE		*fp;
	union record	rec;

	if( argc > 1 )  {
		rt_g.debug |= DEBUG_EBM;
		arg = atoi(argv[1]);
	}

	plotfp = fopen( "ebm.pl", "w" );

	Tdir.d_namep = "Tsolid";
	Tsolid.st_dp = &Tdir;
	Tappl.a_purpose = "testing";
	Tappl.a_resource = &resource;
	mat_idn( Tsolid.st_pathmat );

	strcpy( rec.ss.ss_str, "ebm file=bm.bw w=6 n=6 d=6.0" );

	if( rt_ebm_prep( &Tsolid, &rec, 0 ) != 0 )  {
		printf("prep failed\n");
		exit(1);
	}
	rt_ebm_print( &Tsolid );
	ebmp = bmsp = (struct ebm_specific *)Tsolid.st_specific;

	outline( Tsolid.st_pathmat, &rec );

#if 1
	if( (fp = fopen("ebm.rays", "r")) == NULL )  {
		perror("ebm.rays");
		exit(1);
	}
	for(;;)  {
		x = fscanf( fp, "%lf %lf %lf %lf %lf %lf\n",
			&pt1[X], &pt1[Y], &pt1[Z],
			&pt2[X], &pt2[Y], &pt2[Z] );
		if( x < 6 )  break;
		VADD2( pt2, pt2, pt1 );
		trial( pt1, pt2 );
	}
#endif
#if 0
	y = arg;
	for( x=0; x<=ebmp->ebm_xdim; x++ )  {
		VSET( pt1, 0, y, 1 );
		VSET( pt2, x, 0, 1 );
		trial( pt1, pt2 );
	}
#endif
#if 0
	y = arg;
	for( x=0; x<=ebmp->ebm_xdim; x++ )  {
		VSET( pt1, 0, y, 2 );
		VSET( pt2, x, 0, 4 );
		trial( pt1, pt2 );
	}
#endif
#if 0
	for( y=0; y<=ebmp->ebm_ydim; y++ )  {
		for( x=0; x<=ebmp->ebm_xdim; x++ )  {
			VSET( pt1, 0, y, 2 );
			VSET( pt2, x, 0, 4 );
			trial( pt1, pt2 );
		}
	}
#endif
#if 0
	for( y= -1; y<=ebmp->ebm_ydim; y++ )  {
		for( x= -1; x<=ebmp->ebm_xdim; x++ )  {
			VSET( pt1, x, y, 10 );
			VSET( pt2, x+2, y+3, -4 );
			trial( pt1, pt2 );
		}
	}
#endif
#if 0
	for( y=0; y<=ebmp->ebm_ydim; y++ )  {
		for( x=0; x<=ebmp->ebm_xdim; x++ )  {
			VSET( pt1, ebmp->ebm_xdim, y, 2 );
			VSET( pt2, x, ebmp->ebm_ydim, 4 );
			trial( pt1, pt2 );
		}
	}
#endif

#if 0
	for( yy = 2.0; yy < 6.0; yy += 0.3 )  {
		VSET( pt1, 0, yy, 2 );
		VSET( pt2, 6, 0, 4 );
		trial( pt1, pt2 );
	}
#endif
#if 0
	for( yy=0; yy<=ebmp->ebm_ydim; yy += 0.3 )  {
		for( xx=0; xx<=ebmp->ebm_xdim; xx += 0.3 )  {
			VSET( pt1, 0, yy, 2 );
			VSET( pt2, xx, 0, 4 );
			trial( pt1, pt2 );
		}
	}
#endif
#if 0
	for( yy=0; yy<=ebmp->ebm_ydim; yy += 0.3 )  {
		for( xx=0; xx<=ebmp->ebm_xdim; xx += 0.3 )  {
			VSET( pt1, ebmp->ebm_xdim, yy, 2 );
			VSET( pt2, xx, ebmp->ebm_ydim, 4 );
			trial( pt1, pt2 );
		}
	}
#endif

#if 0
	/* (6,5) (2,5) (3,4) (1.2,1.2)
	 * were especially troublesome */
	xx=6;
	yy=1.5;
	VSET( pt1, 0, yy, 2 );
	VSET( pt2, xx, 0, 4 );
	trial( pt1, pt2 );
#endif

#if 0
	/* (1,2) (3,2) (0,2) (0,0.3)
	 * were especially troublesome */
	xx=0;
	yy=0.3;
	VSET( pt1, ebmp->ebm_xdim, yy, 2 );
	VSET( pt2, xx, ebmp->ebm_ydim, 4 );
	trial( pt1, pt2 );
#endif

#if 0
	VSET( pt1, 0, 1.5, 2 );
	VSET( pt2, 4.75, 6, 4 );
	trial( pt1, pt2 );

	VSET( pt1, 0, 2.5, 2 );
	VSET( pt2, 4.5, 0, 4 );
	trial( pt1, pt2 );
#endif


#if 0
	/* With Z=-10, it works, but looks like trouble, due to
	 * the Z-clipping causing lots of green vectors on the 2d proj.
	 * VSET( pt1, 0.75, 1.1, -10 );
	 */
	VSET( pt1, 0.75, 1.1, 0 );
	{
		for( yy=0; yy<=ebmp->ebm_ydim; yy += 0.3 )  {
			for( xx=0; xx<=ebmp->ebm_xdim; xx += 0.3 )  {
				VSET( pt2, xx, yy, 4 );
				trial( pt1, pt2 );
			}
		}
	}
#endif
#if 0
	for( x=0; x<ebmp->ebm_xdim; x++ )  {
		VSET( pt1, x+0.75, 1.1, -10 );
		VSET( pt2, x+0.75, 1.1, 4 );
		trial( pt1, pt2 );
	}
#endif

	exit(0);
}

trial(p1, p2)
vect_t	p1, p2;
{
	struct seg	*segp, *next;
	fastf_t		lastk;
	struct xray	ray;
	register struct ebm_specific *ebmp = bmsp;

	VMOVE( ray.r_pt, p1 );
	VSUB2( ray.r_dir, p2, p1 );
	if( MAGNITUDE( ray.r_dir ) < 1.0e-10 )
		return;
	VUNITIZE( ray.r_dir );

	printf("------- (%g, %g, %g) to (%g, %g, %g), dir=(%g, %g, %g)\n",
		ray.r_pt[X], ray.r_pt[Y], ray.r_pt[Z],
		p2[X], p2[Y], p2[Z],
		ray.r_dir[X], ray.r_dir[Y], ray.r_dir[Z] );


	segp = rt_ebm_shot( &Tsolid, &ray, &Tappl );

	lastk = 0;
	while( segp != SEG_NULL )  {
		/* Draw 2-D segments */
		draw2seg( ray.r_pt, ray.r_dir, lastk, segp->seg_in.hit_dist, 0 );
		draw2seg( ray.r_pt, ray.r_dir, segp->seg_in.hit_dist, segp->seg_out.hit_dist, 1 );
		lastk = segp->seg_out.hit_dist;

		draw3seg( segp, ray.r_pt, ray.r_dir );

		next = segp->seg_next;
		FREE_SEG(segp, Tappl.a_resource);
		segp = next;
	}
}

outline(mat, rp)
mat_t	mat;
union record	*rp;
{
	register struct ebm_specific *ebmp = bmsp;
	register struct vlist	*vp;
	struct vlhead	vhead;

	vhead.vh_first = vhead.vh_last = VL_NULL;

	pl_3space( plotfp, -BIT_XWIDEN,-BIT_YWIDEN,-BIT_XWIDEN,
		 ebmp->ebm_xdim+BIT_XWIDEN, ebmp->ebm_ydim+BIT_YWIDEN, (int)(ebmp->ebm_tallness+1.99) );
	pl_3box( plotfp, -BIT_XWIDEN,-BIT_YWIDEN,-BIT_XWIDEN,
		 ebmp->ebm_xdim+BIT_XWIDEN, ebmp->ebm_ydim+BIT_YWIDEN, (int)(ebmp->ebm_tallness+1.99) );

	/* Get vlist, then just draw the vlist */
	rt_ebm_plot( rp, mat, &vhead, 0 );

	for( vp = vhead.vh_first; vp != VL_NULL; vp = vp->vl_forw )  {
		if( vp->vl_draw == 0 )
			pdv_3move( plotfp, vp->vl_pnt );
		else
			pdv_3cont( plotfp, vp->vl_pnt );
	}
	FREE_VL( vhead.vh_first );
}


draw2seg( pt, dir, k1, k2, inside )
vect_t	pt, dir;
fastf_t	k1, k2;
int	inside;
{
	vect_t	a, b;

	a[0] = pt[0] + k1 * dir[0];
	a[1] = pt[1] + k1 * dir[1];
	a[2] = 0;
	b[0] = pt[0] + k2 * dir[0];
	b[1] = pt[1] + k2 * dir[1];
	b[2] = 0;

	if( inside )
		pl_color( plotfp, 255, 0, 0 );	/* R */
	else
		pl_color( plotfp, 0, 255, 0 );	/* G */
	pdv_3line( plotfp, a, b );
}

draw3seg( segp, pt, dir )
register struct seg	*segp;
vect_t	pt, dir;
{
	vect_t	a, b;

	VJOIN1( a, pt, segp->seg_in.hit_dist, dir );
	VJOIN1( b, pt, segp->seg_out.hit_dist, dir );
	pl_color( plotfp, 0, 0, 255 );	/* B */
	pdv_3line( plotfp, a, b );
}
#endif /* test driver */
