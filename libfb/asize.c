/*
 *			A S I Z E . C
 *
 * Image file AutoSizing code.
 *
 *  Currently #included by bw-fb, pix-fb, and others.
 *  Might want to go into a library (libfb?)
 *
 *  Author -
 *	Phil Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */

#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

struct sizes {
	int	width;		/* image width in pixels */
	int	height;		/* image height in pixels */
};
struct sizes fb_common_sizes[] = {
	{  160,  120 },		/* quarter 640x480 */
	{  192,  128 },		/* Kodak Photo-CD, level 1, Base/16 */
	{  320,  200 },		/* PC screen format */
	{  320,  240 },		/* half 640x480 */
	{  384,  256 },		/* Kodak Photo-CD, level 2, Base/4 */
	{  640,  512 },		/* half 1280x1024 */
	{  640,  400 },		/* PC screen format */
	{  640,	 480 },		/* Common video format */
	{  640,	 485 },		/* Common video format, most correct */
	{  640,	 486 },		/* Common video format */
	{  720,	 485 },		/* Abekas video format, most correct */
	{  720,	 486 },		/* Abekas video format */
	{  768,  512 },		/* Kodak Photo-CD, level 3, Base */
	{ 1024,	 768 },		/* SGI-3D screen size */
	{ 1152,  900 },		/* Sun screen size */
	{ 1280,  960 },		/* twice 640x480 */
	{ 1280,	1024 },		/* SGI-4D screen size */
	{ 1440,  972 },		/* double Abekas video format */
	{ 1536, 1024 },		/* Kodak Photo-CD, level 4, 4*Base */
	{ 3072, 2048 },		/* Kodak Photo-CD, level 5, 16*Base */
	{ 3200, 4000 },		/* 8x10 inches, 400 dpi */
	{ 3400, 4400 },		/* 8.5x11 inches, 400 dpi */
	{    0,	   0 }
};

/*
 *			F B _ C O M M O N _ F I L E _ S I Z E
 *
 *  Returns non-zero if it finds a matching size
 */
int
fb_common_file_size( widthp, heightp, filename, pixel_size )
int	*widthp;		/* pointer to returned width */
int	*heightp;		/* pointer to returned height */
char	*filename;		/* image file to stat */
int	pixel_size;		/* bytes per pixel */
{
	struct	stat	sbuf;
	int	size;

	if( filename == NULL || *filename == '\0' )
		return	0;
	if( stat( filename, &sbuf ) < 0 )
		return	0;

	size = sbuf.st_size / pixel_size;

	return fb_common_image_size( widthp, heightp, size );
}

/*
 *			F B _ C O M M O N _ I M A G E _ S I Z E
 *
 *  Given the number of pixels in an image file,
 *  if this is a "common" image size,
 *  return the width and height of the image.
 *
 *  Returns -
 *	0	size unknown
 *	1	width and height returned
 */
int
fb_common_image_size( widthp, heightp, npixels )
int	*widthp;		/* pointer to returned width */
int	*heightp;		/* pointer to returned height */
register int	npixels;	/* Number of pixels */
{
	register struct	sizes	*sp;
	int			root;

	if( npixels <= 0 )
		return	0;

	sp = fb_common_sizes;
	while( sp->width != 0 ) {
		if( npixels == sp->width * sp->height ) {
			*widthp = sp->width;
			*heightp = sp->height;
			return	1;
		}
		sp++;
	}

	/* If the size is a perfect square, then use that. */
	root = (int)(sqrt((double)npixels)+0.999);
	if( root*root == npixels )  {
		*widthp = root;
		*heightp = root;
		return	1;
	}

	/* Nope, we are clueless. */
	return	0;
}
