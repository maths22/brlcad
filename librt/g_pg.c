/*
 *			P G . C
 *
 *  Function -
 *	Intersect a ray with a Polygonal Object
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
static char RCSpg[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"
#include "./plane.h"

#define TRI_NULL	((struct tri_specific *)0)

/* Describe algorithm here */

#define PGMINMAX(a,b,c)	{ FAST fastf_t ftemp;\
			if( (ftemp = (c)) < (a) )  a = ftemp;\
			if( ftemp > (b) )  b = ftemp; }

/*
 *			P G _ P R E P
 *  
 *  This routine is used to prepare a list of planar faces for
 *  being shot at by the triangle routines.
 *
 * Process a PG, which is represented as a vector
 * from the origin to the first point, and many vectors
 * from the first point to the remaining points.
 *  
 *  This routine is unusual in that it has to read additional
 *  database records to obtain all the necessary information.
 */
pg_prep( vecxx, stp, mat, sp, rtip )
fastf_t		*vecxx;
struct soltab	*stp;
matp_t		mat;
struct solidrec	*sp;
struct rt_i	*rtip;
{
	LOCAL union record rec;
	LOCAL fastf_t dx, dy, dz;	/* For finding the bounding spheres */
	LOCAL struct tri_specific *plp;
	LOCAL fastf_t f;
	LOCAL vect_t work;
	register int	i;

	for( ;; )  {
		i = fread( (char *)&rec, sizeof(rec), 1, rtip->fp );
		if( feof(rtip->fp) )
			break;
		if( i != 1 )
			rt_bomb("pg_prep():  read error");
		if( rec.u_id != ID_P_DATA )  break;
		if( rec.q.q_count != 3 )  {
			rt_log("pg_prep(%s):  q_count = %d\n",
				stp->st_name, rec.q.q_count);
			return(-1);		/* BAD */
		}
		for( i=0; i < rec.q.q_count; i++ )  {
			MAT4X3PNT( work, mat, rec.q.q_verts[i] );
			VMOVE( rec.q.q_verts[i], work );
			PGMINMAX( stp->st_min[X], stp->st_max[X], work[X] );
			PGMINMAX( stp->st_min[Y], stp->st_max[Y], work[Y] );
			PGMINMAX( stp->st_min[Z], stp->st_max[Z], work[Z] );
		}
		(void)pgface( stp,
			&(rec.q.q_verts[0][X]),
			&(rec.q.q_verts[1][X]),
			&(rec.q.q_verts[2][X]),
			&(rec.q.q_norms[0][X]) );
	}
	if( stp->st_specific == 0 )  {
		rt_log("pg(%s):  no faces\n", stp->st_name);
		return(-1);		/* BAD */
	}
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

	return(0);		/* OK */
}

/*
 *			P G F A C E
 *
 *  This function is called with pointers to 3 points,
 *  and is used to prepare PG faces.
 *  ap, bp, cp point to vect_t points.
 *
 * Return -
 *	0	if the 3 points didn't form a plane (eg, colinear, etc).
 *	#pts	(3) if a valid plane resulted.
 */
pgface( stp, ap, bp, cp, np )
struct soltab *stp;
float *ap, *bp, *cp;
float *np;
{
	register struct tri_specific *trip;
	vect_t work;
	LOCAL fastf_t m1, m2, m3, m4;

	GETSTRUCT( trip, tri_specific );
	VMOVE( trip->tri_A, ap );
	VSUB2( trip->tri_BA, bp, ap );
	VSUB2( trip->tri_CA, cp, ap );
	VCROSS( trip->tri_wn, trip->tri_BA, trip->tri_CA );

	/* Check to see if this plane is a line or pnt */
	m1 = MAGNITUDE( trip->tri_BA );
	m2 = MAGNITUDE( trip->tri_CA );
	VSUB2( work, bp, cp );
	m3 = MAGNITUDE( work );
	m4 = MAGNITUDE( trip->tri_wn );
	if( NEAR_ZERO(m1, 0.0001) || NEAR_ZERO(m2, 0.0001) ||
	    NEAR_ZERO(m3, 0.0001) || NEAR_ZERO(m4, 0.0001) )  {
		free( (char *)trip);
		if( rt_g.debug & DEBUG_ARB8 )
			(void)rt_log("pg(%s): degenerate facet\n", stp->st_name);
		return(0);			/* BAD */
	}		

	/*  wn is a GIFT-style
	 *  normal, needing to point inwards, whereas N is an outward
	 *  pointing normal.
	 *  Eventually, N should be computed as a blend of the given normals.
	 */
	m3 = MAGNITUDE( np );
	if( !NEAR_ZERO( m3, 0.0001 ) )  {
		VMOVE( trip->tri_N, np );
		m3 = 1 / m3;
		VSCALE( trip->tri_N, trip->tri_N, m3 );
	} else {
		VREVERSE( trip->tri_N, trip->tri_wn );
		VUNITIZE( trip->tri_N );
	}

	/* Add this face onto the linked list for this solid */
	trip->tri_forw = (struct tri_specific *)stp->st_specific;
	stp->st_specific = (int *)trip;
	return(3);				/* OK */
}

/*
 *  			P G _ P R I N T
 */
pg_print( stp )
register struct soltab *stp;
{
	register struct tri_specific *trip =
		(struct tri_specific *)stp->st_specific;
	register int i;

	if( trip == TRI_NULL )  {
		rt_log("pg(%s):  no faces\n", stp->st_name);
		return;
	}
	do {
		VPRINT( "A", trip->tri_A );
		VPRINT( "B-A", trip->tri_BA );
		VPRINT( "C-A", trip->tri_CA );
		VPRINT( "BA x CA", trip->tri_wn );
		VPRINT( "Normal", trip->tri_N );
		rt_log("\n");
	} while( trip = trip->tri_forw );
}

/*
 *			P G _ S H O T
 *  
 * Function -
 *	Shoot a ray at a polygonal object.
 *  
 * Returns -
 *	0	MISS
 *  	segp	HIT
 */
struct seg *
pg_shot( stp, rp, ap )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
{
	register struct tri_specific *trip =
		(struct tri_specific *)stp->st_specific;
#define MAXHITS 12		/* # surfaces hit, must be even */
	LOCAL struct hit hits[MAXHITS];
	register struct hit *hp;
	LOCAL int	nhits;

	nhits = 0;
	hp = &hits[0];

	/* consider each face */
	for( ; trip; trip = trip->tri_forw )  {
		FAST fastf_t	dn;		/* Direction dot Normal */
		LOCAL fastf_t	abs_dn;
		FAST fastf_t	k;
		LOCAL fastf_t	alpha, beta;
		LOCAL fastf_t	ds;
		LOCAL vect_t	wxb;		/* vertex - ray_start */
		LOCAL vect_t	xp;		/* wxb cross ray_dir */

		/*
		 *  Ray Direction dot N.  (N is outward-pointing normal)
		 */
		dn = VDOT( trip->tri_wn, rp->r_dir );
		if( rt_g.debug & DEBUG_ARB8 )
			rt_log("Face N.Dir=%f\n", dn );
		/*
		 *  If ray lies directly along the face, drop this face.
		 */
		abs_dn = dn >= 0.0 ? dn : (-dn);
		if( abs_dn <= 0.0001 )
			continue;
		VSUB2( wxb, trip->tri_A, rp->r_pt );
		VCROSS( xp, wxb, rp->r_dir );

		/* Check for exceeding along the one side */
		alpha = VDOT( trip->tri_CA, xp );
		if( dn < 0.0 )  alpha = -alpha;
		if( alpha < 0.0 || alpha > abs_dn )
			continue;

		/* Check for exceeding along the other side */
		beta = VDOT( trip->tri_BA, xp );
		if( dn > 0.0 )  beta = -beta;
		if( beta < 0.0 || beta > abs_dn )
			continue;
		if( alpha+beta > abs_dn )
			continue;
		ds = VDOT( wxb, trip->tri_wn );
		k = ds / dn;		/* shot distance */

		/* For hits other than the first one, might check
		 *  to see it this is approx. equal to previous one */

		/*  If dn < 0, we should be entering the solid.
		 *  However, we just assume in/out sorting later will work.
		 *  Really should mark and check this!
		 */
		VJOIN1( hp->hit_point, rp->r_pt, k, rp->r_dir );

		/* HIT is within planar face */
		hp->hit_dist = k;
		VMOVE( hp->hit_normal, trip->tri_N );
		if(rt_g.debug&DEBUG_ARB8) rt_log("pg: hit dist=%f, dn=%f, k=%f\n", hp->hit_dist, dn, k );
		if( nhits++ >= MAXHITS )  {
			rt_log("pg(%s): too many hits\n", stp->st_name);
			break;
		}
		hp++;
	}
	if( nhits == 0 )
		return(SEG_NULL);		/* MISS */

	/* Sort hits, Near to Far */
	pg_hit_sort( hits, nhits );

	if( nhits&1 )  {
		register int i;
		/*
		 * If this condition exists, it is almost certainly due to
		 * the dn==0 check above.  Thus, we will make the last
		 * surface infinitely thin and just replicate the entry
		 * point as the exit point.  This at least makes the
		 * presence of this solid known.  There may be something
		 * better we can do.
		 */
		hits[nhits] = hits[nhits-1];	/* struct copy */
		VREVERSE( hits[nhits].hit_normal, hits[nhits-1].hit_normal );
		hits[nhits].hit_dist += 0.1;	/* mm thick */
		rt_log("ERROR: pg(%s): %d hits, false exit\n",
			stp->st_name, nhits);
		nhits++;
		for(i=0; i < nhits; i++ )
			rt_log("%f, ", hits[i].hit_dist );
		rt_log("\n");
	}

	/* nhits is even, build segments */
	{
		register struct seg *segp;			/* XXX */
		segp = SEG_NULL;
		while( nhits > 0 )  {
			register struct seg *newseg;		/* XXX */
			GET_SEG(newseg, ap->a_resource);
			newseg->seg_next = segp;
			segp = newseg;
			segp->seg_stp = stp;
			segp->seg_in = hits[nhits-2];	/* struct copy */
			segp->seg_out = hits[nhits-1];	/* struct copy */
			nhits -= 2;
		}
		return(segp);			/* HIT */
	}
	/* NOTREACHED */
}

pg_hit_sort( h, nh )
register struct hit h[];
register int nh;
{
	register int i, j;
	LOCAL struct hit temp;

	for( i=0; i < nh-1; i++ )  {
		for( j=i+1; j < nh; j++ )  {
			if( h[i].hit_dist <= h[j].hit_dist )
				continue;
			temp = h[j];		/* struct copy */
			h[j] = h[i];		/* struct copy */
			h[i] = temp;		/* struct copy */
		}
	}
}

/*
 *			P G _ F R E E
 */
pg_free( stp )
struct soltab *stp;
{
	register struct tri_specific *trip =
		(struct tri_specific *)stp->st_specific;

	while( trip != TRI_NULL )  {
		register struct tri_specific *nexttri = trip->tri_forw;

		rt_free( (char *)trip, "pg tri_specific");
		trip = nexttri;
	}
}

pg_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	/* Normals computed in pg_shot, nothing to do here */
}

pg_uv()
{
}

pg_class()
{
}

pg_plot()
{
}

pg_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
{
	rt_orthovec( cvp->crv_pdir, hitp->hit_normal );
	cvp->crv_c1 = cvp->crv_c2 = 0;
}

