/*
 *  			M A T E R . C
 *  
 *  Code to deal with establishing and maintaining the tables which
 *  map region ID codes into worthwhile material information
 *  (colors and outboard database "handles").
 *
 *  Functions -
 *	color_addrec	Called by rt_dirbuild on startup
 *	color_map	Map one region reference to a material
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
static char RCSmater[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "mater.h"
#include "raytrace.h"
#include "./debug.h"

/*
 *  It is expected that entries on this mater list will be sorted
 *  in strictly ascending order, with no overlaps (ie, monotonicly
 * increasing).
 */
struct mater *MaterHead = MATER_NULL;

void	rt_insert_color();

static void
rt_pr_mater( mp )
register struct mater *mp;
{
	(void)fprintf(stderr, "%5d..%d\t", mp->mt_low, mp->mt_high );
	(void)fprintf(stderr, "%d,%d,%d\t", mp->mt_r, mp->mt_g, mp->mt_b);
}

/*
 *  			R T _ C O L O R _ A D D R E C
 *  
 *  Called from rt_dirbuild() when initially scanning database.
 */
void
rt_color_addrec( recp, addr )
union record *recp;
long addr;
{
	register struct mater *mp;

	GETSTRUCT( mp, mater );
	mp->mt_low = recp->md.md_low;
	mp->mt_high = recp->md.md_hi;
	mp->mt_r = recp->md.md_r;
	mp->mt_g = recp->md.md_g;
	mp->mt_b = recp->md.md_b;
/*	mp->mt_handle = rt_strdup( recp->md.md_material ); */
	mp->mt_daddr = addr;
	rt_insert_color( mp );
}

/*
 *  			I N S E R T _ C O L O R
 *
 *  While any additional database records are created and written here,
 *  it is the responsibility of the caller to color_putrec(newp) if needed.
 */
void
rt_insert_color( newp )
register struct mater *newp;
{
	register struct mater *mp;
	register struct mater *zot;

	if( MaterHead == MATER_NULL || newp->mt_high < MaterHead->mt_low )  {
		/* Insert at head of list */
		newp->mt_forw = MaterHead;
		MaterHead = newp;
		return;
	}
	if( newp->mt_low < MaterHead->mt_low )  {
		/* Insert at head of list, check for redefinition */
		newp->mt_forw = MaterHead;
		MaterHead = newp;
		goto check_overlap;
	}
	for( mp = MaterHead; mp != MATER_NULL; mp = mp->mt_forw )  {
		if( mp->mt_low == newp->mt_low  &&
		    mp->mt_high <= newp->mt_high )  {
			(void)fprintf(stderr,"dropping overwritten entry:\n");
			newp->mt_forw = mp->mt_forw;
			rt_pr_mater( mp );
			*mp = *newp;		/* struct copy */
			free( newp );
			newp = mp;
			goto check_overlap;
		}
		if( mp->mt_low  < newp->mt_low  &&
		    mp->mt_high > newp->mt_high )  {
			/* New range entirely contained in old range; split */
			(void)fprintf(stderr,"Splitting into 3 ranges\n");
			GETSTRUCT( zot, mater );
			*zot = *mp;		/* struct copy */
			zot->mt_daddr = MATER_NO_ADDR;
			/* zot->mt_high = mp->mt_high; */
			zot->mt_low = newp->mt_high+1;
			mp->mt_high = newp->mt_low-1;
			/* mp, newp, zot */
			/* zot->mt_forw = mp->mt_forw; */
			newp->mt_forw = zot;
			mp->mt_forw = newp;
			rt_pr_mater( mp );
			rt_pr_mater( newp );
			rt_pr_mater( zot );
			return;
		}
		if( mp->mt_high > newp->mt_low )  {
			/* Overlap to the left: Shorten preceeding entry */
			(void)fprintf(stderr,"Shortening lhs range, from:\n");
			rt_pr_mater( mp );
			(void)fprintf(stderr,"to:\n");
			mp->mt_high = newp->mt_low-1;
			rt_pr_mater( mp );
			/* Now append */
			newp->mt_forw = mp->mt_forw;
			mp->mt_forw = newp;
			goto check_overlap;
		}
		if( mp->mt_forw == MATER_NULL ||
		    newp->mt_low < mp->mt_forw->mt_low )  {
			/* Append */
			newp->mt_forw = mp->mt_forw;
			mp->mt_forw = newp;
			goto check_overlap;
		}
	}
	(void)fprintf(stderr,"fell out of rt_insert_color loop, append to end\n");
	/* Append at end */
	newp->mt_forw = MATER_NULL;
	mp->mt_forw = newp;
	return;

	/* Check for overlap, ie, redefinition of following colors */
check_overlap:
	while( newp->mt_forw != MATER_NULL &&
	       newp->mt_high >= newp->mt_forw->mt_low )  {
		if( newp->mt_high >= newp->mt_forw->mt_high )  {
			/* Drop this mater struct */
			zot = newp->mt_forw;
			newp->mt_forw = zot->mt_forw;
			(void)fprintf(stderr,"dropping overlaping entry:\n");
			rt_pr_mater( zot );
			free( zot );
			continue;
		}
		if( newp->mt_high >= newp->mt_forw->mt_low )  {
			/* Shorten this mater struct, then done */
			(void)fprintf(stderr,"Shortening rhs range, from:\n");
			rt_pr_mater( newp->mt_forw );
			(void)fprintf(stderr,"to:\n");
			newp->mt_forw->mt_low = newp->mt_high+1;
			rt_pr_mater( newp->mt_forw );
			continue;	/* more conservative than returning */
		}
	}
}

/*
 *  			R T _ R E G I O N _ C O L O R _ M A P
 *
 *  If the GIFT regionid of this region falls into a mapped area
 *  of regionid-driven color override.
 */
void
rt_region_color_map( regp )
register struct region *regp;
{
	register struct mater *mp;

	if( regp == REGION_NULL )  {
		fprintf(stderr,"color_map(NULL)\n");
		return;
	}
	for( mp = MaterHead; mp != MATER_NULL; mp = mp->mt_forw )  {
		if( regp->reg_regionid <= mp->mt_high &&
		    regp->reg_regionid >= mp->mt_low ) {
		    	regp->reg_mater.ma_override = 1;
		    	regp->reg_mater.ma_color[0] = (mp->mt_r+0.5)*rt_inv255;
		    	regp->reg_mater.ma_color[1] = (mp->mt_g+0.5)*rt_inv255;
		    	regp->reg_mater.ma_color[2] = (mp->mt_b+0.5)*rt_inv255;
			return;
		}
	}
}
