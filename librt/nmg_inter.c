/*
 *			N M G _ I N T E R . C
 *
 *	Routines to intersect two NMG regions.
 *	There are no "user interface" routines in here.
 *
 *  Author -
 *	Lee A. Butler
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
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
#include "raytrace.h"

struct nmg_boolstruct {
	struct nmg_ptbl *l1, *l2;
	fastf_t tol;
};



/*
 *	T B L _ V S O R T
 *	sort list of vertices in fu1 on plane of fu2 
 *
 *	W A R N I N G:
 *		This function makes gross assumptions about the contents
 *	and structure of an nmg_ptbl list!  If I could figure a way of folding
 *	this into the nmg_tbl routine, I would do it.
 */
static void tbl_vsort(b, fu1, fu2)
struct nmg_ptbl *b;		/* table of vertexuses on intercept line */
struct faceuse *fu1, *fu2;
{
	point_t pt, min_pt;
	vect_t	dir, vect;
	union {
		struct vertexuse **vu;
		long **magic_p;
	} p;
	struct vertexuse *tvu;
	fastf_t *mag, tmag;
	int i, j;

	VMOVE(min_pt, fu1->f_p->fg_p->min_pt);
	VMIN(min_pt, fu2->f_p->fg_p->min_pt);
	rt_isect_2planes(pt, dir, fu1->f_p->fg_p->N, fu2->f_p->fg_p->N, min_pt);
	mag = (fastf_t *)rt_calloc(b->end, sizeof(fastf_t),
					"vector magnitudes for sort");

	p.magic_p = b->buffer;
	/* check vertexuses and compute distance from start of line */
	for(i = 0 ; i < b->end ; ++i) {
		NMG_CK_VERTEXUSE(p.vu[i]);

		VSUB2(vect, pt, p.vu[i]->v_p->vg_p->coord);
		mag[i] = MAGNITUDE(vect);
	}

	/* a trashy bubble-head sort, because I hope this list is never
	 * very long.
	 */
	for(i=0 ; i < b->end - 1 ; ++i) {
		for (j=i+1; j < b->end ; ++j) {
			if (mag[i] > mag[j]) {
				tvu = p.vu[i];
				p.vu[i] = p.vu[j];
				p.vu[j] = tvu;

				tmag = mag[i];
				mag[i] = mag[j];
				mag[j] = tmag;
			}
		}
	}
	/*
	 * We should do something here to "properly"
	 * order vertexuses which share a vertex
	 * or whose coordinates are equal.
	 *
	 * Just what should be done & how is not
	 * clear to me at this hour of the night.
	 * for (i=0 ; i < b->end - 1 ; ++i) {
	 *	if (p.vu[i]->v_p == p.vu[i+1]->v_p ||
	 *	    VAPPROXEQUAL(p.vu[i]->v_p->vg_p->coord,
	 *	    p.vu[i+1]->v_p->vg_p->coord, VDIVIDE_TOL) ) {
	 *	}
	 * }
	 */
	rt_free((char *)mag, "vector magnitudes");
}


/*	V E R T E X _ O N _ F A C E
 *
 *	Perform a topological check to
 *	determine if the given vertex can be found in the given faceuse.
 *	If it can, return a pointer to the vertexuse which was found in the
 *	faceuse.
 */
static struct vertexuse *vertex_on_face(v, fu)
struct vertex *v;
struct faceuse *fu;
{
	struct vertexuse *vu;
	struct edgeuse *eu;
	struct loopuse *lu;

#define CKLU_FOR_FU(_lu, _fu, _vu) \
	if (*_lu->up.magic_p == NMG_FACEUSE_MAGIC && _lu->up.fu_p == _fu) \
		return(_vu)

	NMG_CK_VERTEX(v);

	vu = v->vu_p;
	do {
		NMG_CK_VERTEXUSE(vu);
		if (*vu->up.magic_p == NMG_EDGEUSE_MAGIC) {
			eu = vu->up.eu_p;
			if (*eu->up.magic_p == NMG_LOOPUSE_MAGIC) {
				lu = eu->up.lu_p;
				CKLU_FOR_FU(lu, fu, vu);
			}
		} else if (*vu->up.magic_p == NMG_LOOPUSE_MAGIC) {
				lu = vu->up.lu_p;
				CKLU_FOR_FU(lu, fu, vu);
		}

		vu = vu->next;
	} while (vu != v->vu_p);

	return((struct vertexuse *)NULL);
}

/*	I S E C T _ V E R T E X _ F A C E
 *
 *	intersect a vertex with a face (primarily for intersecting
 *	loops of a single vertex with a face).
 */
static void isect_vertex_face(bs, vu, fu)
struct nmg_boolstruct *bs;
struct vertexuse *vu;
struct faceuse *fu;
{
	struct loopuse *plu;
	struct vertexuse *vup;
	pointp_t pt;
	fastf_t dist;

	/* check the topology first */	
	if (vup=vertex_on_face(vu->v_p, fu)) {
		if (nmg_tbl(bs->l1, TBL_LOC, &vu->magic) < 0)
			(void)nmg_tbl(bs->l1, TBL_INS, &vu->magic);
		if (nmg_tbl(bs->l2, TBL_LOC, &vup->magic) < 0)
			(void)nmg_tbl(bs->l2, TBL_INS, &vup->magic);

		return;
	}


	/* since the topology didn't tell us anything, we need to check with
	 * the geometry
	 */
	pt = vu->v_p->vg_p->coord;
	dist = NMG_DIST_PT_PLANE(pt, fu->f_p->fg_p->N);

	if (NEAR_ZERO(dist, bs->tol) &&
	    nmg_tbl(bs->l1, TBL_LOC, &vu->magic) < 0) {

		(void)nmg_tbl(bs->l1, TBL_INS, &vu->magic);

		if (rt_g.NMG_debug & DEBUG_POLYSECT)
		    	VPRINT("Making vertexloop", vu->v_p->vg_p->coord);

		plu = nmg_mlv(&fu->magic, vu->v_p, OT_UNSPEC);
	    	(void)nmg_tbl(bs->l2, TBL_INS, &plu->down.vu_p->magic);
	}
}

/*	I S E C T _ E D G E _ F A C E
 *
 *	Intersect an edge with a face
 */
static void isect_edge_face(bs, eu, fu)
struct nmg_boolstruct *bs;
struct edgeuse *eu;
struct faceuse *fu;
{
	struct vertexuse *vu;
	point_t		hit_pt;
	fastf_t		vect_len;	/* MAGNITUDE(vect) */
	pointp_t p1, p2, p3;
	vect_t vect, delta;
	fastf_t dist, mag;
	int status;
	struct loopuse *plu;

	NMG_CK_EDGEUSE(eu);
	NMG_CK_VERTEXUSE(eu->vu_p);
	NMG_CK_VERTEX(eu->vu_p->v_p);
	NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);

	if (*eu->up.magic_p == NMG_LOOPUSE_MAGIC) {
		/* some edge sanity checking */
		if (eu->vu_p->v_p != eu->last->eumate_p->vu_p->v_p) {
			VPRINT("unshared vertex (mine): ", eu->vu_p->v_p->vg_p->coord);
			VPRINT("\t\t (last->eumate_p): ", eu->last->eumate_p->vu_p->v_p->vg_p->coord);
			rt_bomb("discontiuous edgeloop\n");
		}
		if (eu->next->vu_p->v_p != eu->eumate_p->vu_p->v_p) {
VPRINT("unshared vertex (my mate): ", eu->eumate_p->vu_p->v_p->vg_p->coord);
VPRINT("\t\t (next): ", eu->next->vu_p->v_p->vg_p->coord);
			rt_bomb("discontiuous edgeloop\n");
		}


	}



	/* We check to see if the edge crosses the plane of face fu.
	 * First we check the topology.  If the topology says that the start
	 * vertex of this edgeuse is on the other face, we enter the
	 * vertexuses in the list and it's all over.
	 *
	 * If the vertex on the other end of this edgeuse is on the face,
	 * then make a linkage to an existing face vertex (if found),
	 * and give up on this edge, knowing that we'll pick up the
	 * intersection of the next edgeuse with the face later.
	 *
	 * This all assumes that an edge can only intersect a face at one
	 * point.  This is probably a bad assumption for the future
	 */
	if (vu=vertex_on_face(eu->vu_p->v_p, fu)) {

		if (rt_g.NMG_debug & DEBUG_POLYSECT) {
			p1 = eu->vu_p->v_p->vg_p->coord;
			NMG_CK_EDGEUSE(eu->eumate_p);
			NMG_CK_VERTEXUSE(eu->eumate_p->vu_p);
			NMG_CK_VERTEX(eu->eumate_p->vu_p->v_p);
			NMG_CK_VERTEX_G(eu->eumate_p->vu_p->v_p->vg_p);
			p2 = eu->eumate_p->vu_p->v_p->vg_p->coord;
			rt_log("Edgeuse %g, %g, %g -> %g, %g, %g\n",
				p1[0], p1[1], p1[2], p2[0], p2[1], p2[2]);
			rt_log("\tvertex topologically on intersection plane\n");
		}

		if (nmg_tbl(bs->l1, TBL_LOC, &eu->vu_p->magic) < 0)
			(void)nmg_tbl(bs->l1, TBL_INS, &eu->vu_p->magic);
		if (nmg_tbl(bs->l2, TBL_LOC, &vu->magic) < 0)
			(void)nmg_tbl(bs->l2, TBL_INS, &vu->magic);
		return;
	} else if (vu=vertex_on_face(eu->eumate_p->vu_p->v_p, fu)) {

		if (rt_g.NMG_debug & DEBUG_POLYSECT) {
			p1 = eu->vu_p->v_p->vg_p->coord;
			NMG_CK_EDGEUSE(eu->eumate_p);
			NMG_CK_VERTEXUSE(eu->eumate_p->vu_p);
			NMG_CK_VERTEX(eu->eumate_p->vu_p->v_p);
			NMG_CK_VERTEX_G(eu->eumate_p->vu_p->v_p->vg_p);
			p2 = eu->eumate_p->vu_p->v_p->vg_p->coord;
			rt_log("Edgeuse %g, %g, %g -> %g, %g, %g\n",
				p1[0], p1[1], p1[2], p2[0], p2[1], p2[2]);
			rt_log("\tMATE vertex topologically on intersection plane. skipping edgeuse\n");
		}
		return;
	}

	/*
	 *  Neither vertex lies on the face according to topology.
	 *  Form a ray that starts at one vertex of the edgeuse
	 *  and points to the other vertex.
	 */
	p1 = eu->vu_p->v_p->vg_p->coord;
	p2 = eu->eumate_p->vu_p->v_p->vg_p->coord;
	VSUB2(vect, p2, p1);

	if (rt_g.NMG_debug & DEBUG_POLYSECT)
		rt_log("Testing %g, %g, %g -> %g, %g, %g\n",
			p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);


	status = rt_isect_ray_plane(&dist, p1, vect, fu->f_p->fg_p->N);

	if (rt_g.NMG_debug & DEBUG_POLYSECT) {
		if (status >= 0)
			rt_log("\tStatus of rt_isect_ray_plane: %d dist: %g\n",
				status, dist);
		else
			rt_log("\tBoring status of rt_isect_ray_plane: %d dist: %g\n",
				status, dist);
	}

	/* if this edge doesn't intersect the other face by geometry,
	 * we're done.
	 */
	if (status < 0)
		return;

	/* the ray defined by the edgeuse intersects the plane 
	 * of the other face.  Check to see if the distance to
         * intersection is between limits of the endpoints of
	 * this edge(use).
	 */
	VJOIN1( hit_pt, p1, dist, vect );
	vect_len = MAGNITUDE(vect);
	VSCALE(delta, vect, dist);
	mag = MAGNITUDE(delta);

	if (rt_g.NMG_debug & DEBUG_POLYSECT)
		rt_log("\tmag of vect:%g  Dist to plane:%g\n",
			MAGNITUDE(vect), mag);

	if( mag < -(bs->tol) )  {
		/* Hit is behind first point */
		if (rt_g.NMG_debug & DEBUG_POLYSECT)
			rt_log("\tplane behind first point\n");
		return;
	}

	if( mag < bs->tol )  {
		/* First point is on plane of face, by geometry */
		if (rt_g.NMG_debug & DEBUG_POLYSECT)
			rt_log("\tedge starts at plane intersect\n");

		/* Add to list of verts on intersection line */
		if (nmg_tbl(bs->l1, TBL_LOC, &eu->vu_p->magic) < 0)
			(void)nmg_tbl(bs->l1, TBL_INS, &eu->vu_p->magic);

		/* If face doesn't have a "very similar" point, give it one */
		vu = nmg_find_vu_in_face(p1, fu, bs->tol);
		if (vu) {
			/* Face has a vertex very similar to this one.
			 * Add vertex to face's list of vertices on
			 * intersection line.
			 */
			if (nmg_tbl(bs->l2, TBL_LOC, &vu->magic) < 0)
				(void)nmg_tbl(bs->l2, TBL_INS, &vu->magic);

			/* new coordinates are the midpoint between
			 * the two existing coordinates
			 */
			p3 = vu->v_p->vg_p->coord;
			VADD2SCALE(p1, p1, p3, 0.5);
			/* Combine the two vertices */
			nmg_jv(eu->vu_p->v_p, vu->v_p);
		} else {
			/* Since the other face doesn't have a vertex quite
			 * like this one, we make a copy of this one and make.
			 * sure it's in the other face's list of intersect
			 * verticies.
			 */

			if (rt_g.NMG_debug & DEBUG_POLYSECT)
			    	VPRINT("Making vertexloop",
			    		eu->vu_p->v_p->vg_p->coord);

			plu = nmg_mlv(&fu->magic, eu->vu_p->v_p, OT_UNSPEC);

			/* make sure this vertex is in other face's list of
			 * points to deal with
			 */
			(void)nmg_tbl(bs->l2, TBL_INS, &plu->down.vu_p->magic);
		}
		return;
	}
	if ( mag < vect_len - bs->tol) {
		/* Intersection is between first and second vertex points.
		 * Insert new vertex at intersection point.
		 */
		if (rt_g.NMG_debug & DEBUG_POLYSECT)  {
			rt_log("Splitting %g, %g, %g <-> %g, %g, %g\n",
			p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
			VPRINT("\tPoint of intersection", hit_pt);
		}

		/* if we can't find the appropriate vertex in the
		 * other face, we'll build a new vertex.  Otherwise
		 * we re-use an old one.
		 */
		vu = nmg_find_vu_in_face(hit_pt, fu, bs->tol);
		if (vu) {
			/* the other face has a convenient vertex for us */

			if (rt_g.NMG_debug & DEBUG_POLYSECT)
				rt_log("re-using vertex from other face\n");

			(void)nmg_esplit(vu->v_p, eu->e_p);
			if (nmg_tbl(bs->l2, TBL_LOC, &vu->magic) < 0)
				(void)nmg_tbl(bs->l2, TBL_INS, &vu->magic);
		} else {

			if (rt_g.NMG_debug & DEBUG_POLYSECT)
				rt_log("Making new vertex\n");

			(void)nmg_esplit((struct vertex *)NULL, eu->e_p);

			/* given the trouble that nmg_esplit was to create,
			 * we're going to check this to the limit
			 */
			NMG_CK_EDGEUSE(eu);
			NMG_CK_EDGEUSE(eu->eumate_p);
			NMG_CK_EDGEUSE(eu->next);
			NMG_CK_EDGEUSE(eu->next->eumate_p);

			NMG_CK_VERTEXUSE(eu->vu_p);
			NMG_CK_VERTEXUSE(eu->eumate_p->vu_p);
			NMG_CK_VERTEXUSE(eu->next->vu_p);
			NMG_CK_VERTEXUSE(eu->next->eumate_p->vu_p);

			NMG_CK_VERTEX(eu->vu_p->v_p);
			NMG_CK_VERTEX(eu->eumate_p->vu_p->v_p);
			NMG_CK_VERTEX(eu->next->vu_p->v_p);
			NMG_CK_VERTEX(eu->next->eumate_p->vu_p->v_p);

			/* check to make sure we know the right place to
			 * stick the geometry
			 */
			if (eu->eumate_p->vu_p->v_p->vg_p != 
			    (struct vertex_g *)NULL) {
				VPRINT("where'd this geometry come from?",
					eu->eumate_p->vu_p->v_p->vg_p->coord);
			}
			nmg_vertex_gv(eu->eumate_p->vu_p->v_p, hit_pt);

			NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
			NMG_CK_VERTEX_G(eu->eumate_p->vu_p->v_p->vg_p);
			NMG_CK_VERTEX_G(eu->next->vu_p->v_p->vg_p);
			NMG_CK_VERTEX_G(eu->next->eumate_p->vu_p->v_p->vg_p);

			if (eu->next->vu_p->v_p != eu->eumate_p->vu_p->v_p)
				rt_bomb("I was supposed to share verticies!\n");

			if (rt_g.NMG_debug & DEBUG_POLYSECT) {
				rt_log("Just split %g, %g, %g -> %g, %g, %g\n\t\t\t%g, %g, %g -> %g, %g, %g\n",
					eu->vu_p->v_p->vg_p->coord[0],
					eu->vu_p->v_p->vg_p->coord[1],
					eu->vu_p->v_p->vg_p->coord[2],
					eu->eumate_p->vu_p->v_p->vg_p->coord[0],
					eu->eumate_p->vu_p->v_p->vg_p->coord[1],
					eu->eumate_p->vu_p->v_p->vg_p->coord[2],
					eu->next->vu_p->v_p->vg_p->coord[0],
					eu->next->vu_p->v_p->vg_p->coord[1],
					eu->next->vu_p->v_p->vg_p->coord[2],
					eu->next->eumate_p->vu_p->v_p->vg_p->coord[0],
					eu->next->eumate_p->vu_p->v_p->vg_p->coord[1],
					eu->next->eumate_p->vu_p->v_p->vg_p->coord[2]);

			}
			/* stick this vertex in the other shell
			 * and make sure it is in the other shell's
			 * list of vertices on the instersect line
			 */
			plu = nmg_mlv(&fu->magic,
				eu->eumate_p->vu_p->v_p, OT_SAME);

			if (rt_g.NMG_debug & DEBUG_POLYSECT)
			    	VPRINT("Making vertexloop",
			    		plu->down.vu_p->v_p->vg_p->coord);

			(void)nmg_tbl(bs->l2, TBL_INS, &plu->down.vu_p->magic);
		}

		if (rt_g.NMG_debug & DEBUG_POLYSECT) {
			p1 = eu->vu_p->v_p->vg_p->coord;
			p2 = eu->eumate_p->vu_p->v_p->vg_p->coord;
			rt_log("\tNow %g, %g, %g <-> %g, %g, %g\n",
				p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
			p1 = eu->next->vu_p->v_p->vg_p->coord;
			p2 = eu->next->eumate_p->vu_p->v_p->vg_p->coord;
			rt_log("\tand %g, %g, %g <-> %g, %g, %g\n\n",
				p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
		}

		(void)nmg_tbl(bs->l1, TBL_INS, &eu->next->vu_p->magic);
		return;
	}
	if ( mag < vect_len + bs->tol) {
		/* Second point is on plane of face, by geometry */
		/* Make no entries in intersection lists,
		 * because it will be handled on the next call.
		 */
		if (rt_g.NMG_debug & DEBUG_POLYSECT)
			rt_log("\tedge ends at plane intersect\n");

		if( eu->next->vu_p->v_p != eu->eumate_p->vu_p->v_p )
			rt_bomb("isect_edge_face: discontinuous eu loop\n");

		/* If face has a "very similar" point, connect up with it */
		vu = nmg_find_vu_in_face(p2, fu, bs->tol);
		if (vu) {
			/* new coordinates are the midpoint between
			 * the two existing coordinates
			 */
			p3 = vu->v_p->vg_p->coord;
			VADD2SCALE(p2, p2, p3, 0.5);
			/* Combine the two vertices */
			nmg_jv(eu->eumate_p->vu_p->v_p, vu->v_p);
		}
		return;
	}

	if (rt_g.NMG_debug & DEBUG_POLYSECT)
		rt_log("\tdist to plane X: X > MAGNITUDE(edge)\n");


}
/*	I S E C T _ L O O P _ F A C E
 *
 *	Intersect a single loop with another face
 */
static void isect_loop_face(bs, lu, fu)
struct nmg_boolstruct *bs;
struct loopuse *lu;
struct faceuse *fu;
{
	struct edgeuse *eu, *eu_start;


	if (rt_g.NMG_debug & DEBUG_POLYSECT)
		rt_log("isect_loop_faces\n");

	/* loop overlaps intersection face? */
	if (*lu->down.magic_p == NMG_VERTEXUSE_MAGIC) {
		/* this is most likely a loop inserted when we split
		 * up fu2 wrt fu1 (we're now splitting fu1 wrt fu2)
		 */
		isect_vertex_face(bs, lu->down.vu_p, fu);
	} else if (*lu->down.magic_p == NMG_EDGEUSE_MAGIC) {
		/* Process a loop consisting of a list of edgeuses.
		 */ 
		eu_start = eu = lu->down.eu_p;
		do {
			NMG_CK_EDGEUSE(eu);

			if (eu->up.magic_p != &lu->magic) {
				rt_bomb("edge does not share loop\n");
			}

			isect_edge_face(bs, eu, fu);

			/* by going backwards around the list we avoid
			 * re-processing an edgeuse that was just created
			 * by isect_edge_face.  This is because the edgeuses
			 * point in the "next" direction, and when one of
			 * them is split, it inserts a new edge AHEAD or
			 * "nextward" of the current edgeuse.
			 */
			eu = eu->last;
		} while (eu != eu_start);
	} else {
		rt_bomb("Unknown type of NMG loopuse\n");
	}
}
/*	I S E C T _ L O O P S
 *
 *	intersect loops of face 1 with face 2
 */
static void isect_loops(bs, fu1, fu)
struct nmg_boolstruct *bs;
struct faceuse *fu1, *fu;
{
	struct loopuse *lu_start, *lu;

	NMG_CK_FACE_G(fu->f_p->fg_p);
	NMG_CK_FACE_G(fu1->f_p->fg_p);

	/* process each loop in face 1 */
	lu_start = lu = fu1->lu_p;
	do {

		if (rt_g.NMG_debug & DEBUG_POLYSECT)
			rt_log("\nLoop %8x\n", lu);

		if (lu->up.fu_p != fu1) {
			rt_bomb("Child loop doesn't share parent!\n");
		}

		isect_loop_face(bs, lu, fu);

		lu = lu->next;
	} while (lu != lu_start);
}


/*	I S E C T _ F A C E S
 *
 *	Intersect a pair of faces
 */
static void nmg_isect_faces(fu1, fu2, tol)
struct faceuse *fu1, *fu2;
fastf_t tol;
{
	struct nmg_ptbl vert_list1, vert_list2;
	struct nmg_boolstruct bs;
	int i;
	fastf_t *pl1, *pl2;

	union {
		struct vertexuse **vu;
		long **magic_p;
	} p;


	if (rt_g.NMG_debug & DEBUG_POLYSECT) {
		pl1 = fu1->f_p->fg_p->N;
		pl2 = fu2->f_p->fg_p->N;

		rt_log("\nPlanes\t%gx + %gy + %gz = %g\n\t%gx + %gy + %gz = %g\n",
			pl1[0], pl1[1], pl1[2], pl1[3],
			pl2[0], pl2[1], pl2[2], pl2[3]);
	}

	NMG_CK_FACEUSE(fu1);
	NMG_CK_FACE(fu1->f_p);
	NMG_CK_FACE_G(fu1->f_p->fg_p);
	NMG_CK_FACEUSE(fu2);
	NMG_CK_FACE(fu2->f_p);
	NMG_CK_FACE_G(fu2->f_p->fg_p);

	if ( !NMG_EXTENT_OVERLAP(fu2->f_p->fg_p->min_pt,fu2->f_p->fg_p->max_pt,
	    fu1->f_p->fg_p->min_pt,fu1->f_p->fg_p->max_pt) )  return;

	/* Extents of face1 overlap face2 */
	(void)nmg_tbl(&vert_list1, TBL_INIT,(long *)NULL);
	(void)nmg_tbl(&vert_list2, TBL_INIT,(long *)NULL);

    	bs.l1 = &vert_list1;
    	bs.l2 = &vert_list2;
    	bs.tol = tol/50.0;

    	if (rt_g.NMG_debug & (DEBUG_POLYSECT|DEBUG_COMBINE|DEBUG_MESH)
    	    && rt_g.NMG_debug & DEBUG_PLOTEM) {
    		FILE *fd, *fopen();
    		static char name[32];
    		static struct nmg_ptbl b;
    		static int fno=1;

    		(void)nmg_tbl(&b, TBL_INIT, (long *)NULL);
    		(void)sprintf(name, "Isect_faces%d.pl", fno++);
    		rt_log("plotting to %s\n", name);
    		if ((fd=fopen(name, "w")) == (FILE *)NULL)
    			rt_bomb(name);

    		(void)nmg_pl_fu(fd, fu1, &b, 100, 100, 180);
/*    		(void)nmg_pl_fu(fd, fu1->fumate_p, &b, 100, 100, 180);
*/
    		(void)nmg_pl_fu(fd, fu2, &b, 100, 100, 180);
/*    		(void)nmg_pl_fu(fd, fu2->fumate_p, &b, 100, 100, 180);
*/
    		(void)fclose(fd);
    		(void)nmg_tbl(&b, TBL_FREE, (long *)NULL);
    	}

	isect_loops(&bs, fu1, fu2);

    	if (rt_g.NMG_debug & DEBUG_COMBINE) {
	    	p.magic_p = vert_list1.buffer;
	    	rt_log("vert_list1\n");
		for (i=0 ; i < vert_list1.end ; ++i) {
			rt_log("%d\t%g, %g, %g\t", i,
				p.vu[i]->v_p->vg_p->coord[X],
				p.vu[i]->v_p->vg_p->coord[Y],
				p.vu[i]->v_p->vg_p->coord[Z]);
			if (*p.vu[i]->up.magic_p == NMG_EDGEUSE_MAGIC) {
				rt_log("EDGEUSE\n");
			} else if (*p.vu[i]->up.magic_p == NMG_LOOPUSE_MAGIC){
				rt_log("LOOPUSE\n");
			} else
				rt_log("UNKNOWN\n");
		}
    	}

    	bs.l2 = &vert_list1;
    	bs.l1 = &vert_list2;
	isect_loops(&bs, fu2, fu1);

    	if (vert_list1.end == 0) {
    		/* there were no intersections */
		(void)nmg_tbl(&vert_list1, TBL_FREE, (long *)NULL);
		(void)nmg_tbl(&vert_list2, TBL_FREE, (long *)NULL);
    		return;
    	}
	tbl_vsort(&vert_list1, fu1, fu2);
	nmg_face_combine(&vert_list1, fu1, fu2);

	tbl_vsort(&vert_list2, fu2, fu1);
	nmg_face_combine(&vert_list2, fu2, fu1);

	/* When two faces are intersected
	 * with each other, they should
	 * share the same edge(s) of
	 * intersection. 
	 */
    	if (rt_g.NMG_debug & DEBUG_MESH &&
    	    rt_g.NMG_debug & DEBUG_PLOTEM) {
    		FILE *fd, *fopen();
    		static char name[32];
    		static struct nmg_ptbl b;
    		static int fnum=1;

    		(void)nmg_tbl(&b, TBL_INIT, (long *)NULL);
    		(void)sprintf(name, "Before_mesh%d.pl", fnum++);
    		if ((fd=fopen(name, "w")) == (FILE *)NULL)
    			rt_bomb(name);

    		rt_log("plotting %s\n", name);
    		(void)nmg_pl_fu(fd, fu1, &b, 100, 100, 180);
    		(void)nmg_pl_fu(fd, fu1->fumate_p, &b, 100, 100, 180);

    		(void)nmg_pl_fu(fd, fu2, &b, 100, 100, 180);
    		(void)nmg_pl_fu(fd, fu2->fumate_p, &b, 100, 100, 180);

    		(void)fclose(fd);
    		(void)nmg_tbl(&b, TBL_FREE, (long *)NULL);
    	}

	nmg_mesh_faces(fu1, fu2);

    	if (rt_g.NMG_debug & DEBUG_MESH &&
    	    rt_g.NMG_debug & DEBUG_PLOTEM) {
    		FILE *fd, *fopen();
    		static char name[32];
    		static struct nmg_ptbl b;
    		static int fno=1;

    		(void)nmg_tbl(&b, TBL_INIT, (long *)NULL);
    		(void)sprintf(name, "After_mesh%d.pl", fno++);
    		rt_log("plotting to %s\n", name);
    		if ((fd=fopen(name, "w")) == (FILE *)NULL)
    			rt_bomb(name);

    		(void)nmg_pl_fu(fd, fu1, &b, 100, 100, 180);
    		(void)nmg_pl_fu(fd, fu1->fumate_p, &b, 100, 100, 180);

    		(void)nmg_pl_fu(fd, fu2, &b, 100, 100, 180);
    		(void)nmg_pl_fu(fd, fu2->fumate_p, &b, 100, 100, 180);

    		(void)fclose(fd);
    		(void)nmg_tbl(&b, TBL_FREE, (long *)NULL);
    	}

	(void)nmg_tbl(&vert_list1, TBL_FREE, (long *)NULL);
	(void)nmg_tbl(&vert_list2, TBL_FREE, (long *)NULL);
}

/*	C R A C K S H E L L S
 *
 *	split the faces of two shells in preparation for performing boolean
 *	operations with them.
 */
void nmg_crackshells(s1, s2, tol)
struct shell *s1, *s2;
fastf_t tol;
{
	struct faceuse *fu1, *fu2;
	struct nmg_ptbl faces;

	NMG_CK_SHELL(s2);
	NMG_CK_SHELL_A(s2->sa_p);
	NMG_CK_SHELL(s1);
	NMG_CK_SHELL_A(s1->sa_p);

	if (!s1->fu_p || !s2->fu_p)
		rt_bomb("ERROR:shells must contain faces for boolean operations.");

	(void)nmg_tbl(&faces, TBL_INIT, (long *)NULL);

	if (NMG_EXTENT_OVERLAP(s1->sa_p->min_pt, s1->sa_p->max_pt,
	    s2->sa_p->min_pt, s2->sa_p->max_pt) ) {

		/* shells overlap */
		fu1 = s1->fu_p;
		do {/* check each of the faces in shell 1 to see
		     * if they overlap the extent of shell 2
		     */
			NMG_CK_FACEUSE(fu1);
			NMG_CK_FACE(fu1->f_p);
			NMG_CK_FACE_G(fu1->f_p->fg_p);

			if (nmg_tbl(&faces, TBL_LOC, &fu1->f_p->magic) < 0 &&
			    NMG_EXTENT_OVERLAP(s2->sa_p->min_pt,
			    s2->sa_p->max_pt, fu1->f_p->fg_p->min_pt,
			    fu1->f_p->fg_p->max_pt) ) {

				/* poly1 overlaps shell2 */
				fu2 = s2->fu_p;
				do {
					/* now we check the face of shell 1
					 * against each of the faces of shell
					 * 2.
		 			 */
					nmg_isect_faces(fu1, fu2, tol);

					/* try to avoid processing redundant
					 * faceuses.
					 */
					if (fu2->next != s2->fu_p &&
					    fu2->next->f_p == fu2->f_p)
						fu2 = fu2->next->next;
					else
						fu2 = fu2->next;

				} while (fu2 != s2->fu_p);
				(void)nmg_tbl(&faces, TBL_INS, &fu1->f_p->magic);
			    }

			/* try not to process redundant faceuses */
			if (fu1->next != s1->fu_p && fu1->next->f_p == fu1->f_p)
				fu1 = fu1->next->next;
			else
				fu1 = fu1->next;

		} while (fu1 != s1->fu_p);
	}

}
