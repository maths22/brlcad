/*
 *  			F B - P I X . C
 *  
 *  Dumb little program to take a frame buffer image and
 *  write a .pix image.
 *  
 *  Author -
 *	Michael John Muuss
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

#define MAX_LINE	1024		/* Max pixels/line */
static char scanline[MAX_LINE*3];	/* 1 scanline pixel buffer */
static int scanbytes;			/* # of bytes of scanline */

struct pixel outline[MAX_LINE];

int inverse = 0;			/* Draw upside-down */

char usage[] = "Usage: fb-pix [-h] [-i] [width] > file.pix\n";

main(argc, argv)
int argc;
char **argv;
{
	static int y;
	static int nlines;		/* Square:  nlines, npixels/line */
	static int fbsize;

	if( argc < 1 || isatty(fileno(stdout)) )  {
		fprintf(stderr,"%s", usage);
		exit(1);
	}

	fbsize = 512;
	nlines = 512;
	while( argv[1][0] == '-' )  {
		if( strcmp( argv[1], "-h" ) == 0 )  {
			fbsize = 1024;
			nlines = 1024;
			argc--; argv++;
			continue;
		}
		if( strcmp( argv[1], "-i" ) == 0 )  {
			inverse = 1;
			argc--; argv++;
			continue;
		}
	}
	if( argc == 2 )
		nlines = atoi(argv[1]);
	if( argc > 2 )  {
		fprintf(stderr,"%s", usage);
		exit(1);
	}
	if( nlines > 512 )
		fbsetsize(fbsize);

	scanbytes = nlines * 3;

	if( fbopen( NULL, APPEND ) < 0 )  {
		fprintf(stderr,"fbopen failed\n");
		exit(12);
	}

	if( !inverse )  {
		/*  Regular -- draw bottom to top */
		for( y = nlines-1; y >= 0; y-- )  {
			register char *in;
			register struct pixel *out;
			register int i;

			fbread( 0, y, outline, nlines );

			in = scanline;
			out = outline;
			for( i=0; i<nlines; i++ )  {
				*in++ = out->red;
				*in++ = out->green;
				*in++ = out->blue;
				out++;
			}
			if( write( 1, (char *)scanline, scanbytes ) != scanbytes )  {
				perror("write");
				exit(1);
			}
		}
	}  else  {
		/*  Inverse -- draw top to bottom */
		for( y=0; y < nlines; y++ )  {
			register char *in;
			register struct pixel *out;
			register int i;

			fbread( 0, y, outline, nlines );

			in = scanline;
			out = outline;
			for( i=0; i<nlines; i++ )  {
				*in++ = out->red;
				*in++ = out->green;
				*in++ = out->blue;
				out++;
			}
			if( write( 1, (char *)scanline, scanbytes ) != scanbytes )  {
				perror("write");
				exit(1);
			}
		}
	}
	exit(0);
}
