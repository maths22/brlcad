/*
 *			V I E W . C
 *
 *	Ray Tracing program, lighting model manager.
 *
 *  Output is either interactive to a frame buffer, or written in a file.
 *  The output format is a .PIX file (a byte stream of R,G,B as u_char's).
 *
 *  The extern "lightmodel" selects which one is being used:
 *	0	model with color, based on Moss's LGT
 *	1	1-light, from the eye.
 *	2	Spencer's surface-normals-as-colors display
 *	3	3-light debugging model
 *	4	curvature debugging display (inv radius of curvature)
 *	5	curvature debugging (principal direction)
 *
 *  Notes -
 *	The normals on all surfaces point OUT of the solid.
 *	The incomming light rays point IN.
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
static char RCSview[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "fb.h"
#include "./ext.h"
#include "./rdebug.h"
#include "./material.h"
#include "./mathtab.h"
#include "./light.h"

int		use_air = 0;		/* Handling of air in librt */
int		using_mlib = 1;		/* Material routines used */

char usage[] = "\
Usage:  rt [options] model.g objects...\n\
Options:\n\
 -s #		Square grid size in pixels (default 512)\n\
 -w # -n #	Grid size width and height in pixels\n\
 -V #		View (pixel) aspect ratio (width/height)\n\
 -a #		Azimuth in degrees\n\
 -e #		Elevation in degrees\n\
 -M		Read matrix, cmds on stdin\n\
 -o model.pix	Specify output file, .pix format (default=fb)\n\
 -x #		Set librt debug flags\n\
 -X #		Set rt debug flags\n\
 -p #		Perspective viewing, in degrees side to side\n\
 -P #		Set number of processors\n\
";

extern FBIO	*fbp;			/* Framebuffer handle */

extern int	max_bounces;		/* from refract.c */
extern int	max_ireflect;		/* from refract.c */

extern struct region	env_region;	/* from text.c */

extern struct light_specific *LightHeadp;
vect_t ambient_color = { 1, 1, 1 };	/* Ambient white light */

vect_t	background = { 0.25, 0, 0.5 };	/* Dark Blue Background */
int	ibackground[3];			/* integer 0..255 version */

#ifdef RTSRV
extern int	srv_startpix;		/* offset for view_pixel */
extern int	srv_scanlen;		/* buf_mode=4 buffer length */
#endif
static int	buf_mode=0;	/* 0=pixel, 1=line, 2=frame, ... */
static int	*npix_left;	/* only used in buf_mode=2 */

/* Viewing module specific "set" variables */
struct structparse view_parse[] = {
	"%d",	"bounces",	(int)&max_bounces,		FUNC_NULL,
	"%d",	"ireflect",	(int)&max_ireflect,		FUNC_NULL,
	"%V",	"background",	(int)background,		FUNC_NULL,
	(char *)0,(char *)0,	0,				FUNC_NULL
};

/*
 *  			V I E W _ P I X E L
 *  
 *  Arrange to have the pixel output.
 *
 *  The buffering strategy for output files:
 *	In serial mode, let stdio handle the buffering.
 *	In parallel mode, save whole image until the end (view_end).
 *  The buffering strategy for an "online" libfb framebuffer:
 *	buf_mode = 0	single pixel I/O
 *	buf_mode = 1	line buffering
 *	buf_mode = 2	full frame buf, dump to fb+file at end of scanline
 *	buf_mode = 3	full frame buf, dump to fb+file at end of frame
 *	buf_mode = 4	multi-line buffering for RTSRV
 */
void
view_pixel(ap)
register struct application *ap;
{
	register int r,g,b;

	if( ap->a_user == 0 )  {
		/* Shot missed the model, don't dither */
		r = ibackground[0];
		g = ibackground[1];
		b = ibackground[2];
	} else {
		/*
		 *  To prevent bad color aliasing, add some color dither.
		 *  Be certain to NOT output the background color here.
		 *  Random numbers in the range 0 to 1 are used, so
		 *  that integer valued colors (eg, from texture maps)
		 *  retain their original values.
		 */
		r = ap->a_color[0]*255.+rand0to1(ap->a_resource->re_randptr);
		g = ap->a_color[1]*255.+rand0to1(ap->a_resource->re_randptr);
		b = ap->a_color[2]*255.+rand0to1(ap->a_resource->re_randptr);
		if( r > 255 ) r = 255;
		else if( r < 0 )  r = 0;
		if( g > 255 ) g = 255;
		else if( g < 0 )  g = 0;
		if( b > 255 ) b = 255;
		else if( b < 0 )  b = 0;
		if( r == ibackground[0] && g == ibackground[1] &&
		    b == ibackground[2] )  {
		    	register int i;
		    	int newcolor[3];
		    	/*  Find largest color channel to perterb.
		    	 *  It should happen infrequently.
		    	 *  If you have a faster algorithm, tell me.
		    	 */
		    	if( r > g )  {
		    		if( r > b )  i = 0;
		    		else i = 2;
		    	} else {
		    		if( g > b ) i = 1;
		    		else i = 2;
		    	}
		    	newcolor[0] = r;
		    	newcolor[1] = g;
		    	newcolor[2] = b;
			if( newcolor[i] < 127 ) newcolor[i]++;
		    	else newcolor[i]--;
		    	r = newcolor[0];
		    	g = newcolor[1];
		    	b = newcolor[2];
		}
	}

	if(rdebug&RDEBUG_HITS) rt_log("rgb=%3d,%3d,%3d xy=%3d,%3d\n", r,g,b, ap->a_x, ap->a_y);

	/*
	 *  Handle file output in the simple cases where we let stdio
	 *  do the buffering.
	 */
	if( buf_mode <= 1 && outfp != NULL )  {
		/* Single pixel I/O or "line buffering" (to screen) case */
		{
			unsigned char p[4];
			p[0] = r;
			p[1] = g;
			p[2] = b;
			if( fwrite( (char *)p, 3, 1, outfp ) != 1 )
				rt_bomb("pixel fwrite error");
		}
	}

	/*
	 *  Handle framebuffer output
	 */
	if( buf_mode == 0 )  {
		/* Single Pixel I/O */

		if( fbp != FBIO_NULL )  {
			RGBpixel p;
			p[RED] = r;
			p[GRN] = g;
			p[BLU] = b;
			RES_ACQUIRE( &rt_g.res_syscall );
			fb_write( fbp, ap->a_x, ap->a_y, p, 1 );
			RES_RELEASE( &rt_g.res_syscall );
		}
	} else {
		register char *pixelp;
		register int do_eol = 0;

		switch( buf_mode )  {
		case 1:
			/* Here, the buffer is only one line long */
			pixelp = scanbuf+ap->a_x*3;
			break;
		case 2:
		case 3:
			/* Buffering a full frame */
			pixelp = scanbuf+((ap->a_y*width)+ap->a_x)*3;
#ifdef RTSRV
		case 4:
			/* Multi-pixel buffer */
			pixelp = scanbuf+ 3 * 
				((ap->a_y*width) + ap->a_x - srv_startpix);
#endif
		}

		/* Don't depend on interlocked hardware byte-splice */
		RES_ACQUIRE( &rt_g.res_results );
		if( incr_mode )  {
			register int dx,dy;
			register int spread;

			spread = 1<<(incr_nlevel-incr_level);
			for( dy=0; dy<spread; dy++ )  {
				pixelp = scanbuf+
					(((ap->a_y+dy)*width)+ap->a_x)*3;
				for( dx=0; dx<spread; dx++ )  {
					*pixelp++ = r ;
					*pixelp++ = g ;
					*pixelp++ = b ;
				}
			}
			/* If incremental, first 3 iterations are boring */
			if( buf_mode == 2 && incr_level > 3 )  {
				if( --(npix_left[ap->a_y]) <= 0 )
					do_eol = 1;
			}
		} else {
			*pixelp++ = r ;
			*pixelp++ = g ;
			*pixelp++ = b ;
			if( buf_mode == 2 && --(npix_left[ap->a_y]) <= 0 )
				do_eol = 1;
		}
		RES_RELEASE( &rt_g.res_results );

		if( do_eol )  {
			/* buf_mode == 2 if we got here */
			if( fbp != FBIO_NULL )  {
				RES_ACQUIRE( &rt_g.res_syscall );
				if( incr_mode )  {
					register int dy, yy;
					register int spread;

					spread = 1<<(incr_nlevel-incr_level);
					for( dy=0; dy<spread; dy++ )  {
						yy = ap->a_y + dy;
						fb_write( fbp, 0, yy,
						    scanbuf+yy*width*3, width );
					}
				} else {
					fb_write( fbp, 0, ap->a_y,
					    scanbuf+ap->a_y*width*3, width );
				}
				RES_RELEASE( &rt_g.res_syscall );
			}
			if( outfp != NULL )  {
				int	count;

				RES_ACQUIRE( &rt_g.res_syscall );
				fseek( outfp, ap->a_y*width*3L, 0 );
				count = fwrite( scanbuf+ap->a_y*width*3,
					sizeof(char), width*3, outfp );
				RES_RELEASE( &rt_g.res_syscall );
				if( count != width*3 )
					rt_bomb("view_pixel:  fwrite failure\n");
			}
		}
	}
}

/*
 *  			V I E W _ E O L
 *  
 *  This routine is called by main when the last pixel of a scanline
 *  has been finished.  When in parallel mode, this routine is not
 *  used;  the do_eol check in view_pixel() handles things.
 *  This routine handles framebuffer output only, all file output
 *  is done elsewhere.
 */
void
view_eol(ap)
register struct application *ap;
{
	if( buf_mode <= 0 || fbp == FBIO_NULL )
		return;

	switch( buf_mode )  {
	case 4:
		break;
	case 3:
		break;
	case 2:
		break;
	default:
		RES_ACQUIRE( &rt_g.res_syscall );
		fb_write( fbp, 0, ap->a_y, scanbuf, width );
		RES_RELEASE( &rt_g.res_syscall );
		break;
	}
}

/*
 *			V I E W _ E N D
 */
view_end(ap)
struct application *ap;
{
	register struct light_specific *lp, *nlp;

	if( buf_mode == 3 )  {
		if( fbp != FBIO_NULL )  {
			/* Dump full screen */
			if( fb_getwidth(fbp) == width && fb_getheight(fbp) == height )  {
				fb_write( fbp, 0, 0, scanbuf, width*height );
			} else {
				register int y;
				for( y=0; y<height; y++ )
					fb_write( fbp, 0, y, scanbuf+y*width*3, width );
			}
		}
		if( (outfp != NULL) &&
		    fwrite( scanbuf, sizeof(char), width*height*3, outfp ) != width*height*3 )  {
			fprintf(stderr,"view_end:  fwrite failure\n");
			return(-1);		/* BAD */
		}
	}
	if( incr_mode )  {
		if( incr_level < incr_nlevel )
			return(0);		 /* more res to come */
	}

	/* Eliminate invisible lights (typ. implicit lights) */
	lp=LightHeadp;
	while( lp != LIGHT_NULL )  {
		if( lp->lt_invisible )  {
			nlp = lp->lt_forw;
			light_free( (char *)lp );
			lp = nlp;
			continue;
		}
		/* will be cleaned by mlib_free() */
		lp = lp->lt_forw;
	}
	return(0);		/* OK */
}

/*
 *			H I T _ N O T H I N G
 *
 *  a_miss() routine called when no part of the model is hit.
 *  Background texture mapping could be done here.
 *  For now, return a pleasant dark blue.
 */
hit_nothing( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
{
	if( rdebug&RDEBUG_MISSPLOT )  {
		vect_t	out;

		/* XXX length should be 1 model diameter */
		VJOIN1( out, ap->a_ray.r_pt,
			10000, ap->a_ray.r_dir );	/* to imply direction */
		pl_color( stdout, 190, 0, 0 );
		pdv_3line( stdout, ap->a_ray.r_pt, out );
	}

	if( env_region.reg_mfuncs  /* && ap->a_level > 0 */ )  {
		struct gunk {
			struct partition part;
			struct hit	hit;
			struct shadework sw;
		} u;

		bzero( (char *)&u, sizeof(u) );
		/* Make "miss" hit the environment map */
		/* Build up the fakery */
		u.part.pt_inhit = u.part.pt_outhit = &u.hit;
		u.part.pt_regionp = &env_region;
		u.hit.hit_dist = 0.0;	/* XXX should be = 1 model diameter */

		u.sw.sw_transmit = u.sw.sw_reflect = 0.0;
		u.sw.sw_refrac_index = 1.0;
		u.sw.sw_extinction = 0;
		u.sw.sw_xmitonly = 1;		/* don't shade env map! */

		/* "Surface" Normal points inward, UV is azim/elev of ray */
		u.sw.sw_inputs = MFI_NORMAL|MFI_UV;
		VREVERSE( u.sw.sw_hit.hit_normal, ap->a_ray.r_dir );
		/* U is azimuth, atan() range: -pi to +pi */
		u.sw.sw_uv.uv_u = mat_atan2( ap->a_ray.r_dir[Y],
			ap->a_ray.r_dir[X] ) * rt_inv2pi;
		if( u.sw.sw_uv.uv_u < 0 )
			u.sw.sw_uv.uv_u += 1.0;
		/*
		 *  V is elevation, atan() range: -pi/2 to +pi/2,
		 *  because sqrt() ensures that X parameter is always >0
		 */
		u.sw.sw_uv.uv_v = mat_atan2( ap->a_ray.r_dir[Z],
			sqrt( ap->a_ray.r_dir[X] * ap->a_ray.r_dir[X] +
			ap->a_ray.r_dir[Y] * ap->a_ray.r_dir[Y]) ) *
			rt_invpi + 0.5;
		u.sw.sw_uv.uv_du = u.sw.sw_uv.uv_dv = 0;

		VSETALL( u.sw.sw_color, 1 );
		VSETALL( u.sw.sw_basecolor, 1 );

		(void)viewshade( ap, &u.part, &u.sw );

		VMOVE( ap->a_color, u.sw.sw_color );
		ap->a_user = 1;		/* Signal view_pixel:  HIT */
		return(1);
	}

	ap->a_user = 0;		/* Signal view_pixel:  MISS */
	VMOVE( ap->a_color, background );	/* In case someone looks */
	return(0);
}

/*
 *			C O L O R V I E W
 *
 *  Manage the coloring of whatever it was we just hit.
 *  This can be a recursive procedure.
 */
int
colorview( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
{
	register struct partition *pp;
	register struct hit *hitp;
	register struct mfuncs *mfp;
	struct shadework sw;

	for( pp=PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw )
		if( pp->pt_outhit->hit_dist >= 0.0 )  break;
	if( pp == PartHeadp )  {
		rt_log("colorview:  no hit out front?\n");
		return(0);
	}
	hitp = pp->pt_inhit;

	if(rdebug&RDEBUG_HITS)  {
		rt_log("colorview: lvl=%d coloring %s\n",
			ap->a_level,
			pp->pt_regionp->reg_name);
		rt_pr_pt( ap->a_rt_i, pp );
	}
	if( hitp->hit_dist >= INFINITY )  {
		rt_log("colorview:  entry beyond infinity\n");
		VSET( ap->a_color, .5, 0, 0 );
		ap->a_user = 1;		/* Signal view_pixel:  HIT */
		goto out;
	}

	/* Check to see if eye is "inside" the solid */
	/* It might only be worthwhile doing all this in perspective mode */
	if( hitp->hit_dist < 0.0 )  {
		struct application sub_ap;
		FAST fastf_t f;

		if( pp->pt_outhit->hit_dist >= INFINITY ||
		    ap->a_level > max_bounces )  {
		    	if( rdebug&RDEBUG_SHOWERR )  {
				VSET( ap->a_color, 9, 0, 0 );	/* RED */
				rt_log("colorview:  eye inside %s (x=%d, y=%d, lvl=%d)\n",
					pp->pt_regionp->reg_name,
					ap->a_x, ap->a_y, ap->a_level);
		    	} else {
		    		VSETALL( ap->a_color, 0.18 );	/* 18% Grey */
		    	}
			ap->a_user = 1;		/* Signal view_pixel:  HIT */
			goto out;
		}
		/* Push on to exit point, and trace on from there */
		sub_ap = *ap;	/* struct copy */
		sub_ap.a_level = ap->a_level+1;
		f = pp->pt_outhit->hit_dist+0.0001;
		VJOIN1(sub_ap.a_ray.r_pt, ap->a_ray.r_pt, f, ap->a_ray.r_dir);
		sub_ap.a_purpose = "pushed eye position";
		(void)rt_shootray( &sub_ap );
		VSCALE( ap->a_color, sub_ap.a_color, 0.80 );
		ap->a_user = 1;		/* Signal view_pixel: HIT */
		goto out;
	}

	if( rdebug&RDEBUG_RAYWRITE )  {
		/* Record the approach path */
		if( hitp->hit_dist > 0.0001 )  {
			VJOIN1( hitp->hit_point, ap->a_ray.r_pt,
				hitp->hit_dist, ap->a_ray.r_dir );
			wraypts( ap->a_ray.r_pt,
				ap->a_ray.r_dir,
				hitp->hit_point,
				-1, ap, stdout );	/* -1 = air */
		}
	}
	if( rdebug&RDEBUG_RAYPLOT )  {
		/*  There are two parts to plot here.
		 *  Ray start to inhit (purple),
		 *  and inhit to outhit (grey).
		 */
		if( hitp->hit_dist > 0.0001 )  {
			register int i, lvl;
			fastf_t out;
			vect_t inhit, outhit;

			lvl = ap->a_level % 100;
			if( lvl < 0 )  lvl = 0;
			else if( lvl > 3 )  lvl = 3;
			i = 255 - lvl * (128/4);

			VJOIN1( inhit, ap->a_ray.r_pt,
				hitp->hit_dist, ap->a_ray.r_dir );
			pl_color( stdout, i, 0, i );
			pdv_3line( stdout, ap->a_ray.r_pt, inhit );

			if( (out = pp->pt_outhit->hit_dist) >= INFINITY )
				out = 10000;	/* to imply the direction */
			VJOIN1( outhit,
				ap->a_ray.r_pt, out,
				ap->a_ray.r_dir );
			pl_color( stdout, i, i, i );
			pdv_3line( stdout, inhit, outhit );
		}
	}

	bzero( (char *)&sw, sizeof(sw) );
	sw.sw_transmit = sw.sw_reflect = 0.0;
	sw.sw_refrac_index = 1.0;
	sw.sw_extinction = 0;
	sw.sw_xmitonly = 0;		/* want full data */
	sw.sw_inputs = 0;		/* no fields filled yet */
	VSETALL( sw.sw_color, 1 );
	VSETALL( sw.sw_basecolor, 1 );

	(void)viewshade( ap, pp, &sw );

	/* As a special case for now, handle reflection & refraction */
	if( sw.sw_reflect > 0 || sw.sw_transmit > 0 )
		(void)rr_render( ap, pp, &sw );

	VMOVE( ap->a_color, sw.sw_color );
	ap->a_user = 1;		/* Signal view_pixel:  HIT */
out:
	if(rdebug&RDEBUG_HITS)  {
		rt_log("colorview: lvl=%d ret a_user=%d %s\n",
			ap->a_level,
			ap->a_user,
			pp->pt_regionp->reg_name);
		VPRINT("color   ", ap->a_color);
	}
	return(1);
}

/*
 *			V I E W I T
 *
 *  a_hit() routine for simple lighting model.
 */
viewit( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
{
	register struct partition *pp;
	register struct hit *hitp;
	LOCAL fastf_t diffuse2, cosI2;
	LOCAL fastf_t diffuse1, cosI1;
	LOCAL fastf_t diffuse0, cosI0;
	LOCAL vect_t work0, work1;

	for( pp=PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw )
		if( pp->pt_outhit->hit_dist >= 0.0 )  break;
	if( pp == PartHeadp )  {
		rt_log("viewit:  no hit out front?\n");
		return(0);
	}
	hitp = pp->pt_inhit;
	RT_HIT_NORM( hitp, pp->pt_inseg->seg_stp, &(ap->a_ray) );

	/*
	 * Diffuse reflectance from each light source
	 */
	if( pp->pt_inflip )  {
		VREVERSE( hitp->hit_normal, hitp->hit_normal );
	}
	switch( lightmodel )  {
	case 1:
		/* Light from the "eye" (ray source).  Note sign change */
		diffuse0 = 0;
		if( (cosI0 = -VDOT(hitp->hit_normal, ap->a_ray.r_dir)) >= 0.0 )
			diffuse0 = cosI0 * ( 1.0 - AmbientIntensity);
		VSCALE( work0, LightHeadp->lt_color, diffuse0 );

		/* Add in contribution from ambient light */
		VSCALE( work1, ambient_color, AmbientIntensity );
		VADD2( ap->a_color, work0, work1 );
		break;
	case 3:
		/* Simple attempt at a 3-light model. */
		{
			struct light_specific *l0, *l1, *l2;
			l0 = LightHeadp;
			l1 = l0->lt_forw;
			l2 = l1->lt_forw;

			diffuse0 = 0;
			if( (cosI0 = VDOT(hitp->hit_normal, l0->lt_vec)) >= 0.0 )
				diffuse0 = cosI0 * l0->lt_fraction;
			diffuse1 = 0;
			if( (cosI1 = VDOT(hitp->hit_normal, l1->lt_vec)) >= 0.0 )
				diffuse1 = cosI1 * l1->lt_fraction;
			diffuse2 = 0;
			if( (cosI2 = VDOT(hitp->hit_normal, l2->lt_vec)) >= 0.0 )
				diffuse2 = cosI2 * l2->lt_fraction;

			VSCALE( work0, l0->lt_color, diffuse0 );
			VSCALE( work1, l1->lt_color, diffuse1 );
			VADD2( work0, work0, work1 );
			VSCALE( work1, l2->lt_color, diffuse2 );
			VADD2( work0, work0, work1 );
		}
		/* Add in contribution from ambient light */
		VSCALE( work1, ambient_color, AmbientIntensity );
		VADD2( ap->a_color, work0, work1 );
		break;
	case 2:
		/* Store surface normals pointing inwards */
		/* (For Spencer's moving light program) */
		ap->a_color[0] = (hitp->hit_normal[0] * (-.5)) + .5;
		ap->a_color[1] = (hitp->hit_normal[1] * (-.5)) + .5;
		ap->a_color[2] = (hitp->hit_normal[2] * (-.5)) + .5;
		break;
	case 4:
	 	{
			LOCAL struct curvature cv;
			FAST fastf_t f;
			auto int ival;

			RT_CURVE( &cv, hitp, pp->pt_inseg->seg_stp );
	
			f = cv.crv_c1;
			f *= 10;
			if( f < -0.5 )  f = -0.5;
			if( f > 0.5 )  f = 0.5;
			ap->a_color[0] = 0.5 + f;
			ap->a_color[1] = 0;

			f = cv.crv_c2;
			f *= 10;
			if( f < -0.5 )  f = -0.5;
			if( f > 0.5 )  f = 0.5;
			ap->a_color[2] = 0.5 + f;
		}
		break;
	case 5:
	 	{
			LOCAL struct curvature cv;
			FAST fastf_t f;
			auto int ival;

			RT_CURVE( &cv, hitp, pp->pt_inseg->seg_stp );

			ap->a_color[0] = (cv.crv_pdir[0] * (-.5)) + .5;
			ap->a_color[1] = (cv.crv_pdir[1] * (-.5)) + .5;
			ap->a_color[2] = (cv.crv_pdir[2] * (-.5)) + .5;
	 	}
		break;
	}

	if(rdebug&RDEBUG_HITS)  {
		rt_pr_hit( " In", hitp );
		rt_log("cosI0=%f, diffuse0=%f   ", cosI0, diffuse0 );
		VPRINT("RGB", ap->a_color);
	}
	ap->a_user = 1;		/* Signal view_pixel:  HIT */
	return(0);
}

/*
 *  			V I E W _ I N I T
 *
 *  Called once, early on in RT setup.
 */
view_init( ap, file, obj, minus_o )
register struct application *ap;
char *file, *obj;
{

#ifdef RTSRV
	buf_mode = 4;			/* multi-pixel buffering */
#else
	/* buf_mode = 3 presently can't be set, but still is supported */
	if( incr_mode )  {
		buf_mode = 2;		/* Frame buffering, write each line */
	} else if( rt_g.rtg_parallel )  {
		buf_mode = 2;		/* frame buffering, write each line */
	} else if( width <= 96 )  {
		buf_mode = 0;		/* single-pixel I/O */
	}  else  {
		buf_mode = 1;		/* line buffering */
	}
#endif

	switch( buf_mode )  {
	case 0:
		scanbuf = (char *)0;
		rt_log("Single pixel I/O, unbuffered\n");
		break;	
	case 1:
		scanbuf = rt_malloc( width*3 + sizeof(long),
			"scanbuf [line]" );
		rt_log("Buffering single scanlines\n");
		break;
	case 2:
		scanbuf = rt_malloc( width*height*3 + sizeof(long),
			"scanbuf [frame]" );
		npix_left = (int *)rt_malloc( height*sizeof(int),
			"npix_left[]" );
		rt_log("Buffering full frames, fb write at end of line\n");
		break;
	case 3:
		scanbuf = rt_malloc( width*height*3 + sizeof(long),
			"scanbuf [frame]" );
		rt_log("Buffering full frames, fb write at end of frame\n");
		break;
#ifdef RTSRV
	case 4:
		scanbuf = rt_malloc( srv_scanlen*3 + sizeof(long),
			"scanbuf [multi-line]" );
		break;
#endif
	default:
		rt_bomb("bad buf_mode");
	}

	/*
	 *  Connect up material library interfaces
	 *  Note that plastic.c defines the required "default" entry.
	 */
	{
		extern struct mfuncs phg_mfuncs[];
		extern struct mfuncs light_mfuncs[];
		extern struct mfuncs cloud_mfuncs[];
		extern struct mfuncs spm_mfuncs[];
		extern struct mfuncs txt_mfuncs[];
		extern struct mfuncs stk_mfuncs[];
		extern struct mfuncs cook_mfuncs[];
		extern struct mfuncs marble_mfuncs[];
		extern struct mfuncs stxt_mfuncs[];
		extern struct mfuncs points_mfuncs[];

		mlib_add( phg_mfuncs );
		mlib_add( light_mfuncs );
		mlib_add( cloud_mfuncs );
		mlib_add( spm_mfuncs );
		mlib_add( txt_mfuncs );
		mlib_add( stk_mfuncs );
		mlib_add( cook_mfuncs );
		mlib_add( marble_mfuncs );
		mlib_add( stxt_mfuncs );
		mlib_add( points_mfuncs );
	}

	if( minus_o )  {
		/* Output is destined for a pixel file */
		return(0);		/* don't open framebuffer */
	}  else  {
		return(1);		/* open a framebuffer */
	}
}

/*
 *  			V I E W 2 _ I N I T
 *
 *  Called each time a new image is about to be done.
 */
void
view_2init( ap )
register struct application *ap;
{
	register int i;
	extern int hit_nothing();

	ap->a_refrac_index = 1.0;	/* RI_AIR -- might be water? */
	ap->a_cumlen = 0.0;
	ap->a_miss = hit_nothing;
	ap->a_onehit = 1;

	switch( buf_mode )  {
	case 2:
		if( incr_mode )  {
			register int j = 1<<incr_level;
			register int w = 1<<(incr_nlevel-incr_level);

			/* Diminish buffer expectations on work-saved lines */
			for( i=0; i<j; i++ )  {
				if( (i & 1) == 0 )
					npix_left[i*w] = j/2;
				else
					npix_left[i*w] = j;
			}
		} else {
			for( i=0; i<height; i++ )
				npix_left[i] = width;
		}

		/* If not starting with pixel offset > 0,
		 * read in existing pixels first
		 */
		if( outfp != NULL && pix_start > 0 )  {
			/* We depend on file being r+w in this case */
			rewind( outfp );
			if( fread( scanbuf, 3, pix_start, outfp ) != pix_start )
				rt_log("view_2init:  bad initial fread\n");

			/* Account for pixels that don't need to be done */
			i = pix_start/width;
			npix_left[i] -= pix_start%width;
			for( i--; i >= 0; i-- )
				npix_left[i] = 0;
		}
		break;
	default:
		break;
	}
	if( incr_mode && incr_level > 0 )  {
		if( incr_level < incr_nlevel )
			return;		 /* more res to come */
	}

	switch( lightmodel )  {
	case 0:
		ap->a_hit = colorview;
		/* If present, use user-specified light solid */
		if( LightHeadp == LIGHT_NULL )  {
			if(rdebug&RDEBUG_SHOWERR)rt_log("No explicit light\n");
			light_maker(1, view2model);
		}
		break;
	case 2:
		VSETALL( background, 0 );	/* Neutral Normal */
		/* FALL THROUGH */
	case 1:
	case 3:
	case 4:
	case 5:
		ap->a_hit = viewit;
		light_maker(3, view2model);
		break;
	default:
		rt_bomb("bad lighting model #");
	}
	ap->a_rt_i->rti_nlights = light_init();

	ibackground[0] = background[0] * 255;
	ibackground[1] = background[1] * 255;
	ibackground[2] = background[2] * 255;
}
