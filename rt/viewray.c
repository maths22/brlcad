/*
 *			V I E W R A Y
 *
 *  Ray Tracing program RTRAY bottom half.
 *
 *  This module turns RT library partition lists into VLD Standard Format
 *  ray files, as defined by /vld/include/ray.h.  A variety of VLD programs
 *  exist to manipulate these files, including rayvect.
 *
 *  To obtain a UNIX-plot of a ray file, the procedure is:
 *	/vld/bin/rayvect -mMM < file.ray > file.vect
 *	/vld/bin/vectplot -mMM < file.vect > file.plot
 *	tplot -Tmeg file.plot		# or equivalent
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
static char RCSrayview[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"

int		use_air = 1;		/* Handling of air in librt */

char usage[] = "\
Usage:  rtray [options] model.g objects... >file.ray\n\
Options:\n\
 -f #		Grid size in pixels, default 512\n\
 -a Az		Azimuth in degrees	(conflicts with -M)\n\
 -e Elev	Elevation in degrees	(conflicts with -M)\n\
 -M		Read model2view matrix on stdin (conflicts with -a, -e)\n\
 -x #		Set librt debug flags\n\
";

/* Null function -- handle a miss */
int	raymiss() { return(0); }

void	view_pixel() {}

/* "paint" types are negative ==> interpret as "special" air codes */
#define PAINT_FIRST_ENTRY	(-999)
#define PAINT_INTERN_EXIT	(-998)
#define PAINT_INTERN_ENTRY	(-997)
#define PAINT_FINAL_EXIT	(-996)
#define PAINT_AIR		(-1)

/*
 *			R A Y H I T
 *
 *  Write a hit to the ray file.
 *  Also generate various forms of "paint".
 */
int
rayhit( ap, PartHeadp )
struct application *ap;
register struct partition *PartHeadp;
{
	register struct partition *pp = PartHeadp->pt_forw;
	struct partition	*np;	/* next partition */
	struct partition	air;

	if( pp == PartHeadp )
		return(0);		/* nothing was actually hit?? */

	/* "1st entry" paint */
	RT_HIT_NORM( pp->pt_inhit, pp->pt_inseg->seg_stp, &(ap->a_ray) );
	if( pp->pt_inflip )  {
		VREVERSE( pp->pt_inhit->hit_normal, pp->pt_inhit->hit_normal );
		pp->pt_inflip = 0;
	}
	wraypaint( pp->pt_inhit->hit_point, pp->pt_inhit->hit_normal,
		PAINT_FIRST_ENTRY, ap, stdout );

	for( ; pp != PartHeadp; pp = pp->pt_forw )  {
		/* Write the ray for this partition */
		RT_HIT_NORM( pp->pt_inhit, pp->pt_inseg->seg_stp, &(ap->a_ray) );
		if( pp->pt_inflip )  {
			VREVERSE( pp->pt_inhit->hit_normal,
				  pp->pt_inhit->hit_normal );
			pp->pt_inflip = 0;
		}
		RT_HIT_NORM( pp->pt_outhit, pp->pt_outseg->seg_stp, &(ap->a_ray) );
		if( pp->pt_outflip )  {
			VREVERSE( pp->pt_outhit->hit_normal,
				  pp->pt_outhit->hit_normal );
			pp->pt_outflip = 0;
		}
		wray( pp, ap, stdout );

		/*
		 * If there is a subsequent partition that does not
		 * directly join this one, output an invented
		 * "air" partition between them.
		 */
		if( (np = pp->pt_forw) == PartHeadp )
			break;		/* end of list */

		/* Obtain next inhit normals & hit point, for code below */
		RT_HIT_NORM( np->pt_inhit, np->pt_inseg->seg_stp, &(ap->a_ray) );
		if( np->pt_inflip )  {
			VREVERSE( np->pt_inhit->hit_normal,
				  np->pt_inhit->hit_normal );
			np->pt_inflip = 0;
		}

		if( rt_fdiff( pp->pt_outhit->hit_dist,
			      np->pt_inhit->hit_dist) >= 0 )  {
			/*
			 *  The two partitions touch (or overlap!).
			 *  If both are air, or both are solid, then don't
			 *  output any paint.
			 */
			if( pp->pt_regionp->reg_regionid >= 0 )  {
				/* Exiting a solid */
				if( np->pt_regionp->reg_regionid >= 0 )
					continue;	/* both are solid */
				/* output "internal exit" paint */
				wraypaint( pp->pt_outhit->hit_point,
					pp->pt_outhit->hit_normal,
					PAINT_INTERN_EXIT, ap, stdout );
			} else {
				/* Exiting air */
				if( np->pt_regionp->reg_regionid < 0 )
					continue;	/* both are air */
				/* output "internal entry" paint */
				wraypaint( np->pt_inhit->hit_point,
					np->pt_inhit->hit_normal,
					PAINT_INTERN_ENTRY, ap, stdout );
			}
			continue;
		}

		/*
		 *  The two partitions do not touch.
		 *  Put "internal exit" paint on out point,
		 *  Install "general air" in between,
		 *  and put "internal entry" paint on in point.
		 */
		wraypaint( pp->pt_outhit->hit_point,
			pp->pt_outhit->hit_normal,
			PAINT_INTERN_EXIT, ap, stdout );

		wraypts( pp->pt_outhit->hit_point,
			pp->pt_outhit->hit_normal,
			np->pt_inhit->hit_point,
			PAINT_AIR, ap, stdout );

		wraypaint( np->pt_inhit->hit_point,
			np->pt_inhit->hit_normal,
			PAINT_INTERN_ENTRY, ap, stdout );
	}

	/* "final exit" paint -- ray va(r)nishes off into the sunset */
	pp = PartHeadp->pt_back;
	wraypaint( pp->pt_outhit->hit_point,
		pp->pt_outhit->hit_normal,
		PAINT_FINAL_EXIT, ap, stdout );
	return(0);
}

/*
 *  			V I E W _ I N I T
 */
int
view_init( ap, file, obj, minus_o )
register struct application *ap;
char *file, *obj;
{
	ap->a_hit = rayhit;
	ap->a_miss = raymiss;
	ap->a_onehit = 0;

	if( minus_o )
		fprintf(stderr,"Warning:  -o ignored, rays go to stdout\n");
	return(0);		/* No framebuffer needed */
}

void	view_eol() {;}

void
view_end()
{
	fflush(stdout);
}

void	view_2init()  {;}

int	mlib_setup() { return(1); }

void	mlib_free() { ; }
