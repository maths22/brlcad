/*
 *		F B C M A P . C
 *
 *	Usage:	fbcmap [-h] [flavor]
 *
 *	Write a colormap to a framebuffer.
 *	When invoked with no arguments, or with a flavor of 0,
 *	the standard ramp color-map is written.
 *	Other flavors provide interesting alternatives.
 *
 *  Author -
 *	Mike Muuss, 7/17/82
 *	VAX version 10/18/83
 *
 *	Conversion to generic frame buffer utility using libfb(3).
 *	In the process, the name has been changed to fbcmap from ikcmap.
 *	Gary S. Moss, BRL. 03/12/85
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
#include "fb.h"

typedef unsigned char	u_char;
int hires = 0;
static ColorMap cmap;
static int	flavor = 0;
static u_char	utah_cmap[256] = {
	  0,  4,  9, 13, 17, 21, 25, 29, 32, 36, 39, 42, 45, 48, 51, 54,
	 57, 59, 62, 64, 67, 69, 72, 74, 76, 78, 81, 83, 85, 87, 89, 91,
	 92, 94, 96, 98,100,101,103,105,106,108,110,111,113,114,116,117,
	119,120,121,123,124,125,127,128,129,131,132,133,134,136,137,138,
	139,140,141,143,144,145,146,147,148,149,150,151,152,153,154,155,
	156,157,158,159,160,161,162,163,164,164,165,166,167,168,169,170,
	171,171,172,173,174,175,175,176,177,178,179,179,180,181,182,182,
	183,184,185,185,186,187,187,188,189,190,190,191,192,192,193,194,
	194,194,195,196,196,197,197,198,199,199,200,201,201,202,202,203,
	204,204,205,205,206,207,207,208,208,209,209,210,211,211,212,212,
	213,213,214,214,215,215,216,216,217,217,218,219,219,220,220,221,
	221,222,222,223,223,224,224,224,225,225,226,226,227,227,228,228,
	229,229,230,230,231,231,231,232,232,233,233,234,234,235,235,235,
	236,236,237,237,238,238,238,239,239,240,240,240,241,241,242,242,
	242,243,243,244,244,244,245,245,246,246,246,247,247,248,248,248,
	249,249,249,250,250,251,251,251,252,252,252,253,253,254,254,255
};

main(argc, argv)
char *argv[];
{
	register int		i;
	register int		fudge;
	register ColorMap	*cp = &cmap;
	int	size;
	FBIO *fbp;

	size = 512;

	if( ! pars_Argv( argc, argv ) ) {
		usage();
		return	1;
	}
	if( hires ) size = 1024;
	if( (fbp = fb_open( NULL, size, size )) == NULL )
		return	1;

	switch( flavor )  {

	case 0 : /* Standard - Linear color map */
		(void) fprintf( stderr, "Color map #0, linear (standard).\n" );
		cp = (ColorMap *) NULL;
		break;

	case 1 : /* Reverse linear color map */
		(void) fprintf( stderr, "Color map #1, reverse-linear (negative).\n" );
		for( i = 0; i < 256; i++ ) {
			cp->cm_red[255-i] =
			cp->cm_green[255-i] =
			cp->cm_blue[255-i] = i << 8;
		}
		break;

	case 2 :
		/* Experimental correction, for POLAROID 8x10 print film */
		(void) fprintf( stderr,
			"Color map #2, corrected for POLAROID 809/891 film.\n" );
		/* First entry black */
#define BOOST(point, bias) \
	((int)((bias)+((float)(point)/256.*(255-(bias)))))
		for( i = 1; i < 256; i++ )  {
			fudge = BOOST(i, 70);
			cp->cm_red[i] = fudge << 8;		/* B */
		}
		for( i = 1; i < 256; i++ )  {
			fudge = i;
			cp->cm_green[i] = fudge << 8;	/* G */
		}
		for( i = 1; i < 256; i++ )  {
			fudge = BOOST( i, 30 );
			cp->cm_blue[i] = fudge << 8;	/* R */
		}
		break;

	case 3 : /* Standard, with low intensities set to black */
		(void) fprintf( stderr, "Color map #3, low 100 entries black.\n" );
		for( i = 100; i < 256; i++ )  {
			cp->cm_red[i] =
			cp->cm_green[i] =
			cp->cm_blue[i] = i << 8;
		}
		break;

	case 4 : /* Amplify middle of the range, for Moss's dim pictures */
#define UPSHIFT	64
		(void) fprintf( stderr,
			"Color map #4, amplify middle range to boost dim pictures.\n" );
		/* First entry black */
		for( i = 1; i< 256-UPSHIFT; i++ )  {
			register int j = i + UPSHIFT;
			cp->cm_red[i] =
			cp->cm_green[i] =
			cp->cm_blue[i] = j << 8;
		}
		for( i = 256-UPSHIFT; i < 256; i++ )  {
			cp->cm_red[i] =
			cp->cm_green[i] =
			cp->cm_blue[i] = 255 << 8;	/* Full Scale */
		}
		break;

	case 5 : /* University of Utah's color map */
		(void) fprintf( stderr,
			"Color map #5, University of Utah's gamma correcting map.\n" );
		for( i = 0; i < 256; i++ )
			cp->cm_red[i] =
			cp->cm_green[i] =
			cp->cm_blue[i] = utah_cmap[i] << 8;
		break;

	case 6 :	/* Delta's */
		(void) fprintf( stderr, "Color map #6, color deltas.\n" );
		/* white at zero */
		cp->cm_red[0] = 65535;
		cp->cm_green[0] = 65535;
		cp->cm_blue[0] = 65535;
		/* magenta at 32 */
		cp->cm_red[32] = 65535;
		cp->cm_blue[32] = 65535;
		/* Red at 64 */
		cp->cm_red[32*2] = 65535;
		/* Yellow ... */
		cp->cm_red[32*3] = 65535;
		cp->cm_green[32*3] = 65535;
		/* Green */
		cp->cm_green[32*4] = 65535;
		/* Cyan */
		cp->cm_green[32*5] = 65535;
		cp->cm_blue[32*5] = 65535;
		/* Blue */
		cp->cm_blue[32*6] = 65535;
		break;

	case 10:	/* Black */
		(void) fprintf( stderr, "Color map #10, solid black.\n" );
		break;

	case 11:	/* White */
		(void) fprintf( stderr, "Color map #11, solid white.\n" );
		for( i = 0; i < 256; i++ )  {
			cp->cm_red[i] =
			cp->cm_green[i] =
			cp->cm_blue[i] = 255 << 8;
		}
		break;

	case 12:	/* 18% Grey */
		(void) fprintf( stderr, "Color map #12, 18%% neutral grey.\n" );
		for( i = 0; i < 256; i++ )  {
			cp->cm_red[i] =
			cp->cm_green[i] =
			cp->cm_blue[i] = 46 << 8;
		}
		break;

	default:
		(void) fprintf(	stderr,
				"Color map #%d, flavor not implemented!\n",
				flavor );
		usage();
		return	1;
	}
	fb_wmap( fbp, cp );
	return fb_close( fbp );
}

/*	p a r s _ A r g v ( )
 */
int
pars_Argv( argc, argv )
register char	**argv;
{
	register int	c;
	extern int	optind;

	while( (c = getopt( argc, argv, "h" )) != EOF ) {
		switch( c ) {
			case 'h' : /* High resolution frame buffer.	*/
				hires++;
				break;
			case '?' :
				return	0;
		}
	}
	if( argv[optind] != NULL )
		flavor = atoi( argv[optind] );
	return	1;
}


usage()
{
	(void) fprintf( stderr, "Usage : fbcmap	[-h] [map_number]\n" );
	(void) fprintf( stderr,
			"Color map #0, linear (standard).\n" );
	(void) fprintf( stderr,
			"Color map #1, reverse-linear (negative).\n" );
	(void) fprintf( stderr,
		"Color map #2, corrected for POLAROID 809/891 film.\n" );
	(void) fprintf( stderr,
			"Color map #3, low 100 entries black.\n" );
	(void) fprintf( stderr,
		"Color map #4, amplify middle range to boost dim pictures.\n" );
	(void) fprintf( stderr,
		"Color map #5, University of Utah's gamma correcting map.\n" );
	(void) fprintf( stderr, "Color map #6, color deltas.\n" );
	(void) fprintf( stderr, "Color map #10, solid black.\n" );
	(void) fprintf( stderr, "Color map #11, solid white.\n" );
	(void) fprintf( stderr, "Color map #12, 18%% neutral grey.\n" );
}
