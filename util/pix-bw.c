/*
 *		P I X - B W . C
 *
 * Converts a RGB pix file into an 8-bit BW file.
 *
 * By default it will weight the RGB values evenly.
 * -ntsc will use weights for NTSC standard primaries and
 *       NTSC alignment white.
 * -crt  will use weights for "typical" color CRT phosphors and
 *       a D6500 alignment white.
 *
 *  Author -
 *	Phillip Dykstra
 *	13 June 1986
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

double	atof();

unsigned char	ibuf[3*1024], obuf[1024];

/* flags */
int	red   = 0;
int	green = 0;
int	blue  = 0;
double	rweight = 0.0;
double	gweight = 0.0;
double	bweight = 0.0;

static char *Usage = "usage: pix-bw [-ntsc -crt -r[#] -g[#] -b[#]] < file.pix > file.bw\n";
/* someday: [file.pix [file.bw]] */

main( argc, argv )
int argc; char **argv;
{
	int	in, out, num;
	int	multiple_colors, num_color_planes;
	int	clipped_high, clipped_low;
	double	value;
	FILE	*finp, *foutp;

	while( argc > 1 && argv[1][0] == '-' ) {
		if( strcmp(argv[1],"-ntsc") == 0 ) {
			/* NTSC weights */
			rweight = 0.30;
			gweight = 0.59;
			bweight = 0.11;
			red = green = blue = 1;
		} else if( strcmp(argv[1],"-crt") == 0 ) {
			/* CRT weights */
			rweight = 0.26;
			gweight = 0.66;
			bweight = 0.08;
			red = green = blue = 1;
		} else switch( argv[1][1] ) {
			case 'r':
				red++;
				if( argv[1][2] != NULL )
					rweight = atof( &argv[1][2] );
				break;
			case 'g':
				green++;
				if( argv[1][2] != NULL )
					gweight = atof( &argv[1][2] );
				break;
			case 'b':
				blue++;
				if( argv[1][2] != NULL )
					bweight = atof( &argv[1][2] );
				break;
			default:
				fprintf( stderr, "pix-bw: bad flag \"%s\"\n", argv[1] );
				fputs( Usage, stderr );
				exit( 1 );
		}
		argc--;
		argv++;
	}

	/*
	 * Eventually we may accept file names
	 */
	finp = stdin;
	foutp = stdout;

	if( isatty(fileno(finp)) ) {
		fputs( Usage, stderr );
		exit( 2 );
	}

	/* Hack for multiple color planes */
	if( red + green + blue > 1 || rweight != 0.0 || gweight != 0.0 || bweight != 0.0 )
		multiple_colors = 1;
	else
		multiple_colors = 0;

	num_color_planes = red + green + blue;
	if( red != 0 && rweight == 0.0 )
		rweight = 1.0 / (double)num_color_planes;
	if( green != 0 && gweight == 0.0 )
		gweight = 1.0 / (double)num_color_planes;
	if( blue != 0 && bweight == 0.0 )
		bweight = 1.0 / (double)num_color_planes;

	clipped_high = clipped_low = 0;
	while( (num = fread( ibuf, sizeof( char ), 3*1024, finp )) > 0 ) {
		/*
		 * The loops are repeated for efficiency...
		 */
		if( multiple_colors ) {
			for( in = out = 0; out < num/3; out++, in += 3 ) {
				value = rweight*ibuf[in] + gweight*ibuf[in+1] + bweight*ibuf[in+2];
				if( value > 255.0 ) {
					obuf[out] = 255;
					clipped_high++;
				} else if( value < 0.0 ) {
					obuf[out] = 0;
					clipped_low++;
				} else
					obuf[out] = value;
			}
		} else if( red ) {
			for( in = out = 0; out < num/3; out++, in += 3 )
				obuf[out] = ibuf[in];
		} else if( green ) {
			for( in = out = 0; out < num/3; out++, in += 3 )
				obuf[out] = ibuf[in+1];
		} else if( blue ) {
			for( in = out = 0; out < num/3; out++, in += 3 )
				obuf[out] = ibuf[in+2];
		} else {
			/* uniform weight */
			for( in = out = 0; out < num/3; out++, in += 3 )
				obuf[out] = (ibuf[in] + ibuf[in+1] + ibuf[in+2]) / 3;
		}
		fwrite( obuf, sizeof( char ), num/3, foutp );
	}

	if( clipped_high != 0 || clipped_low != 0 ) {
		fprintf( stderr, "pix-bw: clipped %d high, %d, low\n",
			clipped_high, clipped_low );
	}
}
