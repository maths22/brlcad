/*
 *			G _ E P A . C
 *
 *  Purpose -
 *	Librt Geometry Routines for the Elliptical Paraboloid
 *
 *  Authors -
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSsph[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"

struct epa_internal {
	vect_t	V;		/* Vertex */
	vect_t	H;		/* Height Vector */
	vect_t	A;		/* Semi-major axis (unit vector) */
	fastf_t	R1;		/* semi-major axis length */
	fastf_t	R2;		/* semi-minor axis length */
};

struct epa_specific {
	vect_t	epa_V;
};

/*
 *  			E P A _ P R E P
 *  
 *  Given a pointer to a GED database record, and a transformation matrix,
 *  determine if this is a valid EPA, and if so, precompute various
 *  terms of the formula.
 *  
 *  Returns -
 *  	0	EPA is OK
 *  	!0	Error in description
 *  
 *  Implicit return -
 *  	A struct epa_specific is created, and it's address is stored in
 *  	stp->st_specific for use by epa_shot().
 */
int
epa_prep( stp, rec, rtip )
struct soltab		*stp;
struct rt_i		*rtip;
union record		*rec;
{
	register struct epa_specific *epa;
}

void
epa_print( stp )
register struct soltab *stp;
{
	register struct epa_specific *epa =
		(struct epa_specific *)stp->st_specific;
}

/*
 *  			E P A _ S H O T
 *  
 *  Intersect a ray with a epa.
 *  If an intersection occurs, a struct seg will be acquired
 *  and filled in.
 *  
 *  Returns -
 *  	0	MISS
 *  	segp	HIT
 */
struct seg *
epa_shot( stp, rp, ap )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
{
	register struct epa_specific *epa =
		(struct epa_specific *)stp->st_specific;
	register struct seg *segp;

	return(segp);			/* HIT */
}

#define SEG_MISS(SEG)		(SEG).seg_stp=(struct soltab *) 0;	

/*
 *			S P H _ V S H O T
 *
 *  Vectorized version.
 */
void
epa_vshot( stp, rp, segp, n, resp)
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		  	    n; /* Number of ray/object pairs */
struct resource         *resp; /* pointer to a list of free segs */
{
	register struct epa_specific *epa;
}

/*
 *  			E P A _ N O R M
 *  
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
void
epa_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct epa_specific *epa =
		(struct epa_specific *)stp->st_specific;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
}

/*
 *			E P A _ C U R V E
 *
 *  Return the curvature of the epa.
 */
void
epa_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
{
	register struct epa_specific *epa =
		(struct epa_specific *)stp->st_specific;

 	cvp->crv_c1 = cvp->crv_c2 = 0;

	/* any tangent direction */
 	vec_ortho( cvp->crv_pdir, hitp->hit_normal );
}

/*
 *  			E P A _ U V
 *  
 *  For a hit on the surface of an epa, return the (u,v) coordinates
 *  of the hit point, 0 <= u,v <= 1.
 *  u = azimuth
 *  v = elevation
 */
void
epa_uv( ap, stp, hitp, uvp )
struct application *ap;
struct soltab *stp;
register struct hit *hitp;
register struct uvcoord *uvp;
{
	register struct epa_specific *epa =
		(struct epa_specific *)stp->st_specific;
}

/*
 *		E P A _ F R E E
 */
void
epa_free( stp )
register struct soltab *stp;
{
	register struct epa_specific *epa =
		(struct epa_specific *)stp->st_specific;

	rt_free( (char *)epa, "epa_specific" );
}

int
epa_class()
{
	return(0);
}

void
epa_plot()
{
}

int
epa_import( epa, rp, matp )
struct epa_internal	*epa;
union record		*rp;
register matp_t		matp;
{
	return(0);			/* OK */
}
