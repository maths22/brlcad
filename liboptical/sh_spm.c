/*
 *			S P M . C
 *
 *  Spherical Data Structures/Texture Maps
 *
 *  Author -
 *	Phillip Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "fb.h"
#include "spm.h"
#include "./material.h"
#include "./mathtab.h"
#include "./rdebug.h"

struct spm_specific {
	char	sp_file[128];	/* Filename */
	int	sp_w;		/* Width: number of pixels around equator */
	spm_map_t *sp_map;	/* stuff */
};
#define SP_NULL	((struct spm_specific *)0)

struct matparse spm_parse[] = {
	"file",		(mp_off_ty)(SP_NULL->sp_file),	"%s",
	"w",		(mp_off_ty)&(SP_NULL->sp_w),	"%d",
	"n",		(mp_off_ty)&(SP_NULL->sp_w),	"%d",	/*compat*/
	(char *)0,	(mp_off_ty)0,			(char *)0
};

HIDDEN int	spm_setup(), spm_render();
HIDDEN void	spm_print(), spm_mfree();

struct mfuncs spm_mfuncs[] = {
	"spm",		0,		0,		MFI_UV,
	spm_setup,	spm_render,	spm_print,	spm_mfree,

	(char *)0,	0,		0,
	0,		0,		0,		0
};

/*
 *  			S P M _ R E N D E R
 *  
 *  Given a u,v coordinate within the texture ( 0 <= u,v <= 1.0 ),
 *  return a pointer to the relevant pixel.
 */
HIDDEN int
spm_render( ap, pp, swp, dp )
struct application *ap;
struct partition *pp;
struct shadework	*swp;
char	*dp;
{
	register struct spm_specific *spp =
		(struct spm_specific *)dp;
	int	x, y;
	register unsigned char *cp;

	/** spm_read( spp->sp_map, xxx ); **/
	/* Limits checking? */
	y = swp->sw_uv.uv_v * spp->sp_map->ny;
	x = swp->sw_uv.uv_u * spp->sp_map->nx[y];
	cp = &(spp->sp_map->xbin[y][x*3]);
	VSET( swp->sw_color, cp[RED]/256., cp[GRN]/256., cp[BLU]/256. );
	return(1);
}

/*
 *			S P M _ S E T U P
 *
 *  Returns -
 *	<0	failed
 *	>0	success
 */
HIDDEN int
spm_setup( rp, matparm, dpp )
register struct region *rp;
char	*matparm;
char	**dpp;
{
	register struct spm_specific *spp;

	GETSTRUCT( spp, spm_specific );
	*dpp = (char *)spp;

	spp->sp_file[0] = '\0';
	spp->sp_w = -1;
	mlib_parse( matparm, spm_parse, (mp_off_ty)spp );
	if( spp->sp_w < 0 )  spp->sp_w = 512;
	if( spp->sp_file[0] == '\0' )
		goto fail;
	if( (spp->sp_map = spm_init( spp->sp_w, sizeof(RGBpixel) )) == SPM_NULL )
		goto fail;
	if( spm_load( spp->sp_map, spp->sp_file ) < 0 )
		goto fail;
	return(1);
fail:
	rt_free( (char *)spp, "spm_specific" );
	return(-1);
}

/*
 *			S P M _ P R I N T
 */
HIDDEN void
spm_print( rp, dp )
register struct region *rp;
char	*dp;
{
	mlib_print("spm_setup", spm_parse, (mp_off_ty)dp);
	/* Should be more here */
}

HIDDEN void
spm_mfree( cp )
char *cp;
{
	spm_free( (spm_map_t *)cp );
}
