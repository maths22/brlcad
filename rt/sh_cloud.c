/*
 *			C L O U D . C
 *
 * An attempt at 2D Geoffrey Gardner style cloud texture map
 *
 *
 *  Author -
 *	Philip Dykstra
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
static char RCScloud[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"
#include "./mathtab.h"

struct cloud_specific {
	float	cl_thresh;
	float	cl_range;
};
#define CL_NULL	((struct cloud_specific *)0)

struct matparse cloud_parse[] = {
	"thresh",	(int)&(CL_NULL->cl_thresh),	"%f",
	"range",	(int)&(CL_NULL->cl_range),	"%f",
	(char *)0,	0,				(char *)0
};


#define	NUMSINES	4

/*
 *			C L O U D _ T E X T U R E
 *
 * Returns the texture value for a plane point
 */
double
cloud_texture(x,y,Contrast,initFx,initFy)
double x, y;
float Contrast, initFx, initFy;
{
	LOCAL float	t1, t2, k;
	LOCAL double	Px, Py, Fx, Fy, C;
	register int	i;

	t1 = t2 = 0;

	/*
	 * Compute initial Phases and Frequencies
	 * Freq "1" goes through 2Pi as x or y go thru 0.0 -> 1.0
	 */
	Fx = twopi * initFx;
	Fy = twopi * initFy;
	Px = halfpi * tab_sin( 0.5 * Fy * y );
	Py = halfpi * tab_sin( 0.5 * Fx * x );
	C = 1.0;	/* ??? */

	for( i = 0; i < NUMSINES; i++ ) {
		/*
		 * Compute one term of each summation.
		 */
		t1 += C * tab_sin( Fx * x + Px ) + Contrast;
		t2 += C * tab_sin( Fy * y + Py ) + Contrast;

		/*
		 * Compute the new phases and frequencies.
		 * N.B. The phases shouldn't vary the same way!
		 */
		Px = halfpi * tab_sin( Fy * y );
		Py = halfpi * tab_sin( Fx * x );
		Fx *= 2.0;
		Fy *= 2.0;
		C  *= 0.707;
	}

	/* Choose a magic k! */
	/* Compute max possible summation */
	k =  NUMSINES * 2 * NUMSINES;

	return( t1 * t2 / k );
}

extern cloud_render();

int
cloud_setup( rp )
register struct region *rp;
{
	register struct cloud_specific *cp;

	GETSTRUCT( cp, cloud_specific );
	rp->reg_ufunc = cloud_render;
	rp->reg_udata = (char *)cp;

	cp->cl_thresh = 0.35;
	cp->cl_range = 0.3;
	mlib_parse( rp->reg_mater.ma_matparm, cloud_parse, (char *)cp );
	return(1);
}


/*
 *			C L O U D _ R E N D E R
 *
 * Return a sky color with translucency control.
 *  Threshold is the intensity below which it is completely translucent.
 *  Range in the range on intensities over which translucence varies
 *   from 0 to 1.
 *  thresh=0.35, range=0.3 for decent clouds.
 */
cloud_render( ap, pp )
register struct application *ap;
register struct partition *pp;
{
	register struct cloud_specific *cp =
		(struct cloud_specific *)pp->pt_regionp->reg_udata;
	auto struct uvcoord uv;
	double intensity;
	FAST fastf_t	TR;

	VJOIN1( pp->pt_inhit->hit_point, ap->a_ray.r_pt,
		pp->pt_inhit->hit_dist, ap->a_ray.r_dir );
	rt_functab[pp->pt_inseg->seg_stp->st_id].ft_uv(
		ap, pp->pt_inseg->seg_stp, pp->pt_inhit, &uv );
	intensity = cloud_texture( uv.uv_u, uv.uv_v, 1.0, 2.0, 1.0 );

	/* Intensity is normalized - check bounds */
	if( intensity > 1.0 )
		intensity = 1.0;
	else if( intensity < 0.0 )
		intensity = 0.0;

	/* Compute Translucency Function */
	TR = 1.0 - ( intensity - cp->cl_thresh ) / cp->cl_range;
	if (TR < 0.0)
		TR = 0.0;
	else if (TR > 1.0)
		TR = 1.0;

	ap->a_color[0] = ((1-TR) * intensity + (TR * .31));	/* Red */
	ap->a_color[1] = ((1-TR) * intensity + (TR * .31));	/* Green */
	ap->a_color[2] = ((1-TR) * intensity + (TR * .78));	/* Blue */
}
