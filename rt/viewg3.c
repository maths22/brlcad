/*
 *			V I E W G 3
 *
 *  Ray Tracing program RTG3 bottom half.
 *
 *  This module turns RT library partition lists into
 *  the old GIFT type shotlines with three components per card,
 *  and with both the entrance and exit obliquity angles.
 *  The output format is:
 *	overall header card
 *		view header card
 *			ray (shotline) header card
 *				component card(s)
 *			ray (shotline) header card
 *			 :
 *			 :
 *
 *  At present, the main use for this format ray file is
 *  to drive the JTCG-approved COVART2 and COVART3 applications.
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
static char RCSrayg3[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <ctype.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"

#include "rdebug.h"

extern char	*strchr();
extern char	*re_comp();
extern int	re_exec();

int		use_air = 1;		/* Handling of air in librt */

int		using_mlib = 0;		/* Material routines NOT used */

/* Viewing module specific "set" variables */
struct structparse view_parse[] = {
	(char *)0,(char *)0,	(stroff_t)0,				FUNC_NULL
};

extern FILE	*outfp;			/* optional output file */

char usage[] = "\
Usage:  rtray [options] model.g objects... >file.ray\n\
Options:\n\
 -s #		Grid size in pixels, default 512\n\
 -a Az		Azimuth in degrees	(conflicts with -M)\n\
 -e Elev	Elevation in degrees	(conflicts with -M)\n\
 -M		Read model2view matrix on stdin (conflicts with -a, -e)\n\
 -o model.ray	Specify output file, ray(5V) format (default=stdout)\n\
 -U #		Set use_air boolean to #\n\
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
		PAINT_FIRST_ENTRY, ap, outfp );

	for( ; pp != PartHeadp; pp = pp->pt_forw )  {
		/* Write the ray for this partition */
		RT_HIT_NORM( pp->pt_inhit, pp->pt_inseg->seg_stp, &(ap->a_ray) );
		if( pp->pt_inflip )  {
			VREVERSE( pp->pt_inhit->hit_normal,
				  pp->pt_inhit->hit_normal );
			pp->pt_inflip = 0;
		}

		if( pp->pt_outhit->hit_dist < INFINITY )  {
			RT_HIT_NORM( pp->pt_outhit,
				pp->pt_outseg->seg_stp, &(ap->a_ray) );
			if( pp->pt_outflip )  {
				VREVERSE( pp->pt_outhit->hit_normal,
					  pp->pt_outhit->hit_normal );
				pp->pt_outflip = 0;
			}
		}
		wray( pp, ap, outfp );

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
			if( pp->pt_regionp->reg_regionid > 0 )  {
				/* Exiting a solid */
				if( np->pt_regionp->reg_regionid > 0 )
					continue;	/* both are solid */
				/* output "internal exit" paint */
				wraypaint( pp->pt_outhit->hit_point,
					pp->pt_outhit->hit_normal,
					PAINT_INTERN_EXIT, ap, outfp );
			} else {
				/* Exiting air */
				if( np->pt_regionp->reg_regionid <= 0 )
					continue;	/* both are air */
				/* output "internal entry" paint */
				wraypaint( np->pt_inhit->hit_point,
					np->pt_inhit->hit_normal,
					PAINT_INTERN_ENTRY, ap, outfp );
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
			PAINT_INTERN_EXIT, ap, outfp );

		wraypts( pp->pt_outhit->hit_point,
			pp->pt_outhit->hit_normal,
			np->pt_inhit->hit_point,
			PAINT_AIR, ap, outfp );

		wraypaint( np->pt_inhit->hit_point,
			np->pt_inhit->hit_normal,
			PAINT_INTERN_ENTRY, ap, outfp );
	}

	/* "final exit" paint -- ray va(r)nishes off into the sunset */
	pp = PartHeadp->pt_back;
	if( pp->pt_outhit->hit_dist < INFINITY )  {
		wraypaint( pp->pt_outhit->hit_point,
			pp->pt_outhit->hit_normal,
			PAINT_FINAL_EXIT, ap, outfp );
	}
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

	return(0);		/* No framebuffer needed */
}

void	view_eol() {;}

void
view_end()
{
	fflush(outfp);
}

void
view_2init( ap )
struct application	*ap;
{
	char	*file = "rtray.regexp";			/* XXX */
	FILE	*fp;
	char	*line;
	char	*tabp;
	int	linenum = 0;
	char	*err;
	register struct region	*rp;
	int	ret;
	int	oldid;
	int	newid;

	if( outfp == NULL )
		rt_bomb("outfp is NULL\n");

	/*
	 *  Apply any deltas to reg_regionid values
	 *  to allow old applications that use the reg_regionid number
	 *  to distinguish between different instances of the same
	 *  prototype region.
	 */
	if( (fp = fopen( file, "r" )) == NULL )	 {
		perror(file);
		return;
	}

	while( (line = rt_read_cmd( fp )) != (char *) 0 )  {
		linenum++;
		/*  For now, establish a simple format:
		 *  regexp TAB formula SEMICOLON
		 */
		if( (tabp = strchr( line, '\t' )) == (char *)0 )  {
			rt_log("%s: missing TAB on line %d:\n%s\n", file, linenum, line );
			continue;		/* just ignore it */
		}
		*tabp++ = '\0';
		while( *tabp && isspace( *tabp ) )  tabp++;
		if( (err = re_comp(line)) != (char *)0 )  {
			rt_log("%s: line %d, re_comp error '%s'\n", file, line, err );
			continue;		/* just ignore it */
		}
		
		rp = ap->a_rt_i->HeadRegion;
		for( ; rp != REGION_NULL; rp = rp->reg_forw )  {
			ret = re_exec(rp->reg_name);
			if(rdebug&RDEBUG_INSTANCE)  {
				rt_log("'%s' %s '%s'\n", line,
					ret==1 ? "==" : "!=",
					rp->reg_name);
			}
			if( (ret) == 0  )
				continue;	/* didn't match */
			if( ret == -1 )  {
				rt_log("%s: line %d, invalid regular expression\n", file, linenum);
				break;		/* on to next RE */
			}
			/*
			 *  RE matched this name, perform indicated operation
			 *  For now, choices are limited.  Later this might
			 *  become an interpreted expression.  For now:
			 *	99	replace old region id with "num"
			 *	+99	increment old region id with "num"
			 *		(which may itself be a negative number)
			 *	+uses	increment old region id by the
			 *		current instance (use) count.
			 */
			oldid = rp->reg_regionid;
			if( strcmp( tabp, "+uses" ) == 0  )  {
				newid = oldid + rp->reg_instnum;
			} else if( *tabp == '+' )  {
				newid = oldid + atoi( tabp+1 );
			} else {
				newid = atoi( tabp );
				if( newid == 0 )  rt_log("%s, line %d Warning:  new id = 0\n", file, linenum );
			}
			if(rdebug&RDEBUG_INSTANCE)  {
				rt_log("%s instance %d:  region id changed from %d to %d\n",
					rp->reg_name, rp->reg_instnum,
					oldid, newid );
			}
			rp->reg_regionid = newid;
		}
		rt_free( line, "reg_expr line");
	}
	fclose( fp );
}
