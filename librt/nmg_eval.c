/*			N M G _ E V A L . C
 *
 *	Evaluate boolean operations on NMG objects
 *
 *  Authors -
 *	Lee A. Butler
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSnmg_eval[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "externs.h"
#include "machine.h"
#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"

/*
 *			N M G _ T O S S _ L O O P S
 *
 *	throw away all loops in the loopuse list "lu"
 */
static void nmg_toss_loops(lu, n)
struct loopuse **lu;
int n;
{
	int				i;
	struct faceuse			*fu;	

	for (i=0 ; i < n ; ++i) {
		fu = lu[i]->up.fu_p;
		nmg_klu(lu[i]);
		lu[i] = (struct loopuse *)NULL;	/* sanity */
		if (!fu->lu_p) {
			nmg_kfu(fu);
		}
	}
}

/*
 *			N M G _ R E V E R S E _ F A C E
 *
 *  Reverse the orientation of a face.
 *  Manipulate both the topological and geometric aspects of the face.
 */
void
nmg_reverse_face( fu )
register struct faceuse	*fu;
{
	register struct faceuse	*fumate;
	register vectp_t	v;

	NMG_CK_FACEUSE(fu);
	fumate = fu->fumate_p;
	NMG_CK_FACEUSE(fumate);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_FACE_G(fu->f_p->fg_p);

	/* reverse face normal vector */
	v = fu->f_p->fg_p->N;
	VREVERSE(v, v);
	v[H] *= -1.0;

	/* switch which face is "outside" face */
	if (fu->orientation == OT_SAME)  {
		if (fumate->orientation != OT_OPPOSITE)  {
			rt_log("nmg_reverse_face(fu=x%x) fu:SAME, fumate:%d\n",
				fu, fumate->orientation);
			rt_bomb("nmg_reverse_face() orientation clash\n");
		} else {
			fu->orientation = OT_OPPOSITE;
			fumate->orientation = OT_SAME;
		}
	} else if (fu->orientation == OT_OPPOSITE) {
		if (fumate->orientation != OT_SAME)  {
			rt_log("nmg_reverse_face(fu=x%x) fu:OPPOSITE, fumate:%d\n",
				fu, fumate->orientation);
			rt_bomb("nmg_reverse_face() orientation clash\n");
		} else {
			fu->orientation = OT_SAME;
			fumate->orientation = OT_OPPOSITE;
		}
	} else {
		/* Unoriented face? */
		rt_log("ERROR nmg_reverse_face(fu=x%x), orientation=%d.\n",
			fu, fu->orientation );
	}
}

/*
 *			N M G _ M V _ F U _ B E T W E E N _ S H E L L S
 *
 *  Move faceuse from 'src' shell to 'dest' shell.
 */
void
nmg_mv_fu_between_shells( dest, src, fu )
struct shell		*dest;
register struct shell	*src;
register struct faceuse	*fu;
{
	register struct faceuse	*fumate;

	NMG_CK_FACEUSE(fu);
	fumate = fu->fumate_p;
	NMG_CK_FACEUSE(fumate);

	if (fu->s_p != src) {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fu->s_p=x%x isnt src shell\n",
			dest, src, fu, fu->s_p );
		rt_bomb("fu->s_p isnt source shell\n");
	}
	if (fumate->s_p != src) {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fumate->s_p=x%x isn't src shell\n",
			dest, src, fu, fumate->s_p );
		rt_bomb("fumate->s_p isnt source shell\n");
	}

	/* Remove fu from src shell */
	src->fu_p = fu;
	DLLRM(src->fu_p, fu);
	if (src->fu_p == fu) {
		/* This was the last fu in the list, bad news */
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fumate=x%x not in src shell\n",
			dest, src, fu, fumate );
		rt_bomb("src shell emptied before finding fumate\n");
	}

	/* Remove fumate from src shell */
	src->fu_p = fumate;
	DLLRM(src->fu_p, fumate);
	if (src->fu_p == fumate) {
		/* This was the last fu in the list, tidy up */
		src->fu_p = (struct faceuse *)NULL;
	}

	/* Add fu and fumate to dest shell */
	DLLINS(dest->fu_p, fu);
	fu->s_p = dest;
	DLLINS(dest->fu_p, fumate);
	fumate->s_p = dest;
}

#define BACTION_KILL			1
#define BACTION_RETAIN			2
#define BACTION_RETAIN_AND_FLIP		3

static struct nmg_ptbl	*classified_shell_loops[6];

/* Actions are listed: onAinB, onAonB, onAoutB, inAonB, onAonB, outAonB */
static int		subtraction_actions[6] = {
	BACTION_KILL,
	BACTION_RETAIN,		/* _IF_OPPOSITE */
	BACTION_RETAIN,
	BACTION_RETAIN_AND_FLIP,
	BACTION_KILL,
	BACTION_KILL
};

static int		union_actions[6] = {
	BACTION_KILL,
	BACTION_RETAIN,		/* _IF_SAME */
	BACTION_RETAIN,
	BACTION_KILL,
	BACTION_KILL,
	BACTION_RETAIN
};

static int		intersect_actions[6] = {
	BACTION_RETAIN,
	BACTION_RETAIN,		/* If opposite, ==> non-manifold */
	BACTION_KILL,
	BACTION_RETAIN,
	BACTION_KILL,
	BACTION_KILL
};

/*
 *			N M G _ A C T _ O N _ L O O P
 */
void
nmg_act_on_loop( ltbl, action, dest, src )
struct nmg_ptbl	*ltbl;
int		action;
struct shell	*dest;
struct shell	*src;
{
	struct loopuse	**lup;
	struct loopuse	*lu;
	struct faceuse	*fu;
	int		i;

	NMG_CK_SHELL( dest );
	NMG_CK_SHELL( src );

	switch( action )  {
	default:
		rt_bomb("nmg_act_on_loop: bad action\n");
		/* NOTREACHED */

	case BACTION_KILL:
		nmg_toss_loops( (struct loopuse **)(ltbl->buffer), ltbl->end );
		return;

	case BACTION_RETAIN:
		i = ltbl->end-1;
		lup = &((struct loopuse **)(ltbl->buffer))[i];
		for( ; i >= 0; i--, lup-- ) {
			lu = *lup;
			*lup = 0;			/* sanity */
			NMG_CK_LOOPUSE(lu);
			fu = lu->up.fu_p;
			NMG_CK_FACEUSE(fu);
			if( fu->s_p == dest )  continue;
			nmg_mv_fu_between_shells( dest, src, fu );
		}
		return;

	case BACTION_RETAIN_AND_FLIP:
		i = ltbl->end-1;
		lup = &((struct loopuse **)(ltbl->buffer))[i];
		for( ; i >= 0; i--, lup-- ) {
			lu = *lup;
			*lup = 0;			/* sanity */
			NMG_CK_LOOPUSE(lu);
			fu = lu->up.fu_p;
			NMG_CK_FACEUSE(fu);
			nmg_reverse_face( fu );		/* flip */
			if( fu->s_p == dest )  continue;
			nmg_mv_fu_between_shells( dest, src, fu );
		}
		return;
	}
}

/*	S U B T R A C T I O N
 *
 *	reshuffle the faces of two shells to perform a subtraction of
 *	one shell from the other.  Shell "sB" disappears and shell "sA"
 *	contains:
 *		the faces that were in sA outside of sB
 *		the faces that were in sB inside of sA (normals flipped)
 */
void subtraction(sA, AinB, AonB, AoutB, sB, BinA, BonA, BoutA)
struct shell *sA, *sB;
struct nmg_ptbl *AinB, *AonB, *AoutB, *BinA, *BonA, *BoutA;
{
	struct nmg_ptbl faces;
	struct loopuse *lu;
	struct faceuse *fu;
	union {
		struct loopuse **lu;
		long **l;
	} p;
	int i;

	(void)nmg_tbl(&faces, TBL_INIT, (long *)NULL);

	classified_shell_loops[0] = AinB;
	classified_shell_loops[1] = AonB;
	classified_shell_loops[2] = AoutB;
	classified_shell_loops[3] = BinA;
	classified_shell_loops[4] = BonA;
	classified_shell_loops[5] = BoutA;

	for( i=0; i<6; i++ )  {
		nmg_act_on_loop( classified_shell_loops[i],
			subtraction_actions[i],
			sA, sB );
	}

	/* Plot the result */
	if (rt_g.NMG_debug & DEBUG_SUBTRACT) {
		FILE *fp, *fopen();

		if ((fp=fopen("sub.pl", "w")) == (FILE *)NULL) {
			(void)perror("sub.pl");
			exit(-1);
		}
    		rt_log("plotting sub.pl\n");
		nmg_pl_s( fp, sA );
		(void)fclose(fp);
	}


	if (sB->fu_p) {
		rt_log("Why does shell B still have faces?\n");
	} else if (!sB->lu_p && !sB->eu_p && !sB->vu_p) {
		struct nmgregion	*r;

		r = sB->r_p;
		nmg_ks(sB);
		if (!r->s_p && r->next != r) nmg_kr(r);
	}


	(void)nmg_tbl(&faces, TBL_FREE, (long *)NULL);
}
/*	A D D I T I O N
 *
 *	add (union) two shells together
 *
 */
void addition(sA, AinB, AonB, AoutB, sB, BinA, BonA, BoutA)
struct shell *sA, *sB;
struct nmg_ptbl *AinB, *AonB, *AoutB, *BinA, *BonA, *BoutA;
{
	struct nmg_ptbl faces;
	struct loopuse *lu;
	struct faceuse *fu;
	union {
		struct loopuse **lu;
		long **l;
	} p;
	int i;


	(void)nmg_tbl(&faces, TBL_INIT, (long *)NULL);

	classified_shell_loops[0] = AinB;
	classified_shell_loops[1] = AonB;
	classified_shell_loops[2] = AoutB;
	classified_shell_loops[3] = BinA;
	classified_shell_loops[4] = BonA;
	classified_shell_loops[5] = BoutA;

	for( i=0; i<6; i++ )  {
		nmg_act_on_loop( classified_shell_loops[i],
			union_actions[i],
			sA, sB );
	}

	if (sB->fu_p) {
		rt_log("Why does shell B still have faces?\n");
	} else if (!sB->lu_p && !sB->eu_p && !sB->vu_p) {
		nmg_ks(sB);
	}

	(void)nmg_tbl(&faces, TBL_FREE, (long *)NULL);
}

/*	I N T E R S E C T I O N
 *
 *	resulting shell is all faces of sA in sB and all faces of sB in sA
 */
void intersection(sA, AinB, AonB, AoutB, sB, BinA, BonA, BoutA)
struct shell *sA, *sB;
struct nmg_ptbl *AinB, *AonB, *AoutB, *BinA, *BonA, *BoutA;
{
	struct nmg_ptbl faces;
	struct loopuse *lu;
	struct faceuse *fu;
	union {
		struct loopuse **lu;
		long **l;
	} p;
	int i;


	(void)nmg_tbl(&faces, TBL_INIT, (long *)NULL);

	classified_shell_loops[0] = AinB;
	classified_shell_loops[1] = AonB;
	classified_shell_loops[2] = AoutB;
	classified_shell_loops[3] = BinA;
	classified_shell_loops[4] = BonA;
	classified_shell_loops[5] = BoutA;

	for( i=0; i<6; i++ )  {
		nmg_act_on_loop( classified_shell_loops[i],
			intersect_actions[i],
			sA, sB );
	}

	if (sB->fu_p) {
		rt_log("Why does shell B still have faces?\n");
	} else if (!sB->lu_p && !sB->eu_p && !sB->vu_p) {
		nmg_ks(sB);
	}

	(void)nmg_tbl(&faces, TBL_FREE, (long *)NULL);
}
