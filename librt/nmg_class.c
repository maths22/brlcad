/*
 *			N M G _ C L A S S . C
 *
 *  Subroutines to classify one object with respect to another.
 *  Possible classifications are AinB, AoutB, AinBshared, AinBanti.
 *
 *  The first set of routines (nmg_class_pt_xxx) are used to classify
 *  an arbitrary point specified by it's Cartesian coordinates,
 *  against various kinds of NMG elements.
 *  nmg_class_pt_f() and nmg_class_pt_s() are available to
 *  applications programmers for direct use, and have no side effects.
 *
 *  The second set of routines (class_xxx_vs_s) are used only to support
 *  the routine nmg_class_shells() mid-way through the NMG Boolean
 *  algorithm.  These routines operate with special knowledge about
 *  the state of the data structures after the intersector has been called,
 *  and depends on all geometric equivalences to have been converted into
 *  shared topology.
 *
 *  Authors -
 *	Lee A. Butler
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1993 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "externs.h"
#include "machine.h"
#include "vmath.h"
#include "nmg.h"
#include "rtlist.h"
#include "raytrace.h"
#include "./debug.h"

extern int nmg_class_nothing_broken;

/* XXX These should go the way of the dodo bird. */
#define INSIDE	32
#define ON_SURF	64
#define OUTSIDE	128

/*	Structure for keeping track of how close a point/vertex is to
 *	its potential neighbors.
 */
struct neighbor {
	union {
		CONST struct vertexuse *vu;
		CONST struct edgeuse *eu;
	} p;
	/* XXX For efficiency, this should have been dist_sq */
	fastf_t	dist;	/* distance from point to neighbor */
	int	class;	/* Classification of this neighbor */
};

static void	joint_hitmiss2 RT_ARGS( (struct neighbor *closest,
			CONST struct edgeuse *eu, CONST point_t pt,
			int code) );
static void	nmg_class_pt_e RT_ARGS( (struct neighbor *closest,
			CONST point_t pt, CONST struct edgeuse *eu,
			CONST struct rt_tol *tol) );
static void	nmg_class_pt_l RT_ARGS( (struct neighbor *closest, 
			CONST point_t pt, CONST struct loopuse *lu,
			CONST struct rt_tol *tol) );
static int	class_vu_vs_s RT_ARGS( (struct vertexuse *vu, struct shell *sB,
			long *classlist[4], CONST struct rt_tol	*tol) );
static int	class_eu_vs_s RT_ARGS( (struct edgeuse *eu, struct shell *s,
			long *classlist[4], CONST struct rt_tol	*tol) );
static int	class_lu_vs_s RT_ARGS( (struct loopuse *lu, struct shell *s,
			long *classlist[4], CONST struct rt_tol	*tol) );
static void	class_fu_vs_s RT_ARGS( (struct faceuse *fu, struct shell *s,
			long *classlist[4], CONST struct rt_tol	*tol) );

static CONST vect_t projection_dir = { 1.0, 0.0, 0.0 };

/*
 *			N M G _ C L A S S _ S T A T U S
 *
 *  Convert classification status to string.
 */
CONST char *
nmg_class_status(status)
int	status;
{
	switch(status)  {
	case INSIDE:
		return "INSIDE";
	case OUTSIDE:
		return "OUTSIDE";
	case ON_SURF:
		return "ON_SURF";
	}
	return "BOGUS_CLASS_STATUS";
}

/*
 *			N M G _ P R _ C L A S S _ S T A T U S
 */
void
nmg_pr_class_status( prefix, status )
char	*prefix;
int	status;
{
	rt_log("%s has classification status %s\n",
		prefix, nmg_class_status(status) );
}

/*
 *			J O I N T _ H I T M I S S 2
 *
 *	The ray hit an edge.  We have to decide whether it hit the
 *	edge, or missed it.
 */
static void joint_hitmiss2(closest, eu, pt, code)
struct neighbor		*closest;
CONST struct edgeuse	*eu;
CONST point_t		pt;
int			code;
{
	CONST struct edgeuse *eu_rinf;

	eu_rinf = nmg_faceradial(eu);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("joint_hitmiss2\n");
	}
	if( eu_rinf == eu )  {
		rt_bomb("joint_hitmiss2: radial eu is me?\n");
	}
	/* If eu_rinf == eu->eumate_p, thats OK, this is a dangling face,
	 * or a face that has not been fully hooked up yet.
	 * It's OK as long the the orientations both match.
	 */
	if( eu->up.lu_p->orientation == eu_rinf->up.lu_p->orientation ) {
		if (eu->up.lu_p->orientation == OT_SAME) {
			closest->class = NMG_CLASS_AonBshared;
		} else if (eu->up.lu_p->orientation == OT_OPPOSITE) {
			closest->class = NMG_CLASS_AoutB;
		} else rt_bomb("joint_hitmiss2: bad loop orientation\n");
		closest->dist = 0.0;
		switch(code)  {
		case 0:
			/* Hit the edge */
			closest->p.eu = eu;
			break;
		case 1:
			/* Hit first vertex */
			closest->p.vu = eu->vu_p;
			break;
		case 2:
			/* Hit second vertex */
			closest->p.vu = RT_LIST_PNEXT_CIRC(edgeuse,eu)->vu_p;
			break;
		}
		if (rt_g.NMG_debug & DEBUG_CLASSIFY) rt_log("\t\t%s\n", nmg_class_name(closest->class) );
	    	return;
	}

	/* XXX What goes here? */
	rt_bomb("nmg_class.c/joint_hitmiss2() unable to resolve ray/edge hit\n");
	rt_log("joint_hitmiss2: NO CODE HERE, assuming miss\n");

	if (rt_g.NMG_debug & (DEBUG_CLASSIFY|DEBUG_NMGRT) )  {
		EUPRINT("Hard question time", eu);
		rt_log(" eu_rinf=x%x, eu->eumate_p=x%x, eu=x%x\n", eu_rinf, eu->eumate_p, eu);
		rt_log(" eu lu orient=%s, eu_rinf lu orient=%s\n",
			nmg_orientation(eu->up.lu_p->orientation),
			nmg_orientation(eu_rinf->up.lu_p->orientation) );
	}
}

/*
 *			N M G _ C L A S S _ P T _ E
 *
 *  Given the Cartesian coordinates of a point, determine if the point
 *  is closer to this edgeuse than the previous neighbor(s) as given
 *  in the "closest" structure.
 *  If it is, record how close the point is, and whether it is IN, ON, or OUT.
 *  The neighor's "p" element will indicate the edgeuse or vertexuse closest.
 *
 *  This routine should print everything indented two tab stops.
 *
 *  Implicit Return -
 *	Updated "closest" structure if appropriate.
 */
static void
nmg_class_pt_e(closest, pt, eu, tol)
struct neighbor		*closest;
CONST point_t		pt;
CONST struct edgeuse	*eu;
CONST struct rt_tol	*tol;
{
	vect_t	N,	/* plane normal */
		euvect,	/* vector of edgeuse */
		ptvec;	/* vector from lseg to pt */
	vect_t	left;	/* vector left of edge -- into inside of loop */
	CONST fastf_t	*eupt;
	CONST fastf_t	*matept;
	point_t pca;	/* point of closest approach from pt to edge lseg */
	fastf_t dist;	/* distance from pca to pt */
	fastf_t dot, mag;
	int	code;

	NMG_CK_EDGEUSE(eu);
	NMG_CK_EDGEUSE(eu->eumate_p);
	NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
	NMG_CK_VERTEX_G(eu->eumate_p->vu_p->v_p->vg_p);
	RT_CK_TOL(tol);
	VSETALL(left, 0);

	eupt = eu->vu_p->v_p->vg_p->coord;
	matept = eu->eumate_p->vu_p->v_p->vg_p->coord;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		VPRINT("nmg_class_pt_e\tPt", pt);
		EUPRINT("          \tvs. eu", eu);
	}
	/*
	 * Note that "pca" can be one of the edge endpoints,
	 * even if "pt" is far, far away.  This can be confusing.
	 */
	code = rt_dist_pt3_lseg3( &dist, pca, eupt, matept, pt, tol);
	if( code <= 0 )  dist = 0;
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("          \tcode=%d, dist: %g\n", code, dist);
		VPRINT("          \tpca:", pca);
	}

	if (dist >= closest->dist + tol->dist ) {
 		if(rt_g.NMG_debug & DEBUG_CLASSIFY) {
 			rt_log("\t\tskipping, earlier eu is closer (%g)\n", closest->dist);
 		}
		return;
 	}
 	if( dist >= closest->dist - tol->dist )  {
 		/*
 		 *  Distances are very nearly the same.
 		 *  If "closest" result so far was a NMG_CLASS_AinB or
		 *  or NMG_CLASS_AonB, then keep it,
 		 *  otherwise, replace that result with whatever we find
 		 *  here.  Logic:  Two touching loops, one concave ("A")
		 *  which wraps around part of the other ("B"), with the
 		 *  point inside A near the contact with B.  If loop B is
		 *  processed first, the closest result will be NMG_CLASS_AoutB,
 		 *  and when loop A is visited the distances will be exactly
 		 *  equal, not giving A a chance to claim it's hit.
 		 */
 		if( closest->class == NMG_CLASS_AinB ||
		    closest->class == NMG_CLASS_AonBshared )  {
	 		if(rt_g.NMG_debug & DEBUG_CLASSIFY)
				rt_log("\t\tSkipping, earlier eu at same dist, is IN or ON\n");
 			return;
 		}
 		if(rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("\t\tEarlier eu at same dist, is OUT, continue processing.\n");
 	}

	/* Plane hit point is closer to this edgeuse than previous one(s) */
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("\t\tCloser. dist=%g (closest=%g), tol=%g\n",
			dist, closest->dist, tol->dist);
	}

	if (*eu->up.magic_p != NMG_LOOPUSE_MAGIC ||
	    *eu->up.lu_p->up.magic_p != NMG_FACEUSE_MAGIC) {
		rt_log("Trying to classify a pt (%g, %g, %g)\n\tvs a wire edge? (%g, %g, %g -> %g, %g, %g)\n",
	    		V3ARGS(pt),
	    		V3ARGS(eupt),
	    		V3ARGS(matept)
		);
	    	return;
	}

	if( code <= 2 )  {
		/* code==0:  The point is ON the edge! */
		/* code==1 or 2:  The point is ON a vertex! */
    		if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
    			rt_log("\t\tThe point is ON the edge, calling joint_hitmiss2()\n");
    		}
   		joint_hitmiss2(closest, eu, pt, code);
    		goto out;
    	} else {
    		if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
			rt_log("\t\tThe point is not on the edge\n");
    		}
    	}

	/* The point did not lie exactly ON the edge */
	/* calculate in/out */
	VMOVE(N, eu->up.lu_p->up.fu_p->f_p->fg_p->N);
    	if (eu->up.lu_p->up.fu_p->orientation != OT_SAME) {
    		if (rt_g.NMG_debug & DEBUG_CLASSIFY) rt_log("\t\tReversing normal\n");
		VREVERSE(N,N);
    	}

	VSUB2(euvect, matept, eupt);
    	if (rt_g.NMG_debug & DEBUG_CLASSIFY) VPRINT("\t\teuvect unnorm", euvect);
	VUNITIZE(euvect);

    	/* Get vector which lies on the plane, and points
    	 * left, towards the interior of the CCW loop.
    	 */
    	VCROSS( left, N, euvect );	/* left vector */
	if(eu->up.lu_p->orientation != OT_SAME )  {
		if (rt_g.NMG_debug & DEBUG_CLASSIFY) rt_log("\t\tReversing left vec\n");
		VREVERSE(left, left);
	}

	VSUB2(ptvec, pt, pca);		/* pt - pca */
    	if (rt_g.NMG_debug & DEBUG_CLASSIFY) VPRINT("\t\tptvec unnorm", ptvec);
	VUNITIZE( ptvec );

	dot = VDOT(left, ptvec);
	if( NEAR_ZERO( dot, RT_DOT_TOL )  )  {
	    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
	    		rt_log("\t\tpt lies on line of edge, outside verts. Skipping this edge\n");
		goto out;
	}

	if (dot >= 0.0) {
		closest->dist = dist;
		closest->p.eu = eu;
		closest->class = NMG_CLASS_AinB;
	    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
	    		rt_log("\t\tpt is left of edge, INSIDE loop, dot=%g\n", dot);
	} else {
		closest->dist = dist;
		closest->p.eu = eu;
		closest->class = NMG_CLASS_AoutB;
	    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
	    		rt_log("\t\tpt is right of edge, OUTSIDE loop\n");
	}

out:
	/* XXX Temporary addition for chasing bug in Bradley r5 */
	/* XXX Should at least add DEBUG_PLOTEM check, later */
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		struct faceuse	*fu;
		char	buf[128];
		static int	num;
		FILE	*fp;
		long	*bits;
		point_t	mid_pt;
		point_t	left_pt;
		fu = eu->up.lu_p->up.fu_p;
		bits = (long *)rt_calloc( nmg_find_model(&fu->l.magic)->maxindex, sizeof(long), "bits[]");
		sprintf(buf,"faceclass%d.pl", num++);
		if( (fp = fopen(buf, "w")) == NULL) rt_bomb(buf);
		nmg_pl_fu( fp, fu, bits, 0, 0, 255 );	/* blue */
		pl_color( fp, 0, 255, 0 );	/* green */
		pdv_3line( fp, pca, pt );
		pl_color( fp, 255, 0, 0 );	/* red */
		VADD2SCALE( mid_pt, eupt, matept, 0.5 );
		VJOIN1( left_pt, mid_pt, 500, left);
		pdv_3line( fp, mid_pt, left_pt );
		fclose(fp);
		rt_free( (char *)bits, "bits[]");
		rt_log("wrote %s\n", buf);
	}
}


/*
 *			N M G _ C L A S S _ P T _ L
 *
 *  Given the coordinates of a point which lies on the plane of the face
 *  containing a loopuse, determine if the point is IN, ON, or OUT of the
 *  area enclosed by the loop.
 *
 *  Implicit Return -
 *	Updated "closest" structure if appropriate.
 */
static void
nmg_class_pt_l(closest, pt, lu, tol)
struct neighbor		*closest;
CONST point_t		pt;
CONST struct loopuse	*lu;
CONST struct rt_tol	*tol;
{
	vect_t		delta;
	pointp_t	lu_pt;
	fastf_t		dist;
	struct edgeuse	*eu;
	struct loop_g	*lg;

	NMG_CK_LOOPUSE(lu);
	NMG_CK_LOOP(lu->l_p);
	lg = lu->l_p->lg_p;
	NMG_CK_LOOP_G(lg);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		VPRINT("nmg_class_pt_l\tPt:", pt);

	if (*lu->up.magic_p != NMG_FACEUSE_MAGIC)
		return;

	if( !V3PT_IN_RPP_TOL( pt, lg->min_pt, lg->max_pt, tol ) )  {
		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("\tPoint is outside loop RPP\n");
		return;
	}
	if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_EDGEUSE_MAGIC) {
		for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
			nmg_class_pt_e(closest, pt, eu, tol);
			/* If point lies ON edge, we are done */
			if( closest->class == NMG_CLASS_AonBshared )  break;
		}
	} else if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC) {
		register struct vertexuse *vu;
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		lu_pt = vu->v_p->vg_p->coord;
		VSUB2(delta, pt, lu_pt);
		if ( (dist = MAGNITUDE(delta)) < closest->dist) {
			if (lu->orientation == OT_OPPOSITE) {
				closest->class = NMG_CLASS_AoutB;
			} else if (lu->orientation == OT_SAME) {
				closest->class = NMG_CLASS_AonBshared;
			} else {
				nmg_pr_orient(lu->orientation, "\t");
				rt_bomb("nmg_class_pt_l: bad orientation for face loopuse\n");
			}
			if (rt_g.NMG_debug & DEBUG_CLASSIFY)
				rt_log("\t\t closer to loop pt (%g, %g, %g)\n",
					V3ARGS(lu_pt) );

			closest->dist = dist;
			closest->p.vu = vu;
		}
	} else {
		rt_bomb("nmg_class_pt_l() bad child of loopuse\n");
	}
	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("nmg_class_pt_l\treturning, closest=%g %s\n",
			closest->dist, nmg_class_name(closest->class) );
}

/*
 *			N M G _ C L A S S _ P T _ F
 *
 *  This is intended as a general user interface routine.
 *  Given the Cartesian coordinates for a point which is known to
 *  lie on a face, return the classification for that point
 *  with respect to all the loops on that face.
 *
 *  The algorithm used is to find the edge which the point is closest
 *  to, and classifiy with respect to that.
 *
 *  The point is "A", and the face is "B".
 *
 *  Returns -
 *	NMG_CLASS_AinB		pt is INSIDE the area of the face.
 *	NMG_CLASS_AonBshared	pt is ON a loop boundary.
 *	NMG_CLASS_AoutB		pt is OUTSIDE the area of the face.
 */
int
nmg_class_pt_f(pt, fu, tol)
CONST point_t		pt;
CONST struct faceuse	*fu;
CONST struct rt_tol	*tol;
{
	struct loopuse *lu;
	struct neighbor closest;
	fastf_t		dist;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		VPRINT("nmg_class_pt_f\tPt:", pt);
	}
	NMG_CK_FACEUSE(fu);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_FACE_G(fu->f_p->fg_p);
	RT_CK_TOL(tol);

	/* Validate distance from point to plane */
	if( (dist=fabs(DIST_PT_PLANE( pt, fu->f_p->fg_p->N ))) > tol->dist )  {
		rt_log("nmg_class_pt_f() ERROR, point (%g,%g,%g) not on face, dist=%g\n",
			V3ARGS(pt), dist );
	}

	/* find the closest approach in this face to the projected point */
	closest.dist = MAX_FASTF;
	closest.p.eu = (struct edgeuse *)NULL;
	closest.class = NMG_CLASS_AoutB;	/* default return */

	for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd)) {
		nmg_class_pt_l( &closest, pt, lu, tol );
		/* If point lies ON loop edge, we are done */
		if( closest.class == NMG_CLASS_AonBshared )  break;
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("nmg_class_pt_f\tdist=%g, return=%s\n",
			closest.dist,
			nmg_class_name(closest.class) );
	}
	return closest.class;
}


/*
 *			N M G _ C L A S S _ P T _ S
 *
 *  This is intended as a general user interface routine.
 *  Given the Cartesian coordinates for a point in space,
 *  return the classification for that point with respect to a shell.
 *
 *  The algorithm used is to fire a ray from the point, and count
 *  the number of times it crosses a face.
 *
 *  The point is "A", and the face is "B".
 *
 *  Returns -
 *	NMG_CLASS_AinB		pt is INSIDE the volume of the shell.
 *	NMG_CLASS_AonBshared	pt is ON the shell boundary.
 *	NMG_CLASS_AoutB		pt is OUTSIDE the volume of the shell.
 */
int
nmg_class_pt_s(pt, s, tol)
CONST point_t		pt;
CONST struct shell	*s;
CONST struct rt_tol	*tol;
{
	int		hitcount = 0;
	int		stat;
	fastf_t 	dist;
	point_t 	plane_pt;
	CONST struct faceuse	*fu;
	struct model	*m;
	long		*faces_seen;
	vect_t		region_diagonal;
	fastf_t		region_diameter;
	int		class;

	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("nmg_class_pt_s:\tpt=(%g, %g, %g), s=x%x\n",
			V3ARGS(pt), s );

	if( !V3PT_IN_RPP_TOL(pt, s->sa_p->min_pt, s->sa_p->max_pt, tol) )  {
		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("	OUT, point not in RPP\n");
		return NMG_CLASS_AoutB;
	}

	m = s->r_p->m_p;
	NMG_CK_MODEL(m);
	faces_seen = (long *)rt_calloc( m->maxindex, sizeof(long), "nmg_class_pt_s faces_seen[]" );
	NMG_CK_REGION_A(s->r_p->ra_p);
	VSUB2( region_diagonal, s->r_p->ra_p->max_pt, s->r_p->ra_p->min_pt );
	region_diameter = MAGNITUDE(region_diagonal);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("\tPt=(%g, %g, %g) dir=(%g, %g, %g), reg_diam=%g\n",
			V3ARGS(pt), V3ARGS(projection_dir), region_diameter);

	/*
	 *  First pass:  Try hard to see if point is ON a face.
	 */
	for( RT_LIST_FOR(fu, faceuse, &s->fu_hd) )  {
		/* If this face processed before, skip on */
		if( NMG_INDEX_TEST( faces_seen, fu->f_p ) )  continue;

		/* Mark this face as having been processed */
		NMG_INDEX_SET(faces_seen, fu->f_p);

		/* Only consider the outward pointing faceuses */
		if( fu->orientation != OT_SAME )  continue;

		/* See if this point lies on this face */
		if( (dist = fabs(DIST_PT_PLANE(pt, fu->f_p->fg_p->N))) < tol->dist)  {
			/* Point lies on this plane, it may be possible to
			 * short circuit everything.
			 */
			class = nmg_class_pt_f( pt, fu, tol );
			if( class == NMG_CLASS_AonBshared )  {
				/* Point is ON face, therefore it must be
				 * ON the shell also.
				 */
				class = NMG_CLASS_AonBshared;
				goto out;
			}
			if( class == NMG_CLASS_AinB )  {
				/* Point is IN face, therefor it must be
				 * ON the shell also.
				 */
				class = NMG_CLASS_AonBshared;
				goto out;
			}
			/* Point is OUTside face, its undecided. */
		}

		/* Dangling faces don't participate in Jordan Curve calc */
		if (nmg_dangling_face(fu))  continue;

/* XXX Adding this code in breaks Test1.r! */
#if 0
		/* Un-mark this face, handle it in the second pass */
		NMG_INDEX_CLEAR(faces_seen, fu->f_p);
	}

	/*
	 *  Second pass:  Jordan Curve algorithm.
	 *  Fire a ray in "projection_dir", and count face crossings.
	 */
	for( RT_LIST_FOR(fu, faceuse, &s->fu_hd) )  {
		/* If this face processed before, skip on */
		if( NMG_INDEX_TEST( faces_seen, fu->f_p ) )  continue;

		/* Mark this face as having been processed */
		NMG_INDEX_SET(faces_seen, fu->f_p);
#endif

		/* Only consider the outward pointing faceuses */
		if( fu->orientation != OT_SAME )  continue;

		/* Find point where ray hits the plane. */
		stat = rt_isect_line3_plane(&dist, pt, projection_dir,
			fu->f_p->fg_p->N, tol);

		if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
			rt_log("\tray/plane: stat:%d dist:%g\n", stat, dist);
			PLPRINT("\tplane", fu->f_p->fg_p->N);
		}

		if( stat < 0 )  continue;	/* Ray missed */
		if( dist < 0 )  continue;	/* Hit was behind start pt. */

		/* XXX This case needs special handling */
		if( stat == 0 )  rt_bomb("nmg_class_pt_s: ray is ON face!\n");

		/*
		 *  The ray hit the face.  Determine if this is a reasonable
		 *  hit distance by comparing with the region diameter.
		 */
		if( dist > region_diameter )  {
			if (rt_g.NMG_debug & DEBUG_CLASSIFY)
				rt_log("\tnmg_class_pt_s: hit plane outside region, skipping\n");
			continue;
		}

		/*
		 * Construct coordinates of hit point, and classify.
		 * XXX In the case of an ON result,
		 * XXX this really needs to be a ray/edge classification,
		 * XXX not a point/edge classification.
		 * XXX The ray can go in/in, out/out, in/out, out/in,
		 * XXX with different meanings.  Can't tell the difference here.
		 * XXX One strategy:  keep picking different directions until
		 * XXX no "hard" cases come up.  (Limit 10 per customer).
		 */
	    	VJOIN1(plane_pt, pt, dist, projection_dir);
		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("\tClassify ray/face intercept point\n");
		class = nmg_class_pt_f( plane_pt, fu, tol );
		if( class == NMG_CLASS_AinB )  hitcount++;
		else if( class == NMG_CLASS_AonBshared )  {
			hitcount++;
			rt_bomb("nmg_class_pt_s: ray grazed an edge, could be 1 hit (in/out) or 2 hits (in/in, out/out), can't tell which!\n");
		}
		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("nmg_class_pt_s:\t ray hitcount=%d\n", hitcount);
	}
	rt_free( (char *)faces_seen, "nmg_class_pt_s faces_seen[]" );

	/*  Using Jordan Curve Theorem, if hitcount is even, point is OUT.
	 *  If hiscount is odd, point is IN.
	 */
	if (hitcount & 1) {
		class = NMG_CLASS_AinB;
	} else {
		class = NMG_CLASS_AoutB;
	}
out:
	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("nmg_class_pt_s: returning %s, s=x%x\n",
			nmg_class_name(class), s );
	return class;
}


/*
 *			C L A S S _ V U _ V S _ S
 *
 *	Classify a loopuse/vertexuse from shell A WRT shell B.
 */
static int class_vu_vs_s(vu, sB, classlist, tol)
struct vertexuse	*vu;
struct shell		*sB;
long			*classlist[4];
CONST struct rt_tol	*tol;
{
	struct vertexuse *vup;
	pointp_t pt;
	char	*reason;
	int	status;
	int	class;

	NMG_CK_VERTEXUSE(vu);
	NMG_CK_SHELL(sB);
	RT_CK_TOL(tol);

	pt = vu->v_p->vg_p->coord;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("class_vu_vs_s(vu=x%x) pt=(%g,%g,%g)\n", vu, V3ARGS(pt) );

	if( !(rt_g.NMG_debug & DEBUG_CLASSIFY) )  {
		/* As an efficiency & consistency measure, check for vertex in class list */
		reason = "of classlist";
		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], vu->v_p) )  {
			status = INSIDE;
			goto out;
		}
		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], vu->v_p) )  {
			status = ON_SURF;
			goto out;
		}
		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], vu->v_p) )  {
			status = OUTSIDE;
			goto out;
		}
	}

	/* we use topology to determing if the vertex is "ON" the
	 * other shell.
	 */
	for(RT_LIST_FOR(vup, vertexuse, &vu->v_p->vu_hd)) {

		if (*vup->up.magic_p == NMG_LOOPUSE_MAGIC &&
		    nmg_lups(vup->up.lu_p) == sB) {
		    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared], 
		    		vu->v_p );
		    	reason = "other loopuse of vertex is on shell";
		    	status = ON_SURF;
			goto out;
		}
		else if (*vup->up.magic_p == NMG_EDGEUSE_MAGIC &&
		    nmg_eups(vup->up.eu_p) == sB) {
		    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
		    		vu->v_p );
		    	reason = "other edgeuse of vertex is on shell";
		    	status = ON_SURF;
			goto out;
		}
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("\tCan't classify vertex via topology\n");

	/* The topology doesn't tell us about the vertex being "on" the shell
	 * so now it's time to look at the geometry to determine the vertex
	 * relationship to the shell.
	 *
	 * The vertex should *not* lie ON any of the faces or
	 * edges, since the intersection algorithm would have combined the
	 * topology if that had been the case.
	 */
	reason = "of nmg_class_pt_s()";
	class = nmg_class_pt_s(pt, sB, tol);
	
	if( class == NMG_CLASS_AoutB )  {
		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], vu->v_p);
		status = OUTSIDE;
	}  else if( class == NMG_CLASS_AinB )  {
		NMG_INDEX_SET(classlist[NMG_CLASS_AinB], vu->v_p);
		status = INSIDE;
	}  else if( class == NMG_CLASS_AonBshared )  {
		/* XXX what about corner of a cube touching inside the face of another cube? */
		rt_bomb("class_vu_vs_s:  classifier found point ON, vertex topology should have been shared\n");
	}  else  {
		rt_log("class=%s\n", nmg_class_name(class) );
		VPRINT("pt", pt);
		rt_bomb("class_vu_vs_s: nmg_class_pt_s() failure\n");
	}

out:
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("class_vu_vs_s(vu=x%x) return %s because %s\n",
			vu, nmg_class_status(status), reason );
	}
	return(status);
}

/*
 *			C L A S S _ E U _ V S _ S
 */
static int class_eu_vs_s(eu, s, classlist, tol)
struct edgeuse	*eu;
struct shell	*s;
long		*classlist[4];
CONST struct rt_tol	*tol;
{
	int euv_cl, matev_cl;
	int	status;
	struct edgeuse *eup;
	point_t pt;
	pointp_t eupt, matept;
	char	*reason;
	int	class;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		EUPRINT("class_eu_vs_s\t", eu);

	NMG_CK_EDGEUSE(eu);	
	NMG_CK_SHELL(s);	
	RT_CK_TOL(tol);
	
	if( !(rt_g.NMG_debug & DEBUG_CLASSIFY) )  {
		/* check to see if edge is already in one of the lists */
		reason = "of classlist";
		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], eu->e_p) )  {
			status = INSIDE;
			goto out;
		}
		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], eu->e_p) )  {
			status = ON_SURF;
			goto out;
		}
		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], eu->e_p) )  {
			status = OUTSIDE;
			goto out;
		}
	}
	euv_cl = class_vu_vs_s(eu->vu_p, s, classlist, tol);
	matev_cl = class_vu_vs_s(eu->eumate_p->vu_p, s, classlist, tol);
	
	/* sanity check */
	if ((euv_cl == INSIDE && matev_cl == OUTSIDE) ||
	    (euv_cl == OUTSIDE && matev_cl == INSIDE)) {
	    	static int	num=0;
	    	char	buf[128];
	    	FILE	*fp;

	    	sprintf(buf, "class%d.pl", num++ );
	    	if( (fp = fopen(buf, "w")) == NULL ) rt_bomb(buf);
	    	nmg_pl_s( fp, s );
		/* A yellow line for the angry edge */
		pl_color(fp, 255, 255, 0);
		pdv_3line(fp, eu->vu_p->v_p->vg_p->coord,
			eu->eumate_p->vu_p->v_p->vg_p->coord );
		fclose(fp);

	    	nmg_pr_class_status("eu vu", euv_cl);
	    	nmg_pr_class_status("eumate vu", matev_cl);
	    	if( rt_g.debug || rt_g.NMG_debug )  {
		    	/* Do them over, so we can watch */
/**	    		rt_g.debug |= DEBUG_MATH; **/
			rt_g.NMG_debug |= DEBUG_CLASSIFY;
			(void)class_vu_vs_s(eu->vu_p, s, classlist, tol);
			(void)class_vu_vs_s(eu->eumate_p->vu_p, s, classlist, tol);
		    	EUPRINT("didn't this edge get cut?", eu);
		    	nmg_pr_eu(eu, "  ");
	    	}

		rt_log("wrote %s\n", buf);
		rt_bomb("class_eu_vs_s:  edge didn't get cut\n");
	}

	if (euv_cl == ON_SURF && matev_cl == ON_SURF) {
		/* check for radial uses of this edge by the shell */
		eup = eu->radial_p->eumate_p;
		do {
			NMG_CK_EDGEUSE(eup);
			if (nmg_eups(eup) == s) {
				NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
					eu->e_p );
				reason = "a radial edgeuse is on shell";
				status = ON_SURF;
				goto out;
			}
			eup = eup->radial_p->eumate_p;
		} while (eup != eu->radial_p->eumate_p);

		/* although the two endpoints are "on" the shell,
		 * the edge would appear to be either "inside" or "outside",
		 * since there are no uses of this edge in the shell.
		 *
		 * So we classify the midpoint of the line WRT the shell.
		 */
		eupt = eu->vu_p->v_p->vg_p->coord;
		matept = eu->eumate_p->vu_p->v_p->vg_p->coord;
		VADD2SCALE(pt, eupt, matept, 0.5);

		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			VPRINT("class_eu_vs_s: midpoint of edge", pt);

		class = nmg_class_pt_s(pt, s, tol);
		reason = "midpoint classification (both verts ON)";
		if( class == NMG_CLASS_AoutB )  {
			NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], eu->e_p);
			status = OUTSIDE;
		}  else if( class == NMG_CLASS_AinB )  {
			NMG_INDEX_SET(classlist[NMG_CLASS_AinB], eu->e_p);
			status = INSIDE;
		} else if( class == NMG_CLASS_AonBshared )  {
#if 0
			/* XXX bug hunt helper */
			rt_g.NMG_debug |= DEBUG_FINDEU;
			rt_g.NMG_debug |= DEBUG_MESH;
			eup = nmg_findeu( eu->vu_p->v_p, eu->eumate_p->vu_p->v_p, s, eu, 0 );
			if(!eup) rt_log("Unable to find it\n");
			nmg_mesh_face_shell( eu->up.lu_p->up.fu_p, s );
#endif
			rt_bomb("class_eu_vs_s:  classifier found edge midpoint ON, edge topology should have been shared\n");
		}  else  {
			rt_log("class=%s\n", nmg_class_name(class) );
			EUPRINT("Why wasn't this edge in or out?", eu);
			rt_bomb("class_eu_vs_s: nmg_class_pt_s() midpoint failure\n");
		}
		goto out;
	}

	if (euv_cl == OUTSIDE || matev_cl == OUTSIDE) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], eu->e_p);
		reason = "at least one vert OUT";
		status = OUTSIDE;
		goto out;
	}
	NMG_INDEX_SET(classlist[NMG_CLASS_AinB], eu->e_p);
	status = INSIDE;
out:
	if (rt_g.NMG_debug & DEBUG_GRAPHCL)
		show_broken_stuff((long *)eu, classlist, nmg_class_nothing_broken, 0, (char *)NULL);
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("class_eu_vs_s(eu=x%x) return %s because %s\n",
			eu, nmg_class_status(status), reason );
	}
	return(status);
}

/*
 *			C L A S S _ L U _ V S _ S
 */
static int class_lu_vs_s(lu, s, classlist, tol)
struct loopuse		*lu;
struct shell		*s;
long			*classlist[4];
CONST struct rt_tol	*tol;
{
	int class;
	unsigned in, out, on;
	struct edgeuse *eu, *p, *q;
	struct loopuse *q_lu;
	struct vertexuse *vu;
	long		magic1;
	int		seen_error = 0;

	NMG_CK_LOOPUSE(lu);
	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	/* check to see if loop is already in one of the lists */
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], lu->l_p) )
		return(INSIDE);

	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], lu->l_p) )
		return(ON_SURF);

	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], lu->l_p) )
		return(OUTSIDE);

	magic1 = RT_LIST_FIRST_MAGIC( &lu->down_hd );
	if (magic1 == NMG_VERTEXUSE_MAGIC) {
		/* Loop of a single vertex */
		vu = RT_LIST_PNEXT( vertexuse, &lu->down_hd );
		NMG_CK_VERTEXUSE(vu);
		class = class_vu_vs_s(vu, s, classlist, tol);
		switch (class) {
		case INSIDE:
			NMG_INDEX_SET(classlist[NMG_CLASS_AinB], lu->l_p);
			break;
		case OUTSIDE:
			NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], lu->l_p);
			 break;
		case ON_SURF:
			NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared], lu->l_p);
			break;
		default:
			rt_bomb("class_lu_vs_s: bad vertexloop classification\n");
		}
		return(class);
	} else if (magic1 != NMG_EDGEUSE_MAGIC) {
		rt_bomb("class_lu_vs_s: bad child of loopuse\n");
	}

	/* loop is collection of edgeuses */
retry:
	in = out = on = 0;
	for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
		/* Classify each edgeuse */
		class = class_eu_vs_s(eu, s, classlist, tol);
		switch (class) {
		case INSIDE	: ++in; 
				break;
		case OUTSIDE	: ++out;
				break;
		case ON_SURF	: ++on;
				break;
		default		: rt_bomb("class_lu_vs_s: bad class for edgeuse\n");
		}
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("Loopuse edges in:%d on:%d out:%d\n", in, on, out);

	if (in > 0 && out > 0) {
		FILE *fp;
		rt_log("Loopuse edges in:%d on:%d out:%d\n", in, on, out);
		if( rt_g.NMG_debug & DEBUG_CLASSIFY )  {
			char		buf[128];
			static int	num;
			long		*b;
			struct model	*m;

			m = nmg_find_model(lu->up.magic_p);
			b = (long *)rt_calloc(m->maxindex, sizeof(long), "nmg_pl_lu flag[]");

			for(RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
				if (NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], eu->e_p))
					EUPRINT("In:  edgeuse", eu);
				else if (NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], eu->e_p))
					EUPRINT("Out: edgeuse", eu);
				else if (NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], eu->e_p))
					EUPRINT("On:  edgeuse", eu);
				else
					EUPRINT("BAD: edgeuse", eu);
			}

			sprintf(buf, "badloop%d.pl", num++);
			if ((fp=fopen(buf, "w")) != NULL) {
				nmg_pl_lu(fp, lu, b, 255, 255, 255);
				nmg_pl_s(fp, s);
				fclose(fp);
				rt_log("wrote %s\n", buf);
			}
			nmg_pr_lu(lu, "");
			nmg_stash_model_to_file( "class.g", nmg_find_model((long *)lu), "class_ls_vs_s: loop transits plane of shell/face?");
			rt_free( (char *)b, "nmg_pl_lu flag[]" );
		}
		rt_g.NMG_debug |= DEBUG_CLASSIFY;
		if(seen_error)
			rt_bomb("class_lu_vs_s: loop transits plane of shell/face?\n");
		seen_error = 1;
		goto retry;
	}
	if (out > 0) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], lu->l_p);
		return(OUTSIDE);
	} else if (in > 0) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AinB], lu->l_p);
		return(INSIDE);
	} else if (on == 0)
		rt_bomb("class_lu_vs_s: alright, who's the wiseguy that stole my edgeuses?\n");


	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("\tAll edgeuses of loop are ON");

	/* since all of the edgeuses of this loop are "on" the other shell,
	 * we need to see if this loop is "on" the other shell
	 *
	 * if we're a wire edge loop, simply having all edges "on" is
	 *	sufficient.
	 *
	 * foreach radial edgeuse
	 * 	if edgeuse vertex is same and edgeuse parent shell is the one
	 *	 	desired, then....
	 *
	 *		p = edgeuse, q = radial edgeuse
	 *		while p's vertex equals q's vertex and we
	 *			haven't come full circle
	 *			move p and q forward
	 *		if we made it full circle, loop is on
	 */

	if (*lu->up.magic_p == NMG_SHELL_MAGIC) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared], lu->l_p);
		return(ON_SURF);
	}

	NMG_CK_FACEUSE(lu->up.fu_p);

	eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
	eu = eu->radial_p->eumate_p;
	do {
		/* if the radial edge is a part of a loop which is part of
		 * a face, then it's one that we might be "on"
		 */
		if (*eu->up.magic_p == NMG_LOOPUSE_MAGIC && 
		    *eu->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC && 
		    eu->up.lu_p->up.fu_p->s_p == s ) {

			p = RT_LIST_FIRST(edgeuse, &lu->down_hd);
			q = eu;
			q_lu = q->up.lu_p;

			/* get the starting vertex of each edgeuse to be the
			 * same.
			 */
			if (q->vu_p->v_p != p->vu_p->v_p) {
				q = eu->eumate_p;
				if (q->vu_p->v_p != p->vu_p->v_p)
					rt_bomb("class_lu_vu_s: radial edgeuse doesn't share verticies\n");
			
			}

		    	/* march around the two loops to see if they 
		    	 * are the same all the way around.
		    	 */
		/* XXX why isn't "p" also traversed circularly? */
		    	while (
		    	    RT_LIST_NOT_HEAD(p, &lu->down_hd) &&
			    p->vu_p->v_p == q->vu_p->v_p &&
			    q->up.lu_p == q_lu
			) {
				q = RT_LIST_PNEXT_CIRC(edgeuse, &q->l);
				p = RT_LIST_PNEXT(edgeuse, p);
			}

			if (!RT_LIST_NOT_HEAD(p, &lu->down_hd)) {

				/* the two loops are "on" each other.  All
				 * that remains is to determine
				 * shared/anti-shared status.
				 */
				NMG_CK_FACE_G(q_lu->up.fu_p->f_p->fg_p);
				if (VDOT(q_lu->up.fu_p->f_p->fg_p->N,
				    lu->up.fu_p->f_p->fg_p->N) >= 0) {
				    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
				    		lu->l_p );
					if (rt_g.NMG_debug & DEBUG_CLASSIFY)
						rt_log("Loop is on-shared\n");
				} else {
					NMG_INDEX_SET(classlist[NMG_CLASS_AonBanti],
						lu->l_p );
					if (rt_g.NMG_debug & DEBUG_CLASSIFY)
						rt_log("Loop is on-antishared\n");
				}

				return(ON_SURF);
			}

		}
		eu = eu->radial_p->eumate_p;
	} while (eu != RT_LIST_FIRST(edgeuse, &lu->down_hd) );



	/* OK, the edgeuses are all "on", but the loop isn't.  Time to
	 * decide if the loop is "out" or "in".  To do this, we look for
	 * an edgeuse radial to one of the edgeuses in the loop which is
	 * a part of a face in the other shell.  If/when we find such a
	 * radial edge, we check the direction (in/out) of the faceuse normal.
	 * if the faceuse normal is pointing out of the shell, we are outside.
	 * if the faceuse normal is pointing into the shell, we are inside.
	 */

	for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {

		p = eu->radial_p;
		do {
			if (*p->up.magic_p == NMG_LOOPUSE_MAGIC &&
			    *p->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC &&
			    p->up.lu_p->up.fu_p->s_p == s) {

			    	if (p->up.lu_p->up.fu_p->orientation == OT_OPPOSITE) {
			    		NMG_INDEX_SET(classlist[NMG_CLASS_AinB],
			    			lu->l_p );
					if (rt_g.NMG_debug & DEBUG_CLASSIFY)
						rt_log("Loop is INSIDE\n");
					return(INSIDE);
			    	} else if (p->up.lu_p->up.fu_p->orientation == OT_SAME) {
			    		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB],
			    			lu->l_p );
					if (rt_g.NMG_debug & DEBUG_CLASSIFY)
						rt_log("Loop is OUTSIDE\n");
					return(OUTSIDE);
			    	}

			}
			p = p->eumate_p->radial_p;
		} while (p != eu->eumate_p);

	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("Loop is OUTSIDE 'cause it isn't anything else\n");
	

	/* Since we didn't find any radial faces to classify ourselves against
	 * and we already know that the edges are all "on" that must mean that
	 * the loopuse is "on" a wireframe portion of the shell.
	 */
	NMG_INDEX_SET( classlist[NMG_CLASS_AoutB], lu->l_p );
	return(OUTSIDE);
}

/*
 *			C L A S S _ F U _ V S _ S
 */
static void class_fu_vs_s(fu, s, classlist, tol)
struct faceuse		*fu;
struct shell		*s;
long			*classlist[4];
CONST struct rt_tol	*tol;
{
	struct loopuse *lu;
	
	NMG_CK_FACEUSE(fu);
	NMG_CK_SHELL(s);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
        	PLPRINT("\nclass_fu_vs_s plane equation:", fu->f_p->fg_p->N);

	for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd))
		(void)class_lu_vs_s(lu, s, classlist, tol);

}

/*
 *			N M G _ C L A S S _ S H E L L S
 *
 *	Classify one shell WRT the other shell
 *
 *  Implicit return -
 *	Each element's classification will be represented by a
 *	SET entry in the appropriate classlist[] array.
 */
void
nmg_class_shells(sA, sB, classlist, tol)
struct shell	*sA;
struct shell	*sB;
long		*classlist[4];
CONST struct rt_tol	*tol;
{
	struct faceuse *fu;
	struct loopuse *lu;
	struct edgeuse *eu;

	NMG_CK_SHELL(sA);
	NMG_CK_SHELL(sB);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY &&
	    RT_LIST_NON_EMPTY(&sA->fu_hd))
		rt_log("nmg_class_shells - doing faces\n");

	fu = RT_LIST_FIRST(faceuse, &sA->fu_hd);
	while (RT_LIST_NOT_HEAD(fu, &sA->fu_hd)) {

		class_fu_vs_s(fu, sB, classlist, tol);

		if (RT_LIST_PNEXT(faceuse, fu) == fu->fumate_p)
			fu = RT_LIST_PNEXT_PNEXT(faceuse, fu);
		else
			fu = RT_LIST_PNEXT(faceuse, fu);
	}
	
	if (rt_g.NMG_debug & DEBUG_CLASSIFY &&
	    RT_LIST_NON_EMPTY(&sA->lu_hd))
		rt_log("nmg_class_shells - doing loops\n");

	lu = RT_LIST_FIRST(loopuse, &sA->lu_hd);
	while (RT_LIST_NOT_HEAD(lu, &sA->lu_hd)) {

		(void)class_lu_vs_s(lu, sB, classlist, tol);

		if (RT_LIST_PNEXT(loopuse, lu) == lu->lumate_p)
			lu = RT_LIST_PNEXT_PNEXT(loopuse, lu);
		else
			lu = RT_LIST_PNEXT(loopuse, lu);
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY &&
	    RT_LIST_NON_EMPTY(&sA->eu_hd))
		rt_log("nmg_class_shells - doing edges\n");

	eu = RT_LIST_FIRST(edgeuse, &sA->eu_hd);
	while (RT_LIST_NOT_HEAD(eu, &sA->eu_hd)) {

		(void)class_eu_vs_s(eu, sB, classlist, tol);

		if (RT_LIST_PNEXT(edgeuse, eu) == eu->eumate_p)
			eu = RT_LIST_PNEXT_PNEXT(edgeuse, eu);
		else
			eu = RT_LIST_PNEXT(edgeuse, eu);
	}

	if (sA->vu_p) {
		if (rt_g.NMG_debug)
			rt_log("nmg_class_shells - doing vertex\n");
		(void)class_vu_vs_s(sA->vu_p, sB, classlist, tol);
	}
}
