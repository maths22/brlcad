/*
 *			B O O L . C
 *
 * Ray Tracing program, Boolean region evaluator.
 *
 * Author -
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
static char RCSbool[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./debug.h"

/* Boolean values.  Not easy to change, but defined symbolicly */
#define FALSE	0
#define TRUE	1

RT_EXTERN(void rt_grow_boolstack, (struct resource *resp) );

/*
 *			R T _ B O O L W E A V E
 *
 *  Weave a chain of segments into an existing set of partitions.
 *  The edge of each partition is an inhit or outhit of some solid (seg).
 *
 *  NOTE:  When the final partitions are completed, it is the users
 *  responsibility to honor the inflip and outflip flags.  They can
 *  not be flipped here because an outflip=1 edge and an inflip=0 edge
 *  following it may in fact be the same edge.  This could be dealt with
 *  by giving the partition struct a COPY of the inhit and outhit rather
 *  than a pointer, but that's more cycles than the neatness is worth.
 *
 * Inputs -
 *	Pointer to first segment in seg chain.
 *	Pointer to head of circular doubly-linked list of
 *	partitions of the original ray.
 *
 * Outputs -
 *	Partitions, queued on doubly-linked list specified.
 *
 * Notes -
 *	It is the responsibility of the CALLER to free the seg chain,
 *	as well as the partition list that we return.
 */
void
rt_boolweave( out_hd, in_hd, PartHdp, ap )
struct seg		*out_hd;
struct seg		*in_hd;
struct partition	*PartHdp;
struct application	*ap;
{
	register struct seg *segp;
	register struct partition *pp;
	struct resource		*res = ap->a_resource;

	RT_CHECK_RTI(ap->a_rt_i);

	if(rt_g.debug&DEBUG_PARTITION)
		rt_log("-------------------BOOL_WEAVE\n");
	while( RT_LIST_NON_EMPTY( &(in_hd->l) ) ) {
		register struct partition	*newpp = PT_NULL;
		register struct seg		*lastseg = RT_SEG_NULL;
		register struct hit		*lasthit = HIT_NULL;
		LOCAL int			lastflip = 0;

		segp = RT_LIST_FIRST( seg, &(in_hd->l) );
		RT_CHECK_SEG(segp);
		if(rt_g.debug&DEBUG_PARTITION) rt_pr_seg(segp);
		if( segp->seg_stp->st_bit >= ap->a_rt_i->nsolids) rt_bomb("rt_boolweave: st_bit");

		RT_LIST_DEQUEUE( &(segp->l) );
		RT_LIST_INSERT( &(out_hd->l), &(segp->l) );

		/* Totally ignore things behind the start position */
		if( segp->seg_out.hit_dist < -10.0 )
			continue;

		/*  Eliminate very thin segments, or they will cause
		 *  trouble below.
		 */
		if( rt_fdiff(segp->seg_in.hit_dist,segp->seg_out.hit_dist)==0 ) {
			if(rt_g.debug&DEBUG_PARTITION)  rt_log(
				"rt_boolweave:  Thin seg discarded: %s (%g,%g)\n",
				segp->seg_stp->st_name,
				segp->seg_in.hit_dist,
				segp->seg_out.hit_dist );
			continue;
		}
		if( !(segp->seg_in.hit_dist >= -INFINITY &&
		    segp->seg_out.hit_dist <= INFINITY) )  {
		    	rt_log("rt_boolweave:  Defective segment %s (%g,%g)\n",
				segp->seg_stp->st_name,
				segp->seg_in.hit_dist,
				segp->seg_out.hit_dist );
			continue;
		}

		/*
		 * Weave this segment into the existing partitions,
		 * creating new partitions as necessary.
		 */
		if( PartHdp->pt_forw == PartHdp )  {
			/* No partitions yet, simple! */
			GET_PT_INIT( ap->a_rt_i, pp, res );
			BITSET(pp->pt_solhit, segp->seg_stp->st_bit);
			pp->pt_inseg = segp;
			pp->pt_inhit = &segp->seg_in;
			pp->pt_outseg = segp;
			pp->pt_outhit = &segp->seg_out;
			APPEND_PT( pp, PartHdp );
			if(rt_g.debug&DEBUG_PARTITION) rt_log("First partition\n");
			goto done_weave;
		}
		if( segp->seg_in.hit_dist >= PartHdp->pt_back->pt_outhit->hit_dist )  {
			/*
			 * Segment starts exactly at last partition's end,
			 * or beyond last partitions end.  Make new partition.
			 */
			GET_PT_INIT( ap->a_rt_i, pp, res );
			BITSET(pp->pt_solhit, segp->seg_stp->st_bit);
			pp->pt_inseg = segp;
			pp->pt_inhit = &segp->seg_in;
			pp->pt_outseg = segp;
			pp->pt_outhit = &segp->seg_out;
			APPEND_PT( pp, PartHdp->pt_back );
			if(rt_g.debug&DEBUG_PARTITION) rt_log("seg starts beyond part end\n");
			goto done_weave;
		}

		lastseg = segp;
		lasthit = &segp->seg_in;
		lastflip = 0;
		for( pp=PartHdp->pt_forw; pp != PartHdp; pp=pp->pt_forw ) {
			register int i;

			if( (i=rt_fdiff(lasthit->hit_dist, pp->pt_outhit->hit_dist)) > 0 )  {
				/* Seg starts beyond the END of the
				 * current partition.
				 *	PPPP
				 *	        SSSS
				 * Advance to next partition.
				 */
				if(rt_g.debug&DEBUG_PARTITION)  rt_log("seg start beyond end, skipping\n");
				continue;
			}
			if( i == 0 )  {
				/*
				 * Seg starts almost "precisely" at the
				 * end of the current partition.
				 *	PPPP
				 *	    SSSS
				 * FUSE an exact match of the endpoints,
				 * advance to next partition.
				 */
				lasthit->hit_dist = pp->pt_outhit->hit_dist;
				VMOVE(lasthit->hit_point, pp->pt_outhit->hit_point);
				if(rt_g.debug&DEBUG_PARTITION)  rt_log("seg start fused to partition end\n");
				continue;
			}
			if(rt_g.debug&DEBUG_PARTITION)  rt_pr_pt(ap->a_rt_i, pp);

			/*
			 * i < 0,  Seg starts before current partition ends
			 *	PPPPPPPPPPP
			 *	  SSSS...
			 */

			if( (i=rt_fdiff(lasthit->hit_dist, pp->pt_inhit->hit_dist)) == 0){
equal_start:
				if(rt_g.debug&DEBUG_PARTITION) rt_log("equal_start\n");
				/*
				 * Segment and partition start at
				 * (roughly) the same point.
				 * When fuseing 2 points together
				 * (ie, when rt_fdiff()==0), the two
				 * points MUST be forced to become
				 * exactly equal!
				 */
				if( (i = rt_fdiff(segp->seg_out.hit_dist, pp->pt_outhit->hit_dist)) == 0 )  {
					/*
					 * Segment and partition start & end
					 * (nearly) together.
					 *	PPPP
					 *	SSSS
					 */
					BITSET(pp->pt_solhit, segp->seg_stp->st_bit);
					if(rt_g.debug&DEBUG_PARTITION) rt_log("same start&end\n");
					goto done_weave;
				}
				if( i > 0 )  {
					/*
					 * Seg & partition start at roughly
					 * the same spot,
					 * seg extends beyond partition end.
					 *	PPPP
					 *	SSSSSSSS
					 *	pp  |  newpp
					 */
					BITSET(pp->pt_solhit, segp->seg_stp->st_bit);
					lasthit = pp->pt_outhit;
					lastseg = pp->pt_outseg;
					lastflip = 1;
					if(rt_g.debug&DEBUG_PARTITION) rt_log("seg spans p and beyond\n");
					continue;
				}
				/*
				 *  Segment + Partition start together,
				 *  segment ends before partition ends.
				 *	PPPPPPPPPP
				 *	SSSSSS
				 *	newpp| pp
				 */
				GET_PT( ap->a_rt_i, newpp, res );
				COPY_PT( ap->a_rt_i,newpp,pp);
				/* new partition contains segment */
				BITSET(newpp->pt_solhit, segp->seg_stp->st_bit);
				newpp->pt_outseg = segp;
				newpp->pt_outhit = &segp->seg_out;
				newpp->pt_outflip = 0;
				pp->pt_inseg = segp;
				pp->pt_inhit = &segp->seg_out;
				pp->pt_inflip = 1;
				INSERT_PT( newpp, pp );
				if(rt_g.debug&DEBUG_PARTITION) rt_log("start together, seg shorter\n");
				goto done_weave;
			}
			if( i < 0 )  {
				/*
				 * Seg starts before current partition starts,
				 * but after the previous partition ends.
				 *	SSSSSSSS...
				 *	     PPPPP...
				 *	newpp|pp
				 */
				GET_PT_INIT( ap->a_rt_i, newpp, res );
				BITSET(newpp->pt_solhit, segp->seg_stp->st_bit);
				newpp->pt_inseg = lastseg;
				newpp->pt_inhit = lasthit;
				newpp->pt_inflip = lastflip;
				if( (i=rt_fdiff(segp->seg_out.hit_dist, pp->pt_inhit->hit_dist)) < 0 ){
					/*
					 * Seg starts and ends before current
					 * partition, but after previous
					 * partition ends (diff < 0).
					 *		SSSS
					 *	pppp		PPPPP...
					 *		newpp	pp
					 */
					newpp->pt_outseg = segp;
					newpp->pt_outhit = &segp->seg_out;
					newpp->pt_outflip = 0;
					INSERT_PT( newpp, pp );
					if(rt_g.debug&DEBUG_PARTITION) rt_log("seg between 2 partitions\n");
					goto done_weave;
				}
				if( i==0 )  {
					/* Seg starts before current
					 * partition starts, and ends at or
					 * near the start of the partition.
					 * (diff == 0).  FUSE the points.
					 *	SSSSSS
					 *	     PPPPP
					 *	newpp|pp
					 * NOTE: only copy hit point, not
					 * normals or norm private stuff.
					 */
					newpp->pt_outseg = segp;
					newpp->pt_outhit = &segp->seg_out;
					newpp->pt_outhit->hit_dist = pp->pt_inhit->hit_dist;
					VMOVE(newpp->pt_outhit->hit_point, pp->pt_inhit->hit_point);
					newpp->pt_outflip = 0;
					INSERT_PT( newpp, pp );
					if(rt_g.debug&DEBUG_PARTITION) rt_log("seg ends at p start, fuse\n");
					goto done_weave;
				}
				/*
				 *  Seg starts before current partition
				 *  starts, and ends after the start of the
				 *  partition.  (diff > 0).
				 *	SSSSSSSSSS
				 *	      PPPPPPP
				 *	newpp| pp | ...
				 */
				newpp->pt_outseg = pp->pt_inseg;
				newpp->pt_outhit = pp->pt_inhit;
				newpp->pt_outflip = 1;
				lastseg = pp->pt_inseg;
				lasthit = pp->pt_inhit;
				lastflip = newpp->pt_outflip;
				INSERT_PT( newpp, pp );
				if(rt_g.debug&DEBUG_PARTITION) rt_log("insert seg before p start, ends after p ends\n");
				goto equal_start;
			}
			/*
			 * i > 0
			 *
			 * lasthit->hit_dist > pp->pt_inhit->hit_dist
			 *
			 *  Segment starts after partition starts,
			 *  but before the end of the partition.
			 *  Note:  pt_solhit will be marked in equal_start.
			 *	PPPPPPPPPPPP
			 *	     SSSS...
			 *	newpp|pp
			 */
			GET_PT( ap->a_rt_i, newpp, res );
			COPY_PT( ap->a_rt_i, newpp, pp );
			/* new part. is the span before seg joins partition */
			pp->pt_inseg = segp;
			pp->pt_inhit = &segp->seg_in;
			pp->pt_inflip = 0;
			newpp->pt_outseg = segp;
			newpp->pt_outhit = &segp->seg_in;
			newpp->pt_outflip = 1;
			INSERT_PT( newpp, pp );
			if(rt_g.debug&DEBUG_PARTITION) rt_log("seg starts after p starts, ends after p ends. Split p in two, advance.\n");
			goto equal_start;
		}

		/*
		 *  Segment has portion which extends beyond the end
		 *  of the last partition.  Tack on the remainder.
		 *  	PPPPP
		 *  	     SSSSS
		 */
		if(rt_g.debug&DEBUG_PARTITION) rt_log("seg extends beyond p end\n");
		GET_PT_INIT( ap->a_rt_i, newpp, res );
		BITSET(newpp->pt_solhit, segp->seg_stp->st_bit);
		newpp->pt_inseg = lastseg;
		newpp->pt_inhit = lasthit;
		newpp->pt_inflip = lastflip;
		newpp->pt_outseg = segp;
		newpp->pt_outhit = &segp->seg_out;
		APPEND_PT( newpp, PartHdp->pt_back );

done_weave:	; /* Sorry about the goto's, but they give clarity */
		if(rt_g.debug&DEBUG_PARTITION)
			rt_pr_partitions( ap->a_rt_i, PartHdp, "After weave" );
	}
}

/*
 *			R T _ D E F O V E R L A P
 *
 *  Default handler for overlaps in rt_boolfinal().
 *  Returns -
 *	 0	to eliminate partition with overlap entirely
 *	!0	to retain partition in output list
 */
int
rt_defoverlap( ap, pp, reg1, reg2 )
register struct application	*ap;
register struct partition	*pp;
struct region			*reg1;
struct region			*reg2;
{
	point_t	pt;
	static long count = 0;		/* Not PARALLEL, shouldn't hurt */
	register fastf_t depth;

	RT_CHECK_PT(pp);

	/* Attempt to control tremendous error outputs */
	if( ++count > 100 )  {
		if( (count%100) != 3 )  return(1);
		rt_log("(overlaps omitted)\n");
	}

	VJOIN1( pt, ap->a_ray.r_pt, pp->pt_inhit->hit_dist,
		ap->a_ray.r_dir );
	depth = pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist;
	/*
	 * An application program might want to add code here
	 * to ignore "small" overlap depths.
	 */
	rt_log("OVERLAP1: reg=%s isol=%s\n",
		reg1->reg_name, pp->pt_inseg->seg_stp->st_name);
	rt_log("OVERLAP2: reg=%s osol=%s\n",
		reg2->reg_name, pp->pt_outseg->seg_stp->st_name);
	rt_log("OVERLAP depth %gmm at (%g,%g,%g) x%d y%d lvl%d\n",
		depth, pt[X], pt[Y], pt[Z],
		ap->a_x, ap->a_y, ap->a_level );
	return(1);
}

/*
 *			R T _ B O O L F I N A L
 *
 *
 *  Consider each partition on the sorted & woven input partition list.
 *  If the partition ends before this box's start, discard it immediately.
 *  If the partition begins beyond this box's end, return.
 *
 *  Next, evaluate the boolean expression tree for all regions that have
 *  some presence in the partition.
 *
 * If 0 regions result, continue with next partition.
 * If 1 region results, a valid hit has occured, so transfer
 * the partition from the Input list to the Final list.
 * If 2 or more regions claim the partition, then an overlap exists.
 * If the overlap handler gives a non-zero return, then the overlapping
 * partition is kept, with the region ID being the first one encountered.
 * Otherwise, the partition is eliminated from further consideration. 
 *
 *  All partitions in the indicated range of the ray are evaluated.
 *  All partitions which really exist (booleval is true) are appended
 *  to the Final partition list.
 *  All partitions on the Final partition list have completely valid
 *  entry and exit information, except for the last partition's exit
 *  information when a_onehit>0 and a_onehit is odd.
 *
 *  The flag a_onehit is interpreted as follows:
 *
 *  If a_onehit = 0, then the ray is traced to +infinity, and all
 *  hit points in the final partition list are valid.
 *
 *  If a_onehit > 0, the ray is traced through a_onehit hit points.
 *  (Recall that each partition has 2 hit points, entry and exit).
 *  Thus, if a_onehit is odd, the value of pt_outhit.hit_dist in
 *  the last partition may be incorrect;  this should not mater because
 *  the application specifically said it only wanted pt_inhit there.
 *  This is most commonly seen when a_onehit = 1, which is useful for
 *  lighting models.  Not having to correctly determine the exit point
 *  can result in a significant savings of computer time.
 *
 *  Returns -
 *	0	If more partitions need to be done
 *	1	Requested number of hits are available in FinalHdp
 *
 *  The caller must free whatever is in both partition chains.
 */
int
rt_boolfinal( InputHdp, FinalHdp, startdist, enddist, regionbits, ap )
struct partition *InputHdp;
struct partition *FinalHdp;
fastf_t startdist, enddist;
bitv_t *regionbits;
struct application *ap;
{
	LOCAL struct region *lastregion;
	LOCAL struct region *TrueRg[2];
	register struct partition *pp;
	register int	claiming_regions;
	int		hits_avail = 0;

#define HITS_TODO	(ap->a_onehit - hits_avail)

	RT_CHECK_RTI(ap->a_rt_i);

	if( enddist <= 0 )
		return(0);		/* not done, behind start point! */

	if( ap->a_onehit > 0 )  {
		register struct partition *npp = FinalHdp->pt_forw;

		for(; npp != FinalHdp; npp = npp->pt_forw )  {
			if( npp->pt_inhit->hit_dist < 0.0 )
				continue;
			hits_avail += 2;	/* both in and out listed */
		}
		if( HITS_TODO <= 0 )
			return(1);		/* done */
	}

	pp = InputHdp->pt_forw;
	while( pp != InputHdp )  {
		RT_CHECK_PT(pp);
		claiming_regions = 0;
		if(rt_g.debug&DEBUG_PARTITION)  {
			rt_log("rt_boolfinal: (%g,%g)\n", startdist, enddist );
			rt_pr_pt( ap->a_rt_i, pp );
		}

		/* Sanity checks on sorting.  Remove later. */
		RT_CHECK_SEG(pp->pt_inseg);
		RT_CHECK_SEG(pp->pt_outseg);
		if( pp->pt_inhit->hit_dist >= pp->pt_outhit->hit_dist )  {
			rt_log("rt_boolfinal: thin or inverted partition %.8x\n", pp);
			rt_pr_partitions( ap->a_rt_i, InputHdp, "With problem" );
		}
		if( pp->pt_forw != InputHdp && pp->pt_outhit->hit_dist > pp->pt_forw->pt_inhit->hit_dist )  {
			rt_log("rt_boolfinal:  sorting defect!\n");
			if( !(rt_g.debug & DEBUG_PARTITION) )
				rt_pr_partitions( ap->a_rt_i, InputHdp, "With DEFECT" );
			return(0);	/* give up */
		}

		/*
		 *  If partition ends before current box starts,
		 *  discard it immediately.
		 *  If all the solids are properly located in the space
		 *  partitioning tree's cells, no intersection calculations
		 *  further forward on the ray should ever produce valid
		 *  segments or partitions earlier in the ray.
		 *
		 *  If partition is behind ray start point, also
		 *  discard it.
		 */
		if( pp->pt_outhit->hit_dist < startdist ||
		    pp->pt_outhit->hit_dist <= 0.001 /* milimeters */ )  {
			register struct partition *zap_pp;
			if(rt_g.debug&DEBUG_PARTITION)rt_log(
				"discarding early part x%x, out dist=%g\n",
				pp, pp->pt_outhit->hit_dist);
			zap_pp = pp;
			pp = pp->pt_forw;
			DEQUEUE_PT(zap_pp);
			FREE_PT(zap_pp, ap->a_resource);
			continue;
		}

		/*
		 *  If partition begins beyond current box end,
		 *  the state of the partition is not fully known yet,
		 *  so stop now.
		 */
		if( rt_fdiff( pp->pt_inhit->hit_dist, enddist) > 0 )  {
			if(rt_g.debug&DEBUG_PARTITION)rt_log(
				"partition begins beyond current box end, returning\n");
			return(0);
		}

		/*
		 *  If partition ends somewhere beyond the end of the current
		 *  box, the condition of the outhit information is not fully
		 *  known yet.
		 */
		if( rt_fdiff( pp->pt_outhit->hit_dist, enddist) > 0 )  {
			if(rt_g.debug&DEBUG_PARTITION)rt_log(
				"partition ends beyond current box end\n");
			if( ap->a_onehit <= 0 )
				return(0);
			if( HITS_TODO > 1 )
				return(0);
			/*  In this case, proceed as if pt_outhit was correct,
			 *  even though it probably is not.
			 *  Application asked for this behavior, and it
			 *  saves having to do more ray-tracing.
			 */
		}

		/* Start with a clean slate when evaluating this partition */
		BITZERO( regionbits, ap->a_rt_i->nregions );

		/*
		 *  For each solid that lies in this partition,
		 *  set (OR) that solid's region bit vector into "regionbits".
		 */
		RT_BITV_LOOP_START( pp->pt_solhit, ap->a_rt_i->nsolids )  {
			struct soltab	*stp;
			register int	words;
			register bitv_t *in;
			register bitv_t *out;

			stp = ap->a_rt_i->rti_Solids[RT_BITV_LOOP_INDEX];
			/* It would be nice to validate stp here */
			words = RT_BITV_BITS2WORDS(stp->st_maxreg);
			in = stp->st_regions;
			out = regionbits;

#			include "noalias.h"
			while( words-- > 0 )
				*out++ |= *in++;
		} RT_BITV_LOOP_END;

		if(rt_g.debug&DEBUG_PARTITION)
			rt_pr_bitv( "regionbits", regionbits, ap->a_rt_i->nregions);

		/* Evaluate the boolean trees of any regions involved */
		RT_BITV_LOOP_START( regionbits, ap->a_rt_i->nregions )  {
			register struct region *regp;

			regp = ap->a_rt_i->Regions[RT_BITV_LOOP_INDEX];
			if(rt_g.debug&DEBUG_PARTITION)  {
				rt_pr_tree_val( regp->reg_treetop, pp, 2, 0 );
				rt_pr_tree_val( regp->reg_treetop, pp, 1, 0 );
				rt_pr_tree_val( regp->reg_treetop, pp, 0, 0 );
				rt_log("%.8x=bit%d, %s: ",
					regp, RT_BITV_LOOP_INDEX, regp->reg_name );
			}
			if( rt_booleval( regp->reg_treetop, pp, TrueRg,
			    ap->a_resource ) == FALSE )  {
				if(rt_g.debug&DEBUG_PARTITION) rt_log("FALSE\n");
				continue;
			} else {
				if(rt_g.debug&DEBUG_PARTITION) rt_log("TRUE\n");
			}
			/* This region claims partition */
			if( ++claiming_regions <= 1 )  {
				lastregion = regp;
				continue;
			}

			/*
			 * Two or more regions claim this partition
			 */
			if((	(	regp->reg_aircode == 0
				    &&	lastregion->reg_aircode == 0 
				) /* Neither are air */
			     ||	(	regp->reg_aircode != 0
				   &&	lastregion->reg_aircode != 0
				   &&	regp->reg_aircode != lastregion->reg_aircode
				) /* Both are air, but different types */
			   ) )  {
			   	/*
			   	 *  Hand overlap to application-specific
			   	 *  overlap handler, or default.
			   	 */
			   	if( ap->a_overlap == RT_AFN_NULL )
			   		ap->a_overlap = rt_defoverlap;
				if( ap->a_overlap(ap, pp, regp, lastregion) )  {
				    	/* non-zero => retain last partition */
					if( lastregion->reg_aircode != 0 )
						lastregion = regp;
				    	claiming_regions--;	/* now = 1 */
				}
			} else {
				/* last region is air, replace with solid */
				if( lastregion->reg_aircode != 0 )
					lastregion = regp;
				claiming_regions--;		/* now = 1 */
			}
		} RT_BITV_LOOP_END;
		if( claiming_regions == 0 )  {
			pp=pp->pt_forw;			/* onwards! */
			continue;
		}

		if( claiming_regions > 1 )  {
			/*
			 *  Discard partition containing overlap.
			 *  Nothing further along the ray will fix it.
			 */
			register struct partition	*zap_pp;

			if(rt_g.debug&DEBUG_PARTITION)rt_log("rt_boolfinal discarding overlap partition x%x\n", pp);
			zap_pp = pp;
			pp = pp->pt_forw;
			DEQUEUE_PT( zap_pp );
			FREE_PT( zap_pp, ap->a_resource );
			continue;
		}

		/*
		 *  Remove this partition from the input queue, and
		 *  append it to the result queue.
		 *  XXX Could there be sorting trouble on final call to
		 *  XXX boolfinal when a_onehit > 0??
		 *  XXX No, because input queue shouldn't be interesting on 2nd visit.
		 */
		{
			register struct partition	*newpp;
			register struct partition	*lastpp;

			newpp = pp;
			pp=pp->pt_forw;
			DEQUEUE_PT( newpp );
			RT_CHECK_SEG(newpp->pt_inseg);		/* sanity */
			RT_CHECK_SEG(newpp->pt_outseg);		/* sanity */
			newpp->pt_regionp = lastregion;

			/*  See if this new partition extends the previous
			 *  last partition, "exactly" matching.
			 */
			if( (lastpp = FinalHdp->pt_back) != FinalHdp &&
			    lastregion == lastpp->pt_regionp &&
			    rt_fdiff( newpp->pt_inhit->hit_dist,
				lastpp->pt_outhit->hit_dist ) == 0
			)  {
				/* same region, extend last final partition */
				RT_CHECK_PT(lastpp);
				RT_CHECK_SEG(lastpp->pt_inseg);	/* sanity */
				RT_CHECK_SEG(lastpp->pt_outseg);/* sanity */
				lastpp->pt_outhit = newpp->pt_outhit;
				lastpp->pt_outflip = newpp->pt_outflip;
				lastpp->pt_outseg = newpp->pt_outseg;
				FREE_PT( newpp, ap->a_resource );
				newpp = lastpp;
			}  else  {
				APPEND_PT( newpp, lastpp );
				hits_avail += 2;
			}

			RT_CHECK_SEG(newpp->pt_inseg);		/* sanity */
			RT_CHECK_SEG(newpp->pt_outseg);		/* sanity */
		}

		if( ap->a_onehit > 0 && HITS_TODO <= 0 )
			return(1);		/* done */
	}
	if( rt_g.debug&DEBUG_PARTITION )
		rt_pr_partitions( ap->a_rt_i, FinalHdp, "rt_boolfinal: Partitions returned" );

	return(0);			/* not done */
}

/*
 *  			R T _ B O O L E V A L
 *  
 *  Using a stack to recall state, evaluate a boolean expression
 *  without recursion.
 *
 *  For use with XOR, a pointer to the "first valid subtree" would
 *  be a useful addition, for rt_boolregions().
 *
 *  Returns -
 *	!0	tree is TRUE
 *	 0	tree is FALSE
 *	-1	tree is in error (GUARD)
 */
int
rt_booleval( treep, partp, trueregp, resp )
register union tree *treep;	/* Tree to evaluate */
struct partition *partp;	/* Partition to evaluate */
struct region	**trueregp;	/* XOR true (and overlap) return */
struct resource	*resp;		/* resource pointer for this CPU */
{
	static union tree tree_not;		/* for OP_NOT nodes */
	static union tree tree_guard;		/* for OP_GUARD nodes */
	static union tree tree_xnop;		/* for OP_XNOP nodes */
	register union tree **sp;
	register int ret;
	register union tree **stackend;

	if( treep->tr_op != OP_XOR )
		trueregp[0] = treep->tr_regionp;
	else
		trueregp[0] = trueregp[1] = REGION_NULL;
	while( (sp = resp->re_boolstack) == (union tree **)0 )
		rt_grow_boolstack( resp );
	stackend = &(resp->re_boolstack[resp->re_boolslen]);
	*sp++ = TREE_NULL;
stack:
	switch( treep->tr_op )  {
	case OP_SOLID:
		ret = treep->tr_a.tu_stp->st_bit;	/* register temp */
		ret = BITTEST( partp->pt_solhit, ret );
		goto pop;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		*sp++ = treep;
		if( sp >= stackend )  {
			register int off = sp - resp->re_boolstack;
			rt_grow_boolstack( resp );
			sp = &(resp->re_boolstack[off]);
			stackend = &(resp->re_boolstack[resp->re_boolslen]);
		}
		treep = treep->tr_b.tb_left;
		goto stack;
	default:
		rt_log("rt_booleval:  bad stack op x%x\n",treep->tr_op);
		return(TRUE);	/* screw up output */
	}
pop:
	if( (treep = *--sp) == TREE_NULL )
		return(ret);		/* top of tree again */
	/*
	 *  Here, each operation will look at the operation just
	 *  completed (the left branch of the tree generally),
	 *  and rewrite the top of the stack and/or branch
	 *  accordingly.
	 */
	switch( treep->tr_op )  {
	case OP_SOLID:
		rt_log("rt_booleval:  pop SOLID?\n");
		return(TRUE);	/* screw up output */
	case OP_UNION:
		if( ret )  goto pop;	/* TRUE, we are done */
		/* lhs was false, rewrite as rhs tree */
		treep = treep->tr_b.tb_right;
		goto stack;
	case OP_INTERSECT:
		if( !ret )  {
			ret = FALSE;
			goto pop;
		}
		/* lhs was true, rewrite as rhs tree */
		treep = treep->tr_b.tb_right;
		goto stack;
	case OP_SUBTRACT:
		if( !ret )  goto pop;	/* FALSE, we are done */
		/* lhs was true, rewrite as NOT of rhs tree */
		/* We introduce the special NOT operator here */
		tree_not.tr_op = OP_NOT;
		*sp++ = &tree_not;
		treep = treep->tr_b.tb_right;
		goto stack;
	case OP_NOT:
		/* Special operation for subtraction */
		ret = !ret;
		goto pop;
	case OP_XOR:
		if( ret )  {
			/* lhs was true, rhs better not be, or we have
			 * an overlap condition.  Rewrite as guard node
			 * followed by rhs.
			 */
			if( treep->tr_b.tb_left->tr_regionp )
				trueregp[0] = treep->tr_b.tb_left->tr_regionp;
			tree_guard.tr_op = OP_GUARD;
			treep = treep->tr_b.tb_right;
			*sp++ = treep;		/* temp val for guard node */
			*sp++ = &tree_guard;
		} else {
			/* lhs was false, rewrite as xnop node and
			 * result of rhs.
			 */
			tree_xnop.tr_op = OP_XNOP;
			treep = treep->tr_b.tb_right;
			*sp++ = treep;		/* temp val for xnop */
			*sp++ = &tree_xnop;
		}
		goto stack;
	case OP_GUARD:
		/*
		 *  Special operation for XOR.  lhs was true.
		 *  If rhs subtree was true, an
		 *  overlap condition exists (both sides of the XOR are
		 *  TRUE).  Return error condition.
		 *  If subtree is false, then return TRUE (from lhs).
		 */
		if( ret )  {
			/* stacked temp val: rhs */
			if( sp[-1]->tr_regionp )
				trueregp[1] = sp[-1]->tr_regionp;
			return(-1);	/* GUARD error */
		}
		ret = TRUE;
		sp--;			/* pop temp val */
		goto pop;
	case OP_XNOP:
		/*
		 *  Special NOP for XOR.  lhs was false.
		 *  If rhs is true, take note of it's regionp.
		 */
		sp--;			/* pop temp val */
		if( ret )  {
			if( (*sp)->tr_regionp )
				trueregp[0] = (*sp)->tr_regionp;
		}
		goto pop;
	default:
		rt_log("rt_booleval:  bad pop op x%x\n",treep->tr_op);
		return(TRUE);	/* screw up output */
	}
	/* NOTREACHED */
}

/*
 *			R T _ F D I F F
 *  
 *  Compares two floating point numbers.  If they are within "epsilon"
 *  of each other, they are considered the same.
 *  NOTE:  This is a "fuzzy" difference.  It is important NOT to
 *  use the results of this function in compound comparisons,
 *  because a return of 0 makes no statement about the PRECISE
 *  relationships between the two numbers.  Eg,
 *	if( rt_fdiff( a, b ) <= 0 )
 *  is poison!
 *
 *  Returns -
 *  	-1	if a < b
 *  	 0	if a ~= b
 *  	+1	if a > b
 */
int
rt_fdiff( a, b )
double a, b;
{
	FAST double diff;
	FAST double d;
	register int ret;

	/* d = Max(Abs(a),Abs(b)) */
	d = (a >= 0.0) ? a : -a;
	if( b >= 0.0 )  {
		if( b > d )  d = b;
	} else {
		if( (-b) > d )  d = (-b);
	}
	if( d <= 1.0e-6 )  {
		ret = 0;	/* both nearly zero */
		goto out;
	}
	if( d >= INFINITY )  {
		if( a == b )  {
			ret = 0;
			goto out;
		}
		if( a < b )  {
			ret = -1;
			goto out;
		}
		ret = 1;
		goto out;
	}
	if( (diff = a - b) < 0.0 )  diff = -diff;
	if( diff < 0.001 )  {
		ret = 0;	/* absolute difference is small, < 1/1000mm */
		goto out;
	}
	if( diff < 0.000001 * d )  {
		ret = 0;	/* relative difference is small, < 1ppm */
		goto out;
	}
	if( a < b )  {
		ret = -1;
		goto out;
	}
	ret = 1;
out:
	if(rt_g.debug&DEBUG_FDIFF) rt_log("rt_fdiff(%g,%g)=%d\n", a, b, ret);
	return(ret);
}

/*
 *			R T _ R E L D I F F
 *
 * Compute the relative difference of two floating point numbers.
 *
 * Returns -
 *	0.0 if exactly the same, otherwise
 *	ratio of difference, relative to the larger of the two (range 0.0-1.0)
 */
double
rt_reldiff( a, b )
double	a, b;
{
	FAST fastf_t	d;
	FAST fastf_t	diff;

	/* d = Max(Abs(a),Abs(b)) */
	d = (a >= 0.0) ? a : -a;
	if( b >= 0.0 )  {
		if( b > d )  d = b;
	} else {
		if( (-b) > d )  d = (-b);
	}
	if( d==0.0 )
		return( 0.0 );
	if( (diff = a - b) < 0.0 )  diff = -diff;
	return( diff / d );
}

/*
 *			R T _ G R O W _ B O O L S T A C K
 *
 *  Increase the size of re_boolstack to double the previous size.
 *  Depend on rt_realloc() to copy the previous data to the new area
 *  when the size is increased.
 *  Return the new pointer for what was previously the last element.
 */
void
rt_grow_boolstack( resp )
register struct resource	*resp;
{
	if( resp->re_boolstack == (union tree **)0 || resp->re_boolslen <= 0 )  {
		resp->re_boolslen = 128;	/* default len */
		resp->re_boolstack = (union tree **)rt_malloc(
			sizeof(union tree *) * resp->re_boolslen,
			"initial boolstack");
	} else {
		resp->re_boolslen <<= 1;
		resp->re_boolstack = (union tree **)rt_realloc(
			(char *)resp->re_boolstack,
			sizeof(union tree *) * resp->re_boolslen,
			"extend boolstack" );
	}
}

/*
 *			R T _ P A R T I T I O N _ L E N
 *
 *  Return the length of a partition linked list.
 */
int
rt_partition_len( partheadp )
register struct partition	*partheadp;
{
	register struct partition	*pp;
	register long	count = 0;

	pp = partheadp->pt_forw;
	if( pp == PT_NULL )
		return(0);		/* Header not initialized yet */
	for( ; pp != partheadp; pp = pp->pt_forw )  {
		if( pp->pt_magic != 0 )  {
			/* Partitions on the free queue have pt_magic = 0 */
			RT_CHECK_PT(pp);
		}
		if( ++count > 1000000 )  rt_bomb("partition length > 10000000 elements\n");
	}
	return( (int)count );
}
