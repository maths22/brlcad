/*
 *			V I E W C H E C K
 *
 *  Ray Tracing program RTCHECK bottom half.
 *
 *  This module outputs overlapping partitions, no other information.
 *  The partitions are written to the output file (typically stdout)
 *  as BRL-UNIX-plot 3-D floating point lines, so that they can be
 *  processed by any tool that reads UNIX-plot.  Because the BRL UNIX
 *  plot format is defined in a machine independent way, this program
 *  can be run anywhere, and the results piped back for local viewing,
 *  for example, on a workstation.
 *
 *  Authors -
 *	Michael John Muuss
 *	Gary S. Moss
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
static char RCScheckview[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"

extern int	rpt_overlap;		/* report overlapping region names */
int		use_air = 0;		/* Handling of air in librt */
int		using_mlib = 0;		/* Material routines NOT used */

/* Viewing module specific "set" variables */
struct structparse view_parse[] = {
	(char *)0,(char *)0,	0,			FUNC_NULL
};

extern FILE	*outfp;

char usage[] = "Usage:  rtcheck [options] model.g objects...\n";

static int	noverlaps;		/* Number of overlaps seen */
static int	overlap_count;		/* Number of differentiable overlaps seen */

/*
 *			H I T
 *
 * Null function -- handle a hit
 */
/*ARGSUSED*/
int
hit( ap, PartHeadp )
struct application *ap;
register struct partition *PartHeadp;
{
	return	1;
}

/*
 *			M I S S
 *
 *  Null function -- handle a miss
 */
/*ARGSUSED*/
int
miss( ap )
struct application *ap;
{
	return	1;
}


struct overlap_list {
	char 	*reg1,			/* overlapping region 1 */
		*reg2;			/* overlapping region 2 */
	struct overlap_list *next;	/* next one */
};


/*
 *			O V E R L A P
 *
 *  Write end points of partition to the standard output.
 */
int
overlap( ap, pp, reg1, reg2 )
struct application	*ap;
struct partition	*pp;
struct region		*reg1;
struct region		*reg2;
{	
	register struct xray	*rp = &ap->a_ray;
	register struct hit	*ihitp = pp->pt_inhit;
	register struct hit	*ohitp = pp->pt_outhit;
	vect_t	ihit;
	vect_t	ohit;

	VJOIN1( ihit, rp->r_pt, ihitp->hit_dist, rp->r_dir );
	VJOIN1( ohit, rp->r_pt, ohitp->hit_dist, rp->r_dir );

	RES_ACQUIRE( &rt_g.res_syscall );
	pdv_3line( outfp, ihit, ohit );
	noverlaps++;

	/* If we report overlaps, don't print if already noted once.
	 * Build up a linked list of known overlapping regions and compare 
	 * againt it.
	 */
	if( rpt_overlap ) {

		static struct overlap_list *root=NULL;		/* root of the list*/
		struct overlap_list	*prev_ol,*olist;	/* overlap list */

		for( olist=root; olist; prev_ol=olist,olist=olist->next ) {
			if( (strcmp(reg1->reg_name,olist->reg1) == 0)
			 && (strcmp(reg2->reg_name,olist->reg2) == 0) ) {
				RES_RELEASE( &rt_g.res_syscall );
				return	0;	/* already on list */
			}
		}

		overlap_count++;
		fprintf(stderr,"OVERLAP %d: %s\n",overlap_count,reg1->reg_name);
		fprintf(stderr,"OVERLAP %d: %s\n",overlap_count,reg2->reg_name);
		fprintf(stderr,"-----<>-----<>-----<>-----<>-----<>------<>-----\n");

		if( (olist =(struct overlap_list *)rt_malloc(sizeof(struct overlap_list),"overlap list")) != NULL ){
			if( root )		/* previous entry exists */
				prev_ol->next = olist;
			else
				root = olist;	/* finally initialize root */
			olist->reg1 = reg1->reg_name;
			olist->reg2 = reg2->reg_name;
			olist->next = NULL;

		} else {
			fprintf(stderr,"rtcheck: can't allocate enough space for overlap list!!\n");
			exit(1);
		}
	}
	RES_RELEASE( &rt_g.res_syscall );

	return(0);	/* No further consideration to this partition */
}

/*
 *  			V I E W _ I N I T
 *
 *  Called once for this run.
 */
int
view_init( ap, file, obj, minus_o )
register struct application *ap;
char *file, *obj;
int minus_o;
{
	ap->a_hit = hit;
	ap->a_miss = miss;
	ap->a_overlap = overlap;
	ap->a_onehit = 0;
	if( !minus_o)			/* Needs to be set to  stdout */
		outfp = stdout;

	return	0;		/* No framebuffer needed */
}

/*
 *			V I E W _ 2 I N I T
 *
 *  Called at the beginning of each frame
 */
void
view_2init( ap )
register struct application *ap;
{
	register struct rt_i *rtip = ap->a_rt_i;
	
	pdv_3space( outfp, rtip->rti_pmin, rtip->rti_pmax );
	noverlaps = 0;
	overlap_count = 0;
}

/*
 *			V I E W _ E N D
 *
 *  Called at the end of each frame
 */
void
view_end() {
	pl_flush(outfp);
	fflush(outfp);
	rt_log("%d overlaps detected\n", noverlaps);
}

/*
 *	Stubs
 */
void view_pixel() {}

void view_eol() {}
