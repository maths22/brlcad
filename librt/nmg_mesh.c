/*
 *			N M G _ M E S H . C
 *
 *	Meshing routines for n-Manifold Geometry
 *  This stuff is destined to be absorbed into nmg_fuse.c.
 *  "meshing" here refers to the sorting of faceuses around an edge
 *  as two edges sharing the same end points (vertex structs) are fused.
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
#include "rtlist.h"
#include "nmg.h"
#include "raytrace.h"


/*
 *			N M G _ I S _ A N G L E _ I N _ W E D G E
 *
 *  Determine if T lies within angle AB, such that A < T < B.
 *  The angle B is expected to be "more ccw" than A.
 *  Because of the wrap from 2pi to 0, B may have a smaller numeric value.
 *
 *  Returns -
 *	-2	t is equal to a
 *	-1	t is equal to b
 *	 0	t is outside angle ab
 *	 1	t is inside angle ab
 */
int
nmg_is_angle_in_wedge( a, b, t )
double	a;
double	b;
double	t;
{
	/* XXX What tolerance to use here (in radians)? */
	if( NEAR_ZERO( a-t, 1.0e-8 ) )  return -2;
	if( NEAR_ZERO( b-t, 1.0e-8 ) )  return -1;

	/* If A==B, if T is not also equal, it's outside the wedge */
	if( NEAR_ZERO( a-b, 1.0e-8 ) )  return 0;

	if( b < a )  {
		/* B angle has wrapped past zero, add on 2pi */
		if( t <= b )  {
			/* Range is A..0, 0..B, and 0<t<B; so T is in wedge */
			return 1;
		}
		b += rt_twopi;
	}
	if( NEAR_ZERO( b-t, 1.0e-8 ) )  return -1;

	if( t < a )  return 0;
	if( t > b )  return 0;
	return 1;
}

/*
 *			N M G _ R A D I A L _ J O I N _ E U
 *
 *	Make all the edgeuses around eu2's edge to refer to eu1's edge,
 *	taking care to organize them into the proper angular orientation,
 *	so that the attached faces are correctly arranged radially
 *	around the edge.
 *
 *	This depends on both edges being part of face loops,
 *	with vertex and face geometry already associated.
 *
 *  The two edgeuses being joined might well be from separate shells,
 *  so the issue of preserving (simple) faceuse orientation parity
 *  (SAME, OPPOSITE, OPPOSITE, SAME, ...)
 *  can't be used here -- that only applies to faceuses from the same shell.
 *
 *  Some of the edgeuses around both edges may be wires.
 */
void
nmg_radial_join_eu(eu1, eu2, tol)
struct edgeuse		*eu1;
struct edgeuse		*eu2;
CONST struct rt_tol	*tol;
{
	struct edgeuse	*original_eu1 = eu1;
	struct edgeuse	*nexteu;
	struct edgeuse	*eur;
	struct faceuse	*fu1;
	struct faceuse	*fu2;
	int		iteration1, iteration2;
	vect_t		xvec, yvec, zvec;
	fastf_t		abs1;
	fastf_t		abs2;
	fastf_t		absr;

	NMG_CK_EDGEUSE(eu1);
	NMG_CK_EDGEUSE(eu1->radial_p);
	NMG_CK_EDGEUSE(eu1->eumate_p);
	NMG_CK_EDGEUSE(eu2);
	NMG_CK_EDGEUSE(eu2->radial_p);
	NMG_CK_EDGEUSE(eu2->eumate_p);
	RT_CK_TOL(tol);

	if( eu1->e_p == eu2->e_p )  return;

	if( !NMG_ARE_EUS_ADJACENT(eu1, eu2) )
		rt_bomb("nmg_radial_join_eu() edgeuses don't share vertices.\n");

	if( eu1->vu_p->v_p == eu1->eumate_p->vu_p->v_p )  rt_bomb("nmg_radial_join_eu(): 0 length edge (topology)\n");

	if( rt_pt3_pt3_equal( eu1->vu_p->v_p->vg_p->coord,
	    eu1->eumate_p->vu_p->v_p->vg_p->coord, tol ) )
		rt_bomb("nmg_radial_join_eu(): 0 length edge (geometry)\n");

	/* Ensure faces are of same orientation, if both eu's have faces */
	fu1 = nmg_find_fu_of_eu(eu1);
	fu2 = nmg_find_fu_of_eu(eu2);
	if( fu1 && fu2 )  {
		if( fu1->orientation != fu2->orientation ){
			eu2 = eu2->eumate_p;
			fu2 = nmg_find_fu_of_eu(eu2);
			if( fu1->orientation != fu2->orientation )
				rt_bomb( "nmg_radial_join_eu(): Cannot find matching orientations for faceuses\n" );
		}
	}

	/*  Construct local coordinate system for this edge,
	 *  so all angles can be measured relative to a common reference.
	 */
	nmg_eu_2vecs_perp( xvec, yvec, zvec, original_eu1, tol );

	if (rt_g.NMG_debug & DEBUG_MESH_EU ) {
		rt_log("nmg_radial_join_eu(eu1=x%x, eu2=x%x) e1=x%x, e2=x%x\n",
			eu1, eu2,
			eu1->e_p, eu2->e_p);
		nmg_euprint("\tJoining", eu1);
		nmg_euprint("\t     to", eu2);
		rt_log( "Faces around eu1:\n" );
		nmg_pr_fu_around_eu_vecs( eu1, xvec, yvec, zvec, tol );
		rt_log( "Faces around eu2:\n" );
		nmg_pr_fu_around_eu_vecs( eu2, xvec, yvec, zvec, tol );
	}

	for ( iteration1=0; eu2 && iteration1 < 10000; iteration1++ ) {
		int	code = 0;
		struct edgeuse	*first_eu1 = eu1;
		int	wire_skip = 0;
		/* Resume where we left off from last eu2 insertion */

		/* because faces are always created with counter-clockwise
		 * exterior loops and clockwise interior loops, radial
		 * edgeuses will never share the same vertex.  We thus make
		 * sure that eu2 is an edgeuse which might be radial to eu1
		 */
		if (eu2->vu_p->v_p == eu1->vu_p->v_p)
			eu2 = eu2->eumate_p;

		/* find a place to insert eu2 around eu1's edge */
		for ( iteration2=0; iteration2 < 10000; iteration2++ ) {
			struct faceuse	*fur;

			abs1 = abs2 = absr = -rt_twopi;

			eur = eu1->radial_p;
			NMG_CK_EDGEUSE(eur);

			fu2 = nmg_find_fu_of_eu(eu2);
			if( fu2 == (struct faceuse *)NULL )  {
				/* eu2 is a wire, it can go anywhere */
rt_log("eu2=x%x is a wire, insert after eu1=x%x\n", eu2, eu1);
				goto insert;
			}
			fu1 = nmg_find_fu_of_eu(eu1);
			if( fu1 == (struct faceuse *)NULL )  {
				/* eu1 is a wire, skip on to real face eu */
rt_log("eu1=x%x is a wire, skipping on\n", eu1);
				wire_skip++;
				goto cont;
			}
			fur = nmg_find_fu_of_eu(eur);
			while( fur == (struct faceuse *)NULL )  {
				/* eur is wire, advance eur */
rt_log("eur=x%x is a wire, advancing to non-wire eur\n", eur);
				eur = eur->eumate_p->radial_p;
				wire_skip++;
				if( eur == eu1->eumate_p )  {
rt_log("went all the way around\n");
					/* Went all the way around */
					goto insert;
				}
				fur = nmg_find_fu_of_eu(eur);
			}
			NMG_CK_FACEUSE(fu1);
			NMG_CK_FACEUSE(fu2);
			NMG_CK_FACEUSE(fur);

			/*
			 *  Can't just check for shared fg here,
			 *  the angle changes by +/- 180 degrees,
			 *  depending on which side of the eu the loop is on
			 *  along this edge.
			 */
			abs1 = nmg_measure_fu_angle( eu1, xvec, yvec, zvec );
			abs2 = nmg_measure_fu_angle( eu2, xvec, yvec, zvec );
			absr = nmg_measure_fu_angle( eur, xvec, yvec, zvec );

			if (rt_g.NMG_debug & DEBUG_MESH_EU )  {
				rt_log("  abs1=%g, abs2=%g, absr=%g\n",
					abs1*rt_radtodeg,
					abs2*rt_radtodeg,
					absr*rt_radtodeg );
			}

			/* If abs1 == absr, warn about unfused faces, and skip. */
			if( NEAR_ZERO( abs1-absr, 1.0e-8 ) )  {
				if( fu1->f_p->fg_p == fur->f_p->fg_p )  {
					/* abs1 == absr, faces are fused, don't insert here. */
					if (rt_g.NMG_debug & DEBUG_MESH_EU )  {
						rt_log("fu1 and fur share face geometry x%x (flip1=%d, flip2=%d), skip\n",
							fu1->f_p->fg_p, fu1->f_p->flip, fur->f_p->flip );
					}
					goto cont;
				}

				rt_log("nmg_radial_join_eu: WARNING 2 faces should have been fused, may be ambiguous.\n  abs1=%e, absr=%e, asb2=%e\n",
					abs1*rt_radtodeg, absr*rt_radtodeg, abs2*rt_radtodeg);
				rt_log("  fu1=x%x, f1=x%x, f1->flip=%d, fg1=x%x\n",
					fu1, fu1->f_p, fu1->f_p->flip, fu1->f_p->fg_p );
				rt_log("  fu2=x%x, f2=x%x, f2->flip=%d, fg2=x%x\n",
					fu2, fu2->f_p, fu2->f_p->flip, fu2->f_p->fg_p );
				rt_log("  fur=x%x, fr=x%x, fr->flip=%d, fgr=x%x\n",
					fur, fur->f_p, fur->f_p->flip, fur->f_p->fg_p );
				PLPRINT("  fu1", fu1->f_p->fg_p->N );
				PLPRINT("  fu2", fu2->f_p->fg_p->N );
				PLPRINT("  fur", fur->f_p->fg_p->N );
				{
					int debug = rt_g.NMG_debug;
					rt_g.NMG_debug |= DEBUG_MESH;
					if( nmg_two_face_fuse(fu1->f_p, fur->f_p, tol) == 0 )
						rt_bomb("faces didn't fuse?\n");
					rt_g.NMG_debug = debug;
				}
				rt_log("  nmg_radial_join_eu() skipping this eu\n");
				goto cont;
			}

			/*
			 *  If abs1 < abs2 < absr
			 *  (taking into account 360 wrap),
			 *  then insert face here.
			 *  Special handling if abs1==abs2 or abs2==absr.
			 */
			code = nmg_is_angle_in_wedge( abs1, absr, abs2 );
			if (rt_g.NMG_debug & DEBUG_MESH_EU )
				rt_log("    code=%d %s\n", code, (code!=0)?"INSERT_HERE":"skip");
			if( code > 0 )  break;
			if( code == -1 )  {
				/* absr == abs2 */
				break;
			}
			if( code <= -2 )  {
				/* abs1 == abs2 */
				break;
			}

cont:
			if( iteration2 > 9997 )  rt_g.NMG_debug |= DEBUG_MESH_EU;
			/* If eu1 is only one pair of edgeuses, done */
			if( eu1 == eur->eumate_p )  break;
			eu1 = eur->eumate_p;
			if( eu1 == first_eu1 )  {
				/* If all eu's were wires, here is fine */
				if( wire_skip >= iteration2 )  break;
				/* Nope, something bad happened */
				rt_bomb("nmg_radial_join_eu():  went full circle, no face insertion point.\n");
				break;
			}
		}
		if(iteration2 >= 10000)  {
			rt_bomb("nmg_radial_join_eu: infinite loop (2)\n");
		}

		/* find the next use of the edge eu2 is on.  If eu2 and it's
		 * mate are the last uses of the edge, there will be no next
		 * edgeuse to move. (Loop termination condition).
		 */
insert:
		nexteu = eu2->radial_p;
		if (nexteu == eu2->eumate_p)
			nexteu = (struct edgeuse *)NULL;

		if (rt_g.NMG_debug & DEBUG_MESH_EU)  {
			rt_log("  Inserting.  code=%d\n", code);
			rt_log("joining eu1=x%x eu2=x%x with abs1=%g, absr=%g\n",
				eu1, eu2,
				abs1*rt_radtodeg, absr*rt_radtodeg);
		}

		/*
		 *  Make eu2 radial to eu1.
		 *  This should insert eu2 between eu1 and eu1->radial_p
		 *  (which may be less far around than eur, but thats OK).
		 */
		nmg_moveeu(eu1, eu2);

		if (rt_g.NMG_debug & DEBUG_MESH_EU)  {
			rt_log("After nmg_moveeu(), faces around original_eu1 are:\n");
			nmg_pr_fu_around_eu_vecs( original_eu1, xvec, yvec, zvec, tol );
		}

		/* Proceed to the next source edgeuse */
		eu2 = nexteu;
	}
	if( iteration1 >= 10000 )  rt_bomb("nmg_radial_join_eu:  infinite loop (1)\n");

	/* This should catch errors, anyway */
	NMG_CK_EDGEUSE(original_eu1);
	if( nmg_check_radial(original_eu1, tol) )  {
		nmg_plot_lu_around_eu( "around", original_eu1, tol );
		rt_bomb("nmg_radial_join_eu(): radial orientation ERROR\n");
	}

	if (rt_g.NMG_debug & DEBUG_MESH_EU)  rt_log("nmg_radial_join_eu: END\n");
}

/*
 *			N M G _ M E S H _ T W O _ F A C E S
 *
 *  Actuall do the work of meshing two faces.
 *  The two fu arguments may be the same, which causes the face to be
 *  meshed against itself.
 *
 *  The return is the number of edges meshed.
 */
int
nmg_mesh_two_faces(fu1, fu2, tol)
register struct faceuse *fu1, *fu2;
CONST struct rt_tol	*tol;
{
	struct loopuse	*lu1;
	struct loopuse	*lu2;
	struct edgeuse	*eu1;
	struct edgeuse	*eu2;
	struct vertex	*v1a, *v1b;
	struct edge	*e1;
	pointp_t	pt1, pt2;
	int		count = 0;

	NMG_CK_FACEUSE(fu1);
	NMG_CK_FACEUSE(fu2);
	RT_CK_TOL(tol);

	/* Visit all the loopuses in faceuse 1 */
	for (RT_LIST_FOR(lu1, loopuse, &fu1->lu_hd)) {
		NMG_CK_LOOPUSE(lu1);
		/* Ignore self-loops */
		if (RT_LIST_FIRST_MAGIC(&lu1->down_hd) != NMG_EDGEUSE_MAGIC)
			continue;

		/* Visit all the edgeuses in loopuse1 */
		for(RT_LIST_FOR(eu1, edgeuse, &lu1->down_hd)) {
			NMG_CK_EDGEUSE(eu1);

			v1a = eu1->vu_p->v_p;
			v1b = eu1->eumate_p->vu_p->v_p;
			NMG_CK_VERTEX(v1a);
			NMG_CK_VERTEX(v1b);
			e1 = eu1->e_p;
			NMG_CK_EDGE(e1);
			if (rt_g.NMG_debug & DEBUG_MESH)  {
				pt1 = v1a->vg_p->coord;
				pt2 = v1b->vg_p->coord;
				rt_log("ref_e=%8x v:%8x--%8x (%g, %g, %g)->(%g, %g, %g)\n",
					e1, v1a, v1b,
					V3ARGS(pt1), V3ARGS(pt2) );
			}

			/* Visit all the loopuses in faceuse2 */
			for (RT_LIST_FOR(lu2, loopuse, &fu2->lu_hd)) {
				/* Ignore self-loops */
				if(RT_LIST_FIRST_MAGIC(&lu2->down_hd) != NMG_EDGEUSE_MAGIC)
					continue;
				/* Visit all the edgeuses in loopuse2 */
				for( RT_LIST_FOR(eu2, edgeuse, &lu2->down_hd) )  {
					NMG_CK_EDGEUSE(eu2);
					if (rt_g.NMG_debug & DEBUG_MESH) {
						pt1 = eu2->vu_p->v_p->vg_p->coord;
						pt2 = eu2->eumate_p->vu_p->v_p->vg_p->coord;
						rt_log("\te:%8x v:%8x--%8x (%g, %g, %g)->(%g, %g, %g)\n",
							eu2->e_p,
							eu2->vu_p->v_p,
							eu2->eumate_p->vu_p->v_p,
							V3ARGS(pt1), V3ARGS(pt2) );
					}

					/* See if already shared */
					if( eu2->e_p == e1 ) continue;
					if( (eu2->vu_p->v_p == v1a &&
					     eu2->eumate_p->vu_p->v_p == v1b) ||
					    (eu2->eumate_p->vu_p->v_p == v1a &&
					     eu2->vu_p->v_p == v1b) )  {
						nmg_radial_join_eu(eu1, eu2, tol);
					     	count++;
					 }
				}
			}
		}
	}
	return count;
}

/*
 *			N M G _ M E S H _ F A C E S
 *
 *  Scan through all the edges of fu1 and fu2, ensuring that all
 *  edges involving the same vertex pair are indeed shared.
 *  This means worrying about merging ("meshing") all the faces in the
 *  proper radial orientation around the edge.
 *  XXX probably should return(count);
 */
void
nmg_mesh_faces(fu1, fu2, tol)
struct faceuse		*fu1;
struct faceuse		*fu2;
CONST struct rt_tol	*tol;
{
	int	count = 0;

	NMG_CK_FACEUSE(fu1);
	NMG_CK_FACEUSE(fu2);
	RT_CK_TOL(tol);

    	if (rt_g.NMG_debug & DEBUG_MESH_EU && rt_g.NMG_debug & DEBUG_PLOTEM) {
    		static int fnum=1;
    	    	nmg_pl_2fu( "Before_mesh%d.pl", fnum++, fu1, fu2, 1 );
    	}

	if (rt_g.NMG_debug & DEBUG_MESH_EU)
		rt_log("meshing self (fu1 %8x)\n", fu1);
	count += nmg_mesh_two_faces( fu1, fu1, tol );

	if (rt_g.NMG_debug & DEBUG_MESH_EU)
		rt_log("meshing self (fu2 %8x)\n", fu2);
	count += nmg_mesh_two_faces( fu2, fu2, tol );

	if (rt_g.NMG_debug & DEBUG_MESH_EU)
		rt_log("meshing to other (fu1:%8x fu2:%8x)\n", fu1, fu2);
	count += nmg_mesh_two_faces( fu1, fu2, tol );

    	if (rt_g.NMG_debug & DEBUG_MESH_EU && rt_g.NMG_debug & DEBUG_PLOTEM) {
    		static int fno=1;
    	    	nmg_pl_2fu( "After_mesh%d.pl", fno++, fu1, fu2, 1 );
    	}
}

/*
 *			N M G _ M E S H _ F A C E _ S H E L L
 *
 *  The return is the number of edges meshed.
 */
int
nmg_mesh_face_shell( fu1, s, tol )
struct faceuse	*fu1;
struct shell	*s;
CONST struct rt_tol	*tol;
{
	register struct faceuse	*fu2;
	int		count = 0;

	NMG_CK_FACEUSE(fu1);
	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	count += nmg_mesh_two_faces( fu1, fu1 );
	for( RT_LIST_FOR( fu2, faceuse, &s->fu_hd ) )  {
		NMG_CK_FACEUSE(fu2);
		count += nmg_mesh_two_faces( fu2, fu2, tol );
		count += nmg_mesh_two_faces( fu1, fu2, tol );
	}
	/* XXX What about wire edges in the shell? */
	return count;
}

/*
 *			N M G _ M E S H _ S H E L L _ S H E L L
 *
 *  Mesh every edge in shell 1 with every edge in shell 2.
 *  The return is the number of edges meshed.
 *
 *  Does not use nmg_mesh_face_shell() to keep face/self meshing
 *  to the absolute minimum necessary.
 */
int
nmg_mesh_shell_shell( s1, s2, tol )
struct shell	*s1;
struct shell	*s2;
CONST struct rt_tol	*tol;
{
	struct faceuse	*fu1;
	struct faceuse	*fu2;
	int		count = 0;

	NMG_CK_SHELL(s1);
	NMG_CK_SHELL(s2);
	RT_CK_TOL(tol);

nmg_region_v_unique( s1->r_p, tol );
nmg_region_v_unique( s2->r_p, tol );

	/* First, mesh all faces of shell 2 with themselves */
	for( RT_LIST_FOR( fu2, faceuse, &s2->fu_hd ) )  {
		NMG_CK_FACEUSE(fu2);
		count += nmg_mesh_two_faces( fu2, fu2, tol );
	}

	/* Visit every face in shell 1 */
	for( RT_LIST_FOR( fu1, faceuse, &s1->fu_hd ) )  {
		NMG_CK_FACEUSE(fu1);

		/* First, mesh each face in shell 1 with itself */
		count += nmg_mesh_two_faces( fu1, fu1, tol );

		/* Visit every face in shell 2 */
		for( RT_LIST_FOR( fu2, faceuse, &s2->fu_hd ) )  {
			NMG_CK_FACEUSE(fu2);
			count += nmg_mesh_two_faces( fu1, fu2, tol );
		}
	}

	/* XXX What about wire edges in the shell? */

	/* Visit every wire loop in shell 1 */

	/* Visit every wire edge in shell 1 */

	return count;
}
