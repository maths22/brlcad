/*
 *			P L A S T I C
 *
 *  Notes -
 *	The normals on all surfaces point OUT of the solid.
 *	The incomming light rays point IN.  Thus the sign change.
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
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSplastic[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/mater.h"
#include "../h/raytrace.h"
#include "../librt/debug.h"
#include "material.h"

extern int colorview();		/* from view.c */

/* HACKS from view.c */
extern int lightmodel;		/* lighting model # to use */
extern struct soltab *l0stp;
extern vect_t l0color;
extern vect_t l0pos;
extern double AmbientIntensity;
#define MAX_IREFLECT	9	/* Maximum internal reflection level */
#define MAX_BOUNCE	4	/* Maximum recursion level */

/* Local information */
struct phong_specific {
	int	shine;
	double	wgt_specular;
	double	wgt_diffuse;
	double	transmit;	/* Moss "transparency" */
	double	reflect;	/* Moss "transmission" */
	double	refrac_index;
};
#define PL_NULL	((struct phong_specific *)0)

struct matparse phong_parse[] = {
	"shine",	(int)&(PL_NULL->shine),		"%d",
	"specular",	(int)&(PL_NULL->wgt_specular),	"%f",
	"diffuse",	(int)&(PL_NULL->wgt_diffuse),	"%f",
	"transmit",	(int)&(PL_NULL->transmit),	"%f",
	"reflect",	(int)&(PL_NULL->reflect),	"%f",
	"ri",		(int)&(PL_NULL->refrac_index),	"%f",
	(char *)0,	0,				(char *)0
};

extern int phong_render();
HIDDEN int phg_rmiss();
HIDDEN int phg_rhit();
HIDDEN int phg_refract();

extern int light_hit(), light_miss();
extern int hit_nothing();
extern double ipow();

#define RI_AIR		1.0    /* Refractive index of air.		*/
#define RI_GLASS	1.3    /* Refractive index of glass.		*/

#ifdef BENCHMARK
#define rand0to1()	(0.5)
#define rand_half()	(0)
#else BENCHMARK
/*
 *  			R A N D 0 T O 1
 *
 *  Returns a random number in the range 0 to 1
 */
double rand0to1()
{
	FAST fastf_t f;
	/* On BSD, might want to use random() */
	/* / (double)0x7FFFFFFFL */
	f = ((double)rand()) *
		0.00000000046566128752457969241057508271679984532147;
	if( f > 1.0 || f < 0 )  {
		rt_log("rand0to1 out of range\n");
		return(0.42);
	}
	return(f);
}

/*
 *  			R A N D _ H A L F
 *
 *  Returns a random number in the range -0.5 to +0.5
 */
#define rand_half()	(rand0to1()-0.5)

#endif BENCHMARK

/*
 *			P L A S T I C _ S E T U P
 */
int
phong_setup( rp )
register struct region *rp;
{
	register struct phong_specific *pp;

	GETSTRUCT( pp, phong_specific );
	rp->reg_ufunc = phong_render;
	rp->reg_udata = (char *)pp;

	pp->shine = 7;
	pp->wgt_specular = 0.6;
	pp->wgt_diffuse = 0.3;
	pp->transmit = 0.0;
	pp->reflect = 0.0;
	pp->refrac_index = 0.0;

	mlib_parse( rp->reg_mater.ma_matparm, phong_parse, (char *)pp );
	return(1);
}


/*
 *			P L A S T I C _ R E N D E R
 *
	Color pixel based on the energy of a point light source (Eps)
	plus some diffuse illumination (Epd) reflected from the point
	<x,y> :

				E = Epd + Eps		(1)

	The energy reflected from diffuse illumination is the product
	of the reflectance coefficient at point P (Rp) and the diffuse
	illumination (Id) :

				Epd = Rp * Id		(2)

	The energy reflected from the point light source is calculated
	by the sum of the diffuse reflectance (Rd) and the specular
	reflectance (Rs), multiplied by the intensity of the light
	source (Ips) :

				Eps = (Rd + Rs) * Ips	(3)

	The diffuse reflectance is calculated by the product of the
	reflectance coefficient (Rp) and the cosine of the angle of
	incidence (I) :

				Rd = Rp * cos(I)	(4)

	The specular reflectance is calculated by the product of the
	specular reflectance coeffient and (the cosine of the angle (S)
	raised to the nth power) :

				Rs = W(I) * cos(S)**n	(5)

	Where,
		I is the angle of incidence.
		S is the angle between the reflected ray and the observer.
		W returns the specular reflection coefficient as a function
	of the angle of incidence.
		n (roughly 1 to 10) represents the shininess of the surface.
 *
	This is the heart of the lighting model which is based on a model
	developed by Bui-Tuong Phong, [see Wm M. Newman and R. F. Sproull,
	"Principles of Interactive Computer Graphics", 	McGraw-Hill, 1979]

	Er = Ra(m)*cos(Ia) + Rd(m)*cos(I1) + W(I1,m)*cos(s)^^n
	where,
 
	Er	is the energy reflected in the observer's direction.
	Ra	is the diffuse reflectance coefficient at the point
		of intersection due to ambient lighting.
	Ia	is the angle of incidence associated with the ambient
		light source (angle between ray direction (negated) and
		surface normal).
	Rd	is the diffuse reflectance coefficient at the point
		of intersection due to primary lighting.
	I1	is the angle of incidence associated with the primary
		light source (angle between light source direction and
		surface normal).
	m	is the material identification code.
	W	is the specular reflectance coefficient,
		a function of the angle of incidence, range 0.0 to 1.0,
		for the material.
	s	is the angle between the reflected ray and the observer.
	n	'Shininess' of the material,  range 1 to 10.
 */
phong_render( ap, pp )
register struct application *ap;
register struct partition *pp;
{
	register struct hit *hitp= pp->pt_inhit;
	auto struct application sub_ap;
	auto int light_visible;
	auto fastf_t	Rd1;
	auto fastf_t	d_a;		/* ambient diffuse */
	auto double	cosI1, cosI2;
	auto fastf_t	f;
	auto vect_t	work;
	auto vect_t	reflected;
	auto vect_t	to_eye;
	auto vect_t	to_light;
	auto point_t	mcolor;		/* Material color */
	struct phong_specific *ps =
		(struct phong_specific *)pp->pt_regionp->reg_udata;


	/* Get default color-by-ident for region.			*/
	{
		register struct region *rp;
		if( (rp=pp->pt_regionp) == REGION_NULL )  {
			rt_log("bad region pointer\n");
			VSET( ap->a_color, 0.7, 0.7, 0.7 );
			goto finish;
		}
		if( rp->reg_mater.ma_override )  {
			VSET( mcolor,
				rp->reg_mater.ma_rgb[0]/255.,
				rp->reg_mater.ma_rgb[1]/255.,
				rp->reg_mater.ma_rgb[2]/255. );
		} else {
			VSET( mcolor, 1.0, 0.0, 0.0 );	/* default: red */
		}
	}

	VREVERSE( to_eye, ap->a_ray.r_dir );

	/* Diminish intensity of reflected light the as a function of
	 * the distance from your eye.
	 */
/**	dist_gradient = kCons / (hitp->hit_dist + cCons);  */

	/* Diffuse reflectance from primary light source. */
	VSUB2( to_light, l0pos, hitp->hit_point );
	VUNITIZE( to_light );
	Rd1 = 0;
	if( (cosI1 = VDOT( hitp->hit_normal, to_light )) > 0.0 )  {
		if( cosI1 > 1 )  {
			rt_log("cosI1=%f (x%d,y%d,lvl%d)\n", cosI1,
				ap->a_x, ap->a_y, ap->a_level);
			cosI1 = 1;
		}
		Rd1 = cosI1 * (1 - AmbientIntensity);
	}

	/* Diffuse reflectance from secondary light source (at eye) */
	d_a = 0;
	if( (cosI2 = VDOT( hitp->hit_normal, to_eye )) > 0.0 )  {
		if( cosI2 > 1 )  {
			rt_log("cosI2=%f (x%d,y%d,lvl%d)\n", cosI2,
				ap->a_x, ap->a_y, ap->a_level);
			cosI2 = 1;
		}
		d_a = cosI2 * AmbientIntensity;
	}

	/* Apply secondary (ambient) (white) lighting. */
	VSCALE( ap->a_color, mcolor, d_a );
	if( l0stp )  {
		/* An actual light solid exists */
		FAST fastf_t f;

		/* Fire ray at light source to check for shadowing */
		/* This SHOULD actually return an energy value */
		sub_ap.a_hit = light_hit;
		sub_ap.a_miss = light_miss;
		sub_ap.a_onehit = 1;
		sub_ap.a_level = ap->a_level + 1;
		sub_ap.a_x = ap->a_x;
		sub_ap.a_y = ap->a_y;
		VMOVE( sub_ap.a_ray.r_pt, hitp->hit_point );

		/* Dither light pos for penumbra by +/- 0.5 light radius */
		f = l0stp->st_aradius * 0.9;
		sub_ap.a_ray.r_dir[X] =  l0pos[X] + rand_half()*f - hitp->hit_point[X];
		sub_ap.a_ray.r_dir[Y] =  l0pos[Y] + rand_half()*f - hitp->hit_point[Y];
		sub_ap.a_ray.r_dir[Z] =  l0pos[Z] + rand_half()*f - hitp->hit_point[Z];
		VUNITIZE( sub_ap.a_ray.r_dir );
		light_visible = rt_shootray( &sub_ap );
	} else {
		light_visible = 1;
	}
	
	/* If not shadowed add primary lighting. */
	if( light_visible )  {
		auto fastf_t specular;
		auto fastf_t cosS;

		/* Diffuse */
		VJOIN1( ap->a_color, ap->a_color, Rd1, mcolor );

		/* Calculate specular reflectance.
		 *	Reflected ray = (2 * cos(i) * Normal) - Incident ray.
		 * 	Cos(s) = Reflected ray DOT Incident ray.
		 */
		cosI1 *= 2;
		VSCALE( work, hitp->hit_normal, cosI1 );
		VSUB2( reflected, work, to_light );
		if( (cosS = VDOT( reflected, to_eye )) > 0 )  {
			if( cosS > 1 )  {
				rt_log("cosS=%f (x%d,y%d,lvl%d)\n", cosS,
					ap->a_x, ap->a_y, ap->a_level);
				cosS = 1;
			}
			specular = ps->wgt_specular *
				ipow(cosS,(int)ps->shine);
			VJOIN1( ap->a_color, ap->a_color, specular, l0color );
		}
	}

	if( (ps->reflect <= 0 && ps->transmit <= 0) ||
	    ap->a_level > MAX_BOUNCE )  {
		if( rt_g.debug&DEBUG_RAYWRITE )  {
			register struct soltab *stp;
			/* Record passing through the solid */
			stp = pp->pt_outseg->seg_stp;
			rt_functab[stp->st_id].ft_norm(
				pp->pt_outhit, stp, &(ap->a_ray) );
			wray( pp, ap, stdout );
		}
		/* Nothing more to do for this ray */
		goto finish;
	}

	/* Add in contributions from mirror reflection & transparency */
	f = 1 - (ps->reflect + ps->transmit);
	if( f > 0 )  {
		VSCALE( ap->a_color, ap->a_color, f );
	}
	if( ps->reflect > 0 )  {
		/* Mirror reflection */
		sub_ap.a_level = ap->a_level+1;
		sub_ap.a_hit = colorview;
		sub_ap.a_miss = hit_nothing;
		sub_ap.a_onehit = 1;
		VMOVE( sub_ap.a_ray.r_pt, hitp->hit_point );
		f = 2 * VDOT( to_eye, hitp->hit_normal );
		VSCALE( work, hitp->hit_normal, f );
		/* I have been told this has unit length */
		VSUB2( sub_ap.a_ray.r_dir, work, to_eye );
		(void)rt_shootray( &sub_ap );
		VJOIN1( ap->a_color, ap->a_color,
			ps->reflect, sub_ap.a_color );
	}
	if( ps->transmit > 0 )  {
		/* Calculate refraction at entrance. */
		sub_ap.a_level = ap->a_level * 100;	/* flag */
		sub_ap.a_x = ap->a_x;
		sub_ap.a_y = ap->a_y;
		if( !phg_refract(ap->a_ray.r_dir, /* Incident ray (IN) */
			hitp->hit_normal,
			RI_AIR, ps->refrac_index,
			sub_ap.a_ray.r_dir	/* Refracted ray (OUT) */
		) )  {
			/* Reflected back outside solid */
			VMOVE( sub_ap.a_ray.r_pt, hitp->hit_point );
			goto do_exit;
		}
		/* Find new exit point from the inside. */
		VMOVE( sub_ap.a_ray.r_pt, hitp->hit_point );
do_inside:
		sub_ap.a_hit =  phg_rhit;
		sub_ap.a_miss = phg_rmiss;
		(void) rt_shootray( &sub_ap );
		/* NOTE: phg_rhit returns EXIT point in sub_ap.a_uvec,
		 *  and returns EXIT normal in sub_ap.a_color.
		 */
		if( rt_g.debug&DEBUG_RAYWRITE )  {
			wraypts( sub_ap.a_ray.r_pt, sub_ap.a_uvec,
				ap, stdout );
		}
		VMOVE( sub_ap.a_ray.r_pt, sub_ap.a_uvec );

		/* Calculate refraction at exit. */
		if( !phg_refract( sub_ap.a_ray.r_dir,	/* input direction */
			sub_ap.a_color,			/* exit normal */
			ps->refrac_index, RI_AIR,
			sub_ap.a_ray.r_dir		/* output direction */
		) )  {
			/* Reflected internally -- keep going */
			if( ++sub_ap.a_level > 100+MAX_IREFLECT )  {
				rt_log("Excessive internal reflection (x%d,y%d, lvl%d)\n",
					sub_ap.a_x, sub_ap.a_y, sub_ap.a_level );
				if(rt_g.debug) {
					VSET( ap->a_color, 0, 1, 0 );	/* green */
				} else {
					VSET( ap->a_color, .16, .16, .16 );	/* grey */
				}
				goto finish;
			}
			goto do_inside;
		}
do_exit:
		sub_ap.a_hit =  colorview;
		sub_ap.a_miss = hit_nothing;
		sub_ap.a_level++;
		(void) rt_shootray( &sub_ap );
		VJOIN1( ap->a_color, ap->a_color,
			ps->transmit, sub_ap.a_color );
	}
finish:
	return(1);
}

/*
 *			R F R _ M I S S
 */
HIDDEN int
/*ARGSUSED*/
phg_rmiss( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
{
	rt_log("phg_rmiss: Refracted ray missed!\n" );
	/* Return entry point as exit point */
	VREVERSE( ap->a_color, ap->a_ray.r_dir );	/* inward pointing */
	VMOVE( ap->a_uvec, ap->a_ray.r_pt );
	return(0);
}

/*
 *			R F R _ H I T
 */
HIDDEN int
phg_rhit( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
{
	register struct hit	*hitp = PartHeadp->pt_forw->pt_outhit;
	register struct soltab *stp;

	stp = PartHeadp->pt_forw->pt_outseg->seg_stp;
	rt_functab[stp->st_id].ft_norm(
		hitp, stp, &(ap->a_ray) );
	VMOVE( ap->a_uvec, hitp->hit_point );
	/* For refraction, want exit normal to point inward. */
	VREVERSE( ap->a_color, hitp->hit_normal );
	return(1);
}

/*
 *			R E F R A C T
 *
 *	Compute the refracted ray 'v_2' from the incident ray 'v_1' with
 *	the refractive indices 'ri_2' and 'ri_1' respectively.
 *	Using Schnell's Law:
 *
 *		theta_1 = angle of v_1 with surface normal
 *		theta_2 = angle of v_2 with reversed surface normal
 *		ri_1 * sin( theta_1 ) = ri_2 * sin( theta_2 )
 *
 *		sin( theta_2 ) = ri_1/ri_2 * sin( theta_1 )
 *		
 *	The above condition is undefined for ri_1/ri_2 * sin( theta_1 )
 *	being greater than 1, and this represents the condition for total
 *	reflection, the 'critical angle' is the angle theta_1 for which
 *	ri_1/ri_2 * sin( theta_1 ) equals 1.
 *
 *  Returns TRUE if refracted, FALSE if reflected.
 *
 *  Note:  output (v_2) can be same storage as an input.
 */
HIDDEN int
phg_refract( v_1, norml, ri_1, ri_2, v_2 )
register vect_t	v_1;
register vect_t	norml;
double	ri_1, ri_2;
register vect_t	v_2;
{
	LOCAL vect_t	w, u;
	FAST fastf_t	beta;

	if( NEAR_ZERO(ri_1, 0.0001) || NEAR_ZERO( ri_2, 0.0001 ) )  {
		rt_log("phg_refract:ri1=%f, ri2=%f\n", ri_1, ri_2 );
		beta = 1;
	} else {
		beta = ri_1/ri_2;		/* temp */
	}
	VSCALE( w, v_1, beta );
	VCROSS( u, w, norml );
	/*
	 *	|w X norml| = |w||norml| * sin( theta_1 )
	 *	        |u| = ri_1/ri_2 * sin( theta_1 ) = sin( theta_2 )
	 */
	if( (beta = VDOT( u, u )) > 1.0 )  {
		/*  Past critical angle, total reflection.
		 *  Calculate reflected (bounced) incident ray.
		 */
		VREVERSE( u, v_1 );
		beta = 2 * VDOT( u, norml );
		VSCALE( w, norml, beta );
		VSUB2( v_2, w, u );
		return(0);		/* reflected */
	} else {
		/*
		 * 1 - beta = 1 - sin( theta_2 )^^2
		 *	    = cos( theta_2 )^^2.
		 *     beta = -1.0 * cos( theta_2 ) - Dot( w, norml ).
		 */
		beta = -sqrt( 1.0 - beta) - VDOT( w, norml );
		VSCALE( u, norml, beta );
		VADD2( v_2, w, u );
		return(1);		/* refracted */
	}
	/* NOTREACHED */
}

/*
 *  			I P O W
 *  
 *  Raise a floating point number to an integer power
 */
double
ipow( d, cnt )
double d;
register int cnt;
{
	FAST fastf_t result;

	if( d < 1e-8 )  return(0.0);
	result = 1;
	while( cnt-- > 0 )
		result *= d;
	return( result );
}

/* These shadow functions return a boolean "light_visible" */
light_hit(ap, PartHeadp)
struct application *ap;
struct partition *PartHeadp;
{
	/* Check to see if we hit the light source */
	if( PartHeadp->pt_forw->pt_inseg->seg_stp == l0stp )
		return(1);		/* light_visible = 1 */
	return(0);			/* light_visible = 0 */
}

/*
 *  			L I G H T _ M I S S
 *  
 *  If there is no explicit light solid in the model, we will always "miss"
 *  the light, so return light_visible = TRUE.
 */
/* ARGSUSED */
light_miss(ap, PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
	return(1);			/* light_visible = 1 */
}

/* Null function */
nullf() { ; }

/*
 *			H I T _ N O T H I N G
 *
 *  a_miss() routine called when no part of the model is hit.
 *  Background texture mapping could be done here.
 *  For now, return a pleasant dark blue.
 */
hit_nothing( ap )
register struct application *ap;
{
	if( lightmodel == 2 )  {
		VSET( ap->a_color, 0, 0, 0 );
	}  else  {
		VSET( ap->a_color, .25, 0, .5 );	/* Background */
	}
	return(0);
}
