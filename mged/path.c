/*
 *			P A T H . C
 *
 * Functions -
 *	drawHobj	Call drawsolid for all solids in an object
 *	pathHmat	Find matrix across a given path
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
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include	<stdio.h>
#include "ged_types.h"
#include "../h/db.h"
#include "solid.h"
#include "objdir.h"
#include "ged.h"

int	regmemb;	/* # of members left to process in a region */
char	memb_oper;	/* operation for present member of processed region */
int	reg_pathpos;	/* pathpos of a processed region */

struct directory	*path[MAX_PATH];	/* Record of current path */

/*
 *			D R A W H O B J
 *
 * This routine is used to get an object drawn.
 * The actual drawing of solids is performed by drawsolid(),
 * but all transformations down the path are done here.
 */
void
drawHobj( dp, flag, pathpos, old_xlate, regionid )
register struct directory *dp;
matp_t old_xlate;
{
	static union record rec;	/* local record buffer */
	auto mat_t new_xlate;		/* Accumulated xlation matrix */
	auto int i;

	if( pathpos >= MAX_PATH )  {
		(void)printf("nesting exceeds %d levels\n", MAX_PATH );
		for(i=0; i<MAX_PATH; i++)
			(void)printf("/%s", path[i]->d_namep );
		(void)putchar('\n');
		return;			/* ERROR */
	}

	/*
	 * Load the record into local record buffer
	 */
	db_getrec( dp, &rec, 0 );

	if( rec.u_id == ID_SOLID ||
	    rec.u_id == ID_ARS_A ||
	    rec.u_id == ID_B_SPL_HEAD ||
	    rec.u_id == ID_P_HEAD )  {
		register struct solid *sp;	/* XXX */
		/*
		 * Enter new solid (or processed region) into displaylist.
		 */
		path[pathpos] = dp;

		GET_SOLID( sp );
		if( sp == SOLID_NULL )
			return;		/* ERROR */
		if( drawHsolid( sp, flag, pathpos, old_xlate, &rec, regionid ) != 1 ) {
			FREE_SOLID( sp );
		}
		return;
	}

	if( rec.u_id != ID_COMB )  {
		(void)printf("drawobj:  defective input '%c'\n", rec.u_id );
		return;			/* ERROR */
	}
	if( rec.c.c_flags == 'R' )  {
		if( regionid != 0 )
			(void)printf("regionid %d overriden by %d\n",
				regionid, rec.c.c_regionid );
		regionid = rec.c.c_regionid;
	}

	/*
	 *  This node is a combination (eg, a directory).
	 *  Process all the arcs (eg, directory members).
	 */
	if( drawreg && rec.c.c_flags == 'R' && dp->d_len > 1 ) {
		if( regmemb >= 0  ) {
			(void)printf(
			"ERROR: region (%s) is member of region (%s)\n",
				rec.c.c_name,
				path[reg_pathpos]->d_namep);
			return;	/* ERROR */
		}
		/* Well, we are processing regions and this is a region */
		/* if region has only 1 member, don't process as a region */
		if( dp->d_len > 2) {
			regmemb = dp->d_len-1;
			reg_pathpos = pathpos;
		}

	}

	for( i=1; i < dp->d_len; i++ )  {
		register struct member *mp;		/* XXX */
		register struct directory *nextdp;	/* temporary */

		db_getrec( dp, &rec, i );
		mp = &rec.M;
		path[pathpos] = dp;
		if( regmemb > 0  ) { 
			regmemb--;
			memb_oper = rec.M.m_relation;
		}
		if( (nextdp = lookup( mp->m_instname, LOOKUP_NOISY )) == DIR_NULL )
			continue;
		if( mp->m_brname[0] != '\0' )  {
			register struct directory *tdp;		/* XXX */
			/*
			 * Create an alias.  First step towards full
			 * branch naming.  User is responsible for his
			 * branch names being unique.
			 */
			tdp = lookup( mp->m_brname, LOOKUP_QUIET );
			if( tdp != DIR_NULL )
				nextdp = tdp; /* use existing alias */
			else
				nextdp = dir_add( mp->m_brname,
					nextdp->d_addr,	DIR_BRANCH,
					nextdp->d_len );
		}

		/* s' = M3 . M2 . M1 . s
		 * Here, we start at M3 and descend the tree.
		 */
		mat_mul(new_xlate, old_xlate, mp->m_mat);

		/* Recursive call */
		drawHobj(
			nextdp,
			(mp->m_relation != SUBTRACT) ? ROOT : INNER,
			pathpos + 1,
			new_xlate,
			regionid
		);
	}
}

/*
 *  			P A T H h M A T
 *  
 *  Find the transformation matrix obtained when traversing
 *  the arc indicated in sp->s_path[].
 */
void
pathHmat( sp, matp )
register struct solid *sp;
matp_t matp;
{
	register struct directory *parentp;
	register struct directory *kidp;
	register int i, j;
	auto mat_t tmat;
	auto union record rec;

	mat_idn( matp );
	/* Omit s_path[sp->s_last] -- it's a solid */
	for( i=0; i < sp->s_last; i++ )  {
		parentp = sp->s_path[i];
		kidp = sp->s_path[i+1];
		for( j=1; j < parentp->d_len; j++ )  {
			/* Examine Member records */
			db_getrec( parentp, &rec, j );
			if( strcmp( kidp->d_namep, rec.M.m_instname ) == 0 ||
			    strcmp( kidp->d_namep, rec.M.m_brname ) == 0 )  {
				    mat_mul( tmat, matp, rec.M.m_mat );
				    mat_copy( matp, tmat );
				    goto next_level;
			}
		}
		(void)printf("pathHmat: unable to follow %s/%s path\n",
			parentp->d_namep, kidp->d_namep );
		return;
next_level:
		;
	}
}
