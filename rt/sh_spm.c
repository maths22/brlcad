/*
 *			S P H . C
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
#include "./sph.h"
#include "./material.h"
#include "./mathtab.h"
#include "./rdebug.h"

char	*malloc();
char	*calloc();

/*
 *		S P H _ I N I T
 *
 *  Return a sphere map structure initialized for N points around
 *  the equator.  Malloc the storage and fill in the pointers.
 *  This code leaves a ring of "triangular" pixels at the poles.
 *  An alternative would be to have the pole region map to a
 *  single pixel.
 *  Returns SPH_NULL on error.
 */
spm_map_t *
spm_init( N )
int	N;
{
	int	i, nx, total, index;
	register spm_map_t *mapp;

	mapp = (spm_map_t *)rt_malloc( sizeof(spm_map_t), "spm_map_t");
	if( mapp == SPH_NULL )
		return( SPH_NULL );
	bzero( (char *)mapp, sizeof(spm_map_t) );

	mapp->ny = N/2;
	mapp->nx = (int *) rt_malloc( (unsigned)(N/2 * sizeof(*(mapp->nx))), "sph nx" );
	if( mapp->nx == NULL ) {
		spm_free( mapp );
		return( SPH_NULL );
	}
	mapp->xbin = (unsigned char **) rt_malloc( (unsigned)(N/2 * sizeof(char *)), "sph xbin" );
	if( mapp->xbin == NULL ) {
		spm_free( mapp );
		return( SPH_NULL );
	}

	total = 0;
	for( i = 0; i < N/4; i++ ) {
		nx = ceil( N*cos( i*twopi/N ) );
		if( nx > N ) nx = N;
		mapp->nx[ N/4 + i ] = nx;
		mapp->nx[ N/4 - i -1 ] = nx;

		total += 2*nx;
	}

	mapp->_data = (unsigned char *) calloc( (unsigned)total, sizeof(RGBpixel) );
	if( mapp->_data == NULL ) {
		spm_free( mapp );
		return( SPH_NULL );
	}

	index = 0;
	for( i = 0; i < N/2; i++ ) {
		mapp->xbin[i] = &((mapp->_data)[index]);
		index += 3 * mapp->nx[i];
	}

	return( mapp );
}

/*
 *		S P H _ F R E E
 *
 *  Free the storage associated with a sphere structure.
 */
void
spm_free( mp )
spm_map_t *mp;
{
	if( mp == SPH_NULL )
		return;

	if( mp->_data != NULL )
		(void) rt_free( (char *)mp->_data, "sph _data" );

	if( mp->nx != NULL )
		(void) rt_free( (char *)mp->nx, "sph nx" );

	if( mp->xbin != NULL )
		(void) rt_free( (char *)mp->xbin, "sph xbin" );

	(void) rt_free( (char *)mp, "spm_map_t" );
}

/*
 *		S P H _ R E A D
 *
 *  Read the value of the pixel at the given normalized (u,v)
 *  coordinates.  It does NOT check the sanity of the coords.
 *
 *  0.0 <= u < 1.0	Left to Right
 *  0.0 <= v < 1.0	Bottom to Top
 */
void
spm_read( mapp, valp, u, v )
register spm_map_t	*mapp;
register unsigned char	*valp;
double	u, v;
{
	int	x, y;
	register unsigned char *cp;

	y = v * mapp->ny;
	x = u * mapp->nx[y];
	cp = &(mapp->xbin[y][x*3]);

	*valp++ = *cp++;
	*valp++ = *cp++;
	*valp++ = *cp++;
}

/*
 *		S P H _ W R I T E
 *
 *  Write the value of the pixel at the given normalized (u,v)
 *  coordinates.  It does NOT check the sanity of the coords.
 *
 *  0.0 <= u < 1.0	Left to Right
 *  0.0 <= v < 1.0	Bottom to Top
 */
void
spm_write( mapp, valp, u, v )
register spm_map_t	*mapp;
register unsigned char	*valp;
double	u, v;
{
	int	x, y;
	register unsigned char *cp;

	y = v * mapp->ny;
	x = u * mapp->nx[y];
	cp = &(mapp->xbin[y][x*3]);

	*cp++ = *valp++;
	*cp++ = *valp++;
	*cp++ = *valp++;
}

/*
 *		S P H _ L O A D
 *
 *  Read a saved sphere map from a file ("-" for stdin) into
 *  the given map structure.
 *  This does not check for conformity of size, etc.
 *  Returns -1 on error, else 0.
 */
int
spm_load( mapp, filename )
spm_map_t *mapp;
char	*filename;
{
	int	y, total;
	FILE	*fp;

	if( strcmp( filename, "-" ) == 0 )
		fp = stdin;
	else if( (fp = fopen( filename, "r" )) == NULL )
		return( -1 );

	total = 0;
	for( y = 0; y < mapp->ny; y++ )
		total += mapp->nx[y];

	y = fread( (char *)mapp->_data, sizeof(RGBpixel), total, fp );
	(void) fclose( fp );

	if( y != total )
		return( -1 );

	return( 0 );
}

/*
 *		S P H _ S A V E
 *
 *  Write a loaded sphere map to the given file ("-" for stdout).
 *  Returns -1 on error, else 0.
 */
int
spm_save( mapp, filename )
spm_map_t *mapp;
char	*filename;
{
	int	i;
	FILE	*fp;

	if( strcmp( filename, "-" ) == 0 )
		fp = stdout;
	else if( (fp = fopen( filename, "w" )) == NULL )
		return( -1 );

	for( i = 0; i < mapp->ny; i++ ) {
		if( fwrite( (char *)mapp->xbin[i], sizeof(RGBpixel),
		    mapp->nx[i], fp ) != mapp->nx[i] ) {
		    	(void) fclose( fp );
		    	return( -1 );
		}
	}

	(void) fclose( fp );

	return( 0 );
}

/*
 *		S P H _ P I X _ L O A D
 *
 *  Load an 'nx' by 'ny' pix file and filter it into the
 *  given sphere structure.
 *  Returns -1 on error, else 0.
 */
int
spm_px_load( mapp, filename, nx, ny )
spm_map_t *mapp;
char	*filename;
int	nx, ny;
{
	int	i, j;			/* index input file */
	int	x, y;			/* index texture map */
	double	j_per_y, i_per_x;	/* ratios */
	int	nj, ni;			/* ints of ratios */
	unsigned char *cp;
	unsigned char *buffer;
	unsigned long	red, green, blue;
	long	count;
	FILE	*fp;

	if( strcmp( filename, "-" ) == 0 )
		fp = stdin;
	else if( (fp = fopen( filename, "r" )) == NULL )
		return( -1 );

	/* Shamelessly suck it all in */
	buffer = (unsigned char *)malloc( (unsigned)(nx*nx*sizeof(RGBpixel)) );
	/* XXX */
	(void) fread( (char *)buffer, sizeof(RGBpixel), nx*ny, fp );
	(void) fclose( fp );

	j_per_y = (double)ny / (double)mapp->ny;
	nj = (int)j_per_y;
	/* for each bin */
	for( y = 0; y < mapp->ny; y++ ) {
		i_per_x = (double)nx / (double)mapp->nx[y];
		ni = (int)i_per_x;
		/* for each cell in bin */
		for( x = 0; x < mapp->nx[y]; x++ ) {
			/* Average pixels from the input file */
			red = green = blue = 0;
			count = 0;
			for( j = y*j_per_y; j < y*j_per_y+nj; j++ ) {
				for( i = x*i_per_x; i < x*i_per_x+ni; i++ ) {
					red = red + (unsigned long)buffer[ 3*(j*nx+i) ];
					green = green + (unsigned long)buffer[ 3*(j*nx+i)+1 ];
					blue = blue + (unsigned long)buffer[ 3*(j*nx+i)+2 ];
					count++;
				}
			}
			/* Save the color */
			cp = &(mapp->xbin[y][x*3]);
			*cp++ = (unsigned char)(red/count);
			*cp++ = (unsigned char)(green/count);
			*cp++ = (unsigned char)(blue/count);
		}
	}
	(void) free( (char *)buffer );

	return( 0 );
}

/*
 *		S P H _ P I X _ S A V E
 *
 *  Save a sphere structure as an 'nx' by 'ny' pix file.
 *  Returns -1 on error, else 0.
 */
int
spm_px_save( mapp, filename, nx, ny )
spm_map_t *mapp;
char	*filename;
int	nx, ny;
{
	int	x, y;
	FILE	*fp;
	unsigned char pixel[3];

	if( strcmp( filename, "-" ) == 0 )
		fp = stdout;
	else if( (fp = fopen( filename, "w" )) == NULL )
		return( -1 );

	for( y = 0; y < ny; y++ ) {
		for( x = 0; x < nx; x++ ) {
			spm_read( mapp, pixel, (double)x/(double)nx, (double)y/(double)ny );
			(void) fwrite( (char *)pixel, sizeof(pixel), 1, fp );
		}
	}

	return( 0 );
}

/*
 * 		S P H _ D U M P
 *
 *  Display a sphere structure on stderr.
 *  Used for debugging.
 */
void
spm_dump( mp )
spm_map_t *mp;
{
	int	i;

	fprintf( stderr, "ny = %d\n", mp->ny );
	fprintf( stderr, "_data = 0x%x\n", mp->_data );
	for( i = 0; i < mp->ny; i++ ) {
		fprintf( stderr, "  nx[%d] = %3d, xbin[%d] = 0x%x\n",
			i, mp->nx[i], i, mp->xbin[i] );
	}
}

/*****/
struct spm_specific {
	char	sp_file[128];	/* Filename */
	int	sp_n;		/* number of pixels around equator */
	spm_map_t *sp_map;	/* stuff */
};
#define SP_NULL	((struct spm_specific *)0)

struct matparse spm_parse[] = {
#ifndef cray
	"file",		(mp_off_ty)(SP_NULL->sp_file),	"%s",
#else
	"file",		(mp_off_ty)0,			"%s",
#endif
	"n",		(mp_off_ty)&(SP_NULL->sp_n),	"%d",
	(char *)0,	(mp_off_ty)0,			(char *)0
};

HIDDEN int spm_setup(), spm_render(), spm_print(), spm_mfree();

struct mfuncs spm_mfuncs[] = {
	"sph",		0,		0,		MFI_UV,
	spm_setup,	spm_render,	spm_print,	spm_mfree,

	(char *)0,	0,		0,
	0,		0,		0,		0
};

/*
 *  			S P H _ R E N D E R
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
}

/*
 *			S P H _ S E T U P
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
	spp->sp_n = -1;
	mlib_parse( matparm, spm_parse, (mp_off_ty)spp );
	if( spp->sp_n < 0 )  spp->sp_n = 512;
	if( spp->sp_file[0] == '\0' )
		goto fail;
	if( (spp->sp_map = spm_init( spp->sp_n )) == SPH_NULL )
		goto fail;
	if( spm_load( spp->sp_map, spp->sp_file ) < 0 )
		goto fail;
	return(1);
fail:
	rt_free( (char *)spp, "spm_specific" );
	return(-1);
}

/*
 *			S P H _ P R I N T
 */
HIDDEN int
spm_print( rp, dp )
register struct region *rp;
char	*dp;
{
	mlib_print("spm_setup", spm_parse, (mp_off_ty)dp);
	/* Should be more here */
}

HIDDEN int
spm_mfree( cp )
char *cp;
{
	spm_free( (spm_map_t *)cp );
}
