 /*
 *			V I E W H I D E
 *
 *  Ray Tracing program RT_HIDE bottom half.
 *
 *  This module utilizes the RT library to interrogate a MGED
 *  model.  It consists of a front end that writes a ray data
 *  file, and a post-processor, which reads said file and is
 *  responsible for producing the UnixPlot file.
 *  The output format is:
 *  		view title
 *  			ray data
 *  		production of the UnixPlot file from the ray data
 *  
 *  
 *			 :
 *			 :
 *
 *  At present, the main use for this module is to generate
 *  UnixPlot file that can be read by various existing filters
 *  to produce a three dimensional line drawing of an MGED object.
 *  Three dimensionality is is achieved through the rotation of the
 *  MGED model interrogated.
 *
 *  Authors -
 *	Dr. Susanne L. Muuss
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSrayhide[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"

#include "rdebug.h"

/* x, y, region_id, hit_dist */
#define	SHOT_FMT	"%d %d %d %g\n"

/***** view.c variables imported from rt.c *****/
extern int	output_is_binary;	/* !0 means output file is binary */

/***** worker.c variables imported from rt.c *****/
extern int	width;			/* # of pixels in X */
extern int	height;			/* # of lines in Y */
/*****/

int		use_air = 1;		/* Handling of air in librt */

int		using_mlib = 0;		/* Material routines NOT used */

/* Viewing module specific "set" variables */
 struct structparse view_parse[] = {
	(char *)0,(char *)0,	(stroff_t)0,				FUNC_NULL
 };

FILE            *plotfp;		/* optional plotting file */

extern FILE	*outfp;			/* optional output file */


char usage[] = "\
Usage:  rt_hide [options] model.g objects... >file.ray\n\
Options:\n\
 -s #		Grid size in pixels, default 512\n\
 -a Az		Azimuth in degrees	(conflicts with -M)\n\
 -e Elev	Elevation in degrees	(conflicts with -M)\n\
 -M		Read model2view matrix on stdin (conflicts with -a, -e)\n\
 -o model.g	Specify output file (default=stdout)\n\
 -U #		Set use_air boolean to # (default=1)\n\
 -x #		Set librt debug flags\n\
";

int	rayhit(), raymiss();

/*
 *  			V I E W _ I N I T
 *
 *  This routine is called by main().  
 *  Furthermore, pointers to rayhit() and raymiss() are set up
 *  and are later called from do_run().
 */
int
view_init( ap, file, obj, minus_o )
register struct application *ap;
char *file, *obj;
{

	if( outfp == NULL )
		outfp = stdout;

	ap->a_hit = rayhit;
	ap->a_miss = raymiss;
	ap->a_onehit = 0;

	output_is_binary = 0;		/* output is printable ascii */

#ifdef
	if(rdebug & RDEBUG_RAYPLOT) {
		plotfp = fopen("rt_hide.pl", "w");
	}
#endif


	return(0);		/* No framebuffer needed */
}

/*
 *			V I E W _ 2 I N I T
 *
 *  A null-function.
 *  View_2init is called by do_frame(), which in turn is called by
 *  main() in rt.c.
 */
void
view_2init( ap )
struct application	*ap;
{

	if( outfp == NULL )
		rt_bomb("outfp is NULL\n");

	
	regionfix( ap, "rtray.regexp" );		/* XXX */
}

/*
 *			R A Y M I S S
 *
 *  This function is called by rt_shootray(), which is called by
 *  do_frame(). Writes out data necessary to record where a miss
 *  miss was scored.
 */

int
raymiss( ap )
register struct application	*ap;
{

	/*
	 * The distance travelled by the ray and the region_id of the area
	 * missed are both set to 0.  However, the pixel coordinates where
	 * the miss occurred are calculated.
	 */
	fprintf( outfp, SHOT_FMT,
		ap->a_x, ap->a_y,
		0, 0.0 );
	return(0);
}

/*
 *			V I E W _ P I X E L
 *
 *  This routine is called from do_run(), and in this case does nothing.
 */
void
view_pixel()
{
	return;
}

/*
 *			R A Y H I T
 *
 *  Rayhit() is called by rt_shootray() when a hit is detected.  It
 *  writes a hit to the ray file.
 *  An rt_hide file is written for the post-processor to read.
 */
int
rayhit( ap, PartHeadp )
struct application *ap;
register struct partition *PartHeadp;
{
	register struct partition *pp = PartHeadp->pt_forw;
	register struct partition *nextpp = pp->pt_forw;
	struct partition	*np;	/* next partition */
	struct partition	air;
	fastf_t			dist;   	/* ray distance */
	char			*fmt;		/* printf() format string */
	int			region_id;	/* solid region's id */



	if( pp == PartHeadp )
		return(0);		/* nothing was actually hit?? */

	
	/*
	 * Output the ray data: distance to hit, region_id, screen
	 * plane (pixel) coordinates for x and y positions of a ray.
	 * These positions are represented by ap->a_x and ap->a_y.
	 *
	 *  Assume all rays are parallel.
	 */

	dist = pp->pt_inhit->hit_dist;
	region_id = pp->pt_regionp->reg_regionid;
	
	fprintf(outfp, SHOT_FMT,
		ap->a_x, ap->a_y,
		region_id, dist );

#ifdef
		if( (region_id = pp->pt_regionp->reg_regionid) <= 0 )  {
			region_id = 1;
		}


		/* A color rtg3.pl UnixPlot file of output commands
		 * is generated.  This is processed by plot(1)
		 * plotting filters such as pl-fb or pl-xxx.
		 * Inhits are assigned green; outhits are assigned
		 * blue.
		 */

		if(rdebug & RDEBUG_RAYPLOT) {
			vect_t     inpt;
			vect_t     outpt;
			VJOIN1(inpt, ap->a_ray.r_pt, pp->pt_inhit->hit_dist,
				ap->a_ray.r_dir);
			VJOIN1(outpt, ap->a_ray.r_pt, pp->pt_outhit->hit_dist,
				ap->a_ray.r_dir);
				pl_color(plotfp, 0, 255, 0);	/* green */
			pdv_3line(plotfp, inpt,outpt);
			
			if(air_thickness > 0) {
				vect_t     air_end;
				VJOIN1(air_end, ap->a_ray.r_pt,
					pp->pt_outhit->hit_dist + air_thickness,
					ap->a_ray.r_dir);
				pl_color(plotfp, 0, 0, 255);	/* blue */
				pdv_3cont(plotfp, air_end);
			}
		}
	}
#endif

	return(0);
}

/*
 *			V I E W _ E O L
 *
 *  View_eol() is called by rt_shootray() in do_run().  In this case,
 *  it does nothing.
 */
void	view_eol()
{
}

/*
 *			V I E W _ E N D
 *
 *  View_end() is called by rt_shootray in do_run().
 */
void
view_end()
{

	fflush(outfp);
}
